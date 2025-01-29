package main

import (
	"bytes"
	"time"
	"bufio"
	"encoding/base64"
	"encoding/binary"
	"encoding/hex"
	"fmt"
	"image"
	"image/png"
	"io"
	"log"
	"net"
	"net/http"
	"strconv"
	"strings"

	// ApproxBiLinear for CPU SSAA
	"golang.org/x/image/draw"

	"errors"
	"image/color"
	"syscall"

	"flag"
	"os"

	"database/sql"
	_ "github.com/go-sql-driver/mysql"
)

var (
	mysqlAvailable   = false
	db               *sql.DB
	upstreamTCP      string
	useXForwardedFor bool
	loggingEnabled   bool
)

// RenderRequest is the equivalent struct in Go for handling the render request data.
type RenderRequest struct {
	Data            [96]byte
	DataLength      uint16
	ModelFlag       uint8
	ResponseFormat  uint8 // rgba, gltf, tga
	Resolution      uint16
	TexResolution   int16
	ViewType        uint8
	ResourceType    int8
	ShaderType      uint8
	Expression      uint8
	ExpressionFlag  FFLAllExpressionFlag //uint32  // used if there are multiple
	CameraRotate    [3]int16
	ModelRotate     [3]int16
	BackgroundColor [4]uint8

	AAMethod             uint8 // UNUSED
	DrawStageMode        uint8
	VerifyCharInfo       bool
	VerifyCRC16          bool
	LightEnable          bool
	ClothesColor         int8 // default: -1
	PantsColor           int8 // ^^
	BodyType             int8 // ^^

	HeadwearIndex        int8
	HeadwearColor        int8

	InstanceCount        uint8
	InstanceRotationMode uint8
	LightDirection       [3]int16 // default/unset: -1
	SplitMode            uint8
}

const FFL_EXPRESSION_LIMIT = 70

type FFLAllExpressionFlag struct {
	Flags [3]uint32 // 0-96
}

func SetExpressionFlagIndex(ef *FFLAllExpressionFlag, index int, set bool) {
	if index < 0 || index >= FFL_EXPRESSION_LIMIT {
		fmt.Printf("FFLSetExpressionFlagIndex: input out of range: %d\n", index)
		return // Do not set anything.
	}

	part := index / 32       // Determine which 32-bit block
	bitIndex := index % 32   // Determine which bit within the block

	if set {
		ef.Flags[part] |= (1 << bitIndex) // Set the bit
	} else {
		ef.Flags[part] &^= (1 << bitIndex) // Clear the bit
	}
}

// TGAHeader is put in front of render responses.
// It indicates the width, height, and BPP of the render.
// Note that while TGAs are supposed to represent BGRA...
// ... currently the server outputs RGBA. But this could
// be changed in the future to directly output compliant
// TGA if needed. Otherwise most of this is still unused
type TGAHeader struct {
	IDLength        uint8 // unused (0)
	ColorMapType    uint8 // always 0 for no color map
	ImageType       uint8 // image_type_enum, 2 = uncomp_true_color
	ColorMapOrigin  int16 // unused
	ColorMapLength  int16 // unused (0)
	ColorMapDepth   uint8 // unused
	OriginX         int16 // unused (0)
	OriginY         int16 // unused (0)
	Width           int16 // Width of the image in pixels
	Height          int16 // Height of the image in pixels
	BitsPerPixel    uint8 // Number of bits per pixel
	ImageDescriptor uint8 // ???
}

// GLBHeader represents a binary glTF header and it
// is used to return the size of a glTF before it is fully read.
type GLBHeader struct {
	Magic   uint32 // we do not actually care about these
	Version uint32
	Length  uint32
}

var viewTypes = map[string]int{
	"face":             0,
	"face_only":        1,
	"all_body":         2,
	"fflmakeicon":      3,
	"ffliconwithbody":  4,
	"variableiconbody": 5,
	"all_body_sugar":   6,
}

var modelTypes = map[string]int{
	"normal":    0,
	"hat":       1,
	"face_only": 2,
}

var splitModes = map[string]int{
	"none":  0,
	"front": 1,
	"back":  2,
	"both":  3,
}

var drawStageModes = map[string]int{
	"all":                 0,
	"opa_only":            1,
	"xlu_only":            2,
	"mask_only":           3,
	"xlu_depth_mask":      4,
	"body_only":           5,
	"body_inv_depth_mask": 6,
}

var corsOrigin string

func main() {
	// Command-line arguments
	var host, unixSocket, certFile, keyFile, assetsDir, mysqlConnStr, upstreamAddr string
	flag.StringVar(&host, "host", ":5000", "hostname to listen to http on, OR https if you specify cert and key")
	flag.StringVar(&unixSocket, "unix-socket", "", "unix socket to listen on, overrides host")
	flag.StringVar(&certFile, "cert", "", "TLS certificate file")
	flag.StringVar(&keyFile, "key", "", "TLS key file")

	flag.StringVar(&assetsDir, "assets-dir", "", "If you set this, files from here will be served at root.")

	flag.StringVar(&mysqlConnStr, "mysql", "", "MySQL connection string for NNID fetch")
	flag.StringVar(&upstreamAddr, "upstream", "localhost:12346", "Upstream TCP server address")
	flag.BoolVar(&useXForwardedFor, "use-x-forwarded-for", false, "Use X-Forwarded-For header for client IP")
	flag.StringVar(&corsOrigin, "cors", "", "CORS origin to allow. Set to * to allow all origins. Leave blank to disable CORS header.")
	flag.BoolVar(&loggingEnabled, "enable-benchmarking", false, "Log how much time each request is taking.")

	flag.Parse()

	if mysqlConnStr != "" {
		var err error
		db, err = sql.Open("mysql", mysqlConnStr)
		if err != nil {
			log.Fatalf("Failed to connect to MySQL: %v", err)
		}
		mysqlAvailable = true
	}

	upstreamTCP = upstreamAddr

	imagePngEndpoint := "/miis/image.png"

	if assetsDir != "" {
		http.Handle("/", http.FileServer(http.Dir(assetsDir)))
	} else {
		// handler that just tells you where the real endpoint is
		http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
			w.WriteHeader(http.StatusNotFound)
			fmt.Fprintln(w, "you are probably looking for "+imagePngEndpoint)
		})
	}

	http.HandleFunc(imagePngEndpoint, renderImage)
	http.HandleFunc("/miis/image.glb", renderImage)
	http.HandleFunc("/miis/image.tga", renderImage)

	var err error

	var udsListener *net.Listener
	if unixSocket != "" {
		os.Remove(unixSocket)
		var udsListenerNew net.Listener
		udsListenerNew, err = net.Listen("unix", unixSocket)
		if err != nil {
			log.Fatalln("cannot listen on unix socket:", err)
		}
		udsListener = &udsListenerNew
		defer (*udsListener).Close()
		os.Chmod(unixSocket, 0666)
		log.Println("listening on unix socket path:", unixSocket)
	} else {
		log.Println("now listening on", host)
	}

	handler := logRequest(http.DefaultServeMux)

	if certFile != "" && keyFile != "" {
		if udsListener != nil {
			// listen on unix socket
			err = http.ServeTLS(*udsListener, handler, certFile, keyFile)
		} else {
			err = http.ListenAndServeTLS(host, certFile, keyFile, handler)
		}
	} else {
		// no handler because we defined HandleFunc
		if udsListener != nil {
			// listen on unix socket
			err = http.Serve(*udsListener, handler)
		} else {
			err = http.ListenAndServe(host, handler)
		}
	}
	// this will only be reached when either function returns
	log.Fatalln(err)
}

