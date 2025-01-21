#include <Shader.h>
#include <ShaderSwitch.h>
#include <ShaderMiitomo.h>


// for socket
#if RIO_IS_WIN && defined(_WIN32)
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#elif RIO_IS_WIN
    #define closesocket close
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #ifdef USE_SYSTEMD_SOCKET
        #include <systemd/sd-daemon.h>
    #endif // USE_SYSTEMD_SOCKET
#endif // RIO_IS_WIN && defined(_WIN32)

#include <gfx/rio_Camera.h>
#include <gfx/rio_Projection.h>

#include <gfx/mdl/rio_Model.h>
#include <task/rio_Task.h>


#include <nn/ffl.h>
#include <nn/ffl/FFLiMiiData.h>

#include <RenderRequest.h>
#include <Types.h> // enums for RootTask

#include <BodyModel.h>
#include <HeadwearList.h>

#define FFLICHARINFO_SIZE sizeof(FFLiCharInfo)

#if RIO_IS_WIN
#include <vector>
#endif

class Model;

#define RENDERREQUEST_SIZE sizeof(RenderRequest)

#include <mii_ext_MiiPort.h> // used for below function
FFLResult pickupCharInfoFromRenderRequest(FFLiCharInfo* pCharInfo, RenderRequest *buf);

class RootTask : public rio::ITask
{
public:
    RootTask();
    // Static members set to strings or nullptr.
    static const char* sServerOnlyFlag;
    static const char* sServerPort;

    static const char* sResourceSearchPath;
    static const char* sResourceHighPath;


private:
    void prepare_() override;
    void calc_() override;
    void exit_() override;

    void handleRenderRequest(char* buf, Model** ppModel, int socket);
#ifndef NO_GLTF
    void handleGLTFRequest(RenderRequest* renderRequest);
#endif

    void loadResourceFiles_();
    void loadBodyModels_();
    void createModel_();
    FFLResourceType getDefaultResourceType_();

    bool createModel_(RenderRequest* req, int socket_handle);

    void initializeShaders_()
    {
        for (s32 type = 0; type < SHADER_TYPE_MAX; type++)
        {
            switch (type)
            {
                case SHADER_TYPE_WIIU:
                    mpShaders[type] = new Shader();
                    break;
                case SHADER_TYPE_WIIU_BLINN:
                {
                    Shader* s = new Shader();
                    s->setSpecularMode(FFL_SPECULAR_MODE_NORMAL);
                    mpShaders[type] = s;
                    break;
                }
                case SHADER_TYPE_WIIU_FFLICONWITHBODY:
                {
                    Shader* s = new Shader();
                    s->setDefaultLight(
                        { -0.50f, 0.366f, 0.785f }, // direction
                        { 0.5f, 0.5f, 0.5f, 1.0f }, // ambient
                        { 0.9f, 0.9f, 0.9f, 1.0f }, // diffuse
                        { 1.0f, 1.0f, 1.0f, 1.0f }  // specular
                    );

                    mpShaders[type] = s;
                    break;
                }
                case SHADER_TYPE_SWITCH:
                    mpShaders[type] = new ShaderSwitch();
                    break;
                case SHADER_TYPE_MIITOMO:
                    // miitomo shader needs wii u shader to draw mask
                    mpShaders[type] = new ShaderMiitomo(mpShaders[SHADER_TYPE_WIIU]);
                    break;
            }
            mpShaders[type]->initialize();
        }
    }
#if RIO_IS_WIN
    void fillStoreDataArray_();
    void setupSocket_();
#endif

    rio::mdl::Model* getBodyModel_(Model* pModel, BodyType type);
    void setViewTypeParams(ViewType viewType, rio::LookAtCamera* pCamera, rio::PerspectiveProjection* proj, rio::BaseMtx44f* projMtx, float* aspectHeightFactor, bool* isCameraPosAbsolute, bool* willDrawBody, FFLiCharInfo* pCharInfo);

private:
    bool                mInitialized;
    bool                mSocketIsListening;
#if RIO_IS_WIN
    std::vector<std::vector<char>> mStoreDataArray;
#endif
    FFLResourceDesc     mResourceDesc;
    IShader*            mpShaders[SHADER_TYPE_MAX];

    rio::PerspectiveProjection mProj;
    rio::BaseMtx44f     mProjMtx;
    rio::PerspectiveProjection mProjIconBody;
    rio::BaseMtx44f     mProjMtxIconBody;

    rio::LookAtCamera   mCamera;
    f32                 mCounter;
    s32                 mMiiCounter;
    Model*              mpModel;
    rio::mdl::Model*    mpBodyModels[BODY_TYPE_MAX][FFL_GENDER_MAX];
    bool                mHeadwearAvailable;
    HeadwearList        mHeadwearList;

    // For server:
    int                 mServerFD;
    int                 mServerSocket;
    sockaddr_in         mServerAddress;
};
