#pragma once

#include <IShader.h>

#include <gpu/rio_TextureSampler.h>

#if RIO_IS_CAFE
#include <gx2/shaders.h>
#endif // RIO_IS_CAFE

class Shader : public IShader
{
public:
    Shader();
    ~Shader();

    void initialize() override;

    void bind(bool light_enable, FFLiCharInfo* charInfo) override;

    void bindBodyShader(FFLiCharInfo* pCharInfo) override;

    void setViewUniform(const rio::BaseMtx34f& model_mtx, const rio::BaseMtx34f& view_mtx, const rio::BaseMtx44f& proj_mtx) const override;
    void setViewUniformBody(const rio::BaseMtx34f& model_mtx, const rio::BaseMtx34f& view_mtx, const rio::BaseMtx44f& proj_mtx) const override;

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

protected:
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

protected:
    enum VertexUniform
    {
        VERTEX_UNIFORM_IT = 0,  // Inverse transpose of MV
        VERTEX_UNIFORM_MV,
        VERTEX_UNIFORM_PROJ,
        VERTEX_UNIFORM_MAX
    };

    enum PixelUniform
    {
        PIXEL_UNIFORM_CONST1 = 0,
        PIXEL_UNIFORM_CONST2,
        PIXEL_UNIFORM_CONST3,
        PIXEL_UNIFORM_LIGHT_AMBIENT,
        PIXEL_UNIFORM_LIGHT_DIFFUSE,
        PIXEL_UNIFORM_LIGHT_DIR,
        PIXEL_UNIFORM_LIGHT_ENABLE,
        PIXEL_UNIFORM_LIGHT_SPECULAR,
        PIXEL_UNIFORM_MATERIAL_AMBIENT,
        PIXEL_UNIFORM_MATERIAL_DIFFUSE,
        PIXEL_UNIFORM_MATERIAL_SPECULAR,
        PIXEL_UNIFORM_MATERIAL_SPECULAR_MODE,
        PIXEL_UNIFORM_MATERIAL_SPECULAR_POWER,
        PIXEL_UNIFORM_MODE,
        PIXEL_UNIFORM_RIM_COLOR,
        PIXEL_UNIFORM_RIM_POWER,
        PIXEL_UNIFORM_MAX
    };

    rio::Shader             mShader;
    rio::Shader             mBodyShader;
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
