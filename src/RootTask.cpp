#include <nn/ffl/FFLResourceType.h>
#include <nn/ffl/detail/FFLiCharInfo.h>
#include <nn/ffl/FFLiCreateID.h>
#include <EnumStrings.h>
#include <Model.h>
#include <RootTask.h>
#include <Types.h>

#include <filedevice/rio_FileDeviceMgr.h>
#include <gfx/rio_Window.h>
#include <gfx/rio_Graphics.h>

#include <rio.h>
#include <gpu/rio_RenderBuffer.h>
#include <gpu/rio_RenderTarget.h>

#include <gpu/rio_RenderState.h>
#include <gpu/win/rio_Texture2DUtilWin.h>

#include <gfx/mdl/res/rio_ModelCacher.h>

#include <nn/ffl/detail/FFLiCrc.h>
#include <TGAHeader.h>
#include <RenderTexture.h>
#include <HeadwearModel.h>

#include <string>

// Forward declarations
//void handleRenderRequest(char* buf, Model* pModel, int socket);

#ifndef NO_GLTF
void handleGLTFRequest(RenderRequest* renderRequest, Model* pModel, int socket);
#endif

// Static members.
const char* RootTask::sServerOnlyFlag     = nullptr;
const char* RootTask::sServerPort         = nullptr;
const char* RootTask::sResourceSearchPath = nullptr;
const char* RootTask::sResourceHighPath   = nullptr;

RootTask::RootTask()
    : ITask("FFL Testing")
    , mInitialized(false)
    , mSocketIsListening(false)
    // contains pointers
    , mResourceDesc()
    , mpShaders{ nullptr }
    , mProjMtx()
    , mProjMtxIconBody()
    , mCamera()
    , mCounter(0.0f)
    , mMiiCounter(0)
    , mpModel(nullptr)
    , mHeadwearAvailable(false)
{
#ifdef RIO_USE_OSMESA // off screen rendering
    sServerOnlyFlag = "1"; // force it truey
#endif
    rio::MemUtil::set(mpBodyModels, 0, sizeof(mpBodyModels));
}

#include <nn/ffl/FFLiMiiData.h>
#include <nn/ffl/FFLiMiiDataCore.h>
#include <nn/ffl/FFLiCreateID.h>

#if RIO_IS_WIN

// for opening ffsd folder
#include <filesystem>
#include <fstream>

#define STORE_DATA_FOLDER_PATH "place_ffsd_files_here"

// read Mii data from a folder
void RootTask::fillStoreDataArray_()
{
    // Path to the folder
    static const std::string folderPath = STORE_DATA_FOLDER_PATH;
    // Check if the folder exists
    if (!std::filesystem::exists(folderPath) || !std::filesystem::is_directory(folderPath))
        return; // folder is not usable, skip this
    for (const auto& entry : std::filesystem::directory_iterator(folderPath))
    {
        if (!entry.is_regular_file()
            || entry.file_size() < sizeof(charInfoStudio)
            || entry.path().filename().string().at(0) == '.')
            // skip: dotfiles, not regular/too small files
            continue;

        // read file contents into data
        std::ifstream file(entry.path(), std::ios::binary);
        if (!file.is_open())
            continue;

        std::vector<char> data;
        data.resize(entry.file_size());
        file.read(&data[0], entry.file_size());
        //if (file.gcount() == sizeof(FFLStoreData))
        mStoreDataArray.push_back(data);
        file.close();
    }
    RIO_LOG("loaded %zu files from %s into mStoreDataArray\n", mStoreDataArray.size(), folderPath.c_str());
}

// Second argument passed to listen() indicating
// the queue/backlog of requests, set to Apache default.
#define LISTEN_BACKLOG_DEFAULT 511 // httpd uses this value

#define PORT_DEFAULT 12346 // default port to listen on

// Setup socket to send data to, and print message.
void RootTask::setupSocket_()
{
#ifdef USE_SYSTEMD_SOCKET
    if (getenv("LISTEN_FDS"))
    {
        // systemd socket activation detected
        int n_fds = sd_listen_fds(0);
        if (n_fds > 0)
        {
            mServerFD = SD_LISTEN_FDS_START + 0; // Use only one socket
            mSocketIsListening = true;
            RIO_LOG("\033[1mUsing systemd socket activation, socket fd: %d\033[0m\n", mServerFD);

            sServerOnlyFlag = "1"; // force server only when using systemd socket
            return; // Exit the function as the socket is already set up
        }
        else
        {
            RIO_LOG("\033[1mLISTEN_FDS was passed in as %s but sd_listen_fds(0) returned %d so systemd socket will not be used.\033[0m\n", getenv("LISTEN_FDS"), n_fds);
        }
    }
#endif

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
    {
        perror("WSAStartup failed");
        rio::Exit();
        exit(EXIT_FAILURE);
    }
#endif // _WIN32

    // Creating socket file descriptor
    if ((mServerFD = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        rio::Exit();
        exit(EXIT_FAILURE);
    }

    const int opt = 1; // into setsockopt()

    // Attach socket to the address.
    if (setsockopt(mServerFD, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt)))
    {
        perror("setsockopt");
        rio::Exit();
        exit(EXIT_FAILURE);
    }

    mServerAddress.sin_family = AF_INET;
    mServerAddress.sin_addr.s_addr = INADDR_ANY;

    // Get port number from arguments or use the default.
    char portReminder[] =" \033[2m(change with --port)\033[0m"; // will be set to blank

    int port = PORT_DEFAULT; // default port
    if (sServerPort)
    {
        port = atoi(sServerPort);
        portReminder[0] = '\0'; // remove port reminder
    }

    // bind socket handle mServerFD with mServerAddress
    mServerAddress.sin_port = htons(port); // set port
    if (bind(mServerFD, (struct sockaddr *)&mServerAddress, sizeof(mServerAddress)) < 0)
    {
        perror("bind failed");
        RIO_LOG("\033[1m" \
        "TIP: Change the port of %d with the --port argument." \
        "\033[0m\n", port);
        rio::Exit();
        exit(EXIT_FAILURE);
    }

    s32 listenBacklog = LISTEN_BACKLOG_DEFAULT;

    if (listen(mServerFD, listenBacklog) < 0)
    {
        perror("listen");
        rio::Exit();
        exit(EXIT_FAILURE);
    }

    else
    {
#ifdef __APPLE__
        char serverOnlyReminder[] = "\033[1mWARNING: On macOS, when the window is open the server will either fail to respond to requests or emit glitchy images. So, make sure to use the --server argument to hide the window.\n\033[0m";
#else
        char serverOnlyReminder[] = "\033[1mRemember to use the --server argument to hide the window.\n\033[0m";
#endif
        // accept() blocks by default, this is needed
        // in server only mode but the mode will be
        // set to non-blocking without
        if (!sServerOnlyFlag)
        {
#ifdef _WIN32
            u_long mode = 1; // cannot be const
            ioctlsocket(mServerFD, FIONBIO, &mode);
#else
            fcntl(mServerFD, F_SETFL, O_NONBLOCK);
#endif // _WIN32
        }
        else
            // don't show the reminder with server only
            serverOnlyReminder[0] = '\0';

        mSocketIsListening = true;

        // print bold/blue, portReminder, serverOnlyReminder
        RIO_LOG("\033[1m" \
        "tcp server listening on port %d\033[0m" \
        "%s\n" \
        "%s",
        port, portReminder, serverOnlyReminder);
    }
}
#endif

