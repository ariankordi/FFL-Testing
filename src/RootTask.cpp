#include "gpu/rio_Texture.h"
#include "math/rio_Matrix.h"
#include "nn/ffl/FFLResourceType.h"
#include "nn/ffl/detail/FFLiCharInfo.h"
#include <Model.h>
#include <RootTask.h>

#include <filedevice/rio_FileDeviceMgr.h>
#include <gfx/rio_Projection.h>
#include <gfx/rio_Window.h>
#include <gfx/rio_Graphics.h>
#include <gpu/rio_RenderState.h>

#include <gfx/mdl/res/rio_ModelCacher.h>
#include <gfx/mdl/rio_Model.h>

#include <nn/ffl/FFLiSwapEndian.h>

#include <string>
#include <array>

RootTask::RootTask()
    : ITask("FFL Testing")
    , mInitialized(false)
    // disables occasionally drawing mii and makes non blocking
    , mServerOnly(getenv("SERVER_ONLY"))
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

#include <nn/ffl/FFLiMiiData.h>
#include <nn/ffl/FFLiMiiDataCore.h>
#include <nn/ffl/FFLiCreateID.h>

#if RIO_IS_WIN
int server_fd, new_socket;
struct sockaddr_in address;
int opt = 1;
int addrlen = sizeof(address);

// for opening ffsd folder
#include <filesystem>
#include <fstream>
#endif

// forward declarations for drawing body model
//rio::mdl::res::Model* bodyResModels[FFL_GENDER_MAX];
const rio::mdl::Model* bodyModels[FFL_GENDER_MAX];

#include <mii_ext_MiiPort.h>

#if RIO_IS_WIN
// read Mii data from a folder
void RootTask::fillStoreDataArray_()
{
    // Path to the folder
    const std::string folderPath = "place_ffsd_files_here";
    // Check if the folder exists
    if (std::filesystem::exists(folderPath) && std::filesystem::is_directory(folderPath)) {
        for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
            if (entry.is_regular_file()
                && entry.file_size() >= sizeof(charInfoStudio)
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
        RIO_LOG("Loaded %lu FFSD files into mStoreDataArray\n", mStoreDataArray.size());
    }
}
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
    const char* env_port = getenv("PORT");
    int port = env_port ? atoi(env_port) : 12346;
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
        if (!mServerOnly) {
            #ifdef _WIN32
            u_long mode = 1;
            ioctlsocket(server_fd, FIONBIO, &mode);
            #else
            fcntl(server_fd, F_SETFL, O_NONBLOCK);
            #endif
        }

        mSocketIsListening = true;

        RIO_LOG("\033[1m" \
        "tcp server listening on port %d\033[0m\n" \
        "\033[2m(you can change the port with the PORT environment variable)\033[0m\n",
        port);
    }
}
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
                RIO_LOG("\e[1;31mThe FFLResHigh.dat needs to be present, or else this program won't work. It will probably crash right now.\e[0m\n");
                RIO_ASSERT(false);
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

    initializeShaders_();

    // Get window instance
    const rio::Window* const window = rio::Window::instance();
    // Set projection matrix
    {
        // Calculate the aspect ratio based on the window dimensions
        float aspect = f32(window->getWidth()) / f32(window->getHeight());
        // Calculate the field of view (fovy) based on the given parameters
        float fovy;
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

        rio::PerspectiveProjection projIconBody(
            5.0f,//10.0f,
            1000.0f,
            rio::Mathf::deg2rad(15.0f),
            1.0f
        );
        mProjMtxIconBody = new rio::BaseMtx44f(projIconBody.getMatrix());

    }

    #if RIO_IS_WIN
    fillStoreDataArray_();
    setupSocket_();
    // Set window aspect ratio, so that when resizing it will not change
    GLFWwindow* glfwWindow = rio::Window::instance()->getNativeWindow().getGLFWwindow();
    glfwSetWindowAspectRatio(glfwWindow, window->getWidth(), window->getHeight());
    #endif

    mMiiCounter = 0;
    createModel_();

    // load (just male for now) body model
    // the male and female bodies are like identical
    // from the torso up (only perspective we use)
    char key[17] = "mii_static_body0";
    for (u32 i = 0; i < FFL_GENDER_MAX; i++)
    {
        RIO_LOG("loading model: %s\n", key);
        //bodyResModels[i] = rio::mdl::res::ModelCacher::instance()->loadModel(key, key); // "body0", "body1" per gender
        const rio::mdl::res::Model* resModel = rio::mdl::res::ModelCacher::instance()->loadModel(key, key);
        key[sizeof(key)-2]++; // iterate last character
        RIO_ASSERT(resModel);

        bodyModels[i] = new rio::mdl::Model(resModel);
    }
    //RIO_ASSERT(loadBodyGLTF(&gltfBodyModel, bodyGltfMaleFilename));
    //bodyShader.load("FFLBodyShader");

    mInitialized = true;
}

