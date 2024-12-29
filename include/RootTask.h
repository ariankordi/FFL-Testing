#include <Shader.h>

#include <gfx/rio_Camera.h>
#include <task/rio_Task.h>

#include <nn/ffl/detail/FFLiCharInfo.h>

#define FFLICHARINFO_SIZE sizeof(FFLiCharInfo)

#if RIO_IS_WIN
#include <vector>
#endif

class Model;

class RootTask : public rio::ITask
{
public:
    RootTask();

private:
    void prepare_() override;
    void calc_() override;
    void exit_() override;

    void loadResourceFiles_();
    void createModel_();
    void createModel_(char (*buf)[FFLICHARINFO_SIZE]);
#if RIO_IS_WIN
    void fillStoreDataArray_();
    void setupSocket_();
#endif

private:
    bool                mInitialized;
    bool                mSocketIsListening;
#if RIO_IS_WIN
    std::vector<std::vector<char>> mStoreDataArray;
#endif
    FFLResourceDesc     mResourceDesc;
    Shader              mShader;
    rio::BaseMtx44f     mProjMtx;
    rio::LookAtCamera   mCamera;
    f32                 mCounter;
    s32                 mMiiCounter;
    Model*              mpModel;
    const char*         mpNoSpin;
};
