import struct
import socket
import time
import base64
from flask import Flask, request, send_file, jsonify, make_response
from PIL import Image
import io
import argparse
import numpy as np

try:
    import fpnge
    FPNGE_AVAILABLE = True
except ImportError as e:
    print(e)
    FPNGE_AVAILABLE = False

try:
    import mysql.connector
    MYSQL_AVAILABLE = True
except ImportError as e:
    print(e)
    MYSQL_AVAILABLE = False

import colorsys

def shift_hue(image, target_color):
    # Convert target color to HSV
    target_hsv = colorsys.rgb_to_hsv(*target_color[:3])

    # Convert image to numpy array
    np_img = np.array(image)

    # Normalize RGB values to [0, 1]
    np_img_rgb = np_img[:, :, :3] / 255.0

    # Convert RGB to HSV
    hsv_img = np.apply_along_axis(lambda rgb: colorsys.rgb_to_hsv(*rgb), 2, np_img_rgb)

    # Calculate hue shift
    current_hue = colorsys.rgb_to_hsv(0.451, 0.157, 0.678)[0]
    hue_shift = target_hsv[0] - current_hue

    # Apply hue shift
    hsv_img[:, :, 0] = (hsv_img[:, :, 0] + hue_shift) % 1.0

    # Convert HSV back to RGB
    rgb_img = np.apply_along_axis(lambda hsv: colorsys.hsv_to_rgb(*hsv), 2, hsv_img)

    # Denormalize RGB values back to [0, 255]
    rgb_img = (rgb_img * 255).astype(np.uint8)

    # Combine the RGB channels with the original alpha channel
    result_img = np.zeros_like(np_img)
    result_img[:, :, :3] = rgb_img
    result_img[:, :, 3] = np_img[:, :, 3]

    return Image.fromarray(result_img)

app = Flask(__name__)

# Global variable for the body image
body_image = None

# Define the RenderRequest struct
class RenderRequest:
    def __init__(self, store_data, resolution=1024, tex_resolution=1024, is_head_only=False, expression_flag=1):
        self.store_data = bytes(store_data)
        self.resolution = resolution
        self.tex_resolution = tex_resolution
        self.is_head_only = is_head_only
        self.expression_flag = expression_flag

    def pack(self):
        return struct.pack(
            '96sII?I',
            self.store_data,
            self.resolution,
            self.tex_resolution,
            self.is_head_only,
            self.expression_flag
        )

def send_render_request(store_data, resolution, tex_resolution, is_head_only, expression_flag, host='localhost', port=12346):
    request = RenderRequest(store_data, resolution, tex_resolution, is_head_only, expression_flag)
    packed_request = request.pack()

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((host, port))
        s.sendall(packed_request)

        # Receive the data for buffer and buffer2
        buffer_size = resolution * resolution * 4  # RGBA
        buffer2_size = resolution * resolution * 4  # float32 depth
        total_size = buffer_size + buffer2_size

        received_data = bytearray()
        while len(received_data) < total_size:
            packet = s.recv(total_size - len(received_data))
            if not packet:
                break
            received_data.extend(packet)

        s.close()

    if len(received_data) < buffer_size:
        return None, None

    buffer_data = received_data[:buffer_size]
    buffer2_data = received_data[buffer_size:]

    return buffer_data, buffer2_data

def load_rgba_buffer(buffer_data, width, height):
    return [struct.unpack_from('4B', buffer_data, offset=i*4) for i in range(width*height)]

def load_depth_buffer(buffer2_data, width, height):
    return [struct.unpack_from('f', buffer2_data, offset=i*4)[0] for i in range(width*height)]

# Favorite colors as RGB tuples
FAVORITE_COLORS = [
    (0.824, 0.118, 0.078),  # Red
    (1.000, 0.431, 0.098),  # Orange
    (1.000, 0.847, 0.125),  # Yellow
    (0.471, 0.824, 0.125),  # Light Green
    (0.000, 0.471, 0.188),  # Green
    (0.039, 0.282, 0.706),  # Light Blue
    (0.235, 0.667, 0.871),  # Blue
    (0.961, 0.353, 0.490),  # Pink
    (0.451, 0.157, 0.678),  # Purple
    (0.282, 0.220, 0.094),  # Brown
    (0.878, 0.878, 0.878),  # White
    (0.094, 0.094, 0.078)   # Black
]


def blend_images(head_rgba, head_depth, resolution, favorite_color_index, threshold=0.986):  #0.6):
    global body_image
    BODY_IMAGE_ORIGINAL_RESOLUTION = 1600

    if favorite_color_index != 20:
        target_color = FAVORITE_COLORS[favorite_color_index]
        body_image = shift_hue(body_image, target_color)

    # Calculate proportional scaling for the specified resolution
    scale_ratio = resolution / BODY_IMAGE_ORIGINAL_RESOLUTION
    new_width = int(body_image.width * scale_ratio)
    new_height = int(body_image.height * scale_ratio)

    # Resize the body image proportionally
    body_image_resized = body_image.resize((new_width, new_height), Image.LANCZOS)

    # Create an output image
    result_image = head_rgba[:]

    # Calculate the position to place the body image (bottom middle)
    start_x = (resolution - body_image_resized.width) // 2
    start_y = resolution - body_image_resized.height

    body_image_resized_px = list(body_image_resized.getdata())

    for y in range(body_image_resized.height):
        for x in range(body_image_resized.width):
            index = (start_y + y) * resolution + (start_x + x)
            body_pixel = body_image_resized_px[y * body_image_resized.width + x]
            head_pixel = head_rgba[index]
            depth_value = head_depth[index]

            if depth_value > threshold and body_pixel[3] > 0:  # Only use body pixel if it's not fully transparent
                alpha = body_pixel[3] / 255.0
                new_pixel = tuple(
                    int(body_pixel[i] * alpha + head_pixel[i] * (1 - alpha)) for i in range(4)
                )
                result_image[index] = new_pixel

    result_image_pil = Image.new('RGBA', (resolution, resolution))
    result_image_pil.putdata(result_image)

    return result_image_pil