// amount of mii indexes to cycle through
// default source only has 6
// GetMiiDataNum()
int maxMiis = 6;

// forward declaration
bool pickupCharInfoFromRenderRequest(FFLiCharInfo* pCharInfo, int dataLength, RenderRequest *buf);

void RootTask::createModel_() {
    FFLCharModelSource modelSource;

    // default model source if there is no socket
    #if RIO_IS_CAFE
        // use mii maker database on wii u
        modelSource.dataSource = FFL_DATA_SOURCE_OFFICIAL;
        // NOTE: will only use first 6 miis from mii maker database
    #else
    FFLiCharInfo charInfo;
    if (!mStoreDataArray.empty()) {
        // Use the custom Mii data array
        if(!pickupCharInfoFromRenderRequest(&charInfo, mStoreDataArray[mMiiCounter].size(), reinterpret_cast<RenderRequest*>(&mStoreDataArray[mMiiCounter][0])))
        {
            RIO_LOG("pickupCharInfoFromRenderRequest returned false on Mii counter: %d\n", mMiiCounter);
            mpModel = nullptr;
            mCounter = 0.0f;
            mMiiCounter = (mMiiCounter + 1) % maxMiis;
            return;
        }

        modelSource.index = 0;
        modelSource.dataSource = FFL_DATA_SOURCE_DIRECT_POINTER;
        modelSource.pBuffer = &charInfo;
        // limit current counter by the amount of custom miis
        maxMiis = mStoreDataArray.size();
    } else {
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


    Model::InitArgStoreData arg = {
        .desc = {
            .resolution = FFLResolution(768),
            .expressionFlag = 1,
            .modelFlag = 1 << 0 | 1 << 1 | 1 << 2,
            .resourceType = FFL_RESOURCE_TYPE_HIGH,
        },
        .source = modelSource
    };

    mpModel = new Model();
    int shaderType = SHADER_TYPE_WIIU;//(mMiiCounter-1) % (SHADER_TYPE_MAX);
    if (!mpModel->initialize(arg, *mShaders[shaderType])) {
        delete mpModel;
        mpModel = nullptr;
    } /*else {
        mpModel->setScale({ 1.f, 1.f, 1.f });
        //mpModel->setScale({ 1 / 16.f, 1 / 16.f, 1 / 16.f });
    }*/
    mCounter = 0.0f;
}

bool pickupCharInfoFromRenderRequest(FFLiCharInfo* pCharInfo, int dataLength, RenderRequest *buf) {
    MiiDataInputType inputType;

    switch (dataLength) {
        /*case sizeof(FFLStoreData):
        case sizeof(FFLiMiiDataOfficial):
        case sizeof(FFLiMiiDataCore):
            inputType = INPUT_TYPE_FFL_MIIDATACORE;
            break;
        */
        case 76: // RFLStoreData
        case 74: // RFLCharData
            inputType = INPUT_TYPE_RFL_CHARDATA;
            break;
        case sizeof(charInfo): // nx char info
            inputType = INPUT_TYPE_NX_CHARINFO;
            break;
        // todo nx storedata and coredata
        case sizeof(charInfoStudio): // studio raw
            inputType = INPUT_TYPE_STUDIO_RAW;
            break;
        case 47: // have also seen studio encoded come out like this
        case 48: // studio encoded i think
            inputType = INPUT_TYPE_STUDIO_ENCODED;
            break;
        case sizeof(FFLiMiiDataCore):
        case sizeof(FFLiMiiDataOfficial):
        case sizeof(FFLStoreData):
            inputType = INPUT_TYPE_FFL_MIIDATACORE;
            break;
        default:
            // uh oh, we can't detect it
            return false;
            break;
    }

    switch (inputType) {
        case INPUT_TYPE_RFL_CHARDATA:
        {
            // it is NOT FFLiMiiDataCore, it may be RFL tho
            // cast to FFLiMiIDataCoreRFL to check create id
            FFLiMiiDataCoreRFL& charDataRFL = reinterpret_cast<FFLiMiiDataCoreRFL&>(buf->data);
            // look at create id to run FFLiIsNTRMiiID
            // NOTE: FFLiMiiDataCoreRFL2CharInfo is SUPPOSED to check
            // whether it is an NTR mii id or not and store
            // the result as pCharInfo->birthPlatform = FFL_BIRTH_PLATFORM_NTR
            // HOWEVER, it clears out the create ID and runs the compare anyway
            // the create id is actually not the same size either


            // TODO: NOT WORKING ATM
            /*FFLCreateID* createIDRFL = reinterpret_cast<FFLCreateID*>(charDataRFL.m_CreatorID);

            // test for if it is NTR data
            bool isNTR = FFLiIsNTRMiiID(createIDRFL);
            RIO_LOG("IS NTR??? %i\n", isNTR);
    */
            // NOTE: NFLCharData for DS can be little endian tho!!!!!!
#if __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
            // swap endian for rfl
            static const FFLiSwapEndianDesc SWAP_ENDIAN_DESC_RFL[] = {
                { FFLI_SWAP_ENDIAN_TYPE_U16, 1 },  // m_Flag
                { FFLI_SWAP_ENDIAN_TYPE_U16, 10 }, // m_Name
                { FFLI_SWAP_ENDIAN_TYPE_U8,  1 },  // m_Height
                { FFLI_SWAP_ENDIAN_TYPE_U8,  1 },  // m_Build
                { FFLI_SWAP_ENDIAN_TYPE_U8,  4 },  // m_CreatorID
                { FFLI_SWAP_ENDIAN_TYPE_U8,  4 },  // m_SystemID
                { FFLI_SWAP_ENDIAN_TYPE_U16, 1 },  // m_FaceFlag
                { FFLI_SWAP_ENDIAN_TYPE_U16, 1 },  // m_HairFlag
                { FFLI_SWAP_ENDIAN_TYPE_U16, 2 },  // m_EyebrowFlag
                { FFLI_SWAP_ENDIAN_TYPE_U16, 2 },  // m_EyeFlag
                { FFLI_SWAP_ENDIAN_TYPE_U16, 1 },  // m_NoseFlag
                { FFLI_SWAP_ENDIAN_TYPE_U16, 1 },  // m_MouthFlag
                { FFLI_SWAP_ENDIAN_TYPE_U16, 1 },  // m_GlassFlag
                { FFLI_SWAP_ENDIAN_TYPE_U16, 1 },  // m_BeardFlag
                { FFLI_SWAP_ENDIAN_TYPE_U16, 1 },  // m_MoleFlag
                { FFLI_SWAP_ENDIAN_TYPE_U16, 10 } // creator name
            };
            FFLiSwapEndianGroup(buf->data, SWAP_ENDIAN_DESC_RFL, sizeof(SWAP_ENDIAN_DESC_RFL) / sizeof(FFLiSwapEndianDesc));
#endif
            /*if (pCharInfo->birthPlatform != FFL_BIRTH_PLATFORM_NTR) {
                // flip endian
            }
            isNTR = FFLiIsNTRMiiID(createIDRFL);
            RIO_LOG("IS NTR (2)??? %i\n", isNTR);
            */
            // TODO: HANDLE BOTH BIG AND LITTLE ENDIAN RFL DATA
            FFLiMiiDataCoreRFL2CharInfo(pCharInfo,
                charDataRFL,
                NULL, false
                //reinterpret_cast<u16*>(&buf->data[0x36]), true // creator name
                // name offset: https://github.com/SMGCommunity/Petari/blob/53fd4ff9db54cb1c91a96534dcae9f2c2ea426d1/libs/RVLFaceLib/include/RFLi_Types.h#L342
            );
            break;
        }
        case INPUT_TYPE_STUDIO_ENCODED:
        {
            // mii studio url data format is obfuscated
            // this decodes it in place and falls through
            char decodedData[48];
            std::memcpy(decodedData, buf->data, dataLength);
            studioURLObfuscationDecode(decodedData);
            charInfo charInfoNX;
            studioToCharInfoNX(&charInfoNX, reinterpret_cast<::charInfoStudio*>(decodedData));
            charInfoNXToFFLiCharInfo(pCharInfo, &charInfoNX);
            break;
        }
        case INPUT_TYPE_STUDIO_RAW:
            // we may not need this if we decode from and to buf->data but that's confusing
            charInfo charInfoNX;
            studioToCharInfoNX(&charInfoNX, reinterpret_cast<::charInfoStudio*>(buf->data));
            charInfoNXToFFLiCharInfo(pCharInfo, &charInfoNX);
            break;
        case INPUT_TYPE_NX_CHARINFO:
            charInfoNXToFFLiCharInfo(pCharInfo, reinterpret_cast<::charInfo*>(buf->data));
            break;
        // note this is miidatacore
        default:
            // TODO: SWAP ENDIAN ON WII U OR HANDLE BOTH KINDS
            FFLiMiiDataCore2CharInfo(pCharInfo,
                reinterpret_cast<FFLiMiiDataCore&>(buf->data),
            // const u16* pCreatorName, bool resetBirthday
            NULL, false);
            break;
    }
    return true;
}

void RootTask::createModel_(RenderRequest *buf) {
    FFLiCharInfo charInfo;

    if (!pickupCharInfoFromRenderRequest(&charInfo, buf->dataLength, buf)) {
        RIO_LOG("pickupCharInfoFromRenderRequest returned false\n");
        mpModel = nullptr;
        mCounter = 0.0f;
        return;
    }

    // VERIFY CHARINFO
    if (buf->verifyCharInfo) {
        // get verify char info reason, don't verify name
        FFLiVerifyCharInfoReason verifyCharInfoReason =
            FFLiVerifyCharInfoWithReason(&charInfo, false);
        // TODO TODO: rn we are still relying on FFL_TEST_DISABLE_MII_COLOR_VERIFY, so OUT OF BOUNDS COLORS ARE NOT VERIFIED AAAAAAAAAAAAAAAAAA
        // I think I want to separate making the model
        // and picking up CharInfo from the request LATER
        // and then apply it when I do that

        if (verifyCharInfoReason != FFLI_VERIFY_CHAR_INFO_REASON_OK) {
            // CHARINFO IS INVALID, FAIL!
            RIO_LOG("FFLiVerifyCharInfoWithReason failed with result: %d\n", verifyCharInfoReason);
            mpModel = nullptr;
            mCounter = 0.0f;
            return;
        }
    }

    // NOTE NOTE: MAKE SURE TO CHECK NULL MII ID TOO

    FFLCharModelSource modelSource;
    // needs to be initialized to zero
    modelSource.index = 0;
    // don't call PickupCharInfo or verify mii after we already did
    modelSource.dataSource = FFL_DATA_SOURCE_DIRECT_POINTER;
    //modelSource.dataSource = FFL_DATA_SOURCE_BUFFER; // aka CharInfo
    modelSource.pBuffer = &charInfo;

    // otherwise just fall through and use default
    Model::InitArgStoreData arg = {
        .desc = {
            .resolution = buf->texResolution,
            .expressionFlag = buf->expressionFlag,
            .modelFlag = 1 << 0 | 1 << 1 | 1 << 2,
            .resourceType = static_cast<FFLResourceType>(buf->resourceType % FFL_RESOURCE_TYPE_MAX),
        },
        .source = modelSource
    };

    mpModel = new Model();
    ShaderType whichShader = SHADER_TYPE_WIIU;
    if (buf->resourceType > FFL_RESOURCE_TYPE_HIGH)
        whichShader = SHADER_TYPE_SWITCH;
    if (!mpModel->initialize(arg, *mShaders[whichShader])) {
        RIO_LOG("FFLInitCharModelCPUStep FAILED while initializing model: %d\n", mpModel->getInitializeCpuResult());
        delete mpModel;
        mpModel = nullptr;
        return;
    }

    // Reset counter or maintain its state based on the application logic
    mCounter = 0.0f;
}

void RootTask::drawMiiBodyREAL(FFLiCharInfo* charInfo, rio::BaseMtx44f& proj_mtx) {

    // SELECT BODY MODEL
    const rio::mdl::Model* model = bodyModels[charInfo->gender];

    const rio::mdl::Mesh* const meshes = model->meshes();

    // Render each mesh in order
    for (u32 i = 0; i < model->numMeshes(); i++)
    {
        const rio::mdl::Mesh& mesh = meshes[i];

        // BIND SHADER
        //bodyShader.bind();
        mpModel->getShader()->bindBodyShader(charInfo);
        //RIO_GL_CALL(glBindVertexArray(vao));

        rio::BaseMtx34f view_mtx;
        /*rio::LookAtCamera camera;
        camera.pos() = { 0.0f, 130.37f, 415.53f };
        camera.at() = { 0.0f, 130.37f, 0.0f };
        camera.setUp({ 0.0f, 1.0f, 0.0f });
        camera.getMatrix(&view_mtx);
        */

        rio::BaseVec3f scaleFactors;

        float build = charInfo->build;
        float height = charInfo->height;

        // "calculateScaleFactors" anonymous function referenced in nn::mii::detail::VariableIconBodyImpl::CalculateWorldMatrix
        /*scaleFactors.x = (build * (height * 0.003671875f + 0.4f)) / 128.0f +
                        height * 0.001796875f + 0.4f;//0.4f;

        scaleFactors.y = (height * 0.006015625f) + 0.5f;
*/
        // WII U FACTORS?
        scaleFactors.y = height / 128.0f;
        scaleFactors.x = scaleFactors.y * 0.3f + 0.6f;
        scaleFactors.x = ((scaleFactors.y * 0.6f + 0.8f) - scaleFactors.x) * build / 128.0f + scaleFactors.x;

        scaleFactors.y = scaleFactors.y * 0.55f + 0.6f;

        scaleFactors.z = scaleFactors.x;

        // Ensure scaleFactors.y is clamped to a maximum of 1.0
        scaleFactors.y = std::min(scaleFactors.y, 1.0f);


        float bodyHeightOffset = scaleFactors.y * 93.92f;

        mCamera.at() = { mCamera.at().x, mCamera.at().y + bodyHeightOffset, mCamera.at().z };
        mCamera.pos() = { mCamera.pos().x, mCamera.pos().y + bodyHeightOffset, mCamera.pos().z };
        mCamera.getMatrix(&view_mtx);


        // Create the scale matrix
        rio::BaseMtx34f model_mtx = rio::Matrix34f::ident;
        model_mtx.m[0][0] = scaleFactors.x; // Scale x-axis
        model_mtx.m[1][1] = scaleFactors.y; // Scale y-axis
        model_mtx.m[2][2] = scaleFactors.z; // Scale z-axis

        mpModel->getShader()->setViewUniformBody(model_mtx, view_mtx, proj_mtx);

        // finally, the uniforms are all set

        rio::RenderState render_state;
        render_state.setCullingMode(rio::Graphics::CULLING_MODE_BACK);
        render_state.applyCullingAndPolygonModeAndPolygonOffset();
        mesh.draw();
    }
    //bodyShader.unload();
}

void RootTask::handleRenderRequest(char* buf, rio::BaseMtx34f view_mtx) {
    if (mpModel == nullptr) {
        /*const char* errMsg = "mpModel == nullptr";
        send(new_socket, errMsg, strlen(errMsg), 0);
        */#ifdef _WIN32
            closesocket(new_socket);
        #else
            close(new_socket);
        #endif
        return;
    }
    mpModel->enableSpecialDraw();

    // hopefully renderrequest is proper
    RenderRequest* renderRequest = reinterpret_cast<RenderRequest*>(buf);

    // switch between two projection matrxies
    rio::BaseMtx44f *projMtx;
    if (renderRequest->isHeadOnly) {
        // use default if request is head only
        projMtx = &mProjMtx;
    } else {
        // if it has body then use the matrix we just defined
        projMtx = mProjMtxIconBody;
        // NOTE: adjusted to be slightly more accurate but still not right
        // FFLMakeIconWithBody view uses 37.05f, 415.53f
        // nn::mii::VariableIconBody::StoreCameraMatrix uses 37.0f, 380.0f
        mCamera.pos() = { 0.0f, 34.20f, 412.0f };
        mCamera.at() = { 0.0f, 34.20f, 0.0f };
        /*
        mCamera.pos() = { 0.0f, 34.20f, 412.0f };
        mCamera.at() = { 0.0f, 34.20f, 0.0f };
        */
        mCamera.setUp({ 0.0f, 1.0f, 0.0f });
        mCamera.getMatrix(&view_mtx);
        // without this it will use the default view matrix
    }

    #ifdef RIO_NO_CLIP_CONTROL
        // render upside down so glReadPixels is right side up
        if (projMtx->m[1][1] > 0) // if it is not already negative
            projMtx->m[1][1] *= -1.f; // Flip Y axis
        // set front face culling from CCW to CW
        RIO_GL_CALL(glFrontFace(GL_CW));
    #endif

    mpModel->setLightEnable(renderRequest->lightEnable);

    // Create the render buffer with the desired size
    rio::RenderBuffer renderBuffer;
    //renderBuffer.setSize(renderRequest->resolution * 2, renderRequest->resolution * 2);    hasSocketRequest = false;

    int ssaaFactor = 1;  // Super Sampling factor, e.g., 2 for 2x SSAA
    int width = renderRequest->resolution * ssaaFactor;
    int height = renderRequest->resolution * ssaaFactor;

    renderBuffer.setSize(width, height);
    RIO_LOG("Render buffer created with size: %dx%d\n", renderBuffer.getSize().x, renderBuffer.getSize().y);

    //rio::Window::instance()->getNativeWindow()->getColorBufferTextureFormat();
    rio::Texture2D *renderTextureColor = new rio::Texture2D(rio::TEXTURE_FORMAT_R8_G8_B8_A8_UNORM, renderBuffer.getSize().x, renderBuffer.getSize().y, 1);
    rio::RenderTargetColor renderTargetColor;
    renderTargetColor.linkTexture2D(*renderTextureColor);
    renderBuffer.setRenderTargetColor(&renderTargetColor);

    rio::Texture2D *renderTextureDepth = new rio::Texture2D(rio::DEPTH_TEXTURE_FORMAT_R32_FLOAT, renderBuffer.getSize().x, renderBuffer.getSize().y, 1);
    rio::RenderTargetDepth renderTargetDepth;
    renderTargetDepth.linkTexture2D(*renderTextureDepth);

    renderBuffer.setRenderTargetDepth(&renderTargetDepth);

/*
// Create multisampled framebuffer for head rendering
GLuint msaaFBO, msaaColorRBO, msaaDepthRBO;
GLsizei samples = 4; // You can adjust this value

RIO_GL_CALL(glGenFramebuffers(1, &msaaFBO));
RIO_GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, msaaFBO));

RIO_GL_CALL(glGenRenderbuffers(1, &msaaColorRBO));,
RIO_GL_CALL(glBindRenderbuffer(GL_RENDERBUFFER, msaaColorRBO));
RIO_GL_CALL(glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, renderBuffer.getSize().x, renderBuffer.getSize().y));
RIO_GL_CALL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaaColorRBO));

RIO_GL_CALL(glGenRenderbuffers(1, &msaaDepthRBO));
RIO_GL_CALL(glBindRenderbuffer(GL_RENDERBUFFER, msaaDepthRBO));
RIO_GL_CALL(glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT32F, renderBuffer.getSize().x, renderBuffer.getSize().y));
RIO_GL_CALL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, msaaDepthRBO));

*/
    //const rio::Color4f clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
    // NOTE: this calls glViewport
    renderBuffer.clear(rio::RenderBuffer::CLEAR_FLAG_COLOR_DEPTH_STENCIL, renderRequest->backgroundColor);

    // Bind the render buffer
    renderBuffer.bind();
/*        RIO_GL_CALL(glViewport(0, 0, renderRequest->resolution, renderRequest->resolution));

    // Bind MSAA framebuffer
    //RIO_GL_CALL(glViewport(0, 0, renderRequest->resolution, renderRequest->resolution));
    glBindFramebuffer(GL_FRAMEBUFFER, msaaFBO);

    // Clear the buffer
    glClearColor(renderRequest->backgroundColor.r, renderRequest->backgroundColor.g, renderRequest->backgroundColor.b, renderRequest->backgroundColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
*/
    RIO_LOG("Render buffer bound.\n");

    //if (gl_FragCoord.z > 0.98593) discard;
    // Enable depth testing
    /*glEnable(GL_DEPTH_TEST);
    // Set the depth function to discard fragments with depth values greater than the threshold
    glDepthFunc(GL_GREATER);
    glDepthRange(0.98593f, 0.f);
*/
    // Render the first frame to the buffer
    mpModel->drawOpa(view_mtx, *projMtx);
    RIO_LOG("drawOpa rendered to the buffer.\n");

    // draw body?
    if (!renderRequest->isHeadOnly)
    {
        FFLiCharModel *charModel = reinterpret_cast<FFLiCharModel*>(mpModel->getCharModel());

        //drawMiiBodyFAKE(renderTextureColor, renderTextureDepth, favoriteColorIndex);
        drawMiiBodyREAL(&charModel->charInfo,  *projMtx);
    }

    renderBuffer.bind();

    // draw xlu mask only after body is drawn
    // in case there are elements of the mask that go in the body region
    mpModel->drawXlu(view_mtx, *projMtx);
    RIO_LOG("drawXlu rendered to the buffer.\n");

/*
    rio::RenderBuffer renderBufferDownsample;
    renderBufferDownsample.setSize(renderRequest->resolution, renderRequest->resolution);

    RIO_GL_CALL(glViewport(0, 0, renderRequest->resolution, renderRequest->resolution));

    //rio::Window::instance()->getNativeWindow()->getColorBufferTextureFormat();
    rio::Texture2D *renderTextureDownsampleColor = new rio::Texture2D(rio::TEXTURE_FORMAT_R8_G8_B8_A8_UNORM, renderBufferDownsample.getSize().x, renderBufferDownsample.getSize().y, 1);
    rio::RenderTargetColor renderTargetDownsampleColor;
    renderTargetDownsampleColor.linkTexture2D(*renderTextureDownsampleColor);
    renderBufferDownsample.setRenderTargetColor(&renderTargetDownsampleColor);

    renderBufferDownsample.clear(rio::RenderBuffer::CLEAR_FLAG_COLOR_DEPTH_STENCIL, renderRequest->backgroundColor);
    renderBufferDownsample.bind();

    rio::RenderState render_state;
    render_state.setBlendEnable(true);
    /*
    render_state.setBlendFactorSrc(rio::Graphics::BlendFactor::BLEND_MODE_ONE_MINUS_DST_ALPHA);
    render_state.setBlendFactorDst(rio::Graphics::BlendFactor::BLEND_MODE_DST_ALPHA);
    render_state.setBlendFactorSrcAlpha(rio::Graphics::BlendFactor::BLEND_MODE_ONE);
    render_state.setBlendFactorDstAlpha(rio::Graphics::BlendFactor::BLEND_MODE_ONE);
    *
    render_state.setBlendFactorSrc(rio::Graphics::BlendFactor::BLEND_MODE_SRC_ALPHA);
    render_state.setBlendFactorDst(rio::Graphics::BlendFactor::BLEND_MODE_ONE_MINUS_SRC_ALPHA);
    render_state.setBlendFactorSrcAlpha(rio::Graphics::BlendFactor::BLEND_MODE_SRC_ALPHA);
    render_state.setBlendFactorDstAlpha(rio::Graphics::BlendFactor::BLEND_MODE_ONE_MINUS_SRC_ALPHA);

    render_state.apply();
/*
    // Load and compile the downsampling shader
    rio::Shader downsampleShader;
    downsampleShader.load("downsample_shader");
    downsampleShader.bind();

// Bind the high-resolution texture
rio::TextureSampler2D highResSampler;
highResSampler.linkTexture2D(renderTextureColor);
highResSampler.tryBindFS(downsampleShader.getFragmentSamplerLocation("highResTexture"), 0);

// Set shader uniforms if needed
downsampleShader.setUniform(ssaaFactor, u32(-1), downsampleShader.getFragmentUniformLocation("ssaaFactor"));

downsampleShader.setUniform(f32(renderRequest->resolution), f32(renderRequest->resolution), u32(-1), downsampleShader.getFragmentUniformLocation("resolution"));


// Render a full-screen quad to apply the downsampling shader
GLuint quadVAO, quadVBO;
float quadVertices[] = {
-1.0f,  1.0f,  0.0f, 1.0f,
-1.0f, -1.0f,  0.0f, 0.0f,
    1.0f, -1.0f,  1.0f, 0.0f,
-1.0f,  1.0f,  0.0f, 1.0f,
    1.0f, -1.0f,  1.0f, 0.0f,
    1.0f,  1.0f,  1.0f, 1.0f
};

RIO_GL_CALL(glGenVertexArrays(1, &quadVAO));
RIO_GL_CALL(glGenBuffers(1, &quadVBO));
RIO_GL_CALL(glBindVertexArray(quadVAO));
RIO_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, quadVBO));
RIO_GL_CALL(glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW));
RIO_GL_CALL(glEnableVertexAttribArray(0));
RIO_GL_CALL(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0));
RIO_GL_CALL(glEnableVertexAttribArray(1));
RIO_GL_CALL(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float))));

RIO_GL_CALL(glDrawArrays(GL_TRIANGLES, 0, 6));

RIO_GL_CALL(glBindVertexArray(0));

// Unbind the framebuffer
RIO_GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    if (hasSocketRequest) {

// Clean up
RIO_GL_CALL(glDeleteVertexArrays(1, &quadVAO));
RIO_GL_CALL(glDeleteBuffers(1, &quadVBO));

downsampleShader.unload();
*/


    // Read the rendered data into a buffer and save it to a file
    int bufferSize = renderBuffer.getSize().x * renderBuffer.getSize().y * 4; // Assuming 4 bytes per pixel (RGBA)
    //int bufferSize = renderRequest->resolution * renderRequest->resolution * 4;
/*
    // Create a regular framebuffer to resolve the MSAA buffer to
    GLuint resolveFBO, resolveColorRBO;
    glGenFramebuffers(1, &resolveFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, resolveFBO);

    // Create and attach a regular color renderbuffer
    glGenRenderbuffers(1, &resolveColorRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, resolveColorRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, renderRequest->resolution, renderRequest->resolution);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, resolveColorRBO);


    // Blit (resolve) the multisampled framebuffer to the regular framebuffer
    glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFBO);
    glBlitFramebuffer(0, 0, renderRequest->resolution, renderRequest->resolution,
                    0, 0, renderRequest->resolution, renderRequest->resolution,
                    GL_COLOR_BUFFER_BIT, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, resolveFBO);

    RIO_GL_CALL(glReadPixels(0, 0, renderRequest->resolution, renderRequest->resolution, GL_RGBA, GL_UNSIGNED_BYTE, readBuffer));
*/
    //renderBufferDownsample.read(0, readBuffer, renderBufferDownsample.getSize().x, renderBufferDownsample.getSize().y, renderTextureDownsampleColor->getNativeTexture().surface.nativeFormat);
    rio::NativeTextureFormat nativeFormat = renderTextureColor->getNativeTexture().surface.nativeFormat;

#if RIO_IS_WIN
/*
    glReadBuffer(GL_COLOR_ATTACHMENT0 + 0);
    glReadPixels(0, 0, width, height, nativeFormat.format, nativeFormat.type, readBuffer);
*/
    // map a pixel buffer object (DMA?)
    GLuint pbo;
    RIO_GL_CALL(glGenBuffers(1, &pbo));
    RIO_GL_CALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo));
    RIO_GL_CALL(glBufferData(GL_PIXEL_PACK_BUFFER, bufferSize, nullptr, GL_STREAM_READ));
    RIO_GL_CALL(glReadPixels(0, 0, width, height, nativeFormat.format, nativeFormat.type, nullptr));
    void* readBuffer = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
    // Process data in ptr
    RIO_LOG("Rendered data read successfully from the buffer.\n");
    send(new_socket, reinterpret_cast<char*>(readBuffer), bufferSize, 0);
    RIO_GL_CALL(glUnmapBuffer(GL_PIXEL_PACK_BUFFER));
    RIO_GL_CALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
    RIO_GL_CALL(glDeleteBuffers(1, &pbo));