// BEGIN ACCESS LOG SECTION

const (
	// ANSI color codes for access logs
	ANSIReset     = "\033[0m"
	ANSIRed       = "\033[31m"
	ANSIGreen     = "\033[32m"
	ANSIYellow    = "\033[33m"
	ANSIPurple    = "\033[35m"
	ANSIFaint     = "\033[2m"
	ANSIBold      = "\033[1m"
	ANSICyan      = "\033[36m"
	ANSIBgRed     = "\033[101m"
	ANSIBgBlue    = "\033[104m"
	ANSIBgMagenta = "\033[105m"
)

func isColorTerminal() bool {
	// NOTE: hack
	return os.Getenv("TERM") == "xterm-256color"
}

// getClientIP retrieves the client IP address from the request,
// considering the X-Forwarded-For header if present.
func getClientIP(r *http.Request) string {
	if useXForwardedFor {
		xff := r.Header.Get("X-Forwarded-For")
		if xff != "" {
			ips := strings.Split(xff, ",")
			return strings.TrimSpace(ips[0])
		}
	}
	host, _, _ := net.SplitHostPort(r.RemoteAddr)
	return host
}

// responseWriter is a custom http.ResponseWriter that captures the status code
type responseWriter struct {
	http.ResponseWriter
	statusCode int
}

// newResponseWriter creates a new responseWriter
func newResponseWriter(w http.ResponseWriter) *responseWriter {
	return &responseWriter{w, http.StatusOK}
}

// WriteHeader captures the status code
func (rw *responseWriter) WriteHeader(code int) {
	rw.statusCode = code
	rw.ResponseWriter.WriteHeader(code)
}

// logRequest logs each request in Apache/Nginx standard format with ANSI colors
func logRequest(handler http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		start := time.Now()
		rw := newResponseWriter(w)
		handler.ServeHTTP(rw, r)
		status := rw.statusCode

		latency := time.Since(start)
		clientIP := getClientIP(r)

		if isColorTerminal() {
			statusColor := ANSIGreen

			// Determine the status color
			if status >= 400 && status < 500 {
				statusColor = ANSIYellow
			} else if status >= 500 {
				statusColor = ANSIRed
			}
			latencyColor := getLatencyGradientColor(latency)

			clientIPColor := ANSICyan
			if r.Header.Get("X-Forwarded-For") != "" {
				clientIPColor = ANSIBgMagenta
			}

			var query string
			if r.URL.RawQuery != "" {
				query += "?"
			}
			query += r.URL.RawQuery
			queryColored := colorQueryParameters(query)

			// so many colors.....
			/*fmt.Printf("%s%s%s - - [%s] \"%s %s %s%s %s%s%s%s\" %s%d%s %d \"%s%s%s\" \"%s%s%s\" %s%s%s\n",
				clientIPColor, clientIP, ANSIReset,
				start.Format("02/Jan/2006:15:04:05 -0700"),
				ANSIGreen, r.Method, r.URL.Path, queryColored, ANSIReset,
				ANSIFaint, r.Proto, ANSIReset,
				statusColor, status, ANSIReset,
				r.ContentLength,
				ANSIPurple, r.Referer(), ANSIReset,
				ANSIFaint, r.UserAgent(), ANSIReset,
				latencyColor, latency, ANSIReset,
			)*/
			fmt.Println(clientIPColor + clientIP + ANSIReset +
				" - - [" + start.Format("02/Jan/2006:15:04:05 -0700") + "] \"" +
				ANSIGreen + r.Method + " " + r.URL.Path + queryColored + " " + ANSIReset +
				ANSIFaint + r.Proto + ANSIReset + "\" " +
				statusColor + fmt.Sprint(status) + ANSIReset + " " +
				fmt.Sprint(r.ContentLength) + " \"" +
				ANSIPurple + r.Referer() + ANSIReset + "\" \"" +
				ANSIFaint + r.UserAgent() + ANSIReset + "\" " +
				latencyColor + fmt.Sprint(latency) + ANSIReset)
		} else {
			// apache/nginx request format with latency at the end
			/*fmt.Printf("%s - - [%s] \"%s %s %s\" %d %d \"%s\" \"%s\" %s\n",
				clientIP,
				start.Format("02/Jan/2006:15:04:05 -0700"),
				r.Method, r.RequestURI, r.Proto,
				status,
				r.ContentLength,
				r.Referer(),
				r.UserAgent(),
				latency,
			)*/
			fmt.Println(clientIP + " - - [" + start.Format("02/Jan/2006:15:04:05 -0700") + "] \"" +
				r.Method + " " + r.RequestURI + " " + r.Proto + "\" " +
				fmt.Sprint(status) + " " + fmt.Sprint(r.ContentLength) + " \"" +
				r.Referer() + "\" \"" + r.UserAgent() + "\" " +
				fmt.Sprint(latency))
		}
	})
}

