#include <Model.h>
#include <Shader.h>

#include <gfx/rio_Window.h>
#include <gpu/rio_RenderState.h>
#include <math/rio_Matrix.h>

Model::Model()
    : mCharModelDesc()
    , mpShader(nullptr)
    , mIsEnableSpecialDraw(false)
    , mLightEnable(true)
    , mIsInitialized(false)
{
    mpCharModel = new FFLCharModel();
}

Model::~Model()
{
    if (mIsInitialized)
    {
        FFLDeleteCharModel(mpCharModel);
        mIsInitialized = false;
    }
    delete mpCharModel;
}

void Model::initialize_(const FFLCharModelDesc* p_desc, const FFLCharModelSource* p_source)
{
    mCharModelDesc = *p_desc;
    mCharModelSource = *p_source;
}

void Model::enableSpecialDraw()
{
    mIsEnableSpecialDraw = true;
}

void Model::drawOpa(const rio::BaseMtx34f& view_mtx, const rio::BaseMtx44f& proj_mtx)
{
    setViewUniform_(view_mtx, proj_mtx);

    if (mIsEnableSpecialDraw)
        drawOpaSpecial_();
    else
        drawOpaNormal_();
}

void Model::drawXlu(const rio::BaseMtx34f& view_mtx, const rio::BaseMtx44f& proj_mtx)
{
    setViewUniform_(view_mtx, proj_mtx);

    if (mIsEnableSpecialDraw)
    {
        drawXluSpecial_();
        mIsEnableSpecialDraw = false;
    }
    else
    {
        drawXluNormal_();
    }
}

void Model::setViewUniform_(/*const rio::BaseMtx34f& model_mtx,*/ const rio::BaseMtx34f& view_mtx, const rio::BaseMtx44f& proj_mtx)
{
    RIO_ASSERT(mpShader);
    mpShader->bind(mLightEnable, mpCharModel);
    mpShader->setViewUniform(rio::Matrix34f::ident, view_mtx, proj_mtx);
}

void Model::drawOpaNormal_()
{
    RIO_ASSERT(mpShader);

    rio::RenderState render_state;
    render_state.setDepthEnable(true, true);
    render_state.setDepthFunc(rio::Graphics::COMPARE_FUNC_LEQUAL);
    render_state.applyDepthAndStencilTest();
    mpShader->applyAlphaTestEnable();
    render_state.setBlendEnable(false);
    render_state.setBlendFactor(rio::Graphics::BLEND_MODE_ONE, rio::Graphics::BLEND_MODE_ZERO);
    render_state.applyBlendAndFastZ();

    FFLDrawOpa(mpCharModel);
}

void Model::drawOpaSpecial_()
{
    RIO_ASSERT(mpShader);

    rio::RenderState render_state;
    render_state.setDepthEnable(true, true);
    render_state.setDepthFunc(rio::Graphics::COMPARE_FUNC_LEQUAL);
    render_state.setBlendEnable(false);
    render_state.setColorMask(true, true, true, true);
    render_state.apply();

    FFLDrawOpa(mpCharModel);
}

void Model::drawXluNormal_()
{
    RIO_ASSERT(mpShader);

    rio::RenderState render_state;
    render_state.setDepthEnable(true, false);
    render_state.setDepthFunc(rio::Graphics::COMPARE_FUNC_LEQUAL);
    render_state.applyDepthAndStencilTest();
    mpShader->applyAlphaTestEnable();
    render_state.setBlendEnable(true);
    render_state.setBlendFactor(rio::Graphics::BLEND_MODE_SRC_ALPHA, rio::Graphics::BLEND_MODE_ONE_MINUS_SRC_ALPHA);
    render_state.applyBlendAndFastZ();

    FFLDrawXlu(mpCharModel);
}

void Model::drawXluSpecial_()
{
    RIO_ASSERT(mpShader);

    {
        rio::RenderState render_state;
        render_state.setDepthEnable(true, false);
        render_state.setDepthFunc(rio::Graphics::COMPARE_FUNC_LEQUAL);
        render_state.setBlendEnable(true);
        render_state.setBlendFactorSrcRGB(rio::Graphics::BLEND_MODE_SRC_ALPHA);
        render_state.setBlendFactorDstRGB(rio::Graphics::BLEND_MODE_ONE_MINUS_SRC_ALPHA);
        render_state.setBlendEquation(rio::Graphics::BLEND_FUNC_ADD);
        render_state.setColorMask(true, true, true, false);
        render_state.apply();

        mpShader->applyAlphaTestEnable();

        FFLDrawXlu(mpCharModel);
    }

    {
        rio::RenderState render_state;
        render_state.setDepthEnable(true, true);
        render_state.setDepthFunc(rio::Graphics::COMPARE_FUNC_LEQUAL);
        render_state.setBlendEnable(true);
        render_state.setBlendFactorSrcRGB(rio::Graphics::BLEND_MODE_ONE_MINUS_DST_ALPHA);
        render_state.setBlendFactorDstRGB(rio::Graphics::BLEND_MODE_DST_ALPHA);
        render_state.setBlendFactorSrcAlpha(rio::Graphics::BLEND_MODE_ONE);
        render_state.setBlendFactorDstAlpha(rio::Graphics::BLEND_MODE_ONE);
        render_state.setBlendEquation(rio::Graphics::BLEND_FUNC_ADD);
        render_state.setColorMask(true, true, true, true);
        render_state.apply();

        mpShader->applyAlphaTestEnable();

        FFLDrawXlu(mpCharModel);
    }

}

bool Model::initializeCpu_()
{

    mInitializeCpuResult = FFLInitCharModelCPUStep(mpCharModel, &mCharModelSource, &mCharModelDesc);
    if (mInitializeCpuResult != FFL_RESULT_OK) {
        RIO_LOG("FFLInitCharModelCPUStep returned: %i\n", mInitializeCpuResult);
        return false;
    }
    return true;
}

void Model::initializeGpu_(Shader& shader)
{
    mpShader = &shader;
    // disable light when rendering faceline textures
    mpShader->bind(false, mpCharModel);
    FFLInitCharModelGPUStep(mpCharModel);
    //rio::Window::instance()->makeContextCurrent();
}