// NOTE: renderBuffer.read() IS NOT IMPLEMENTED ON CAFE NOR IS THIS MEANT TO RUN ON WII U AT ALL LOL
#else
    u8* readBuffer = new u8[bufferSize];
    //rio::MemUtil::set(readBuffer, 0xFF, bufferSize);
    // NOTE: UNTESTED
    renderBuffer.read(0, readBuffer, renderBuffer.getSize().x, renderBuffer.getSize().y, nativeFormat);
    send(new_socket, reinterpret_cast<char*>(readBuffer), bufferSize, 0);
    delete[] readBuffer;
#endif


    // Unbind the render buffer
    renderBuffer.getRenderTargetColor()->invalidateGPUCache();
    renderBuffer.getRenderTargetDepth()->invalidateGPUCache();
    renderTextureColor->setCompMap(0x00010205);
    delete renderTextureColor;
    delete renderTextureDepth;
    // Clean up
    /*glDeleteFramebuffers(1, &msaaFBO);
    glDeleteRenderbuffers(1, &msaaColorRBO);
    glDeleteRenderbuffers(1, &msaaDepthRBO);
    glDeleteFramebuffers(1, &resolveFBO);
    glDeleteRenderbuffers(1, &resolveColorRBO);
*/

    RIO_LOG("Render buffer unbound and GPU cache invalidated.\n");

    if (!mServerOnly) {
        #ifdef RIO_NO_CLIP_CONTROL
            // set back front face culling
            RIO_GL_CALL(glFrontFace(GL_CCW));
            if (projMtx->m[1][1] < 0) // if it is negative
                projMtx->m[1][1] *= -1.f; // Flip Y axis back
        #endif
        rio::Window::instance()->makeContextCurrent();

        u32 width = rio::Window::instance()->getWidth();
        u32 height = rio::Window::instance()->getHeight();

        rio::Graphics::setViewport(0, 0, width, height);
        rio::Graphics::setScissor(0, 0, width, height);
        RIO_LOG("Viewport and scissor reset to window dimensions: %dx%d\n", width, height);
    }

    #ifdef _WIN32
        closesocket(new_socket);
    #else
        close(new_socket);
    #endif

}