void RootTask::loadResourceFiles_()
{
    // Initialize mResourceDesc with zeroes.
    rio::MemUtil::set(&mResourceDesc, 0, sizeof(FFLResourceDesc));

#if RIO_IS_CAFE
    FSInit();
#endif // RIO_IS_CAFE

    for (u32 resourceType = 0; resourceType < FFL_RESOURCE_TYPE_MAX; resourceType++)
    {
        char resPath[256];

        FFLGetResourcePath(resPath,
            static_cast<u32>(sizeof(resPath)),
            static_cast<FFLResourceType>(resourceType), false); // last arg: linear gamma (LG) resource?

        std::vector<std::string> pathsToTry; // list of paths
        // Convert absolute resPath to std::filesystem::path.
        std::filesystem::path fsPath = resPath;

        if (resourceType == FFL_RESOURCE_TYPE_HIGH
            && RootTask::sResourceHighPath)
            // try that filename directly for high
            pathsToTry.push_back(RootTask::sResourceHighPath);

        if (RootTask::sResourceSearchPath)
        {
            std::string path = RootTask::sResourceSearchPath;
            // join resource search path
            pathsToTry.push_back(
                path + "/" + fsPath.filename().string());
        }

        // as well as the absolute resPath, try relative
        pathsToTry.push_back(fsPath.filename().string());
        pathsToTry.push_back(resPath);

        bool resLoaded = false;

        for (const std::string& path : pathsToTry)
        {
            //RIO_LOG("path to try: %s\n", path.c_str());
            rio::FileDevice::LoadArg arg;
            arg.path = path;
            arg.alignment = 0x2000;

            rio::NativeFileDevice* device = rio::FileDeviceMgr::instance()->getNativeFileDevice();
            u8* buffer = device->tryLoad(arg);
            if (buffer != nullptr)
            {
                RIO_LOG("Loaded resource %s from: %s\n", FFLResourceTypeToString(resourceType), arg.path.c_str());
                mResourceDesc.pData[resourceType] = buffer;
                mResourceDesc.size[resourceType] = arg.read_size;
                resLoaded = true;
                break;
            }
            else
            {
                const rio::RawErrorCode code = device->getLastRawError();
                RIO_LOG("Failed to load resource %s from: %s with error: %s\n", FFLResourceTypeToString(resourceType), arg.path.c_str(), rioRawErrorCodeToString(code));
            }
        }

        if (!resLoaded)
        {
            RIO_LOG("\033[1;31mNot able to use resource %s.\033[0m\n", FFLResourceTypeToString(resourceType));
        }
    }
}


FFLResourceType RootTask::getDefaultResourceType_()
{
    // prefer high by default
    const FFLResourceType preferred = FFL_RESOURCE_TYPE_HIGH;
#ifdef TEST_FFL_DEFAULT_RESOURCE_LOADING
    return preferred;
#else
    if (mResourceDesc.size[preferred])
        return preferred; // return preferred if it is there

    // iterate through all resource types
    for (u32 i = 0; i < FFL_RESOURCE_TYPE_MAX; i++)
        if (mResourceDesc.size[i])
            return FFLResourceType(i); // return first available resource

    RIO_ASSERT(false && "no resources available...???");
    return preferred;
#endif // TEST_FFL_DEFAULT_RESOURCE_LOADING
}

void RootTask::prepare_()
{
    mInitialized = false;

    FFLInitDesc init_desc = {
        .fontRegion = FFL_FONT_REGION_JP_US_EU,
        ._c = false,
        ._10 = true
    };

#ifndef TEST_FFL_DEFAULT_RESOURCE_LOADING

    loadResourceFiles_();

    FFLResult result = FFLInitResEx(&init_desc, &mResourceDesc);
#else
    FFLResult result = FFLInitResEx(&init_desc, nullptr);
#endif
    if (result != FFL_RESULT_OK)
    {
        fprintf(stderr, "FFLInitResEx() failed with result: %s\n", FFLResultToString(result));
#if RIO_RELEASE
        fprintf(stderr, "(Hint: build with RIO_DEBUG for more helpful information.)\n");
#endif
        rio::Exit();
        exit(EXIT_FAILURE);
    }

    RIO_ASSERT(FFLIsAvailable());
    FFLInitResGPUStep(); // No-op on Win but still needed

    initializeShaders_();

    // Create projection matrices.

    const rio::Window* window = rio::Window::instance();
    // Set projection matrix
    {
        // Calculate the aspect ratio based on the window dimensions
        f32 aspect = f32(window->getWidth()) / f32(window->getHeight());
        // Calculate the field of view (fovy) based on the given parameters
        f32 fovy = 2 * atan2f(43.2f / aspect, 500.0f);
        // C_MTXPerspective(Mtx44 m, f32 fovy, f32 aspect, f32 near, f32 far)
        // PerspectiveProjection(f32 near, f32 far, f32 fovy, f32 aspect)
        // RFLiMakeIcon: C_MTXPerspective(projMtx, fovy, aspect, 500.0f, 700.0f)
        // GetFaceMatrix: C_MTXPerspective(projMtx, 15.0, 1.0, 10.0, 1000.0);

        // Create perspective projection instance
        mProj = rio::PerspectiveProjection(
            500.0f,  // near
            1000.0f, // far
            fovy,    // fovy
            aspect   // aspect
        );
        // The near and far values define the depth range of the view frustum (500.0f to 700.0f)

        // Calculate matrix
        mProjMtx = mProj.getMatrix();

        mProjIconBody = rio::PerspectiveProjection(
            10.0f,
            1000.0f,
            rio::Mathf::deg2rad(15.0f),
            1.0f
        );
        mProjMtxIconBody = mProjIconBody.getMatrix();

    }

    // load body models
    static const char* cBodyModelsCSVPath = "body_models.csv";
    if (!BodyModelItem::populateArrayFromCSV(mpBodyModels, cBodyModelsCSVPath))
    {
        fprintf(stderr, "Failed to load body models from %s, cannot continue, failing.\n", cBodyModelsCSVPath);
        rio::Exit();
        exit(EXIT_FAILURE);
    }

    // load headwear models
    mHeadwearAvailable =
        mHeadwearList.loadFromCSV("headwear.csv");

#if RIO_IS_WIN
    fillStoreDataArray_();
    setupSocket_();
#ifndef RIO_NO_GLFW_CALLS
    // Set window aspect ratio, so that when resizing it will not change
    GLFWwindow* glfwWindow = rio::Window::instance()->getNativeWindow().getGLFWwindow();
    glfwSetWindowAspectRatio(glfwWindow, window->getWidth(), window->getHeight());
#endif // RIO_NO_GLFW_CALLS
#endif // RIO_IS_WIN

    mMiiCounter = 0;
    createModel_();

    mInitialized = true;
}

// amount of mii indexes to cycle through
// default source only has 6
// GetMiiDataNum()
s32 maxMiis = 6;