def normalize_nnid(nnid):
    return nnid.lower().translate(str.maketrans('', '', '-_.'))

def fetch_data_from_db(nnid):
    if not MYSQL_AVAILABLE:
        return None

    try:
        connection = mysql.connector.connect(
            host='localhost',
            user='miis',
            password='miis',
            database='miis'
        )
        cursor = connection.cursor()
        normalized_nnid = normalize_nnid(nnid)
        cursor.execute("SELECT data FROM nnid_to_mii_data_map WHERE normalized_nnid = %s LIMIT 1;", (normalized_nnid,))
        result = cursor.fetchone()
        cursor.close()
        connection.close()

        if result:
            return result[0]  # return the binary data
    except mysql.connector.Error as err:
        print(f"Error: {err}")
        return None

    return None


@app.route('/render.png', methods=['GET'])
def render_image():
    data = request.args.get('data')
    type_ = request.args.get('type', 'face')
    expression_flag = request.args.get('expression', '1')
    width = request.args.get('width', '1024')
    tex_resolution = request.args.get('texResolution', width)
    nnid = request.args.get('nnid')
    color_index = request.args.get('clothesColor', 'default')  # Default to purple

    if not data and not nnid:
        return make_response('specify data as FFLStoreData in Base64, or nnid as an nnid', 400)

    if nnid:
        if not MYSQL_AVAILABLE:
            return make_response('oh sh!t sorry for gaslighting you man, the nnid_to_mii_data_map mysql database is not set up sorry', 500)
        store_data = fetch_data_from_db(nnid)
        if not store_data:
            return make_response('did not find that nnid bro', 404)
    else:
        try:
            store_data = base64.b64decode(data)
        except Exception as e:
            return make_response('could not decode your base64: ' + str(e), 400)

    if len(store_data) != 96:
        return make_response('fflstoredata must be 96 bytes please', 400)

    is_head_only = type_ == 'face_only'

    try:
        expression_flag = int(expression_flag)
    except ValueError:
        return make_response('oh, sorry... expression is the expression FLAG, not the name of the expression. find the values <a href="https://github.com/ariankordi/nwf-mii-cemu-toy/blob/master/nwf-app/js/render-listener.js#L138">here</a>', 400)
    expression_flag = 1 if expression_flag < 1 else expression_flag

    try:
        width = int(width)
    except ValueError:
        return make_response('width = resolution, int, no limit on this lmao,', 400)

    if width > 4095:
        return make_response('ok little n**a i set the limit to 4K', 400)

    if color_index == 'default':
        # TODO GET FAVORITE COLOR FROM FFLiMiiDataCore
        color_index = 20
    else:
        try:
            color_index = int(color_index)
        except ValueError:
            return make_response('clothesColor is not a number', 400)

    try:
        tex_resolution = int(tex_resolution)
    except ValueError:
        return make_response('texResolution is not a number', 400)

    # Send the render request and receive buffer and buffer2 data
    try:
        buffer_data, buffer2_data = send_render_request(store_data, width, tex_resolution, is_head_only, expression_flag)
    except ConnectionRefusedError:
        return make_response('upstream tcp server is down :(', 502)

    if buffer_data is None or buffer2_data is None:
        # TODO: rethink this
        return make_response('''incomplete data from backend :( render probably failed bc FFLInitCharModelCPUStep failed... probably because data is invalid
<details>
<summary>
TODO: to make this error better here are the steps where the error is discarded:
</summary>
<pre>
* RootTask::calc_ responds to socket
* Model::initialize makes model nullptr
* Model::setCharModelSource_ calls initializeCpu_
* Model::initializeCpu_ calls FFLInitCharModelCPUStep
  - FFLResult is discarded here
* FFLInitCharModelCPUStep...
* FFLiInitCharModelCPUStep...
* FFLiCharModelCreator::ExecuteCPUStep
* FFLiDatabaseManager::PickupCharInfo
now, PickupCharInfo calls:
* GetCharInfoFromStoreData, fails if StoreData is not big enough or its CRC16 fails - pretty simple.
* FFLiiVerifyCharInfo or FFLiIsNullMiiID are called.
  - i think FFLiIsNullMiiID is for if a mii is marked as deleted by setting its ID to null
  - FFLiiVerifyCharInfo -> FFLiVerifyCharInfoWithReason
    + FFLiVerifyCharInfoReason is discarded here
    + <b>FFLiVerifyCharInfoWithReason IS THE MOST LIKELY REASON</b>
</pre>
</details>
''', 500)

    head_rgba = load_rgba_buffer(buffer_data, width, width)
    head_depth = load_depth_buffer(buffer2_data, width, width)

    blended_image = blend_images(head_rgba, head_depth, width, color_index)

    img_io = io.BytesIO()
    if FPNGE_AVAILABLE:
        png = fpnge.fromPIL(blended_image)
        img_io.write(png)
    else:
        blended_image.save(img_io, 'PNG')

    img_io.seek(0)
    return send_file(img_io, mimetype='image/png')

def initialize_body_image():
    global body_image
    body_image_path = 'body purple 1600 op.tga'
    body_image = Image.open(body_image_path).convert('RGBA')

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Render Request Web Server")
    parser.add_argument('--host', type=str, default='0.0.0.0', help="Host for the web server")
    parser.add_argument('--port', type=int, default=5000, help="Port for the web server")
    args = parser.parse_args()

    initialize_body_image()
    app.run(host=args.host, port=args.port)