void RootTask::calc_()
{
    if (!mInitialized)
        return;

    #if RIO_IS_WIN
    char buf[RENDERREQUEST_SIZE];

    bool hasSocketRequest = false;

    if (mSocketIsListening &&
        (new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) > 0) {
        int read_bytes =
            recv(new_socket, buf, RENDERREQUEST_SIZE, 0);

        if (read_bytes == RENDERREQUEST_SIZE) {
            delete mpModel;
            hasSocketRequest = true;
            createModel_(reinterpret_cast<RenderRequest*>(buf));
        } else {
            RIO_LOG("got a request of length %d (should be %lu), dropping\n", read_bytes, RENDERREQUEST_SIZE);
            #ifdef _WIN32
                closesocket(new_socket);
            #else
                close(new_socket);
            #endif
        }
    } else {
        // otherwise just fall through and use default
        // when mii is directly in front of the camera
    #endif // RIO_IS_WIN
        if (!mServerOnly && mCounter >= rio::Mathf::pi2())
        {
            delete mpModel;
            createModel_();
        }
    #if RIO_IS_WIN
    }
    #endif

    // Distance in the XZ-plane from the center to the camera position
    float radius = 600.0f;
    // Define a constant position in the 3D space for the center position of the camera
    static const rio::Vector3f CENTER_POS = { 0.0f, 34.5f, radius };
    static const rio::Vector3f target = { 0.0f, 34.5f, 0.0f };
    mCamera.at() = target;
    static const rio::Vector3f cameraUp = { 0.0f, 1.0f, 0.0f };
    mCamera.setUp(cameraUp);
    // Move the camera around the target clockwise
    // Define the radius of the orbit in the XZ-plane (distance from the target)
    const char* noSpin = getenv("NO_SPIN");
    if (!noSpin && !mServerOnly && !hasSocketRequest) {
        radius += 380;
        mCamera.pos().set(
            // Set the camera's position using the sin and cos functions to move it in a circle around the target
            std::sin(mCounter) * radius,
            CENTER_POS.y * std::sin(mCounter) * 7.5 - 30,
            // Add a minus sign to the cosine to spin CCW (same as SpinMii)
            std::cos(mCounter) * radius
        );
        mCamera.at() = { target.x, target.y - 30, target.z };
    } else {
        mCamera.pos() = CENTER_POS;
    }
    // Increment the counter to gradually change the camera's position over time
    if (!mServerOnly) {
        mCounter += 1.f / 60;
    }

    // Get the view matrix from the camera, which represents the camera's orientation and position in the world
    rio::BaseMtx34f view_mtx;
    mCamera.getMatrix(&view_mtx);

    if (!mServerOnly) {
        rio::Window::instance()->clearColor(0.2f, 0.3f, 0.3f, 1.0f);
        rio::Window::instance()->clearDepthStencil();
        //rio::Window::instance()->setSwapInterval(0);  // disable v-sync
    }

    if (hasSocketRequest) {
        return handleRenderRequest(buf, view_mtx);
    }

    if (mpModel != nullptr) {
        mpModel->enableSpecialDraw();
        mpModel->drawOpa(view_mtx, mProjMtx);
        drawMiiBodyREAL(&reinterpret_cast<FFLiCharModel*>(mpModel->getCharModel())->charInfo, mProjMtx);
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

    delete mProjMtxIconBody;
    for (int type = 0; type < SHADER_TYPE_MAX; type++) {
        delete mShaders[type];
    }

    mInitialized = false;
}