// create model for displaying on screen
void RootTask::createModel_()
{
    FFLCharModelSource modelSource;

    // default model source if there is no socket
#if RIO_IS_CAFE
        // use mii maker database on wii u
        modelSource.dataSource = FFL_DATA_SOURCE_OFFICIAL;
        // NOTE: will only use first 6 miis from mii maker database
#else
    FFLiCharInfo charInfo;
    if (!mStoreDataArray.empty())
    {
        // Use the custom Mii data array
        if (FFLResult result = pickupCharInfoFromData(&charInfo,
            &mStoreDataArray[mMiiCounter][0],
            static_cast<u32>(mStoreDataArray[mMiiCounter].size()),
            true // verifyCRC16
        ); result != FFL_RESULT_OK)
        {
            RIO_LOG("pickupCharInfoFromData failed on Mii counter: %d with result %s\n", mMiiCounter, FFLResultToString(result));
            mpModel = nullptr;
            mCounter = 0.0f;
            mMiiCounter = (mMiiCounter + 1) % maxMiis;
            return;
        }

        modelSource.index = 0;
        modelSource.dataSource = FFL_DATA_SOURCE_DIRECT_POINTER;
        modelSource.pBuffer = &charInfo;
        // limit current counter by the amount of custom miis
        maxMiis = static_cast<int>(mStoreDataArray.size());
    }
    else
    {
        // default mii source, otherwise known as guest miis
        modelSource.dataSource = FFL_DATA_SOURCE_DEFAULT;
        // guest miis are defined in FFLiDatabaseDefault.cpp
        // fetched from m_MiiDataOfficial, derived from the static array MII_DATA_CORE_RFL
#endif
        modelSource.index = mMiiCounter;
        modelSource.pBuffer = NULL;
#if !RIO_IS_CAFE
    }
#endif

    // limit current counter by the amount
    // of max miis (6 for default/guest miis)
    mMiiCounter = (mMiiCounter + 1) % maxMiis;


    Model::InitArg arg = {
        .desc = {
            .resolution = static_cast<FFLResolution>(FFL_RESOLUTION_TEX_256 | FFL_RESOLUTION_MIP_MAP_ENABLE_MASK),
            .allExpressionFlag = { .flags = { 1 << 0, 0, 0 } },
            .modelFlag = 1 << 0,
            .resourceType = FFL_RESOURCE_TYPE_HIGH,
        },
        .source = modelSource,
        .index = 0
    };

    mpModel = new Model();
    ShaderType shaderType = SHADER_TYPE_WIIU;//(mMiiCounter-1) % (SHADER_TYPE_MAX);

    //mpModel->mpHeadwear = new HeadwearModel(mHeadwearList.getByID(2));
    //mpModel->mpHeadwear->modifyCharInfoAndFlag(&charInfo, &arg.desc.modelFlag);
    if (!mpModel->initialize(arg, *mpShaders[shaderType]))
    {
        delete mpModel;
        mpModel = nullptr;
    }
    /*else
    {
        mpModel->setScale({ 1.f, 1.f, 1.f });
        //mpModel->setScale({ 1 / 16.f, 1 / 16.f, 1 / 16.f });
    }*/
    static const BodyType cBodyType = BODY_TYPE_WIIU_MIIBODYMIDDLE;
    mpModel->mpBody = new BodyModel(&mpBodyModels[cBodyType]);
    mpModel->mpBody->initialize(mpModel, PANTS_COLOR_GRAY);
    if (mpModel->mpHeadwear != nullptr)
        mpModel->mpHeadwear->initialize(mpModel, mpModel->getCharInfo()->favoriteColor);

    mCounter = 0.0f;
}

const std::string socketErrorPrefix = "ERROR: ";

// create model for render request
bool RootTask::createModel_(RenderRequest* req, int socket_handle)
{
    FFLiCharInfo charInfo;


    std::string errMsg;

    FFLResult pickupResult = pickupCharInfoFromData(&charInfo,
                                                    req->data,
                                                    req->dataLength,
                                                    req->verifyCRC16);

    if (pickupResult != FFL_RESULT_OK)
    {
        if (pickupResult == FFL_RESULT_FILE_INVALID) // crc16 fail
            errMsg = "Data CRC16 verification failed.\n";
        else
            errMsg = "Unknown data type (pickupCharInfoFromData failed)\n";

        RIO_LOG("%s", errMsg.c_str());
        errMsg = socketErrorPrefix + errMsg;
        send(socket_handle, errMsg.c_str(), static_cast<int>(errMsg.length()), 0);
        return false;
    }

    // VERIFY CHARINFO
    if (req->verifyCharInfo)
    {
        // get verify char info reason, don't verify name
        FFLiVerifyCharInfoReason verifyCharInfoReason =
            FFLiVerifyCharInfoWithReason(&charInfo, false);
        // I think I want to separate making the model
        // and picking up CharInfo from the request LATER
        // and then apply it when I do that

        if (verifyCharInfoReason != FFLI_VERIFY_CHAR_INFO_REASON_OK)
        {
            // CHARINFO IS INVALID, FAIL!
            errMsg = "FFLiVerifyCharInfoWithReason (data verification) failed: "
            + std::string(FFLiVerifyCharInfoReasonToString(verifyCharInfoReason))
            + "\n";
            RIO_LOG("%s", errMsg.c_str());
            errMsg = socketErrorPrefix + errMsg;
            send(socket_handle, errMsg.c_str(), static_cast<int>(errMsg.length()), 0);
            return false;
        }
/*
        if (FFLiIsNullMiiID(&charInfo.creatorID))
        {
            errMsg = "FFLiIsNullMiiID returned true (this data will not work on a real console)\n";
            RIO_LOG("%s", errMsg.c_str());
            errMsg = socketErrorPrefix + errMsg;
            send(socket_handle, errMsg.c_str(), errMsg.length(), 0);
            return false;
        }

        if (!FFLiIsValidMiiID(&charInfo.creatorID))
        {
            errMsg = "FFLiIsValidMiiID returned false (this data will not work on a real console)\n";
            RIO_LOG("%s", errMsg.c_str());
            errMsg = socketErrorPrefix + errMsg;
            send(socket_handle, errMsg.c_str(), errMsg.length(), 0);
            return false;
        }
*/
    }

    FFLCharModelSource modelSource = {
        // don't call PickupCharInfo or verify mii after we already did
        .dataSource = FFL_DATA_SOURCE_DIRECT_POINTER, // aka CharInfo/CharModel??
                                                      // or _SOURCE_BUFFER (verifies)
        .pBuffer = &charInfo,
        .index = 0 // needs to be initialized to zero
    };

    // initialize expanded expression flags to blank
    FFLAllExpressionFlag expressionFlag = { .flags = { 0, 0, 0 } }; //{ .flags = { 1 << FFL_EXPRESSION_NORMAL } };

    u32 modelFlag = static_cast<u32>(req->modelFlag);
    // This is set because we always initialize all flags to zero:
    modelFlag |= FFL_MODEL_FLAG_NEW_EXPRESSIONS;
    // NOTE: This flag is needed to use expressions past 31 ^^

    if (req->expressionFlag[0] != 0
        || req->expressionFlag[1] != 0
        || req->expressionFlag[2] != 0)
        //expressionFlag.flags[0] = req->expressionFlag;
        rio::MemUtil::copy(expressionFlag.flags, req->expressionFlag, sizeof(u32) * 3);
    else
        FFLSetExpressionFlagIndex(&expressionFlag, req->expression, true); // set that bit

    FFLResolution texResolution;
    if (req->texResolution < 0)
    { // if it is negative...
        texResolution = static_cast<FFLResolution>(
            static_cast<u32>(req->texResolution * -1) // remove negative
            | FFL_RESOLUTION_MIP_MAP_ENABLE_MASK); // enable mipmap
    }
    else
        texResolution = static_cast<FFLResolution>(req->texResolution);

#ifdef FFL_ENABLE_NEW_MASK_ONLY_FLAG
    // Enable special mode that will not initialize shapes.
    if (req->drawStageMode == DRAW_STAGE_MODE_MASK_ONLY)
        modelFlag |= FFL_MODEL_FLAG_NEW_MASK_ONLY;
#endif

    FFLResourceType resourceType = static_cast<FFLResourceType>(req->resourceType);

    // clamp minimum (-1) or maximum to default
    if (req->resourceType < 0 || req->resourceType >= FFL_RESOURCE_TYPE_MAX)
        // may consider "default resource for shader"
        resourceType = getDefaultResourceType_();

    // otherwise just fall through and use default
    Model::InitArg arg = {
        .desc = {
            .resolution = texResolution,
            .allExpressionFlag = expressionFlag,
            // model flag includes model type (required)
            // and flatten nose bit at pos 4
            .modelFlag = modelFlag,
            .resourceType = resourceType,
        },
        .source = modelSource,
        .index = 0
    };

    mpModel = new Model();

    const s32 headwearIndex = req->headwearIndex;
    if (headwearIndex >= 0)
    {
        HeadwearItem* pHeadwearItem = mHeadwearList.getByID(headwearIndex);
        if (pHeadwearItem == nullptr && headwearIndex != 0)
        // HACK: do not error if id is 0
        {
            if (!mHeadwearAvailable)
                // show different error if headwear is unavailable
                errMsg = "headwear.csv could not be loaded on this server so headwear is unavailable";
            else
            {
                errMsg = "unknown headwear index: "
                + std::to_string(headwearIndex)
                + "\n";
            }
            errMsg = socketErrorPrefix + errMsg;
            send(socket_handle, errMsg.c_str(), static_cast<int>(errMsg.length()), 0);
            return false;
        }
        if (pHeadwearItem != nullptr)
        {
            // initialize headwear
            mpModel->mpHeadwear = new HeadwearModel(pHeadwearItem);
            mpModel->mpHeadwear->modifyCharInfoAndFlag(&charInfo, &arg.desc.modelFlag);
        }
    }

    ShaderType whichShader = SHADER_TYPE_WIIU;
    if (req->shaderType < SHADER_TYPE_MAX)
        whichShader = static_cast<ShaderType>(req->shaderType);

    // this should happen after charinfo verification by the way
    if (req->shaderType == SHADER_TYPE_MIITOMO)
        // miitomo erroneously makes glasses larger so we will too
        // NOTE: REMOVE LATER..??
        charInfo.parts.glassScale += 1;

    // shortcut, check if one of the following are true:
    // using default body type which is going to be fflbodyres
    // or body type is fflbodyres
    else if ((req->bodyType == BODY_TYPE_DEFAULT_FOR_SHADER
          && req->shaderType == SHADER_TYPE_WIIU_FFLICONWITHBODY)
          || req->bodyType == BODY_TYPE_FFLBODYRES)
    {
        // this is roughly equivalent to no scale
        charInfo.build = 82;
        charInfo.height = 83;
    }

    if (!mpModel->initialize(arg, *mpShaders[whichShader]))
    {
        errMsg = "FFLInitCharModelCPUStep FAILED while initializing model: "
        + std::string(FFLResultToString(mpModel->getInitializeCpuResult()))
        + "\n";
        RIO_LOG("%s", errMsg.c_str());
        errMsg = socketErrorPrefix + errMsg;
        send(socket_handle, errMsg.c_str(), static_cast<int>(errMsg.length()), 0);
        delete mpModel;
        mpModel = nullptr;
        return false;
    }

    if (mpModel->mpHeadwear != nullptr)
    {
        // get headwear favorite color
        FFLFavoriteColor headwearColor = static_cast<FFLFavoriteColor>(req->headwearColor);
        if (headwearColor < 0 || headwearColor >= FFL_FAVORITE_COLOR_MAX)
            headwearColor = mpModel->getCharInfo()->favoriteColor;
        // initialize headwear with charmodel
        mpModel->mpHeadwear->initialize(mpModel, headwearColor);
    }

    mCounter = 0.0f;
    return true;
}

