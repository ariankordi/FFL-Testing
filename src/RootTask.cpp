#include <Model.h>
#include <RootTask.h>

#include <filedevice/rio_FileDeviceMgr.h>
#include <gfx/rio_Projection.h>
#include <gfx/rio_Window.h>
#include <gfx/rio_Graphics.h>
#include <gpu/rio_RenderState.h>

#include <string>

RootTask::RootTask()
    : ITask("FFL Testing")
    , mInitialized(false)
{
}

#if RIO_IS_WIN && defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#elif RIO_IS_WIN
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include <iostream>

#include <nn/ffl/FFLiMiiData.h>

#if RIO_IS_WIN
int server_fd, new_socket;
struct sockaddr_in address;
int opt = 1;
int addrlen = sizeof(address);
#endif

void RootTask::prepare_()
{
    mInitialized = false;

    FFLInitDesc init_desc;
    init_desc.fontRegion = FFL_FONT_REGION_0;
    init_desc._c = false;
    init_desc._10 = true;

#if RIO_IS_CAFE
    FSInit();
#endif // RIO_IS_CAFE

    {
        std::string resPath;
        resPath.resize(256);
        // Middle
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
                    RIO_ASSERT(false);
                    return;
                }

                mResourceDesc.pData[FFL_RESOURCE_TYPE_MIDDLE] = buffer;
                mResourceDesc.size[FFL_RESOURCE_TYPE_MIDDLE] = arg.read_size;
            }
        }
        // High
        {
            FFLGetResourcePath(resPath.data(), 256, FFL_RESOURCE_TYPE_HIGH, false);
            {
                rio::FileDevice::LoadArg arg;
                arg.path = resPath;
                arg.alignment = 0x2000;

                u8* buffer = rio::FileDeviceMgr::instance()->getNativeFileDevice()->tryLoad(arg);
                if (buffer == nullptr)
                {
                    RIO_LOG("NativeFileDevice failed to load: %s\n", resPath.c_str());
                    RIO_ASSERT(false);
                    return;
                }

                mResourceDesc.pData[FFL_RESOURCE_TYPE_HIGH] = buffer;
                mResourceDesc.size[FFL_RESOURCE_TYPE_HIGH] = arg.read_size;
            }
        }
    }

    FFLResult result = FFLInitResEx(&init_desc, &mResourceDesc);
    if (result != FFL_RESULT_OK)
    {
        RIO_LOG("FFLInitResEx() failed with result: %d\n", (s32)result);
        RIO_ASSERT(false);
        return;
    }

    FFLiEnableSpecialMii(333326543);

    RIO_ASSERT(FFLIsAvailable());

    FFLInitResGPUStep();

    mShader.initialize();

    mMiiCounter = 0;
    createModel_();

    // Set projection matrix
    {
        // Get window instance
        const rio::Window* const window = rio::Window::instance();

        // Create perspective projection instance
        float aspect = f32(window->getWidth()) / f32(window->getHeight());
        /*float fovy = rio::Mathf::deg2rad(43.2);
        rio::PerspectiveProjection proj(
            0.1f, // near
            10000.0f, // far
            fovy,
            aspect // aspect
        );*/
        float fovy = 2 * ((180 / 3.141592653589793f) * atan2f(43.2f, 500.0f));
        // C_MTXPerspective(Mtx44 m, f32 fovy, f32 aspect, f32 near, f32 far)
        // PerspectiveProjection(f32 near, f32 far, f32 fovy, f32 aspect)
        // original: C_MTXPerspective(projMtx, fovy, aspect, 500.0f, 700.0f)
        rio::PerspectiveProjection proj(500.0f, 700.0f, fovy, aspect);

        // Calculate matrix
        mProjMtx = proj.getMatrix();
    }


    #if RIO_IS_WIN
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

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    // Get port number from environment or use default 8080
    const char* env_port = getenv("PORT");
    int port = env_port ? atoi(env_port) : 8080;
    // Forcefully attaching socket to the port
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        std::cout << "\033[1m" <<
        "TIP: change the default port of 8080 with the PORT environment variable"
        << "\033[0m\n";
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Set socket to non-blocking mode
    #ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(server_fd, FIONBIO, &mode);
    #else
    fcntl(server_fd, F_SETFL, O_NONBLOCK);
    #endif

    //mSocketIsListening = true;

    std::cout << "\033[1m" <<
    "tcp server listening on port " << std::to_string(port) << "\033[0m" << std::endl
    << "\033[2m(you can change the port with the PORT environment variable)\033[0m\n"
    << "try sending FFLStoreData or FFLiCharInfo (little endian) to it with netcat"
    << std::endl;

    #endif // RIO_IS_WIN

    mInitialized = true;
}

