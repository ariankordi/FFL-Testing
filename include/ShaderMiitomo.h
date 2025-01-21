#pragma once

#include <IShader.h>

#include <math/rio_Vector.h>

#include <gpu/rio_TextureSampler.h>

#if RIO_IS_CAFE
#include <gx2/shaders.h>
#endif // RIO_IS_CAFE

class ShaderMiitomo : public IShader
{
public:
    ShaderMiitomo(IShader* mpMaskShader);
    ~ShaderMiitomo();

    void initialize() override;

    void bind(bool light_enable, FFLiCharInfo* charInfo) override;

    void setViewUniform(const rio::BaseMtx34f& model_mtx, const rio::BaseMtx34f& view_mtx, const rio::BaseMtx44f& proj_mtx) const override;
    void resetUniformsToDefault() override;

    void setModulate(const FFLModulateParam& modulateParam) override
    {
        setModulate_(modulateParam);
    }
    void setModulatePantsMaterial(PantsColor pantsColor) override;

    void setBoneMatrix(rio::Matrix34f* mtx, s32 boneCount) override;

    void setLightDirection(const rio::Vector3f direction) override;

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


    // Define types of Miitomo LUT textures
    // Located in cache/res/asset/env/lut/
    // LUTs for Mii head parts are located
    // here: cache/res/asset/env/mii/mii_model.xml
    // NOTE so i dont forget: default 02 is also for body/pants
    enum LUTSpecularTextureType
    {
        LUT_SPECULAR_TYPE_NONE,
        LUT_SPECULAR_TYPE_DEFAULT_02,
        LUT_SPECULAR_TYPE_SKIN_01,
        LUT_SPECULAR_TYPE_MAX
    };
    enum LUTFresnelTextureType
    {
        LUT_FRESNEL_TYPE_NONE,
        LUT_FRESNEL_TYPE_DEFAULT_02,
        LUT_FRESNEL_TYPE_SKIN_01,
        LUT_FRESNEL_TYPE_MAX
    };

    const char* cLUTFresnelFileNames[LUT_SPECULAR_TYPE_MAX] = {
        "fresnelNone.tga",       // LUT_FRESNEL_TYPE_NONE
        //"fresnelDefault01.tga",// LUT_FRESNEL_TYPE_DEFAULT_01
        "fresnelDefault02.tga",  // LUT_FRESNEL_TYPE_DEFAULT_02
        "fresnelSkin01.tga",     // LUT_FRESNEL_TYPE_SKIN_01
    };
    const char* cLUTSpecularFileNames[LUT_FRESNEL_TYPE_MAX] = {
        "specularNone.tga",      // LUT_SPECULAR_TYPE_NONE
        //"specularDefault01.tga"// LUT_SPECULAR_TYPE_DEFAULT_01
        "specularDefault02.tga", // LUT_SPECULAR_TYPE_DEFAULT_02
        "specularSkin01.tga",    // LUT_SPECULAR_TYPE_SKIN_01
    };

    const LUTSpecularTextureType
        cModulateToLUTSpecularType[CUSTOM_MATERIAL_PARAM_SIZE] = {
        LUT_SPECULAR_TYPE_SKIN_01,    // FFL_MODULATE_TYPE_SHAPE_FACELINE
        LUT_SPECULAR_TYPE_DEFAULT_02, // FFL_MODULATE_TYPE_SHAPE_BEARD
        LUT_SPECULAR_TYPE_SKIN_01,    // FFL_MODULATE_TYPE_SHAPE_NOSE
        LUT_SPECULAR_TYPE_SKIN_01,    // FFL_MODULATE_TYPE_SHAPE_FOREHEAD
        LUT_SPECULAR_TYPE_DEFAULT_02, // FFL_MODULATE_TYPE_SHAPE_HAIR
        LUT_SPECULAR_TYPE_DEFAULT_02, // FFL_MODULATE_TYPE_SHAPE_CAP
        LUT_SPECULAR_TYPE_DEFAULT_02, // FFL_MODULATE_TYPE_SHAPE_MASK
        LUT_SPECULAR_TYPE_NONE,       // FFL_MODULATE_TYPE_SHAPE_NOSELINE
        LUT_SPECULAR_TYPE_NONE,       // FFL_MODULATE_TYPE_SHAPE_GLASS
        LUT_SPECULAR_TYPE_DEFAULT_02, // CUSTOM_MATERIAL_PARAM_BODY
        LUT_SPECULAR_TYPE_DEFAULT_02, // CUSTOM_MATERIAL_PARAM_PANTS
    };
    const LUTFresnelTextureType
        cModulateToLUTFresnelType[CUSTOM_MATERIAL_PARAM_SIZE] = {
        LUT_FRESNEL_TYPE_SKIN_01,     // FFL_MODULATE_TYPE_SHAPE_FACELINE
        LUT_FRESNEL_TYPE_DEFAULT_02,  // FFL_MODULATE_TYPE_SHAPE_BEARD
        LUT_FRESNEL_TYPE_SKIN_01,     // FFL_MODULATE_TYPE_SHAPE_NOSE
        LUT_FRESNEL_TYPE_SKIN_01,     // FFL_MODULATE_TYPE_SHAPE_FOREHEAD
        LUT_FRESNEL_TYPE_DEFAULT_02,  // FFL_MODULATE_TYPE_SHAPE_HAIR
        LUT_FRESNEL_TYPE_DEFAULT_02,  // FFL_MODULATE_TYPE_SHAPE_CAP
        LUT_FRESNEL_TYPE_DEFAULT_02,  // FFL_MODULATE_TYPE_SHAPE_MASK
        LUT_FRESNEL_TYPE_NONE,        // FFL_MODULATE_TYPE_SHAPE_NOSELINE
        LUT_FRESNEL_TYPE_NONE,        // FFL_MODULATE_TYPE_SHAPE_GLASS
        LUT_FRESNEL_TYPE_DEFAULT_02,  // CUSTOM_MATERIAL_PARAM_BODY
        LUT_FRESNEL_TYPE_DEFAULT_02,  // CUSTOM_MATERIAL_PARAM_PANTS
    };

private:
    static void applyAlphaTestCallback_(void* p_obj, bool enable, rio::Graphics::CompareFunc func, f32 ref);
    void setShaderCallback_();