static void writeTGAHeaderToSocket(int socket, u32 width, u32 height, rio::TextureFormat textureFormat)
{
    const u8 bitsPerPixel = rio::TextureFormatUtil::getPixelByteSize(textureFormat) * 8;

    // create tga header for this texture
    TGAHeader header; // tga header size = 0x12
    // set all fields to 0 initially including unused ones
    rio::MemUtil::set(&header, 0, sizeof(TGAHeader));
    header.imageType = 2; // uncomp_true_color
    header.width = width;
    header.height = height;
    // NOTE: client infers rgba or rgb from bpp (ig type is not set for if it is BGR)
    header.bitsPerPixel = bitsPerPixel;
    header.imageDescriptor = 8;  // 32 = Flag that sets the image origin to the top left
    // nnas standard tgas set this to 8 to be upside down
    // tga header will be written to socket at the same time pixels are reads

    // write tga header out to the socket
    send(socket, reinterpret_cast<char*>(&header), sizeof(TGAHeader), 0);
}


rio::mdl::Model* RootTask::getBodyModel_(Model* pModel, BodyType type)
{
    RIO_ASSERT(type > -1); // make sure it does not stay -1

    FFLiCharInfo* pCharInfo = pModel->getCharInfo();
    FFLGender genderTmp = pCharInfo->gender;

    // Clamp the value of gender.
    const FFLGender gender = static_cast<FFLGender>(genderTmp % FFL_GENDER_MAX);

    // Select body model.
    rio::mdl::Model* model = mpBodyModels[type].mpModels[gender]; // Based on gender.

    RIO_ASSERT(model); // make sure it is not null
    return model;
}



// configures camera, proj mtx, uses height from charinfo...
// ... to handle the view type appropriately
void RootTask::setViewTypeParams(ViewType viewType, rio::LookAtCamera* pCamera, rio::PerspectiveProjection* proj, rio::BaseMtx44f* projMtx, f32* aspectHeightFactor, bool* isCameraPosAbsolute, bool* willDrawBody, FFLiCharInfo* pCharInfo)
{
    switch (viewType)
    {
        case VIEW_TYPE_FACE_ONLY:
            *willDrawBody = false;
            [[fallthrough]]; // goal is actually same view as face
                             // both cdn-mii 2.0.0 and 1.0.0 do this
        case VIEW_TYPE_FACE:
        {
            // if it has body then use the matrix we just defined
            *projMtx = mProjMtxIconBody;
            *proj = mProjIconBody;

            //RIO_LOG("x = %i, y = %i, z = %i\n", req->cameraRotate.x, req->cameraRotate.y, req->cameraRotate.z);
            /*
            rio::Vec3f fCameraPosition = {
                fmod(static_cast<f32>(req->cameraRotate.x), 360),
                fmod(static_cast<f32>(req->cameraRotate.y), 360),
                fmod(static_cast<f32>(req->cameraRotate.z), 360),
            };*/

            // FFLMakeIconWithBody view uses 37.05f, 415.53f
            // below values are extracted from wii u mii maker
            pCamera->pos() = { 0.0f, 34.0f, 411.181793f };
                                  // 33.016785f
            pCamera->at()  = { 0.0f, 34.3f, 0.0f };
                                  // 33.016785f
            pCamera->setUp({ 0.0f, 1.0f, 0.0f });
            break;
        }
        case VIEW_TYPE_NNMII_VARIABLEICONBODY:
        {
            *projMtx = mProjMtxIconBody;
            *proj = mProjIconBody;
            // nn::mii::VariableIconBody::StoreCameraMatrix values
            pCamera->pos() = { 0.0f, 37.0f, 380.0f };
            pCamera->at() = { 0.0f, 37.0f, 0.0f };
            pCamera->setUp({ 0.0f, 1.0f, 0.0f });
            break;
        }
        case VIEW_TYPE_FFLICONWITHBODY:
        {
            *projMtx = mProjMtxIconBody;
            *proj = mProjIconBody;
            // FFLMakeIconWithBody view
            pCamera->pos() = { 0.0f, 37.05f, 415.53f };
            pCamera->at() = { 0.0f, 37.05f, 0.0f };
            pCamera->setUp({ 0.0f, 1.0f, 0.0f });
            break;
        }
        case VIEW_TYPE_ALL_BODY:
        {
            *projMtx = mProjMtxIconBody;
            *proj = mProjIconBody;
            *isCameraPosAbsolute = true;
            // made to be closer to mii studio looking value but still not actually accurate
            //pCamera->pos() = { 0.0f, 50.0f, 805.0f };
            //pCamera->at() = { 0.0f, 98.0f, 0.0f };
            // initial values:
            pCamera->pos() = { 0.0f, 98.0f, 900.0f };
            pCamera->at() = { 0.0f, 105.0f, 0.0f };

            pCamera->setUp({ 0.0f, 1.0f, 0.0f });
            break;
        }
        case VIEW_TYPE_ALL_BODY_SUGAR: // like mii maker/nnid
        {
            static const f32 aspect = 3.0f / 4.0f;
            *aspectHeightFactor = 1.0f / aspect;

            rio::PerspectiveProjection projAllBodyAspect = mProjIconBody;
            projAllBodyAspect.setAspect(aspect);

            static const rio::BaseMtx44f projMtxAllBodyAspect = rio::BaseMtx44f(projAllBodyAspect.getMatrix());
            *projMtx = projMtxAllBodyAspect;
            *proj = projAllBodyAspect;

            *isCameraPosAbsolute = true;

            // NOTE: wii u mii maker does some strange
            // camera zooming, to make the character
            // bigger when it's shorter and smaller
            // when it's taller, purely based on height

            // this is an ATTEMPT??? to simulate that
            // via interpolation which is... meh

            const f32 scaleFactorY = BodyModel::calcBodyScale(pCharInfo->build, pCharInfo->height).y;

            // These camera parameters look right when the character is tallest
            const rio::Vector3f posStart = { 0.0f, 65.0f, 550.0f };
            const rio::Vector3f atStart = { 0.0f, 65.0f, 0.0f };

            // Likewise these look correct when it's shortest.
            const rio::Vector3f posEnd = { 0.0f, 75.0f, 850.0f };
            const rio::Vector3f atEnd = { 0.0f, 88.0f, 0.0f };

            // Calculate interpolation factor (normalized to range [0, 1])
            f32 t = (scaleFactorY - 0.5f) / (1.264f - 0.5f);



            // Interpolate between start and end positions
            rio::Vector3f pos = {
                posStart.x + t * (posEnd.x - posStart.x),
                posStart.y + t * (posEnd.y - posStart.y),
                posStart.z + t * (posEnd.z - posStart.z)
            };

            // Interpolate between start and end target positions
            rio::Vector3f at = {
                atStart.x + t * (atEnd.x - atStart.x),
                atStart.y + t * (atEnd.y - atStart.y),
                atStart.z + t * (atEnd.z - atStart.z)
            };



            // height = 127, 1.264
            //pCamera->pos() = { 0.0f, 75.0f, 850.0f };
            //pCamera->at() = { 0.0f, 88.0f, 0.0f }; // higher = model is lower
            // height = 0,   0.5
            //pCamera->pos() = { 0.0f, 65.0f, 550.0f }; // lower = closer
            //pCamera->at() = { 0.0f, 65.0f, 0.0f };

            pCamera->pos() = pos;
            pCamera->at() = at;

            //pCamera->pos() = { 0.0f, 9.0f, 900.0f };
            //pCamera->at() = { 0.0f, 6.0f, 0.0f };
            pCamera->setUp({ 0.0f, 1.0f, 0.0f });
            break;
        }
        default:
        {
            // default, face only (FFLMakeIcon)
            // use default if request is head only
            *projMtx = mProjMtx;
            *proj = mProj;
            *willDrawBody = false;

            pCamera->at() = { 0.0f, 34.5f, 0.0f };
            pCamera->setUp({ 0.0f, 1.0f, 0.0f });

            pCamera->pos() = { 0.0f, 34.5f, 600.0f };
            break;
        }
    }
}




