#include <Shader.h>

#include <gfx/rio_Camera.h>
#include <task/rio_Task.h>

#include <nn/ffl/FFLiMiiData.h>

#define FFLICHARINFO_SIZE sizeof(FFLiCharInfo)

class Model;

class RootTask : public rio::ITask
{
public:
    RootTask();

private:
    void prepare_() override;
    void calc_() override;
    void exit_() override;

    void createModel_();
    void createModel_(char (*buf)[FFLICHARINFO_SIZE]);

private:
    bool                mInitialized;
    //bool                mSocketIsListening;
    FFLResourceDesc     mResourceDesc;
    Shader              mShader;
    rio::BaseMtx44f     mProjMtx;
    rio::LookAtCamera   mCamera;
    f32                 mCounter;
    s32                 mMiiCounter;
    Model*              mpModel;
};