// Color ranges for latency gradient
var latencyColors = []string{
	"\033[38;5;39m",  // Blue
	"\033[38;5;51m",  // Light blue
	"\033[38;5;27m",  // Added color (Dark blue)
	"\033[38;5;82m",  // Green
	"\033[38;5;34m",  // Added color (Forest green)
	"\033[38;5;154m", // Light green
	"\033[38;5;220m", // Yellow
	"\033[38;5;208m", // Orange
	"\033[38;5;198m", // Light red
}

// getLatencyGradientColor returns a gradient color based on the latency
func getLatencyGradientColor(latency time.Duration) string {
	millis := latency.Milliseconds()
	// Define latency thresholds
	thresholds := []int64{40, 60, 85, 100, 150, 230, 400, 600}

	for i, threshold := range thresholds {
		if millis < threshold {
			return latencyColors[i]
		}
	}
	return latencyColors[len(latencyColors)-1]
}

// getLatencyColor returns a color based on the latency
/*func getLatencyColor(latency time.Duration) string {
	millis := latency.Milliseconds()
	if millis < 100 {
		return ANSIBgBlue
	} else if millis < 500 {
		return ANSIBgBlue
	} else {
		return ANSIBgRed
	}
}*/

// colorQueryParameters colors the query parameters
func colorQueryParameters(query string) string {
	if query == "" {
		return ""
	}
	// NOTE: the question mark and first query key are colored the same
	params := strings.Split(query, "&")
	var coloredParams []string
	for _, param := range params {
		keyValue := strings.Split(param, "=")
		if len(keyValue) == 2 {
			coloredParams = append(coloredParams, fmt.Sprintf("%s%s%s=%s%s%s", ANSICyan, keyValue[0], ANSIReset, ANSIYellow, keyValue[1], ANSIReset))
		} else {
			coloredParams = append(coloredParams, param)
		}
	}
	return strings.Join(coloredParams, "&")
}

// END ACCESS LOG SECTION

func beginTimeMeasure() *time.Time {
	if !loggingEnabled {
		return nil
	}
	time := time.Now()
	return &time
}
func logTimeSincePrintfln(inTime *time.Time, printString string) {
	if inTime == nil {
		return
	}
	ms := time.Since(*inTime).Milliseconds()
	log.Printf(printString+"\n", ms)
}

// decodeBase64 decodes a Base64 string, handling both standard and URL-safe Base64.
func decodeBase64(s string) ([]byte, error) {
	// Normalize URL-safe Base64 by replacing '-' with '+' and '_' with '/'
	s = strings.ReplaceAll(s, "-", "+")
	s = strings.ReplaceAll(s, "_", "/")

	// Add padding if necessary
	switch len(s) % 4 {
	case 2:
		s += "=="
	case 3:
		s += "="
	}

	return base64.StdEncoding.DecodeString(s)
}

// isHex checks if a string is a valid hexadecimal encoded string
func isHex(s string) bool {
	_, err := hex.DecodeString(s)
	return err == nil
}

func isConnectionRefused(err error) bool {
	return errors.Is(err, syscall.ECONNREFUSED) ||
		// WSAECONNREFUSED on windows
		errors.Is(err, syscall.Errno(10061))
}

//func handleConnectionRefused(err error) {
//	if !isConnectionRefused(err) {
//		return
//	}
//}

// fetchDataFromDB fetches the data from the database for a given NNID
func fetchDataFromDB(nnid string) ([]byte, error) {
	normalizedNnid := normalizeNnid(nnid)
	query := "SELECT data FROM nnid_to_mii_data_map WHERE normalized_nnid = ? LIMIT 1"
	var data []byte
	err := db.QueryRow(query, normalizedNnid).Scan(&data)
	if err != nil {
		return nil, err
	}
	return data, nil
}

// normalizeNnid normalizes the NNID by converting to lowercase and removing special characters
func normalizeNnid(nnid string) string {
	return strings.ToLower(strings.ReplaceAll(strings.ReplaceAll(strings.ReplaceAll(nnid, "-", ""), "_", ""), ".", ""))
}

// if a socket response starts with this it
// is always read out to the api response
const socketErrorPrefix = "ERROR: "

// sendRenderRequest sends the render request to the render server
// It returns the first KB and a reader for the data.
func sendRenderRequest(request RenderRequest) ([]byte, io.Reader, error) {
	// Serialize the RenderRequest struct
	var buffer bytes.Buffer
	err := binary.Write(&buffer, binary.LittleEndian, request)
	if err != nil {
		return nil, nil, err
	}

	// Connect to the render server
	conn, err := net.Dial("tcp", upstreamTCP)
	if err != nil {
		return nil, nil, err
	}

	// Send the render request
	_, err = conn.Write(buffer.Bytes())
	if err != nil {
		conn.Close()
		return nil, nil, err
	}

	// Buffer the first KB of the response
	// The caller will use this to parse a header or error
	initialBuffer := make([]byte, 1024)
	_, err = io.ReadFull(conn, initialBuffer)
	if err != nil {
		conn.Close()
		return initialBuffer, nil, err
	}

	return initialBuffer, conn, nil
}