// Convert degrees to radians
static rio::Vector3f convertVec3iToRadians3f(const int16_t x, const int16_t y, const int16_t z)
{
    rio::Vector3f radians;
    radians.x = rio::Mathf::deg2rad(fmod(static_cast<f32>(x), 360.0f));
    radians.y = rio::Mathf::deg2rad(fmod(static_cast<f32>(y), 360.0f));
    radians.z = rio::Mathf::deg2rad(fmod(static_cast<f32>(z), 360.0f));
    return radians;
}

// Calculate position based on spherical coordinates
static rio::Vector3f calculateCameraOrbitPosition(f32 radius, const rio::Vector3f& radians)
{
    return {
        radius * -std::sin(radians.y) * std::cos(radians.x),
        radius * std::sin(radians.x),
        radius * std::cos(radians.y) * std::cos(radians.x)
    };
}

// Calculate up vector based on z rotation
static rio::Vector3f calculateUpVector(const rio::Vector3f& radians)
{
    rio::Vector3f up;
    up.x = std::sin(radians.z);
    up.y = std::cos(radians.z);
    up.z = 0.0f;
    return up;
}

static f32 getTransformedZ(const rio::BaseMtx34f model_mtx, const rio::BaseMtx34f view_mtx)
{
    // Extract translation / object center from the model matrix.
    f32 modelCenter[4] = { // vector4 but we want to index it
        model_mtx.m[0][3], model_mtx.m[1][3], model_mtx.m[2][3], 1.0f
    };
    // Initialize Z value, which will be the Z split.
    f32 zSplit = 0.0f;
    // Transform model matrix/world to view space.
    for (s32 row = 0; row < 4; ++row)
        // Only Z axis/third row of view_mtx
        zSplit += -view_mtx.m[2][row] * modelCenter[row];
        // Use negative ^^ value for -Z forward/right handed

    // Transform world/model matrix to view space.
    /*
    rio::Vector3f modelViewCenter = {
        // x:
        view_mtx.m[0][0] * modelCenter.x +
                view_mtx.m[0][1] * modelCenter.y +
                view_mtx.m[0][2] * modelCenter.z +
                view_mtx.m[0][3],
        // y:
        view_mtx.m[1][0] * modelCenter.x +
                view_mtx.m[1][1] * modelCenter.y +
                view_mtx.m[1][2] * modelCenter.z +
                view_mtx.m[1][3],
        // z:
        view_mtx.m[2][0] * modelCenter.x +
                view_mtx.m[2][1] * modelCenter.y +
                view_mtx.m[2][2] * modelCenter.z +
                view_mtx.m[2][3]
        // (w is not needed)
    };
    */
    return zSplit;
}