    void bindTexture_(const FFLModulateParam& modulateParam);
    void setConstColor_(u32 ps_loc, FFLColor color);
    void setModulateMode_(FFLModulateMode mode);
    void setModulate_(const FFLModulateParam& modulateParam);

    void setMaterial_(const FFLModulateType modulateType);

    void draw_(const FFLDrawParam& draw_param);
    static void drawCallback_(void* p_obj, const FFLDrawParam* draw_param);

    void setMatrix_(const rio::BaseMtx44f& matrix);
    static void setMatrixCallback_(void* p_obj, const rio::BaseMtx44f* matrix);

private:
    enum VertexUniform
    {
        VERTEX_UNIFORM_MVP = 0,  // Inverse transpose of MV
        VERTEX_UNIFORM_MODEL,
        VERTEX_UNIFORM_NORMAL,
        VERTEX_UNIFORM_ALPHA,
        VERTEX_UNIFORM_BONE_COUNT,
        VERTEX_UNIFORM_BONE_MATRICES,
        VERTEX_UNIFORM_HS_LIGHT_GROUND_COLOR,
        VERTEX_UNIFORM_HS_LIGHT_SKY_COLOR,
        VERTEX_UNIFORM_LIGHT_DIR_AND_TYPE0,
        VERTEX_UNIFORM_LIGHT_DIR_AND_TYPE1,
        VERTEX_UNIFORM_EYE_PT,
        VERTEX_UNIFORM_DIR_LIGHT_COUNT,
        VERTEX_UNIFORM_DIR_LIGHT_COLOR0,
        VERTEX_UNIFORM_DIR_LIGHT_COLOR1,
        VERTEX_UNIFORM_MAX
    };

    enum PixelUniform
    {
        PIXEL_UNIFORM_MODE = 0,
        PIXEL_UNIFORM_ALPHA_TEST,
        PIXEL_UNIFORM_CONST1,
        PIXEL_UNIFORM_CONST2,
        PIXEL_UNIFORM_CONST3,
        PIXEL_UNIFORM_LIGHT_COLOR,
        PIXEL_UNIFORM_LIGHT_ENABLE,
        PIXEL_UNIFORM_MAX
    };


    rio::Shader             mShader;
    IShader*                mpMaskShader;
    s32                     mVertexUniformLocation[VERTEX_UNIFORM_MAX];
    s32                     mPixelUniformLocation[PIXEL_UNIFORM_MAX];
    s32                     mSamplerLocation;
    s32                     mLUTSpecSamplerLocation;
    s32                     mLUTFresSamplerLocation;
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
    bool                    mLightEnable;
    rio::TextureSampler2D   mLUTSpecSampler;//[LUT_SPECULAR_TYPE_MAX];
    rio::TextureSampler2D   mLUTFresSampler;//[LUT_FRESNEL_TYPE_MAX];
    bool                    mIsUsingMaskShader;
    rio::BaseVec4f          mLightDirAndType0;
    rio::BaseVec4f          mLightDirAndType1;
};
