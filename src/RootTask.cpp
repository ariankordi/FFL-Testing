#include <Model.h>
#include <RootTask.h>

#include <filedevice/rio_FileDeviceMgr.h>
#include <gfx/rio_Projection.h>
#include <gfx/rio_Window.h>
#include <gfx/rio_Graphics.h>
#include <gpu/rio_RenderState.h>

#include <string>
#include <array>

RootTask::RootTask()
    : ITask("FFL Testing")
    , mInitialized(false)
    , mpNoSpin(getenv("NO_SPIN"))
{
}

#if RIO_IS_WIN && defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#elif RIO_IS_WIN && !defined(__EMSCRIPTEN__)
#define closesocket close
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include <nn/ffl/FFLiMiiData.h>
#include <nn/ffl/FFLDatabase.h>

#if RIO_IS_WIN

// for opening ffsd folder
#include <filesystem>
#include <fstream>

// read Mii data from a folder
void RootTask::fillStoreDataArray_()
{
    // Path to the folder
    const std::string folderPath = "place_ffsd_files_here";
    // Check if the folder exists
    if (std::filesystem::exists(folderPath) && std::filesystem::is_directory(folderPath)) {
        for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
            if (entry.is_regular_file()
                && entry.file_size() >= sizeof(FFLStoreData)
                && entry.path().filename().string().at(0) != '.')
            {
                // Read the file content
                std::ifstream file(entry.path(), std::ios::binary);
                if (file.is_open()) {
                    std::vector<char> data;
                    data.resize(entry.file_size());
                    file.read(&data[0], entry.file_size());
                    //if (file.gcount() == sizeof(FFLStoreData)) {
                        mStoreDataArray.push_back(data);
                    //}
                    file.close();
                }
            }
        }
        RIO_LOG("Loaded %zu FFSD files into mStoreDataArray\n", mStoreDataArray.size());
    }
}

#ifndef __EMSCRIPTEN__

int server_fd, new_socket;
struct sockaddr_in address;
int opt = 1;
int addrlen = sizeof(address);

// Setup socket to send data to
void RootTask::setupSocket_()
{
    #ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        perror("WSAStartup failed");
        exit(EXIT_FAILURE);
    }
    #endif // ifdef _WIN32

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    // Get port number from environment or use default
    char portReminder[] ="\033[2m(you can change the port with the PORT environment variable)\033[0m\n";
    const char* env_port = getenv("PORT");
    int port = 12346; // default port
    if (env_port) {
        port = atoi(env_port);
        portReminder[0] = '\0';
    }
    // Forcefully attaching socket to the port
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        RIO_LOG("\033[1m" \
        "TIP: Change the default port of 12346 with the PORT environment variable" \
        "\033[0m\n");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    } else {
        // Set socket to non-blocking mode
#ifdef _WIN32
        u_long mode = 1;
        ioctlsocket(server_fd, FIONBIO, &mode);
#else
        fcntl(server_fd, F_SETFL, O_NONBLOCK);
#endif

        mSocketIsListening = true;

        RIO_LOG("\033[1m" \
        "tcp server listening on port %d\033[0m\n" \
        "%s",
        port, portReminder);
    }
}
#endif // __EMSCRIPTEN__

#endif