// TODO: this is still using class instances: getBodyModel
void RootTask::handleRenderRequest(char* buf, Model** ppModel, int socket)
{
    // Cast pModel. ppModel is provided so that
    // it can be deleted from inside this function
    Model* pModel = *ppModel;

    if (pModel == nullptr)
    {
        // error was already sent by now?
        closesocket(socket);
        return;
    }
    RIO_LOG("handleRenderRequest: socket handle: %d\n", socket);

    // hopefully renderrequest is proper
    RenderRequest* req = reinterpret_cast<RenderRequest*>(buf);

    if (req->responseFormat == RESPONSE_FORMAT_GLTF_MODEL)
    {
#ifndef NO_GLTF
        ::handleGLTFRequest(req, pModel, socket);
#endif
        closesocket(socket);
        return;
    }

    // mask ONLY - which was already initialized, so nothing else needs to happen
    if (req->drawStageMode == DRAW_STAGE_MODE_MASK_ONLY)
    {
#ifdef FFL_NO_RENDER_TEXTURE
        RIO_ASSERT(false && "FFL_NO_RENDER_TEXTURE is enabled, but mask only draw mode relies on binding to/reading from it.");
#else
        FFLiCharModel* pCharModel = reinterpret_cast<FFLiCharModel*>(pModel->getCharModel());
        /* NOTE: Official FFL has the following methods:
         * FFLiGetCharInfoFromCharModel
         * FFLiGetFaceTextureFromCharModel
         * FFLiGetMaskTextureFromCharModel
         * If those were added to the decompilation
         * they would negate the need to do this.
         */

        // select mask texture for current expression
        const FFLiRenderTexture* pRenderTexture = pCharModel->maskTextures.pRenderTextures[pCharModel->expression];
        RIO_ASSERT(pRenderTexture != nullptr);

        pRenderTexture->pRenderBuffer->bind();

        rio::Texture2D* pTexture = pRenderTexture->pTexture2D;

        writeTGAHeaderToSocket(socket, pTexture->getWidth(), pTexture->getHeight(), pTexture->getTextureFormat());

        // NOTE the resolution of this is the texture resolution so that would have to match what the client expects
        copyAndSendRenderBufferToSocket(pTexture, socket, 1);

        // CharModel does not have shapes (maybe) and
        // should not be drawn anymore
#ifdef FFL_ENABLE_NEW_MASK_ONLY_FLAG
        if (pCharModel->charModelDesc.modelFlag & FFL_MODEL_FLAG_NEW_MASK_ONLY)
        {
            // when you make a model with this flag
            // it will actually make it so that it
            // will crash if you try to draw any shapes
            // so we are simply deleting the model
            delete pModel;
            *ppModel = nullptr;
        }
#endif // FFL_ENABLE_NEW_MASK_ONLY_FLAG

#endif // FFL_NO_RENDER_TEXTURE
        closesocket(socket);
        return;
    }


    RIO_LOG("instance count: %d\n", req->instanceCount);

    s32 instanceTotal = 1;

    s32 instanceCurrent = 0;
    f32 instanceParts; // only used if below is true: vv
    if (req->instanceCount > 1)
    {
        instanceTotal = req->instanceCount;
        instanceParts = 360.0f / instanceTotal;
    }

    // use charinfo for build and height
    FFLiCharInfo* pCharInfo = pModel->getCharInfo();

    // switch between two projection matrxies
    rio::BaseMtx44f projMtx; // set by setViewTypeParams
    rio::PerspectiveProjection proj = mProjIconBody; // ^^

    f32 aspectHeightFactor = 1.0f;
    bool isCameraPosAbsolute = false; // if it should not move the camera to the head
    bool willDrawBody = true; // and if should move camera

    rio::LookAtCamera camera;
    const ViewType viewType = static_cast<ViewType>(req->viewType);
    setViewTypeParams(viewType, &camera, &proj,
                      &projMtx, &aspectHeightFactor,
                      &isCameraPosAbsolute, &willDrawBody, pCharInfo);

    // Total width/height accounting for instance count.
    const u32 totalWidth = req->resolution;
    const u32 totalHeight = static_cast<u32>((ceilf(
        static_cast<f32>(req->resolution
        * aspectHeightFactor * instanceTotal)
    ) / 2) * 2);

    RIO_LOG("Total resolution: %dx%d\n", totalWidth, totalHeight);

    bool hasWrittenTGAHeader = false;

    instanceCountNewRender:

    setViewTypeParams(viewType, &camera,
                      &proj, &projMtx, &aspectHeightFactor,
                      &isCameraPosAbsolute, &willDrawBody, pCharInfo);

    // Update camera position
    const rio::Vector3f cameraPosInitial = camera.pos();
    const f32 radius = cameraPosInitial.z;

    rio::Vector3f cameraRotate = convertVec3iToRadians3f(req->cameraRotate[0], req->cameraRotate[1], req->cameraRotate[2]);
    rio::Vector3f modelRotate = convertVec3iToRadians3f(req->modelRotate[0], req->modelRotate[1], req->modelRotate[2]);


    if (instanceTotal > 1)
    {
        f32 instanceAngle = instanceCurrent * instanceParts;
        f32 instanceAngleRad = rio::Mathf::deg2rad(instanceAngle);
        RIO_LOG("instance %d rotation: %f (rad: %f)\n", instanceCurrent, instanceAngle, instanceAngleRad);

        switch (req->instanceRotationMode)
        {
            case INSTANCE_ROTATION_MODE_MODEL:
                modelRotate.y += instanceAngleRad;
                break;
            case INSTANCE_ROTATION_MODE_CAMERA:
                cameraRotate.y += instanceAngleRad;
                break;
            case INSTANCE_ROTATION_MODE_EXPRESSION:
                RIO_LOG("INSTANCE_ROTATION_MODE_EXPRESSION is not implemented\n");
                break;
        }
    }

    rio::Vector3f position = calculateCameraOrbitPosition(radius, cameraRotate);
    position.y += cameraPosInitial.y;
    const rio::Vector3f upVector = calculateUpVector(cameraRotate);

    BodyType bodyType = static_cast<BodyType>(req->bodyType);
    if (bodyType <= BODY_TYPE_DEFAULT_FOR_SHADER
        || bodyType >= BODY_TYPE_MAX)
        bodyType = cShaderTypeDefaultBodyType[req->shaderType % SHADER_TYPE_MAX];

    pModel->mpBody = new BodyModel(&mpBodyModels[bodyType]);
    PantsColor pantsColor = static_cast<PantsColor>(req->pantsColor);
    if (pantsColor <= PANTS_COLOR_DEFAULT_FOR_SHADER
        || pantsColor >= PANTS_COLOR_MAX)
        pantsColor = cShaderTypeDefaultPantsType[req->shaderType % SHADER_TYPE_MAX];


    if (willDrawBody)
    {
        // Initializes scale factors:
        pModel->mpBody->initialize(pModel, pantsColor);
    }

    camera.pos() = position;
    camera.setUp(upVector);

    rio::Matrix34f model_mtx = rio::Matrix34f::ident;


    rio::Matrix34f rotationMtx;
    rotationMtx.makeR(modelRotate);
    // apply rotation
    model_mtx.setMul(rio::Matrix34f::ident, rotationMtx);

    rio::Matrix34f view_mtx;
    camera.getMatrix(&view_mtx);

    if (willDrawBody)
    {
        rio::Matrix34f bodyHeadMatrix = pModel->mpBody->getHeadModelMatrix();
        // translate head to its location on the body
        model_mtx.setMul(model_mtx, bodyHeadMatrix);

        // applies translation AND ROTATION to the camera
        // note to self: this is matching more
        // with this: https://web.archive.org/web/20170521213000im_/https://cdn-mii.accounts.nintendo.com/1.0.0/miis/8ed7ef7660b3b2cf/image/1dd84ae2a599e70f-ebcc0c4f725b6b02.png?type=face&expression=normal&width=512&instanceCount=1&instanceRotationMode=model
        // but seemingly no rotation is done here?: https://debian.local:8445/assets/jasmine-sample-270-wiiu-no-aa.png

        if (!isCameraPosAbsolute)
        {
            rio::Matrix34f inverseModelMtx;
            inverseModelMtx.setInverse(bodyHeadMatrix);
            view_mtx.setMul(view_mtx, inverseModelMtx);
        }
/*
        if (!isCameraPosAbsolute)
        {
            rio::Vector3f translate = pModel->mpBody->getHeadTranslation();
            // Translate at, if camera is NOT absolute
            camera.at().setAdd(camera.at(), translate);
        }
*/
    }

    pModel->setMtxRT(model_mtx);


    // and NOTE that is like, not to be applied when
    // when camera pos is absolute thiiinkkk

    const SplitMode splitMode = static_cast<SplitMode>(req->splitMode);
    if (splitMode != SPLIT_MODE_NONE)
    {
        // Copy projection matrix and set near/far on it.
        rio::PerspectiveProjection projCopy = proj;

        const f32 zSplit = getTransformedZ(model_mtx, view_mtx);
        RIO_LOG("z split: %f\n", zSplit);

        if (splitMode == SPLIT_MODE_FRONT)
        {
            projCopy.setNear(projCopy.getNear());
            projCopy.setFar(zSplit);
        }
        else if (splitMode == SPLIT_MODE_BACK)
        {
            projCopy.setNear(zSplit);
            projCopy.setFar(projCopy.getFar());
        }
        // TODO: SPLIT_MODE_BOTH.. how are we going to do that.
        projMtx = projCopy.getMatrix();
    }

#if RIO_IS_WIN
    // if default gl clip control is being used, or the response needs flipped y for tga...
    bool flipY = false;

    // When RIO_NO_CLIP_CONTROL is not defined, we only flip Y if the response format requires it
    if (req->responseFormat == RESPONSE_FORMAT_TGA_BGRA_FLIP_Y)
        flipY = true;

#ifdef RIO_NO_CLIP_CONTROL
    // When RIO_NO_CLIP_CONTROL is defined, we need to flip Y to get the image right-side up
    flipY = !flipY;
#endif // RIO_NO_CLIP_CONTROL

    GLint prevFrontFace;

    if (flipY)
    {
        // Flip the Y-axis of the projection matrix
        projMtx.m[1][1] *= -1.f;
        // Save the current front face state
        RIO_GL_CALL(glGetIntegerv(GL_FRONT_FACE, &prevFrontFace));
        // Change the front face culling
        RIO_GL_CALL(glFrontFace(prevFrontFace == GL_CCW ? GL_CW : GL_CCW));
    }
#endif // RIO_IS_WIN

    pModel->setLightEnable(req->lightEnable);

    // Create the render buffer with the desired size

    s32 ssaaFactor =
#ifdef TRY_SCALING
    2;  // Super Sampling factor, e.g., 2 for 2x SSAA
#else
    1;
#endif
    const s32 iResolution = static_cast<s32>(req->resolution);
    const s32 width = iResolution * ssaaFactor;

    f32 fHeight = (ceilf( // round up
        (static_cast<float>(iResolution) * aspectHeightFactor)
        * static_cast<float>(ssaaFactor)
    ) / 2) * 2; // to nearest even number
    s32 height = static_cast<s32>(fHeight);

    RIO_LOG("Render buffer created with size: %dx%d\n", width, height);

    //rio::Window::instance()->getNativeWindow()->getColorBufferTextureFormat();
    rio::TextureFormat textureFormat = rio::TEXTURE_FORMAT_R8_G8_B8_A8_UNORM;
#if RIO_IS_WIN && defined(TRY_BGRA_RENDERBUFFER_FORMAT)
    // ig this works on opengl and is the teeniest tiniest bit more efficient
    // however golang does not support this and png, jpeg, webp aren't using this anyway so
    textureFormat = rio::TEXTURE_FORMAT_B8_G8_R8_A8_UNORM;
#elif RIO_IS_WIN //&& !defined(RIO_GLES) // not supported in gles core
    if (req->responseFormat == RESPONSE_FORMAT_TGA_BGRA_FLIP_Y
#ifdef RIO_GLES
        && GLAD_GL_EXT_texture_format_BGRA8888
#endif
    )
        textureFormat = rio::TEXTURE_FORMAT_B8_G8_R8_A8_UNORM;
#endif

    RenderTexture renderTexture(width, height, textureFormat);


    //const rio::Color4f clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
    // NOTE: this calls glViewport
    // make background color float from RGBA bytes
    rio::Color4f fBackgroundColor;
    //if (iconBGIsFavoriteColor) // TODO at some point
    //{
    //    const FFLColor favoriteColor = FFLGetFavoriteColor(pCharInfo->favoriteColor);
    //    fBackgroundColor = { favoriteColor.r, favoriteColor.g, favoriteColor.b, favoriteColor.a };
    //}
    //else
    //{
    fBackgroundColor = {
        static_cast<f32>(req->backgroundColor[0]) / 256,
        static_cast<f32>(req->backgroundColor[1]) / 256,
        static_cast<f32>(req->backgroundColor[2]) / 256,
        static_cast<f32>(req->backgroundColor[3]) / 256
    };
    //}
    renderTexture.clear(rio::RenderBuffer::CLEAR_FLAG_COLOR_DEPTH_STENCIL, fBackgroundColor);

    // Bind the render buffer
    renderTexture.bind();

    RIO_LOG("Render buffer bound.\n");

    // Set light direction.
    // Reset uniforms first
    pModel->getShader()->resetUniformsToDefault();
    // only set each if all axes are not zero

    // TODO maybe miitomo needs radians
    if (req->lightDirection[0] != 0
        || req->lightDirection[1] != 0
        || req->lightDirection[2] != 0)
    {
        /*
        rio::Vector3f lightDirection = {
            static_cast<f32>(req->lightDirection[0]),
            static_cast<f32>(req->lightDirection[1]),
            static_cast<f32>(req->lightDirection[2])
        };
        */
        rio::Vector3f lightDirection = convertVec3iToRadians3f(req->lightDirection[0], req->lightDirection[1], req->lightDirection[2]);
        pModel->getShader()->setLightDirection(lightDirection);
    }

    DrawStageMode drawStages = static_cast<DrawStageMode>(req->drawStageMode);

    // Render the first frame to the buffer
    if (drawStages == DRAW_STAGE_MODE_ALL
        || drawStages == DRAW_STAGE_MODE_OPA_ONLY
        || drawStages == DRAW_STAGE_MODE_XLU_DEPTH_MASK)
        pModel->drawOpa(view_mtx, projMtx);
    RIO_LOG("drawOpa rendered to the buffer.\n");

    // draw headwear if it is not null
    if (pModel->mpHeadwear != nullptr)
    {
        pModel->mpHeadwear->draw(model_mtx, view_mtx, projMtx);
    }

    // draw body?
    if (willDrawBody)
    {
        const FFLFavoriteColor originalFavoriteColor = pCharInfo->favoriteColor;
        if (req->clothesColor >= 0
            // verify favorite color is in range here bc it is NOT verified in drawMiiBodyREAL
            && req->clothesColor < FFL_FAVORITE_COLOR_MAX
        )
            // change favorite color after drawing opa
            pCharInfo->favoriteColor = static_cast<FFLFavoriteColor>(req->clothesColor);

        pModel->mpBody->draw(rotationMtx, view_mtx, projMtx);
        // restore original favorite color tho
        pCharInfo->favoriteColor = originalFavoriteColor;
    }


    // draw xlu mask only after body is drawn
    // in case there are elements of the mask that go in the body region
    if (drawStages == DRAW_STAGE_MODE_ALL
        || drawStages == DRAW_STAGE_MODE_XLU_ONLY
        || drawStages == DRAW_STAGE_MODE_XLU_DEPTH_MASK)
    {
        if (drawStages == DRAW_STAGE_MODE_XLU_DEPTH_MASK
            || drawStages == DRAW_STAGE_MODE_XLU_ONLY)
        {
            // Use faceline color as background color.
            const FFLColor facelineColor = FFLGetFacelineColor(pCharInfo->parts.facelineColor);
            // Clear color but not depth. This punches out
            // depth for DrawOpa, intended for overlaying
            // this image over a DrawOpa image.
            renderTexture.clear(rio::RenderBuffer::CLEAR_FLAG_COLOR, { facelineColor.r, facelineColor.g, facelineColor.b, fBackgroundColor.a });
            // ^^ use alpha from original background color

            // Color was cleared, now determine to clear depth
            if (drawStages == DRAW_STAGE_MODE_XLU_ONLY)
                renderTexture.clear(rio::RenderBuffer::CLEAR_FLAG_DEPTH_STENCIL, fBackgroundColor);
            // Bind renderbuffer again
            renderTexture.bind();
        }

        pModel->drawXlu(view_mtx, projMtx);
    }
    RIO_LOG("drawXlu rendered to the buffer.\n");


#ifdef ENABLE_BENCHMARK
    std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
    start = std::chrono::high_resolution_clock::now();
#endif
    if (!hasWrittenTGAHeader)
        writeTGAHeaderToSocket(socket, totalWidth, totalHeight, renderTexture.getColorFormat());

    copyAndSendRenderBufferToSocket(renderTexture.getColorTexture(), socket, ssaaFactor);
    hasWrittenTGAHeader = true;
#ifdef ENABLE_BENCHMARK
    end = std::chrono::high_resolution_clock::now();
    long long int duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    RIO_LOG("copyAndSendRenderBufferToSocket: %lld µs\n", duration);
#endif

#if RIO_IS_WIN
    // Restore OpenGL state if we modified it
    if (flipY)
    {
        // Restore the front face culling to its default state
        RIO_GL_CALL(glFrontFace(prevFrontFace));

        // Flip the Y-axis back
        projMtx.m[1][1] *= -1.f;
    }
#endif
    if (instanceCurrent < instanceTotal - 1)
    {
        instanceCurrent++;
        goto instanceCountNewRender; // jump back earlier
    }

    closesocket(socket);
    RIO_LOG("Closed socket %d.\n", socket);
}