// handleRenderRequestError sends a response for an error from the renderer backend.
func handleRenderRequestError(w http.ResponseWriter, bufferData []byte, err error) {
	// Handling incomplete data response
	if err.Error() == "EOF" ||
		err.Error() == "unexpected EOF" || // from io.ReadFull
		errors.Is(err, syscall.ECONNRESET) {
		// if it begins with the prefix
		responseStr := string(bufferData)
		// Find the first occurrence of the null character (0 byte)
		if nullIndex := strings.Index(responseStr, string(byte(0))); nullIndex != -1 {
			// If the null character exists, slice the string until that point
			responseStr = responseStr[:nullIndex]
		}
		if strings.HasPrefix(responseStr, socketErrorPrefix) {
			// in this case, respond with that error
			http.Error(w, "renderer returned "+responseStr, http.StatusInternalServerError)
			return
		}

		http.Error(w, `incomplete data from backend :( render probably failed for one of the following reasons:
* FFLInitCharModelCPUStep failed: internal error or use of out-of-bounds parts
* FFLiVerifyCharInfoWithReason failed: mii data/CharInfo is invalid`, http.StatusInternalServerError)
		return
		// handle connection refused/backend down
	} else if isConnectionRefused(err) {
		msg := "OH NO!!! the site is up, but the renderer backend is down..."
		msg += "\nerror detail: " + err.Error()
		http.Error(w, msg, http.StatusInternalServerError)
		return
	}
	http.Error(w, "incomplete response from backend, error is: "+err.Error(), http.StatusInternalServerError)
}

// ssaaFactor controls the resolution and scale multiplier.
//const ssaaFactor = 2

var encoder = png.Encoder{CompressionLevel: png.BestSpeed}

// renderImage handles the /miis/image.png endpoint

