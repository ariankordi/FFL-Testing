#pragma once

#include <math/rio_MathTypes.h>

#include <nn/ffl.h>

class Shader;

class Model
{
public:
    template <typename T>
    struct InitArg
    {
        FFLCharModelDesc    desc;
        FFLCharModelSource  source;
        u16                 index;
    };

    typedef InitArg<FFLStoreData>   InitArgStoreData;
    typedef InitArg<FFLMiddleDB>    InitArgMiddleDB;

public:
    Model();
    ~Model();

    template <typename T>
    bool initialize(const InitArg<T>& arg, Shader& shader);

    FFLCharModel* getCharModel() const { return mpCharModel; }
    FFLResult getInitializeCpuResult() const { return mInitializeCpuResult; }

    void enableSpecialDraw();

    void drawOpa(const rio::BaseMtx34f& view_mtx, const rio::BaseMtx44f& proj_mtx);
    void drawXlu(const rio::BaseMtx34f& view_mtx, const rio::BaseMtx44f& proj_mtx);


    void setLightEnable(bool lightEnable)
    {
        mLightEnable = lightEnable;
    }

private:
    void setViewUniform_(/*const rio::BaseMtx34f& model_mtx, */const rio::BaseMtx34f& view_mtx, const rio::BaseMtx44f& proj_mtx);

    void drawOpaNormal_();
    void drawOpaSpecial_();

    void drawXluNormal_();
    void drawXluSpecial_();

    void initialize_(const FFLCharModelDesc* p_desc, const FFLCharModelSource* p_source);
    bool initializeCpu_();
    void initializeGpu_(Shader& shader);

private:
    FFLCharModel*       mpCharModel;
    FFLCharModelDesc    mCharModelDesc;
    FFLCharModelSource  mCharModelSource;
    Shader*             mpShader;
    bool                mLightEnable;
    bool                mIsEnableSpecialDraw;
    bool                mIsInitialized;
    FFLResult           mInitializeCpuResult;
};

template <typename T>
bool Model::initialize(const InitArg<T>& arg, Shader& shader)
{
    RIO_ASSERT(mIsInitialized == false);

    initialize_(&arg.desc, &arg.source);

    if (!initializeCpu_())
        return false;

    initializeGpu_(shader);

    mIsInitialized = true;
    return true;
}
