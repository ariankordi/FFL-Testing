#pragma once

#include <IShader.h>

#include <gpu/rio_TextureSampler.h>

#if RIO_IS_CAFE
#include <gx2/shaders.h>
#endif // RIO_IS_CAFE

class ShaderSwitch : public IShader
{
public:
    ShaderSwitch();
    ~ShaderSwitch();

    void initialize() override;

    void bind(bool light_enable, FFLiCharInfo* charInfo) override;

    void bindBodyShader(FFLiCharInfo* pCharInfo) override;

    void setViewUniform(const rio::BaseMtx34f& model_mtx, const rio::BaseMtx34f& view_mtx, const rio::BaseMtx44f& proj_mtx) const override;
    void setViewUniformBody(const rio::BaseMtx34f& model_mtx, const rio::BaseMtx34f& view_mtx, const rio::BaseMtx44f& proj_mtx) const override {
        setViewUniform(model_mtx, view_mtx, proj_mtx);
    };

    void applyAlphaTestEnable() const override
    {
        applyAlphaTest(true, rio::Graphics::COMPARE_FUNC_GREATER, 0.0f);
    }

    void applyAlphaTestDisable() const override
    {
        applyAlphaTest(false, rio::Graphics::COMPARE_FUNC_ALWAYS, 0.0f);
    }

    void applyAlphaTest(bool enable, rio::Graphics::CompareFunc func, f32 ref) const override;

    static void setCulling(FFLCullMode mode);

private:
    static void applyAlphaTestCallback_(void* p_obj, bool enable, rio::Graphics::CompareFunc func, f32 ref);
    void setShaderCallback_();

    void bindTexture_(const FFLModulateParam& modulateParam);
    void setConstColor_(u32 ps_loc, const FFLColor& color);
    void setModulateMode_(FFLModulateMode mode);
    void setModulate_(const FFLModulateParam& modulateParam);

    void setMaterial_(const FFLModulateType modulateType);

    void draw_(const FFLDrawParam& draw_param);
    static void drawCallback_(void* p_obj, const FFLDrawParam& draw_param);

    void setMatrix_(const rio::BaseMtx44f& matrix);
    static void setMatrixCallback_(void* p_obj, const rio::BaseMtx44f& matrix);

private:
    enum VertexUniform
    {
        VERTEX_UNIFORM_MV = 0,
        VERTEX_UNIFORM_PROJ,
        VERTEX_UNIFORM_MAX
    };

    enum PixelUniform
    {
        PIXEL_UNIFORM_MODULATE_TYPE = 0,
        PIXEL_UNIFORM_GAMMA_TYPE,
        PIXEL_UNIFORM_DRAW_TYPE,
        PIXEL_UNIFORM_PAD0,
        PIXEL_UNIFORM_CONST_COLOR1,
        PIXEL_UNIFORM_CONST_COLOR2,
        PIXEL_UNIFORM_CONST_COLOR3,
        PIXEL_UNIFORM_LIGHT_DIR_IN_VIEW,
        PIXEL_UNIFORM_LIGHT_COLOR,
        PIXEL_UNIFORM_SSS_COLOR,
        PIXEL_UNIFORM_SPECULAR_COLOR,
        PIXEL_UNIFORM_RIM_COLOR,
        PIXEL_UNIFORM_HALF_LAMBERT_FACTOR,
        PIXEL_UNIFORM_SSS_SPECULAR_FACTOR,
        PIXEL_UNIFORM_SPECULAR_FACTOR_A,
        PIXEL_UNIFORM_SPECULAR_FACTOR_B,
        PIXEL_UNIFORM_SPECULAR_SHINNESS,
        PIXEL_UNIFORM_RIM_POWER,
        PIXEL_UNIFORM_RIM_WIDTH,
        PIXEL_UNIFORM_LIGHT_ENABLE,
        PIXEL_UNIFORM_MAX
    };

    rio::Shader             mShader;
    s32                     mVertexUniformLocation[VERTEX_UNIFORM_MAX];
    s32                     mPixelUniformLocation[PIXEL_UNIFORM_MAX];
    s32                     mSamplerLocation;
    s32                     mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_MAX];
#if RIO_IS_CAFE
    GX2AttribStream         mAttribute[FFL_ATTRIBUTE_BUFFER_TYPE_MAX];
    GX2FetchShader          mFetchShader;
#elif RIO_IS_WIN
    u32                     mVBOHandle[FFL_ATTRIBUTE_BUFFER_TYPE_MAX];
    u32                     mVAOHandle;
#endif
    FFLShaderCallback       mCallback;
    rio::TextureSampler2D   mSampler;
    FFLiCharInfo*           mpCharInfo;
};