func renderImage(ow http.ResponseWriter, r *http.Request) {

	w := ow // shortcut to original writer
	header := w.Header()
	if corsOrigin != "" {
		// Add permissive CORS headers.
		header.Set("Access-Control-Allow-Private-Network", "true")
		header.Set("Access-Control-Allow-Methods", "POST")
		header.Set("Access-Control-Allow-Headers", "Content-Type")

		header.Set("Access-Control-Allow-Origin", corsOrigin)
	}

	query := r.URL.Query()
	data := query.Get("data")
	typeStr := query.Get("type")
	if typeStr == "" {
		typeStr = "face"
	}
	expressionStr := query.Get("expression")
	widthStr := query.Get("width")
	usingDefaultWidth := false
	if widthStr == "" {
		usingDefaultWidth = true
		widthStr = "270" // default width
	}
	ssaaFactorStr := query.Get("scale")
	if ssaaFactorStr == "" {
		ssaaFactorStr = "2" // default scale is 2x
	}
	// overrideTexResolution is set if the user specified texResolution
	overrideTexResolution := false
	texResolutionStr := query.Get("texResolution")
	if texResolutionStr == "" {
		texResolutionStr = widthStr
	} else {
		overrideTexResolution = true
	}
	nnid := query.Get("nnid")

	resourceTypeStr := query.Get("resourceType")
	if resourceTypeStr == "" {
		resourceTypeStr = "default"
	}
	shaderTypeStr := query.Get("shaderType")
	if shaderTypeStr == "" {
		// TODO: should server determine default (-1)?
		shaderTypeStr = "wiiu"
	}
	bodyTypeStr := query.Get("bodyType")
	if bodyTypeStr == "" {
		bodyTypeStr = "default" // based on shader type
	}
	clothesColorStr := query.Get("clothesColor")
	if clothesColorStr == "" {
		clothesColorStr = "default"
	}
	pantsColorStr := query.Get("pantsColor")
	if pantsColorStr == "" {
		pantsColorStr = "default"
	}


	headwearIndexStr := query.Get("headwearIndex")
	if headwearIndexStr == "" {
		headwearIndexStr = "-1"
	}
	headwearColorStr := query.Get("headwearColor")
	if headwearColorStr == "" {
		headwearColorStr = ""
	}

	var responseFormat uint8 = 0
	if strings.HasSuffix(r.URL.Path, ".glb") {
		responseFormat = 1 // output is gltf
	} else if strings.HasSuffix(r.URL.Path, ".tga") {
		responseFormat = 2 // flips Y, emits BGRA
	}

	var storeData []byte
	var err error

	// Checking for required data
	/*if widthStr == "" {
		http.Error(w, "specify a width", http.StatusBadRequest)
		return
	}*/

	// inform them that pnid is not here yet
	if query.Get("pnid") != "" || query.Get("api_id") == "1" {
		http.Error(w, "no support for pretendo yet sorry, try this: https://mii-unsecure.ariankordi.net/mii_data/PN_Jon?api_id=1", http.StatusNotImplemented)
		return
	}

	// show usage
	if data == "" && nnid == "" {
		http.Error(w, "specify \"data\" as FFLStoreData/mii studio data in hex/base64, or \"nnid\" as an nnid (add &api_id=1 if it is a pnid), finally specify \"width\" as the resolution", http.StatusBadRequest)
		return
	}

	// Fetching data from database if nnid is provided
	if nnid != "" {
		if !mysqlAvailable {
			http.Error(w, "oh sh!t sorry for gaslighting you man, the nnid_to_mii_data_map mysql database is not set up sorry", http.StatusInternalServerError)
			return
		}
		storeData, err = fetchDataFromDB(nnid)
		if err != nil && err != sql.ErrNoRows {
			http.Error(w, fmt.Sprintf("Failed to fetch data from database: %v", err), http.StatusInternalServerError)
			return
		}
		if err == sql.ErrNoRows || storeData == nil {
			http.Error(w, "did not find that nnid bro", http.StatusNotFound)
			return
		}
	} else {
		// Decoding data from hex or base64
		data = strings.ReplaceAll(data, " ", "")
		if isHex(data) {
			storeData, err = hex.DecodeString(data)
		} else {
			storeData, err = decodeBase64(data)
		}
		if err != nil {
			http.Error(w, fmt.Sprintf("failed to decode data: %v", err), http.StatusBadRequest)
			return
		}
	}

	// Data length validation
	/*
		if len(storeData) != 96 {
			http.Error(w, "fflstoredata must be 96 bytes please", http.StatusBadRequest)
			return
		}
	*/
	// 46: size of studio data raw
	// 96: length of FFLStoreData
	if len(storeData) < 46 || len(storeData) > 96 {
		http.Error(w, "data length should be between 46-96 bytes", http.StatusBadRequest)
		return
	}

	// parse background color
	var bgColor color.RGBA
	// set default background color
	// NOTE: DEFAULT BACKGROUND COLOR IS NOT ALL ZEROES!!!!
	// IT IS TRANSPARENT WHITE. NOT USING THAT MAKES GLASSES TINTS WRONG
	bgColor = color.RGBA{R: 0xFF, G: 0xFF, B: 0xFF, A: 0x0}
	// taken from nwf-mii-cemu-toy miiPostHandler
	bgColorParam := query.Get("bgColor")
	// only process bgColor if it  exists
	if bgColorParam != "" {
		// this function will panic if length is 0
		bgColor, err = ParseHexColorFast(bgColorParam)
		// ignore alpha zero error
		if err != nil && err != errAlphaZero {
			http.Error(w, "bgColor format is wrong: "+err.Error(), http.StatusBadRequest)
			return
		}
	}

	instanceCountStr := query.Get("instanceCount")
	if instanceCountStr == "" {
		instanceCountStr = "1"
	}

	viewTypeStr := query.Get("type")
	if viewTypeStr == "" {
		viewTypeStr = "face"
	}

	viewType, exists := viewTypes[viewTypeStr]
	if !exists {
		http.Error(w, "we did not implement that view sorry", http.StatusBadRequest)
		return
	}

	modelTypeStr := query.Get("modelType")
	if modelTypeStr == "" {
		modelTypeStr = "normal"
	}
	modelType, exists := modelTypes[modelTypeStr]
	if !exists {
		http.Error(w, "valid model types: normal, hat, face_only", http.StatusBadRequest)
		return
	}

	splitModeStr := query.Get("splitMode")
	if splitModeStr == "" {
		splitModeStr = "none"
	}
	splitMode, exists := splitModes[splitModeStr]
	if !exists {
		http.Error(w, "valid split modes: none, front, back, both", http.StatusBadRequest)
		return
	}

	flattenNose := query.Get("flattenNose") != ""

	modelFlag := (1 << modelType)
	if flattenNose {
		modelFlag |= (1 << 3)
	}

	drawStageModeStr := query.Get("drawStageMode")
	if drawStageModeStr == "" {
		drawStageModeStr = "all"
	}
	drawStageMode, exists := drawStageModes[drawStageModeStr]
	if !exists {
		drawStageMode = 0
	}

	// NOTE: WHAT SHOULD BOOLS LOOK LIKE...???
	mipmapEnable := query.Get("mipmapEnable") != ""      // is present?
	lightEnable := query.Get("lightEnable") != "0"       // 0 = no lighting
	verifyCharInfo := query.Get("verifyCharInfo") != "0" // verify default

	var resourceType int
	resourceType, err = strconv.Atoi(resourceTypeStr)
	if err != nil {
		resourceType = getMapToInt(resourceTypeStr, resourceTypeMap, -1)
	}

	verifyCRC16 := query.Get("verifyCRC16") != "0" // 0 = no verify

	// Parsing and validating expression flag
	/*expressionFlag, err := strconv.Atoi(expressionFlagStr)
	if err != nil {
		http.Error(w, `oh, sorry... expression is the expression FLAG, not the name of the expression. find the values at https://github.com/ariankordi/nwf-mii-cemu-toy/blob/master/nwf-app/js/render-listener.js#L138`, http.StatusBadRequest)
		return
	}
	if expressionFlag < 1 {
		expressionFlag = 1
	}*/
	//expressionFlag := getExpressionFlag(expressionStr)
	// first try to parse the expression as int
	// should be safe as long as the server wraps around exp flags
	var expression int
	expression, err = strconv.Atoi(expressionStr)
	if err != nil {
		// now try to parse it as a string
		// this defaults to normal if it fails
		expression = getMapToInt(expressionStr, expressionMap, FFL_EXPRESSION_NORMAL)
	}

	if expression > 18 && resourceType == 0 {
		http.Error(w, "🥺🥺 🥺🥺🥺🥺 🥺🥺🥺, 🥺🥺🥺 😔 (Translation: Sorry, you cannot use this expression with the middle resource.)", http.StatusBadRequest)
		return
	}

	var expressionFlag FFLAllExpressionFlag //uint32 = 0
	// check for multiple expressions
	// (only applies for glTF!!!!!!!)
	if responseFormat == 1 && query.Has("expression") {
		expressionStr := query.Get("expression")
		// Split the comma-separated expressions
		expressions := strings.Split(expressionStr, ",")

		// Multiple expressions provided, compose the expression flag.
		for _, expressionStr := range expressions {
			// Trim spaces around each expression.
			expressionStr = strings.TrimSpace(expressionStr)

			// parse expression or use as an int
			var expression int
			expression, err = strconv.Atoi(expressionStr)
			if err != nil {
				// now try to parse it as a string
				// this defaults to normal if it fails
				expression = getMapToInt(expressionStr, expressionMap, FFL_EXPRESSION_NORMAL)
			}
			// set expression flag
			// Bitwise OR to combine all the flags
			//expressionFlag |= (1 << expression)
			SetExpressionFlagIndex(&expressionFlag, expression, true)
		}
	}

	var clothesColor int
	clothesColor, err = strconv.Atoi(clothesColorStr)
	if err != nil {
		clothesColor = getMapToInt(clothesColorStr, clothesColorMap, -1)
	}
	var pantsColor int
	pantsColor, err = strconv.Atoi(pantsColorStr)
	if err != nil {
		pantsColor = getMapToInt(pantsColorStr, pantsColorMap, -1)
	}

	var headwearColor int
	headwearColor, err = strconv.Atoi(headwearColorStr)
	if err != nil {
		headwearColor = getMapToInt(headwearColorStr, clothesColorMap, -1)
	}

	// Parsing and validating width
	width, err := strconv.Atoi(widthStr)
	if err != nil {
		http.Error(w, "width = resolution, int", http.StatusBadRequest)
		return
	}
	if width > 4096 {
		http.Error(w, "ok bro i set the limit to 4K", http.StatusBadRequest)
		return
	}

	// Parsing and validating texture resolution
	texResolution, err := strconv.Atoi(texResolutionStr)
	if err != nil || texResolution < 2 {
		http.Error(w, "texResolution is not a number", http.StatusBadRequest)
		return
	}

	// NOTE: excessive high texture resolutions crash (assert fail) the renderer
	if texResolution > 6000 {
		http.Error(w, "you cannot make texture resolution this high it will make your balls explode", http.StatusBadRequest)
		return
	}

	ssaaFactor, err := strconv.Atoi(ssaaFactorStr)
	if err != nil || ssaaFactor > 2 {
		http.Error(w, "scale must be a number less than 2", http.StatusBadRequest)
		return
	}

	headwearIndex, err := strconv.Atoi(headwearIndexStr)
	if err != nil {
		http.Error(w, "headwearIndex is not a number", http.StatusBadRequest)
		return
	}

	cameraRotateVec3i := [3]int16{0, 0, 0}

	// Read and parse query parameters
	if camXPos := query.Get("cameraXRotate"); camXPos != "" {
		x, err := strconv.Atoi(camXPos)
		if err == nil {
			cameraRotateVec3i[0] = int16(x)
		}
	}
	if camYPos := query.Get("cameraYRotate"); camYPos != "" {
		y, err := strconv.Atoi(camYPos)
		if err == nil {
			cameraRotateVec3i[1] = int16(y)
		}
	}
	if camZPos := query.Get("cameraZRotate"); camZPos != "" {
		z, err := strconv.Atoi(camZPos)
		if err == nil {
			cameraRotateVec3i[2] = int16(z)
		}
	}

	modelRotateVec3i := [3]int16{0, 0, 0}

	if charXPos := query.Get("characterXRotate"); charXPos != "" {
		x, err := strconv.Atoi(charXPos)
		if err == nil {
			modelRotateVec3i[0] = int16(x)
		}
	}
	if charYPos := query.Get("characterYRotate"); charYPos != "" {
		y, err := strconv.Atoi(charYPos)
		if err == nil {
			modelRotateVec3i[1] = int16(y)
		}
	}
	if charZPos := query.Get("characterZRotate"); charZPos != "" {
		z, err := strconv.Atoi(charZPos)
		if err == nil {
			modelRotateVec3i[2] = int16(z)
		}
	}

	lightDirectionVec3i := [3]int16{-1, -1, -1}

	if lightXDir := query.Get("lightXDirection"); lightXDir != "" {
		x, err := strconv.Atoi(lightXDir)
		if err == nil {
			lightDirectionVec3i[0] = int16(x)
		}
	}
	if lightYDir := query.Get("lightYDirection"); lightYDir != "" {
		y, err := strconv.Atoi(lightYDir)
		if err == nil {
			lightDirectionVec3i[1] = int16(y)
		}
	}
	if lightZDir := query.Get("lightZDirection"); lightZDir != "" {
		z, err := strconv.Atoi(lightZDir)
		if err == nil {
			lightDirectionVec3i[2] = int16(z)
		}
	}

	instanceCount, err := strconv.Atoi(instanceCountStr)
	if err != nil || instanceCount > 20 {
		http.Error(w, "instanceCount must be a number less than 20 or whatever i just set the maximum to", http.StatusBadRequest)
		return
	}

	// Also multiply it by two if it's particularly low...
	if width < 256 && !overrideTexResolution {
		texResolution *= 2
	}
	if drawStageMode == 3 { // mask only means it will return the texResolution
		if usingDefaultWidth {
			width = texResolution
		} else {
			texResolution = width
		}
		ssaaFactor = 1
	} else { // do not upscale aa or change texresolution then
		// Apply ssaaFactor to width
		width *= ssaaFactor
		// Also apply it to texResolution
		if !overrideTexResolution {
			texResolution *= ssaaFactor
		}
	}

	// convert bgColor to floats
	//bgColor4f := [4]float32{float32(bgColor.R), float32(bgColor.G), float32(bgColor.B), float32(bgColor.A)}

	bgColor4u8 := [4]uint8{bgColor.R, bgColor.G, bgColor.B, bgColor.A}


	shaderType, err := strconv.Atoi(shaderTypeStr)
	if err != nil {
		shaderType = getMapToInt(shaderTypeStr, shaderTypeMap, 0)
	}
	bodyType, err := strconv.Atoi(bodyTypeStr)
	if err != nil {
		bodyType = getMapToInt(bodyTypeStr, bodyTypeMap, -1)
	}

	// Creating the render request
	renderRequest := RenderRequest{
		Data:            [96]byte{},
		DataLength:      uint16(len(storeData)),
		ModelFlag:       uint8(modelFlag),
		ResponseFormat:  responseFormat,
		Resolution:      uint16(width),
		TexResolution:   int16(texResolution),
		ViewType:        uint8(viewType),
		ResourceType:    int8(resourceType),
		ShaderType:      uint8(shaderType),
		Expression:      uint8(expression),
		ExpressionFlag:  expressionFlag,
		CameraRotate:    cameraRotateVec3i,
		ModelRotate:     modelRotateVec3i,
		BackgroundColor: bgColor4u8,
		DrawStageMode:   uint8(drawStageMode),
		VerifyCharInfo:  verifyCharInfo,
		VerifyCRC16:     verifyCRC16,
		LightEnable:     lightEnable,
		ClothesColor:    int8(clothesColor),
		PantsColor:      int8(pantsColor),
		BodyType:        int8(bodyType),
		HeadwearIndex:   int8(headwearIndex),
		HeadwearColor:   int8(headwearColor),
		InstanceCount:   uint8(instanceCount),
		InstanceRotationMode: 0, // TODO
		LightDirection:  lightDirectionVec3i,
		SplitMode:       uint8(splitMode),
	}

	// Enabling mipmap if specified
	if mipmapEnable {
		renderRequest.TexResolution *= -1 // negative value means mipmap enabled
	}

	// Copying store data into the request data buffer
	copy(renderRequest.Data[:], storeData)

	// Time taken for sendRenderRequest to respond
	durationSendRequest := beginTimeMeasure()

	var bufferData []byte
	var reader io.Reader
	// Send the render request and receive the initial buffer and reader
	bufferData, reader, err = sendRenderRequest(renderRequest)
	if err != nil {
		handleRenderRequestError(w, bufferData, err)
		return
	}

	fullReader := bufio.NewReader(io.MultiReader(bytes.NewReader(bufferData), reader)) // use bufio to allow discard

	if responseFormat == 1 { // gltf
		logTimeSincePrintfln(durationSendRequest, "Time for streamRenderRequest (glTF export): %d ms")
		// Read size from GLB header
		var glbHeader GLBHeader
		if err := binary.Read(bytes.NewReader(bufferData), binary.LittleEndian, &glbHeader); err != nil {
			http.Error(w, "failed to parse glb header from backend for some reason: "+err.Error(), http.StatusInternalServerError)
			return
		}
		// all we care about is the length
		glbSize := strconv.Itoa(int(glbHeader.Length))
		// set content-length from it
		header.Set("Content-Length", glbSize)

		// now set filename
		filename := time.Now().Format("2006-01-02_15-04-05-")
		if nnid != "" {
			filename += nnid
		} else {
			filename += "mii-data"
		}
		filename += ".glb"
		header.Add("Content-Disposition", "attachment; filename="+filename)

		// Stream the data directly to the HTTP response without buffering
		_, err = io.Copy(w, fullReader)
		if err != nil {
			handleRenderRequestError(w, bufferData, err)
			return
		}

		return // done copying the gltf response
	} else {
		logTimeSincePrintfln(durationSendRequest, "Time for sendRenderRequest: %d ms")
	}

	// If no error, interpret initial buffer as TGA header
	var tgaHeader TGAHeader
	if err := binary.Read(bytes.NewReader(bufferData), binary.LittleEndian, &tgaHeader); err != nil {
		http.Error(w, "failed to parse tga header from backend for some reason: "+err.Error(), http.StatusInternalServerError)
		return
	}
	fullReader.Discard(18) // tga header length, move past the tga reader

	bytesPerPixel := int(tgaHeader.BitsPerPixel) / 8
	imageDataSize := int(tgaHeader.Width) * int(tgaHeader.Height) * bytesPerPixel

	imageData := make([]byte, imageDataSize)
	if _, err := io.ReadFull(fullReader, imageData); err != nil {
		handleRenderRequestError(w, bufferData, err)
		return
	}

	logTimeSincePrintfln(durationSendRequest, "Time from send to image read full: %d ms")

	// Create an image directly using the read data
	img := &image.NRGBA{
		Pix: imageData,
		// NOTE: ig this assumes it could be not rgba
		// but we are... composing rgba
		Stride: int(tgaHeader.Width) * bytesPerPixel,
		Rect:   image.Rect(0, 0, int(tgaHeader.Width), int(tgaHeader.Height)),
	}

	if ssaaFactor != 1 {
		// Start measuring time for scaling the image
		startScaling := beginTimeMeasure()
		// Scale down image by the ssaaFactor
		width := int(tgaHeader.Width) / ssaaFactor
		height := int(tgaHeader.Height) / ssaaFactor
		scaledImg := image.NewNRGBA(image.Rect(0, 0, width, height))
		// Use draw.ApproxBiLinear method
		// TODO: try better scaling but this is already pretty fast
		draw.ApproxBiLinear.Scale(scaledImg, scaledImg.Bounds(), img, img.Bounds(), draw.Over, nil)
		// replace the image with the scaled version
		img = scaledImg

		// Time taken for sendRenderRequest to respond
		logTimeSincePrintfln(startScaling, "Time to scale the image: %d ms")
	}

	if responseFormat == 2 { // tga
		// change tga header to reflect img
		tgaHeader.Width = int16(img.Rect.Dx())
		tgaHeader.Height = int16(img.Rect.Dy())
		tgaHeader.BitsPerPixel = 32 // NRGBA

		// size is deterministic so set it
		imageDataSize := int(img.Rect.Dx()) * int(img.Rect.Dy()) * 4 // NRGBA
		size := imageDataSize + 18 // tga header size
		header.Set("Content-Length", strconv.Itoa(size))
		header.Set("Content-Type", "image/tga")

		if err := binary.Write(w, binary.LittleEndian, &tgaHeader); err != nil {
			http.Error(w, "failed to write out tga header: "+err.Error(), http.StatusInternalServerError)
			return
		}

		pixReader := bytes.NewReader(img.Pix)
		_, err = io.Copy(w, pixReader)
		if err != nil {
			http.Error(w, "failed to write out tga data: "+err.Error(), http.StatusInternalServerError)
			return
		}

		return // done copying the tga response
	}
	// otherwise encode as png
	startEncoding := beginTimeMeasure()

	// Sending the image as a PNG response
	header.Set("Content-Type", "image/png")

	encoder.Encode(w, img) // png.Encoder
	logTimeSincePrintfln(startEncoding, "Time to encode PNG: %d ms")
}