void RootTask::calc_()
{
    if (!mInitialized)
        return;

#if RIO_IS_WIN
    char buf[RENDERREQUEST_SIZE];

    bool hasSocketRequest = false;

    const size_t addrlen = sizeof(mServerAddress);

    if (mSocketIsListening &&
        (mServerSocket = accept(mServerFD, (struct sockaddr *)&mServerAddress, (socklen_t*)&addrlen)) > 0)
    {
        int read_bytes =
            recv(mServerSocket, buf, RENDERREQUEST_SIZE, 0);

        if (read_bytes == RENDERREQUEST_SIZE)
        {
            delete mpModel;
            hasSocketRequest = true;

            RenderRequest* req = reinterpret_cast<RenderRequest*>(buf);

            if (!createModel_(req, mServerSocket))
            {
                mpModel = nullptr;
                mCounter = 0.0f;
            };
        }
        else
        {
            RIO_LOG("got a request of length %i (should be %d), dropping\n", read_bytes, static_cast<u32>(RENDERREQUEST_SIZE));
            closesocket(mServerSocket);
        }
    }
    else
    {
        // otherwise just fall through and use default
        // when mii is directly in front of the camera
#endif // RIO_IS_WIN
        if (!sServerOnlyFlag && mCounter >= rio::Mathf::pi2())
        {
            delete mpModel;
            createModel_();
        }
#if RIO_IS_WIN
    }
#endif


    if (hasSocketRequest)
    {
        handleRenderRequest(buf, &mpModel, mServerSocket);
        if (!sServerOnlyFlag)
        {
            rio::Window::instance()->makeContextCurrent();

            u32 width = rio::Window::instance()->getWidth();
            u32 height = rio::Window::instance()->getHeight();

            rio::Graphics::setViewport(0, 0, width, height);
            rio::Graphics::setScissor(0, 0, width, height);
            RIO_LOG("Viewport and scissor reset to window dimensions: %dx%d\n", width, height);
        }
        return;
    }

    if (!sServerOnlyFlag)
    {
        rio::Window::instance()->clearColor(0.2f, 0.3f, 0.3f, 1.0f);
        rio::Window::instance()->clearDepthStencil();
        //rio::Window::instance()->setSwapInterval(0);  // disable v-sync
    }

    if (mpModel == nullptr)
        return;

    // Distance in the XZ-plane from the center to the camera position
    f32 radius = 700.0f;
    // Define a constant position in the 3D space for the center position of the camera
    static const rio::Vector3f CENTER_POS = { 0.0f, 90.0f, radius };
    mCamera.at() = { 0.0f, 80.0f, 0.0f };
    mCamera.setUp({ 0.0f, 1.0f, 0.0f });
    // Move the camera around the target clockwise
    // Define the radius of the orbit in the XZ-plane (distance from the target)
    rio::Matrix34f model_mtx = rio::Matrix34f::ident;
    /*
    mCamera.pos().set(
        // Set the camera's position using the sin and cos functions to move it in a circle around the target
        std::sin(mCounter) * radius,
        CENTER_POS.y * std::sin(mCounter) * 7.5 - 30,
        // Add a minus sign to the cosine to spin CCW
        std::cos(mCounter) * radius
    );
    */
    mCamera.pos() = { CENTER_POS.x, CENTER_POS.y, radius };

    model_mtx.makeR({
        0.0f,
        mCounter,
        0.0f
    });

    mpModel->setMtxRT(model_mtx);

    // Increment the counter to gradually change the camera's position over time
    if (!sServerOnlyFlag)
    {
        mCounter += 1.f / 60;
    }

    // Get the view matrix from the camera, which represents the camera's orientation and position in the world
    rio::BaseMtx34f view_mtx;
    mCamera.getMatrix(&view_mtx);

    rio::Matrix34f rotationMtx = model_mtx;

    //model_mtx.setMul(rio::Matrix34f::ident, rotationMtx);
    if (mpModel->mpBody != nullptr)
        model_mtx.setMul(model_mtx,  mpModel->mpBody->getHeadModelMatrix());

    mpModel->setMtxRT(model_mtx);

    mpModel->drawOpa(view_mtx, mProjMtxIconBody);
    if (mpModel->mpHeadwear != nullptr)
        mpModel->mpHeadwear->draw(model_mtx, view_mtx, mProjMtxIconBody);
    if (mpModel->mpBody != nullptr)
        mpModel->mpBody->draw(rotationMtx, view_mtx, mProjMtxIconBody);
    mpModel->drawXlu(view_mtx, mProjMtxIconBody);
}

