#include <Shader.h>
#include <ShaderSwitch.h>

#include <gfx/rio_Camera.h>
#include <gfx/mdl/rio_Model.h>
#include <task/rio_Task.h>


#include <nn/ffl.h>
#include <nn/ffl/FFLiMiiData.h>

#define FFLICHARINFO_SIZE sizeof(FFLiCharInfo)

#if RIO_IS_WIN
#include <vector>
#endif

class Model;

enum MiiDataInputType {
    INPUT_TYPE_FFL_MIIDATACORE = 0,
    INPUT_TYPE_RFL_CHARDATA,
    INPUT_TYPE_NX_CHARINFO,
    INPUT_TYPE_STUDIO_ENCODED,
    INPUT_TYPE_STUDIO_RAW,
    INPUT_TYPE_NX_COREDATA,
    INPUT_TYPE_NX_STOREDATA,
    INPUT_TYPE_RFL_CHARDATA_LE
};

enum ShaderType {
    SHADER_TYPE_WIIU,
    SHADER_TYPE_SWITCH,
    /* FUTURE OPTIONS:
     * miitomo
     * 3ds
       - potentially downscaling
     * wii...????????
       - it would have to be some TEV to GLSL sheeee
     */
    SHADER_TYPE_MAX = 2,
};

enum ViewType {
    VIEW_TYPE_FACE,  // head with body
    VIEW_TYPE_FACE_ONLY,  // head only
    VIEW_TYPE_ALL_BODY,
    // nn::mii::VariableIconBody::StoreCameraMatrix
    VIEW_TYPE_NNMII_VARIABLEICONBODY_VIEW,
    VIEW_TYPE_MAX = 4,
};

struct RenderRequest {
    char            data[96];   // just a buffer that accounts for maximum size
    uint16_t        dataLength; // determines the mii data format
    unsigned int    resolution; // resolution for render buffer
    // NOTE: texture resolution can control whether mipmap is enabled (1 << 30)
    FFLResolution   texResolution; // u32, or just uint, i think
    //unsigned int    scaleFactor;
    //bool            isHeadOnly;  // in favor of view type
    uint8_t         viewType;
    //rio::BaseVec3f  lightDir;
    uint8_t         expression;
    uint8_t         resourceType;
    uint8_t         shaderType;
    rio::Vector3i   cameraRotate;
    uint8_t         backgroundColor[4]; // passed to clearcolor

    // at the end to help with alignment
    bool            verifyCharInfo; // for FFLiVerifyCharInfoWithReason
    // TBD you may need another one for verifying StoreData CRC16
    bool            lightEnable;
    //bool            setLightDirection;
};
#define RENDERREQUEST_SIZE sizeof(RenderRequest)

// used for pickupCharInfoFromRenderRequest
#include <mii_ext_MiiPort.h>
bool pickupCharInfoFromRenderRequest(FFLiCharInfo* pCharInfo, int dataLength, RenderRequest *buf);

class RootTask : public rio::ITask
{
public:
    RootTask();

private:
    void prepare_() override;
    void calc_() override;
    void exit_() override;

    void handleRenderRequest(char* buf, rio::BaseMtx34f view_mtx);

    void createModel_();
    //void createModel_(char (*buf)[FFLICHARINFO_SIZE]);
    void createModel_(RenderRequest *buf);

    void initializeShaders_() {
        for (int type = 0; type < SHADER_TYPE_MAX; type++) {
            switch (type) {
                case SHADER_TYPE_WIIU:
                    mpShaders[type] = new Shader();
                    break;
                case SHADER_TYPE_SWITCH:
                    mpShaders[type] = new ShaderSwitch();
                    break;
            }
            mpShaders[type]->initialize();
        }
    }
    #if RIO_IS_WIN
    void fillStoreDataArray_();
    void setupSocket_();
    #endif

    void drawMiiBodyREAL(FFLiCharInfo* charInfo, rio::BaseMtx44f& proj_mtx);

private:
    bool                mInitialized;
    bool                mSocketIsListening;
    #if RIO_IS_WIN
    std::vector<std::vector<char>> mStoreDataArray;
    #endif
    FFLResourceDesc     mResourceDesc;
    IShader*            mpShaders[SHADER_TYPE_MAX];
    rio::BaseMtx44f     mProjMtx;
    rio::BaseMtx44f*    mProjMtxIconBody;
    rio::LookAtCamera   mCamera;
    f32                 mCounter;
    s32                 mMiiCounter;
    Model*              mpModel;
    rio::mdl::Model*    mpBodyModels[FFL_GENDER_MAX];
    const char*         mpServerOnly;
    const char*         mpNoSpin;
};