// Expression constants
const (
	FFL_EXPRESSION_NORMAL                = 0
	FFL_EXPRESSION_SMILE                 = 1
	FFL_EXPRESSION_ANGER                 = 2
	FFL_EXPRESSION_SORROW                = 3
	FFL_EXPRESSION_SURPRISE              = 4
	FFL_EXPRESSION_BLINK                 = 5
	FFL_EXPRESSION_OPEN_MOUTH            = 6
	FFL_EXPRESSION_HAPPY                 = 7
	FFL_EXPRESSION_ANGER_OPEN_MOUTH      = 8
	FFL_EXPRESSION_SORROW_OPEN_MOUTH     = 9
	FFL_EXPRESSION_SURPRISE_OPEN_MOUTH   = 10
	FFL_EXPRESSION_BLINK_OPEN_MOUTH      = 11
	FFL_EXPRESSION_WINK_LEFT             = 12
	FFL_EXPRESSION_WINK_RIGHT            = 13
	FFL_EXPRESSION_WINK_LEFT_OPEN_MOUTH  = 14
	FFL_EXPRESSION_WINK_RIGHT_OPEN_MOUTH = 15
	FFL_EXPRESSION_LIKE                  = 16
	FFL_EXPRESSION_LIKE_WINK_RIGHT       = 17
	FFL_EXPRESSION_FRUSTRATED            = 18
)