#ifndef NO_GLTF

#include <GLTFExportCallback.h>

void handleGLTFRequest(RenderRequest* req, Model* pModel, int socket)
{
    // Initialize ExportShader
    GLTFExportCallback exportShader;

    exportShader.SetCharModel(pModel->getCharModel());

    RIO_LOG("Created glTF export callback.\n");

    // Get the shader callback
    FFLShaderCallback callback = exportShader.GetShaderCallback();

    DrawStageMode drawStages = static_cast<DrawStageMode>(req->drawStageMode);

    if (req->drawStageMode == DRAW_STAGE_MODE_MASK_ONLY)
    {
        // only draw the xlu mask in this mode
        const FFLDrawParam* maskParam = FFLGetDrawParamXluMask(pModel->getCharModel());
        exportShader.Draw(*maskParam);
    }
    else
    {
        if (drawStages == DRAW_STAGE_MODE_ALL || drawStages == DRAW_STAGE_MODE_OPA_ONLY)
            FFLDrawOpaWithCallback(pModel->getCharModel(), &callback);

        if (drawStages == DRAW_STAGE_MODE_ALL || drawStages == DRAW_STAGE_MODE_XLU_ONLY)
            FFLDrawXluWithCallback(pModel->getCharModel(), &callback);
    }
    RIO_LOG("Drawn model to glTF callback data.\n");
    /*
    const std::time_t now = std::time(nullptr);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&now), "/dev/shm/%Y-%m-%d_%H-%M-%S.glb");
    std::string output = oss.str();
    RIO_LOG("outputting glb to: %s\n", output.c_str());
    exportShader.ExportModelToFile(output);
    */

    // Create a stream buffer to hold the GLTF model data
    std::ostringstream modelStream;

    // Export the GLTF model to the stream
    if (!exportShader.ExportModelToStream(&modelStream))
    {
        RIO_LOG("Failed to export GLTF model to stream.\n");
        return;
    }

    // Get the model data from the stream
    std::string modelData = modelStream.str();


    // Send the actual GLTF model data
    unsigned long totalSent = 0;
    const unsigned long fileSize = modelData.size();
    while (totalSent < fileSize)
    {
        long sent = send(socket, modelData.data() + totalSent, fileSize - totalSent, 0);
        if (sent < 0)
        {
            RIO_LOG("Failed to send GLTF data\n");
            return;
        }
        totalSent += sent;
    }
    RIO_LOG("Wrote %lu bytes out to socket.\n", fileSize);
}

#endif // NO_GLTF

void RootTask::exit_()
{
    if (!mInitialized)
        return;

    // we need to free stuff that's allocated in our program
    // ... which is usually all pointers

    delete mpModel; // FFLCharModel destruction must happen before FFLExit
    mpModel = nullptr;

    FFLExit();

    for (u32 type = 0; type < FFL_RESOURCE_TYPE_MAX; type++)
        if (mResourceDesc.size[type] != 0)
            rio::MemUtil::free(mResourceDesc.pData[type]);

    // delete all shaders that were initialized
    for (u32 type = 0; type < SHADER_TYPE_MAX; type++)
        delete mpShaders[type];

    mInitialized = false;
}