void RootTask::prepare_()
{
    mInitialized = false;

    FFLInitDesc init_desc;
    init_desc.fontRegion = FFL_FONT_REGION_JP_US_EU;
    init_desc._c = false;
    init_desc._10 = true;

#ifndef TEST_FFL_DEFAULT_RESOURCE_LOADING

#if RIO_IS_CAFE
    FSInit();
#endif // RIO_IS_CAFE

    {
        std::string resPath;
        resPath.resize(256);
        // Middle (now being skipped bc it is not even used here)
        {
            FFLGetResourcePath(resPath.data(), 256, FFL_RESOURCE_TYPE_MIDDLE, false);
            {
                rio::FileDevice::LoadArg arg;
                arg.path = resPath;
                arg.alignment = 0x2000;

                u8* buffer = rio::FileDeviceMgr::instance()->getNativeFileDevice()->tryLoad(arg);
                if (buffer == nullptr)
                {
                    RIO_LOG("NativeFileDevice failed to load: %s\n", resPath.c_str());
                    RIO_LOG("Skipping loading FFL_RESOURCE_TYPE_MIDDLE\n");
                    // I added a line that skips the resource if the size is zero
                    mResourceDesc.size[FFL_RESOURCE_TYPE_MIDDLE] = 0;
                } else {
                    mResourceDesc.pData[FFL_RESOURCE_TYPE_MIDDLE] = buffer;
                    mResourceDesc.size[FFL_RESOURCE_TYPE_MIDDLE] = arg.read_size;
                }
            }
        }
        //mResourceDesc.size[FFL_RESOURCE_TYPE_MIDDLE] = 0;
        // High, load from FFL path or current working directory
        {
            // Two different paths
            std::array<std::string, 2> resPaths = {"", "./FFLResHigh.dat"};
            resPaths[0].resize(256);
            FFLGetResourcePath(resPaths[0].data(), 256, FFL_RESOURCE_TYPE_HIGH, false);

            bool resLoaded = false;

            // Try two different paths
            for (const auto& resPath : resPaths) {
                rio::FileDevice::LoadArg arg;
                arg.path = resPath;
                arg.alignment = 0x2000;

                u8* buffer = rio::FileDeviceMgr::instance()->getNativeFileDevice()->tryLoad(arg);
                if (buffer != nullptr) {
                    mResourceDesc.pData[FFL_RESOURCE_TYPE_HIGH] = buffer;
                    mResourceDesc.size[FFL_RESOURCE_TYPE_HIGH] = arg.read_size;
                    resLoaded = true;
                    // Break when one loads successfully
                    break;
                } else {
                    RIO_LOG("NativeFileDevice failed to load: %s\n", arg.path.c_str());
                }
            }

            if (!resLoaded) {
                RIO_LOG("Was not able to load high resource!!!\n");
                RIO_LOG("\033[1;31mThe FFLResHigh.dat needs to be present, or else this program won't work. It will probably crash right now.\033[0m\n");
                RIO_ASSERT(false);
            }
        }
    }

    FFLResult result = FFLInitResEx(&init_desc, &mResourceDesc);
#else
    FFLResult result = FFLInitResEx(&init_desc, nullptr);
#endif
    if (result != FFL_RESULT_OK)
    {
        RIO_LOG("FFLInitResEx() failed with result: %d\n", (s32)result);
        RIO_ASSERT(false);
        return;
    }

    RIO_ASSERT(FFLIsAvailable());

    FFLInitResGPUStep();

    mShader.initialize();

    // Get window instance
    const rio::Window* const window = rio::Window::instance();
    // Set projection matrix
    {
        // Calculate the aspect ratio based on the window dimensions
        f32 aspect = f32(window->getWidth()) / f32(window->getHeight());
        // Calculate the field of view (fovy) based on the given parameters
        f32 fovy;
        if (f32(window->getWidth()) < f32(window->getHeight())) {
            fovy = 2 * atan2f(43.2f / aspect, 500.0f);
        } else {
            fovy = 2 * atan2f(43.2f, 500.0f);
        }
        // C_MTXPerspective(Mtx44 m, f32 fovy, f32 aspect, f32 near, f32 far)
        // PerspectiveProjection(f32 near, f32 far, f32 fovy, f32 aspect)
        // RFLiMakeIcon: C_MTXPerspective(projMtx, fovy, aspect, 500.0f, 700.0f)
        // GetFaceMatrix: C_MTXPerspective(projMtx, 15.0, 1.0, 10.0, 1000.0);

        // Create perspective projection instance
        rio::PerspectiveProjection proj(
            500.0f, // Near
            1200.0f, // Far
            fovy,// * 1.6f, // fovy
            aspect // Aspect ratio
        );
        // The near and far values define the depth range of the view frustum (500.0f to 700.0f)

        // Calculate matrix
        mProjMtx = proj.getMatrix();
    }


    // read Mii data from a folder
#if RIO_IS_WIN
    fillStoreDataArray_();
#ifndef __EMSCRIPTEN__
    setupSocket_();
    // Set window aspect ratio, so that when resizing it will not change
    GLFWwindow* glfwWindow = rio::Window::instance()->getNativeWindow().getGLFWwindow();
    glfwSetWindowAspectRatio(glfwWindow, window->getWidth(), window->getHeight());
#endif // __EMSCRIPTEN__
#endif // RIO_IS_WIN

    mMiiCounter = 0;
    createModel_();

    mInitialized = true;
}

// amount of mii indexes to cycle through
// default source only has 6
// GetMiiDataNum()
int maxMiis = 6;