var shaderTypeMap = map[string]int{
	//"default":     -1,
	"wiiu":            0,
	"switch":          1,
	"miitomo":         2,
	"wiiu_blinn":      3,
	"ffliconwithbody": 4,
}

var bodyTypeMap = map[string]int{
	"default":    -1,
	"wiiu":       0,
	"switch":     1,
	"miitomo":    2,
	"fflbodyres": 3,
	"3ds":        4,
}

// Map of expression strings to their respective flags
// NOTE: Mii Studio expression parameters, which
// this aims to be compatible with, use
// the opposite direction for all wink expressions.
var expressionMap = map[string]int{
	"surprise":              FFL_EXPRESSION_SURPRISE,
	"surprise_open_mouth":   FFL_EXPRESSION_SURPRISE_OPEN_MOUTH,
	"wink_left_open_mouth":  FFL_EXPRESSION_WINK_RIGHT_OPEN_MOUTH,
	"like":                  FFL_EXPRESSION_LIKE,
	"anger_open_mouth":      FFL_EXPRESSION_ANGER_OPEN_MOUTH,
	"blink_open_mouth":      FFL_EXPRESSION_BLINK_OPEN_MOUTH,
	"anger":                 FFL_EXPRESSION_ANGER,
	"like_wink_left":        FFL_EXPRESSION_LIKE_WINK_RIGHT,
	"happy":                 FFL_EXPRESSION_HAPPY,
	"blink":                 FFL_EXPRESSION_BLINK,
	"smile":                 FFL_EXPRESSION_SMILE,
	"sorrow_open_mouth":     FFL_EXPRESSION_SORROW_OPEN_MOUTH,
	"wink_right":            FFL_EXPRESSION_WINK_LEFT,
	"sorrow":                FFL_EXPRESSION_SORROW,
	"normal":                FFL_EXPRESSION_NORMAL,
	"like_wink_right":       FFL_EXPRESSION_LIKE,
	"wink_right_open_mouth": FFL_EXPRESSION_WINK_LEFT_OPEN_MOUTH,
	"smile_open_mouth":      FFL_EXPRESSION_HAPPY,
	"frustrated":            FFL_EXPRESSION_FRUSTRATED,
	"surprised":             FFL_EXPRESSION_SURPRISE,
	"wink_left":             FFL_EXPRESSION_WINK_RIGHT,
	"open_mouth":            FFL_EXPRESSION_OPEN_MOUTH,
	"puzzled":               FFL_EXPRESSION_SORROW, // assuming PUZZLED is similar to SORROW
	"normal_open_mouth":     FFL_EXPRESSION_OPEN_MOUTH,
}

