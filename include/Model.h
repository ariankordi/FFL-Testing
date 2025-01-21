#pragma once

#include <math/rio_MathTypes.h>

#include <nn/ffl.h>

#include <IShader.h>
#include <BodyModel.h>

class Model
{
public:
    struct InitArg
    {
        FFLCharModelDesc    desc;
        FFLCharModelSource  source;
        u16                 index;
    };

public:
    Model();
    ~Model();
    BodyModel*     mpBody;

    bool initialize(const InitArg& arg, IShader& shader);

    FFLCharModel* getCharModel() const { return mpCharModel; }
    FFLPartsTransform getPartsTransform() const
    {
        FFLPartsTransform partsTransform;
        FFLGetPartsTransform(&partsTransform, mpCharModel);
        return partsTransform;
    }
    FFLiCharInfo* getCharInfo()
    {
        FFLiCharModel* pCharModel = reinterpret_cast<FFLiCharModel*>(mpCharModel);
        return &pCharModel->charInfo;
    }
    IShader* getShader() const { return mpShader; }

    FFLResult getInitializeCpuResult() const { return mInitializeCpuResult; }

    void enableSpecialDraw();

    void drawOpa(const rio::BaseMtx34f& view_mtx, const rio::BaseMtx44f& proj_mtx);
    void drawXlu(const rio::BaseMtx34f& view_mtx, const rio::BaseMtx44f& proj_mtx);

    const rio::BaseMtx34f& getMtxRT() const
    {
        return mMtxRT;
    }

    void setMtxRT(const rio::BaseMtx34f& mtx)
    {
        mMtxRT = mtx;
        updateMtxSRT_();
    }

    void setLightEnable(bool lightEnable)
    {
        mLightEnable = lightEnable;
    }
    bool getLightEnable() const { return mLightEnable; }

    const rio::BaseVec3f& getScale() const
    {
        return mScale;
    }

    void setScale(const rio::BaseVec3f& scale)
    {
        mScale = scale;
        updateMtxSRT_();
    }

private:
    void updateMtxSRT_();

    void setViewUniform_(const rio::BaseMtx34f& model_mtx, const rio::BaseMtx34f& view_mtx, const rio::BaseMtx44f& proj_mtx);

    void drawOpaNormal_();
    void drawOpaSpecial_();

    void drawXluNormal_();
    void drawXluSpecial_();

    void initialize_(const FFLCharModelDesc* p_desc, const FFLCharModelSource* p_source);
    bool initializeCpu_();
    void initializeGpu_(IShader& shader);

private:
    FFLCharModel*       mpCharModel;
    FFLCharModelDesc    mCharModelDesc;
    FFLCharModelSource  mCharModelSource;
    rio::BaseMtx34f     mMtxRT;
    rio::BaseVec3f      mScale;
    rio::BaseMtx34f     mMtxSRT;
    IShader*            mpShader;
    bool                mIsEnableSpecialDraw;
    bool                mLightEnable;
    bool                mIsInitialized;
    FFLResult           mInitializeCpuResult;
};