// create model for displaying on screen
void RootTask::createModel_() {
    FFLCharModelSource modelSource;

    RIO_LOG("mMiiCounter: %d\n", mMiiCounter);

#ifdef USE_RANDOM_MIIS
    FFLiCharInfo charInfoRandom;
    FFLiGetRandomCharInfo(&charInfoRandom, FFL_GENDER_FEMALE, FFL_AGE_ADULT, FFL_RACE_WHITE);
    modelSource.dataSource = FFL_DATA_SOURCE_DIRECT_POINTER;
                             // needed bc the mii id is null
    modelSource.index = 0;
    modelSource.pBuffer = &charInfoRandom;
#else

    // default model source if there is no socket
    /*#if RIO_IS_CAFE
        // use mii maker database on wii u
        modelSource.dataSource = FFL_DATA_SOURCE_OFFICIAL;
        // NOTE: will only use first 6 miis from mii maker database
    #else
    if (!mStoreDataArray.empty()) {
        // Use the custom Mii data array
        modelSource.index = 0;
        modelSource.dataSource = FFL_DATA_SOURCE_STORE_DATA;
        modelSource.pBuffer = &mStoreDataArray[mMiiCounter][0];
        // limit current counter by the amount of custom miis
        maxMiis = static_cast<int>(mStoreDataArray.size());
    } else {
      */  // default mii source, otherwise known as guest miis
        modelSource.dataSource = FFL_DATA_SOURCE_DEFAULT;
        // guest miis are defined in FFLiDatabaseDefault.cpp
        // fetched from m_MiiDataOfficial, derived from the static array MII_DATA_CORE_RFL
    //#endif
        modelSource.index = mMiiCounter;
        modelSource.pBuffer = NULL;
    /*#if !RIO_IS_CAFE
    }
    #endif
*/
    // limit current counter by the amount
    // of max miis (6 for default/guest miis)
    mMiiCounter = (mMiiCounter + 1) % maxMiis;
#endif

    Model::InitArgStoreData arg = {
        .desc = {
            .resolution = FFLResolution(768 | FFL_RESOLUTION_MIP_MAP_ENABLE_MASK),
            .expressionFlag = 1 << 0,
            .modelFlag = 1 << 0 | 1 << 1 | 1 << 2,
            .resourceType = FFL_RESOURCE_TYPE_HIGH,
        },
        .source = modelSource
    };

    mpModel = new Model();
    if (!mpModel->initialize(arg, mShader)) {
        delete mpModel;
        mpModel = nullptr;
    } else {
        mpModel->setScale({ 1.f, 1.f, 1.f });
    }
    mCounter = 0.0f;
}

#include <codecvt>
#include <locale>

void RootTask::createModel_(char (*buf)[FFLICHARINFO_SIZE]) {
    FFLCharModelSource modelSource;
    //FFLStoreData storeData; // to be used in case you provide it
    // initialize charInfo (could also be done with memset)
    // this will be filled by FFLiStoreDataCFLToCharInfo
    FFLiCharInfo charInfo;
    // this will either be our blank new charInfo
    // ... or it will be redefined to the received buffer
    FFLiCharInfo* pCharInfo = &charInfo;


    FFLResult result = FFLiStoreDataCFLToCharInfo(pCharInfo,
                    reinterpret_cast<const FFLiStoreDataCFL&>(*buf));
    if (result != FFL_RESULT_OK) {
        RIO_LOG("input is not FFLStoreData (failed result %i), treating as FFLiCharInfo\n", result);
        // not store data, so buf is char info. right?
        pCharInfo = (FFLiCharInfo*)buf;
    }
    // at this point it should either be charinfo or converted from it
    //modelSource.dataSource = FFL_DATA_SOURCE_STORE_DATA;
    modelSource.dataSource = FFL_DATA_SOURCE_BUFFER; // i.e. CharInfo
    modelSource.index = 0;
    modelSource.pBuffer = pCharInfo; //&storeData;

    // Convert UTF-16 to UTF-8
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    // print in bold
    RIO_LOG("\033[1mLoaded Mii: %s\033[0m\n",
            convert.to_bytes((char16_t*) pCharInfo->name).c_str());
    // close connection which should happen as soon as we read it
    #if RIO_IS_WIN && !defined(__EMSCRIPTEN__)
        closesocket(new_socket);
    #endif
    // otherwise just fall through and use default

    Model::InitArgStoreData arg = {
        .desc = {
            .resolution = FFLResolution(1024),
            .expressionFlag = 1,
            .modelFlag = 1 << 0 | 1 << 1 | 1 << 2,
            .resourceType = FFL_RESOURCE_TYPE_HIGH,
        },
        .source = modelSource
    };

    mpModel = new Model();
    if (!mpModel->initialize(arg, mShader)) {
        RIO_LOG("FFLInitCharModelCPUStep FAILED while initializing model: %d\n", mpModel->getInitializeCpuResult());
        delete mpModel;
        mpModel = nullptr;
    } else {
        mpModel->setScale({ 1.f, 1.f, 1.f });
    }

    mCounter = 0.0f;
}