void RootTask::createModel_() {
    FFLCharModelSource modelSource;

    // default model source if there is no socket
    modelSource.dataSource = FFL_DATA_SOURCE_DEFAULT;
    modelSource.index = mMiiCounter;
    modelSource.pBuffer = NULL;

    // 6 = count of guest Miis
    mMiiCounter = (mMiiCounter + 1) % 6;

    Model::InitArgStoreData arg = {
        .desc = {
            .resolution = FFLResolution(2048),
            .expressionFlag = 1,
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
        mpModel->setScale({ 1 / 16.f, 1 / 16.f, 1 / 16.f });
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

    std::cout << "\033[1mLoaded Mii: \033[0m";

    // Convert UTF-16 to UTF-8
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    std::cout << convert.to_bytes((char16_t*) pCharInfo->name)
        << std::endl;
    // close connection which should happen as soon as we read it
    #if RIO_IS_WIN && defined(_WIN32)
        closesocket(new_socket);
    #elif defined(_WIN32)
        close(new_socket);
    #endif
    // otherwise just fall through and use default

    Model::InitArgStoreData arg = {
        .desc = {
            .resolution = FFLResolution(2048),
            .expressionFlag = 1,
            .modelFlag = 1 << 0 | 1 << 1 | 1 << 2,
            .resourceType = FFL_RESOURCE_TYPE_HIGH,
        },
        .source = modelSource
    };

    mpModel = new Model();
    if (!mpModel->initialize(arg, mShader)) {
        RIO_LOG("mpModel initialize, initializeCpu_, FFLInitCharModelCPUStep: ONE OF THESE FAILED!\n");
        delete mpModel;
        mpModel = nullptr;
    } else {
        mpModel->setScale({ 1 / 16.f, 1 / 16.f, 1 / 16.f });
    }

    // Reset counter or maintain its state based on the application logic
    mCounter = 0.0f;
}



void RootTask::calc_()
{
    if (!mInitialized)
        return;

    rio::RenderState render_state;
    const char* transparency = getenv("TRANSPARENCY");
    if (transparency) {
        rio::Window::instance()->clearColor(0.0f, 0.0f, 0.0f, 0.0f);
    } else {
        // blue-gray-ish
        rio::Window::instance()->clearColor(0.2f, 0.3f, 0.3f, 1.0f);
    }
    rio::Window::instance()->clearDepthStencil();
    // trans parent cy ?
    render_state.setBlendEnable(true);
    render_state.setBlendFactorSrcAlpha(rio::Graphics::BlendFactor::BLEND_MODE_SRC_ALPHA);
    render_state.setBlendFactorDstAlpha(rio::Graphics::BlendFactor::BLEND_MODE_ONE_MINUS_SRC_ALPHA);
    render_state.apply();

    // maximum received is the size of FFLiCharInfo
    char buf[sizeof(FFLiCharInfo)];

    #if RIO_IS_WIN
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) > 0) {
        // Assuming data directly received is FFLStoreData
        ssize_t read_bytes =
            //read(new_socket, &storeData, sizeof(FFLStoreData));
            // NOTE: this will store BOTH charinfo AND storedata so uh
            recv(new_socket, buf,
                // setting read maximum to the size of CharInfo
                sizeof(FFLiCharInfo), 0);

        if (read_bytes >= sizeof(FFLStoreData)) {
            createModel_(&buf);
        }
    } else {
        // close connection which should happen as soon as we read it
        #ifdef _WIN32
            closesocket(new_socket);
        #else
            close(new_socket);
        #endif
        // otherwise just fall through and use default
        // when mii is directly in front of the camera
    #endif // RIO_IS_WIN
        if (mCounter >= rio::Mathf::pi2())
        {
            delete mpModel;
            createModel_();
        }
    #if RIO_IS_WIN
    }
    #endif

/*    static const rio::Vector3f CENTER_POS = { 0.0f, 2.0f, -0.25f };

    mCamera.at() = CENTER_POS;

    // Move camera
    mCamera.pos().set(
        // 10 = how close it is??
        CENTER_POS.x + std::sin(mCounter) * 10,
        CENTER_POS.y,
        CENTER_POS.z + std::cos(mCounter) * 10
    );
    // if you reduce 60 it'll speed this up (not framerate)
    mCounter += 1.f / 60;
*/
    const rio::Vector3f cameraPos = {0.0f, 34.5f, 600.0f};
    const rio::Vector3f target = {0.0f, 34.5f, 0.0f};
    const rio::Vector3f cameraUp = {0.0f, 1.0f, 0.0f};
    // C_MTXLookAt(Mtx m, const Vec* camPos, const Vec* camUp, const Vec* target)
    // LookAtCamera(const Vector3f& pos, const Vector3f& at, const Vector3f& up)
    // original: C_MTXLookAt(viewMtx, &cameraPos, &cameraUp, &target);
    rio::LookAtCamera mCamera(cameraPos, target, cameraUp);
    // Get view matrix
    rio::BaseMtx34f view_mtx;
    mCamera.getMatrix(&view_mtx);

  //mpModel->enableSpecialDraw();

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
    rio::MemUtil::free(mResourceDesc.pData[FFL_RESOURCE_TYPE_MIDDLE]);

    mInitialized = false;
}