var clothesColorMap = map[string]int{
	// NOTE: solely based on mii studio consts, not FFL enums
	"default":     -1,
	"red":         0,
	"orange":      1,
	"yellow":      2,
	"yellowgreen": 3,
	"green":       4,
	"blue":        5,
	"skyblue":     6,
	"pink":        7,
	"purple":      8,
	"brown":       9,
	"white":       10,
	"black":       11,
}

var pantsColorMap = map[string]int{
	"default": -1,
	"gray":    0,
	"blue":    1,
	"red":     2,
	"gold":    3,
	"body":    4,
	"none":    5,
}

var resourceTypeMap = map[string]int{
	"default":   -1, // server will select preferred
	"middle":    0,  // FFL_RESOURCE_TYPE_MIDDLE
	"high":      1,  // FFL_RESOURCE_TYPE_HIGH
	"very_high": 2,
	"low":       3,
}

func getMapToInt(input string, theMap map[string]int, defaultValue int) int {
	input = strings.ToLower(input)
	if value, exists := theMap[input]; exists {
		return value
	}
	// NOTE: mii studio rejects requests if the string doesn't match ..
	// .. perhaps we should do the same
	return defaultValue
}

// NOTE: BELOW IS IN nwf-mii-cemu-toy handlers.go

// adapted from https://stackoverflow.com/a/54200713
var errInvalidFormat = errors.New("invalid format")
var errAlphaZero = errors.New("alpha component is zero")

func ParseHexColorFast(s string) (c color.RGBA, err error) {
	// initialize A to full opacity
	c.A = 0xff

	hexToByte := func(b byte) byte {
		switch {
		case b >= '0' && b <= '9':
			return b - '0'
		// NOTE: the official mii studio api DOES NOT accept lowercase
		// ... however, this function is used for both studio format RGBA hex
		// as well as traditional RGB hex so we will forgive it
		case b >= 'a' && b <= 'f':
			return b - 'a' + 10
		case b >= 'A' && b <= 'F':
			return b - 'A' + 10
		}
		err = errInvalidFormat
		return 0
	}

	if s[0] == '#' {
		switch len(s) {
		case 7: // #RRGGBB
			c.R = hexToByte(s[1])<<4 + hexToByte(s[2])
			c.G = hexToByte(s[3])<<4 + hexToByte(s[4])
			c.B = hexToByte(s[5])<<4 + hexToByte(s[6])
		// TODO: is this format really necessary to have?
		case 4: // #RGB
			c.R = hexToByte(s[1]) * 17
			c.G = hexToByte(s[2]) * 17
			c.B = hexToByte(s[3]) * 17
		default:
			err = errInvalidFormat
		}
	} else {
		// Assuming the string is 8 hex digits representing RGBA without '#'
		if len(s) != 8 {
			err = errInvalidFormat
			return
		}

		// Parse RGBA
		r := hexToByte(s[0])<<4 + hexToByte(s[1])
		g := hexToByte(s[2])<<4 + hexToByte(s[3])
		b := hexToByte(s[4])<<4 + hexToByte(s[5])
		a := hexToByte(s[6])<<4 + hexToByte(s[7])

		// Only set RGB if A > 0
		c.R, c.G, c.B, c.A = r, g, b, a
		if a > 0 {
			// alpha is zero, meaning transparent
			err = errAlphaZero
		}
	}

	return
}