void RootTask::calc_()
{
    if (!mInitialized)
        return;

#if RIO_IS_WIN && !defined(__EMSCRIPTEN__)
    // maximum received is the size of FFLiCharInfo
    char buf[sizeof(FFLiCharInfo)];

    if (mSocketIsListening &&
        (new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) > 0) {
        // Assuming data directly received is FFLStoreData
        unsigned int read_bytes =
            //read(new_socket, &storeData, sizeof(FFLStoreData));
            // NOTE: this will store BOTH charinfo AND storedata so uh
            recv(new_socket, buf,
                // setting read maximum to the size of CharInfo
                sizeof(FFLiCharInfo), 0);

        if (read_bytes >= sizeof(FFLStoreData)) {
            createModel_(&buf);
        } else {
            RIO_LOG("got a request of length %d (should be %d), dropping\n", read_bytes, static_cast<u32>(sizeof(FFLiCharInfo)));
            closesocket(new_socket);
        }
    } else {
#endif // RIO_IS_WIN && !defined(__EMSCRIPTEN__)
        // otherwise just fall through and use default
        // when mii is directly in front of the camera
        if (mCounter >= rio::Mathf::pi2())
        {
            delete mpModel;
            createModel_();
        }
#if RIO_IS_WIN && !defined(__EMSCRIPTEN__)
    }
    // close connection which should happen as soon as we read it
    closesocket(new_socket);
#endif // RIO_IS_WIN && !defined(__EMSCRIPTEN__)

    // Distance in the XZ-plane from the center to the camera position
    static const f32 radius = 600.0f;
    // Define a constant position in the 3D space for the center position of the camera
    static const rio::Vector3f CENTER_POS = { 0.0f, 34.5f, radius };

    // Define the target position that the camera will look at (center of the scene)
    static const rio::Vector3f target = { 0.0f, 34.5f, 0.0f };
    mCamera.at() = target;

    // Define the up vector for the camera to maintain the correct orientation (Y-axis is up)
    static const rio::Vector3f cameraUp = { 0.0f, 1.0f, 0.0f };
    mCamera.setUp(cameraUp);

    // Move the camera around the target clockwise
    // Define the radius of the orbit in the XZ-plane (distance from the target)
    if (!mpNoSpin) {
        mCamera.pos().set(
            // Set the camera's position using the sin and cos functions to move it in a circle around the target
            std::sin(mCounter) * radius,
            CENTER_POS.y,
            // Add a minus sign to the cosine to spin CCW (same as SpinMii)
            std::cos(mCounter) * radius
        );
    } else {
        mCamera.pos() = CENTER_POS;
    }
    // Increment the counter to gradually change the camera's position over time
    mCounter += 1.f / 60;

    // Get the view matrix from the camera, which represents the camera's orientation and position in the world
    rio::BaseMtx34f view_mtx;
    mCamera.getMatrix(&view_mtx);

    rio::RenderState render_state;
    const char* transparency = getenv("TRANSPARENCY");
    if (transparency) {
        rio::Window::instance()->clearColor(0.0f, 0.0f, 0.0f, 0.0f);
        // enable blending for transparency
        render_state.setBlendEnable(true);
        render_state.setBlendFactorSrcAlpha(rio::Graphics::BlendFactor::BLEND_MODE_SRC_ALPHA);
        render_state.setBlendFactorDstAlpha(rio::Graphics::BlendFactor::BLEND_MODE_ONE_MINUS_SRC_ALPHA);
        render_state.apply();
    } else {
        // blue-gray-ish
        rio::Window::instance()->clearColor(0.2f, 0.3f, 0.3f, 1.0f);
    }
    rio::Window::instance()->clearDepthStencil();
    //rio::Window::instance()->setSwapInterval(0);  // disable v-sync

    if (mpModel != nullptr) {
        mpModel->drawOpa(view_mtx, mProjMtx);
        mpModel->drawXlu(view_mtx, mProjMtx);
    }
}

void RootTask::exit_()
{
    if (!mInitialized)
        return;

    delete mpModel; // FFLCharModel destruction must happen before FFLExit
    mpModel = nullptr;

    FFLExit();

    rio::MemUtil::free(mResourceDesc.pData[FFL_RESOURCE_TYPE_HIGH]);
    if (mResourceDesc.size[FFL_RESOURCE_TYPE_MIDDLE] != 0) {
      rio::MemUtil::free(mResourceDesc.pData[FFL_RESOURCE_TYPE_MIDDLE]);
    }

    mInitialized = false;
}
