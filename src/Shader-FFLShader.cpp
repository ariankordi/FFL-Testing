#include <Shader-FFLShader.h>

#include <gpu/rio_RenderState.h>
#include <math/rio_Matrix.h>
#include <misc/rio_MemUtil.h>

#if RIO_IS_CAFE
#include <gx2/registers.h>
#include <gx2/utils.h>
#endif // RIO_IS_CAFE

namespace {

#if RIO_IS_CAFE

#define GX2_ATTRIB_FORMAT_SNORM_10_10_10_2 (GX2AttribFormat(GX2_ATTRIB_FLAG_SIGNED | GX2_ATTRIB_TYPE_10_10_10_2))
#define GX2_SHADER_ALIGNMENT GX2_SHADER_PROGRAM_ALIGNMENT

#define GX2_COMP_SEL_X001 GX2_SEL_MASK(GX2_SQ_SEL_X, GX2_SQ_SEL_0, GX2_SQ_SEL_0, GX2_SQ_SEL_1)
#define GX2_COMP_SEL_XY01 GX2_SEL_MASK(GX2_SQ_SEL_X, GX2_SQ_SEL_Y, GX2_SQ_SEL_0, GX2_SQ_SEL_1)
#define GX2_COMP_SEL_XYZ1 GX2_SEL_MASK(GX2_SQ_SEL_X, GX2_SQ_SEL_Y, GX2_SQ_SEL_Z, GX2_SQ_SEL_1)
#define GX2_COMP_SEL_XYZW GX2_SEL_MASK(GX2_SQ_SEL_X, GX2_SQ_SEL_Y, GX2_SQ_SEL_Z, GX2_SQ_SEL_W)

inline void GX2InitAttribStream(
    GX2AttribStream* p_attribute,
    u32 location,
    u32 buffer,
    u32 offset,
    GX2AttribFormat format
)
{
    static constexpr u32 sFormatMask[] = {
        GX2_COMP_SEL_X001, GX2_COMP_SEL_XY01,
        GX2_COMP_SEL_X001, GX2_COMP_SEL_X001,
        GX2_COMP_SEL_XY01, GX2_COMP_SEL_X001,
        GX2_COMP_SEL_X001, GX2_COMP_SEL_XY01,
        GX2_COMP_SEL_XY01, GX2_COMP_SEL_XYZ1,
        GX2_COMP_SEL_XYZW, GX2_COMP_SEL_XYZW,
        GX2_COMP_SEL_XY01, GX2_COMP_SEL_XY01,
        GX2_COMP_SEL_XYZW, GX2_COMP_SEL_XYZW,
        GX2_COMP_SEL_XYZ1, GX2_COMP_SEL_XYZ1,
        GX2_COMP_SEL_XYZW, GX2_COMP_SEL_XYZW
    };

    p_attribute->buffer     = buffer;
    p_attribute->offset     = offset;
    p_attribute->location   = location;
    p_attribute->format     = format;
    p_attribute->mask       = sFormatMask[format & 0xff];
    p_attribute->type       = GX2_ATTRIB_INDEX_PER_VERTEX;
    p_attribute->aluDivisor = 0;
    p_attribute->endianSwap = GX2_ENDIAN_SWAP_DEFAULT;
}

#endif // RIO_IS_CAFE


const rio::BaseVec3f& getColorUniform(const FFLColor& color)
{
    return reinterpret_cast<const rio::BaseVec3f&>(color.r);
}

}

Shader::Shader()
#if RIO_IS_CAFE
    : mAttribute()
    , mFetchShader()
#elif RIO_IS_WIN
    : mVBOHandle()
    , mVAOHandle()
#endif
{
    rio::MemUtil::set(mVertexUniformLocation, u8(-1), sizeof(mVertexUniformLocation));
    rio::MemUtil::set(mPixelUniformLocation, u8(-1), sizeof(mPixelUniformLocation));
    mSamplerLocation = -1;
    rio::MemUtil::set(mAttributeLocation, u8(-1), sizeof(mAttributeLocation));
}

Shader::~Shader()
{
#if RIO_IS_CAFE
    if (mFetchShader.program != nullptr)
    {
        rio::MemUtil::free(mFetchShader.program);
        mFetchShader.program = nullptr;
    }
#elif RIO_IS_WIN
    if (mVAOHandle != GL_NONE)
    {
        RIO_GL_CALL(glDeleteVertexArrays(1, &mVAOHandle));
        RIO_GL_CALL(glDeleteBuffers(FFL_ATTRIBUTE_BUFFER_TYPE_MAX, mVBOHandle));
        rio::MemUtil::set(mVBOHandle, 0, sizeof(mVBOHandle));
        mVAOHandle = GL_NONE;
    }
#endif
}

void Shader::initialize()
{
    mShader.load("FFLShader", rio::Shader::MODE_UNIFORM_REGISTER);

    mVertexUniformLocation[VERTEX_UNIFORM_IT]   = mShader.getVertexUniformLocation("u_it");
    mVertexUniformLocation[VERTEX_UNIFORM_MV]   = mShader.getVertexUniformLocation("u_mv");
    mVertexUniformLocation[VERTEX_UNIFORM_PROJ] = mShader.getVertexUniformLocation("u_proj");

    mPixelUniformLocation[PIXEL_UNIFORM_CONST1]                     = mShader.getFragmentUniformLocation("u_const1");
    mPixelUniformLocation[PIXEL_UNIFORM_CONST2]                     = mShader.getFragmentUniformLocation("u_const2");
    mPixelUniformLocation[PIXEL_UNIFORM_CONST3]                     = mShader.getFragmentUniformLocation("u_const3");
    mPixelUniformLocation[PIXEL_UNIFORM_LIGHT_AMBIENT]              = mShader.getFragmentUniformLocation("u_light_ambient");
    mPixelUniformLocation[PIXEL_UNIFORM_LIGHT_DIFFUSE]              = mShader.getFragmentUniformLocation("u_light_diffuse");
    mPixelUniformLocation[PIXEL_UNIFORM_LIGHT_DIR]                  = mShader.getFragmentUniformLocation("u_light_dir");
    mPixelUniformLocation[PIXEL_UNIFORM_LIGHT_ENABLE]               = mShader.getFragmentUniformLocation("u_light_enable");
    mPixelUniformLocation[PIXEL_UNIFORM_LIGHT_SPECULAR]             = mShader.getFragmentUniformLocation("u_light_specular");
    mPixelUniformLocation[PIXEL_UNIFORM_MATERIAL_AMBIENT]           = mShader.getFragmentUniformLocation("u_material_ambient");
    mPixelUniformLocation[PIXEL_UNIFORM_MATERIAL_DIFFUSE]           = mShader.getFragmentUniformLocation("u_material_diffuse");
    mPixelUniformLocation[PIXEL_UNIFORM_MATERIAL_SPECULAR]          = mShader.getFragmentUniformLocation("u_material_specular");
    mPixelUniformLocation[PIXEL_UNIFORM_MATERIAL_SPECULAR_MODE]     = mShader.getFragmentUniformLocation("u_material_specular_mode");
    mPixelUniformLocation[PIXEL_UNIFORM_MATERIAL_SPECULAR_POWER]    = mShader.getFragmentUniformLocation("u_material_specular_power");
    mPixelUniformLocation[PIXEL_UNIFORM_MODE]                       = mShader.getFragmentUniformLocation("u_mode");
    mPixelUniformLocation[PIXEL_UNIFORM_RIM_COLOR]                  = mShader.getFragmentUniformLocation("u_rim_color");
    mPixelUniformLocation[PIXEL_UNIFORM_RIM_POWER]                  = mShader.getFragmentUniformLocation("u_rim_power");

    mSamplerLocation = mShader.getFragmentSamplerLocation("s_texture");

    mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_COLOR]     = mShader.getVertexAttribLocation("a_color");
    mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_NORMAL]    = mShader.getVertexAttribLocation("a_normal");
    mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_POSITION]  = mShader.getVertexAttribLocation("a_position");
    mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_TANGENT]   = mShader.getVertexAttribLocation("a_tangent");
    mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_TEXCOORD]  = mShader.getVertexAttribLocation("a_texCoord");

#if RIO_IS_CAFE
    GX2InitAttribStream(
        &(mAttribute[FFL_ATTRIBUTE_BUFFER_TYPE_POSITION]),
        mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_POSITION],
        FFL_ATTRIBUTE_BUFFER_TYPE_POSITION,
        0,
        GX2_ATTRIB_FORMAT_FLOAT_32_32_32
    );
    GX2InitAttribStream(
        &(mAttribute[FFL_ATTRIBUTE_BUFFER_TYPE_TEXCOORD]),
        mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_TEXCOORD],
        FFL_ATTRIBUTE_BUFFER_TYPE_TEXCOORD,
        0,
        GX2_ATTRIB_FORMAT_FLOAT_32_32
    );
    GX2InitAttribStream(
        &(mAttribute[FFL_ATTRIBUTE_BUFFER_TYPE_NORMAL]),
        mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_NORMAL],
        FFL_ATTRIBUTE_BUFFER_TYPE_NORMAL,
        0,
        GX2_ATTRIB_FORMAT_SNORM_10_10_10_2
    );
    GX2InitAttribStream(
        &(mAttribute[FFL_ATTRIBUTE_BUFFER_TYPE_TANGENT]),
        mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_TANGENT],
        FFL_ATTRIBUTE_BUFFER_TYPE_TANGENT,
        0,
        GX2_ATTRIB_FORMAT_SNORM_8_8_8_8
    );
    GX2InitAttribStream(
        &(mAttribute[FFL_ATTRIBUTE_BUFFER_TYPE_COLOR]),
        mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_COLOR],
        FFL_ATTRIBUTE_BUFFER_TYPE_COLOR,
        0,
        GX2_ATTRIB_FORMAT_UNORM_8_8_8_8
    );

    u32 size = GX2CalcFetchShaderSizeEx(FFL_ATTRIBUTE_BUFFER_TYPE_MAX, GX2_FETCH_SHADER_TESSELLATION_NONE, GX2_TESSELLATION_MODE_DISCRETE);
    void* buffer = rio::MemUtil::alloc(size, GX2_SHADER_ALIGNMENT);
    GX2InitFetchShaderEx(&mFetchShader, (u8*)buffer, FFL_ATTRIBUTE_BUFFER_TYPE_MAX, mAttribute, GX2_FETCH_SHADER_TESSELLATION_NONE, GX2_TESSELLATION_MODE_DISCRETE);
#elif RIO_IS_WIN
    RIO_ASSERT(mVAOHandle == GL_NONE);

    // OpenGL <4.5 compatible buffer creation
    GLuint buffers[FFL_ATTRIBUTE_BUFFER_TYPE_MAX];
    RIO_GL_CALL(glGenBuffers(FFL_ATTRIBUTE_BUFFER_TYPE_MAX, buffers));
    for (int i = 0; i < FFL_ATTRIBUTE_BUFFER_TYPE_MAX; ++i) {
        mVBOHandle[i] = buffers[i];
    }

    // Generate Vertex Array Object (VAO)
    RIO_GL_CALL(glGenVertexArrays(1, &mVAOHandle));
    RIO_ASSERT(mVAOHandle != GL_NONE);
#endif

    mSampler.setWrap(rio::TEX_WRAP_MODE_MIRROR, rio::TEX_WRAP_MODE_MIRROR, rio::TEX_WRAP_MODE_MIRROR);

    mCallback.pObj = this;
    mCallback.pApplyAlphaTestFunc = &Shader::applyAlphaTestCallback_;
    mCallback.pDrawFunc = &Shader::drawCallback_;
    mCallback.pSetMatrixFunc = &Shader::setMatrixCallback_;
    FFLSetShaderCallback(&mCallback);
}

void Shader::bind() const
{
    mShader.bind();
#if RIO_IS_CAFE
    GX2SetFetchShader(&mFetchShader);
#elif RIO_IS_WIN
    RIO_GL_CALL(glBindVertexArray(mVAOHandle));
    for (u32 i = 0; i < FFL_ATTRIBUTE_BUFFER_TYPE_MAX; i++)
        RIO_GL_CALL(glDisableVertexAttribArray(i));
#endif

    mShader.setUniform(0.0f, 0.0f, 1.0f, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_LIGHT_DIR]);
    //mShader.setUniform(-1.0f, 0.5f, 0.78f, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_LIGHT_DIR]);

    mShader.setUniform(true, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_LIGHT_ENABLE]);
    mShader.setUniform(0.73f, 0.73f, 0.73f, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_LIGHT_AMBIENT]);
    mShader.setUniform(0.6f, 0.6f, 0.6f, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_LIGHT_DIFFUSE]);
    mShader.setUniform(0.69999999f, 0.69999999f, 0.69999999f, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_LIGHT_SPECULAR]);
    /*
    mShader.setUniform(0.75f, 0.75f, 0.75f, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_MATERIAL_AMBIENT]);
    mShader.setUniform(0.75f, 0.75f, 1.0f, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_MATERIAL_DIFFUSE]);
    mShader.setUniform(1.0f, 1.0f, 1.0f, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_MATERIAL_SPECULAR]);
    mShader.setUniform(40.0f, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_MATERIAL_SPECULAR_POWER]);
    mShader.setUniform(s32(0.0), s32(-1), mPixelUniformLocation[PIXEL_UNIFORM_MATERIAL_SPECULAR_MODE]);
    */
    mShader.setUniform(0.3f, 0.3f, 0.3f, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_RIM_COLOR]);
    mShader.setUniform(2.0f, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_RIM_POWER]);
}

void Shader::setViewUniform(const rio::BaseMtx34f& model_mtx, const rio::BaseMtx34f& view_mtx, const rio::BaseMtx44f& proj_mtx) const
{
    rio::Matrix34f mv;
    mv.setMul(static_cast<const rio::Matrix34f&>(view_mtx), static_cast<const rio::Matrix34f&>(model_mtx));
    rio::Matrix44f mv44;
    mv44.fromMatrix34(mv);
    mShader.setUniform(mv44, mVertexUniformLocation[VERTEX_UNIFORM_MV], u32(-1));

    mShader.setUniform(proj_mtx, mVertexUniformLocation[VERTEX_UNIFORM_PROJ], u32(-1));

    rio::Matrix44f it44;
    if (it44.setInverse(mv44))
    {
        const rio::BaseMtx33f it33 {
            it44.m[0][0], it44.m[0][1], it44.m[0][2],
            it44.m[1][0], it44.m[1][1], it44.m[1][2],
            it44.m[2][0], it44.m[2][1], it44.m[2][2]
        };

        mShader.setUniformColumnMajor(it33, mVertexUniformLocation[VERTEX_UNIFORM_IT], u32(-1));
    }
    else
    {
        static const rio::BaseMtx33f ident33 {
            1.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 1.0f
        };

        mShader.setUniformColumnMajor(ident33, mVertexUniformLocation[VERTEX_UNIFORM_IT], u32(-1));
    }
}

void Shader::applyAlphaTest(bool enable, rio::Graphics::CompareFunc func, f32 ref) const
{
#if RIO_IS_CAFE
    GX2SetAlphaTest(enable, GX2CompareFunction(func), ref);
#elif RIO_IS_WIN
    mShader.setUniform(u32(func - GL_NEVER), u32(-1), mShader.getFragmentUniformLocation("PS_PUSH.alphaFunc"));
    mShader.setUniform(ref,                  u32(-1), mShader.getFragmentUniformLocation("PS_PUSH.alphaRef"));
#endif
}

void Shader::setCulling(FFLCullMode mode)
{
    if (mode > FFL_CULL_MODE_FRONT)
        return;

    rio::RenderState render_state;

    switch (mode)
    {
    case FFL_CULL_MODE_NONE:
        render_state.setCullingMode(rio::Graphics::CULLING_MODE_NONE);
        break;
    case FFL_CULL_MODE_BACK:
        render_state.setCullingMode(rio::Graphics::CULLING_MODE_BACK);
        break;
    case FFL_CULL_MODE_FRONT:
        render_state.setCullingMode(rio::Graphics::CULLING_MODE_FRONT);
        break;
    default:
        break;
    }

    render_state.applyCullingAndPolygonModeAndPolygonOffset();
}

void Shader::applyAlphaTestCallback_(void* p_obj, bool enable, rio::Graphics::CompareFunc func, f32 ref)
{
    static_cast<Shader*>(p_obj)->applyAlphaTest(enable, func, ref);
}

#ifndef NO_MATERIAL
enum FFLDrawType {
    FFL_DRAW_TYPE_DRAW_OPA_FACELINE = 0,
    FFL_DRAW_TYPE_DRAW_OPA_BEARD = 1,
    FFL_DRAW_TYPE_DRAW_OPA_NOSE = 2,
    FFL_DRAW_TYPE_DRAW_OPA_FOREHEAD = 3,
    FFL_DRAW_TYPE_DRAW_OPA_HAIR = 4,
    FFL_DRAW_TYPE_DRAW_OPA_CAP = 5,
    FFL_DRAW_TYPE_DRAW_XLU_MASK = 6,
    FFL_DRAW_TYPE_DRAW_XLU_NOSELINE = 7,
    FFL_DRAW_TYPE_DRAW_XLU_GLASS = 8,
    FFL_DRAW_TYPE_DRAW_TERM = 8
};

// map FFLModulateType to FFLDrawType
FFLDrawType mapFFLModulateTypeToFFLDrawType(FFLModulateType modulateType) {
    switch (modulateType) {
        case FFL_MODULATE_TYPE_SHAPE_FACELINE:
        case FFL_MODULATE_TYPE_FACE_MAKE:
        case FFL_MODULATE_TYPE_FACE_LINE:
            return FFL_DRAW_TYPE_DRAW_OPA_FACELINE;
        case FFL_MODULATE_TYPE_SHAPE_BEARD:
            return FFL_DRAW_TYPE_DRAW_OPA_BEARD;
        case FFL_MODULATE_TYPE_SHAPE_NOSE:
            return FFL_DRAW_TYPE_DRAW_OPA_NOSE;
        case FFL_MODULATE_TYPE_SHAPE_FOREHEAD:
            return FFL_DRAW_TYPE_DRAW_OPA_FOREHEAD;
        case FFL_MODULATE_TYPE_SHAPE_HAIR:
            return FFL_DRAW_TYPE_DRAW_OPA_HAIR;
        case FFL_MODULATE_TYPE_SHAPE_CAP:
            return FFL_DRAW_TYPE_DRAW_OPA_CAP;
        case FFL_MODULATE_TYPE_SHAPE_MASK:
        case FFL_MODULATE_TYPE_MUSTACHE:
        case FFL_MODULATE_TYPE_MOUTH:
        case FFL_MODULATE_TYPE_EYEBROW:
        case FFL_MODULATE_TYPE_FACE_BEARD:
        case FFL_MODULATE_TYPE_EYE:
        case FFL_MODULATE_TYPE_MOLE:
        case FFL_MODULATE_TYPE_FILL:
            return FFL_DRAW_TYPE_DRAW_XLU_MASK;
        case FFL_MODULATE_TYPE_SHAPE_NOSELINE:
            return FFL_DRAW_TYPE_DRAW_XLU_NOSELINE;
        case FFL_MODULATE_TYPE_SHAPE_GLASS:
            return FFL_DRAW_TYPE_DRAW_XLU_GLASS;
        default:
            return FFL_DRAW_TYPE_DRAW_OPA_FACELINE;
    }
}

struct FFLDefaultShaderMaterial {
    FFLColor materialAmbient;
    FFLColor materialDiffuse;
    FFLColor materialSpecular;
    float materialSpecularPower;
    u32 materialSpecularMode;
};

struct FFLShaderLight {
    FFLColor lightAmbient;
    FFLColor lightDiffuse;
    FFLColor lightSpecular;
};

struct FFLDefaultShaderRim {
    FFLColor rimColor;
    float rimPower;
};

struct FFLShaderParam {
    FFLDefaultShaderMaterial materialParam[9];
    FFLShaderLight lightParam;
    FFLDefaultShaderRim rimParam;
};

const FFLShaderParam shaderParam = {
    // materialParam
    {
        // FFL_DRAW_TYPE_DRAW_OPA_FACELINE
        {
            {0.85f, 0.75f, 0.75f, 1.0f},
            {0.75f, 0.75f, 0.75f, 1.0f},
            {0.3f, 0.3f, 0.3f, 1.0f},
            1.2f,
            0
        },
        // FFL_DRAW_TYPE_DRAW_OPA_BEARD
        {
            {1.0f, 1.0f, 1.0f, 1.0f},
            {0.69999999f, 0.69999999f, 0.69999999f, 1.0f},
            {0.0f, 0.0f, 0.0f, 1.0f},
            40.0f,
            1
        },
        // FFL_DRAW_TYPE_DRAW_OPA_NOSE
        {
            {0.89999998f, 0.85f, 0.85f, 1.0f},
            {0.75f, 0.75f, 0.75f, 1.0f},
            {0.22f, 0.22f, 0.22f, 1.0f},
            1.5f,
            0
        },
        // FFL_DRAW_TYPE_DRAW_OPA_FOREHEAD
        {
            {0.85f, 0.75f, 0.75f, 1.0f},
            {0.75f, 0.75f, 0.75f, 1.0f},
            {0.3f, 0.3f, 0.3f, 1.0f},
            1.2f,
            0
        },
        // FFL_DRAW_TYPE_DRAW_OPA_HAIR
        {
            {1.0f, 1.0f, 1.0f, 1.0f},
            {0.69999999f, 0.69999999f, 0.69999999f, 1.0f},
            {0.0f, 0.0f, 0.0f, 1.0f},
            40.0f,
            1
        },
        // FFL_DRAW_TYPE_DRAW_OPA_CAP
        {
            {0.75f, 0.75f, 0.75f, 1.0f},
            {0.72f, 0.72f, 0.72f, 1.0f},
            {0.3f, 0.3f, 0.3f, 1.0f},
            1.5f,
            0
        },
        // FFL_DRAW_TYPE_DRAW_XLU_MASK
        {
            {1.0f, 1.0f, 1.0f, 1.0f},
            {0.69999999f, 0.69999999f, 0.69999999f, 1.0f},
            {0.0f, 0.0f, 0.0f, 1.0f},
            40.0f,
            1
        },
        // FFL_DRAW_TYPE_DRAW_XLU_NOSELINE
        {
            {1.0f, 1.0f, 1.0f, 1.0f},
            {0.69999999f, 0.69999999f, 0.69999999f, 1.0f},
            {0.0f, 0.0f, 0.0f, 1.0f},
            40.0f,
            1
        },
        // FFL_DRAW_TYPE_DRAW_XLU_GLASS
        {
            {1.0f, 1.0f, 1.0f, 1.0f},
            {0.69999999f, 0.69999999f, 0.69999999f, 1.0f},
            {0.0f, 0.0f, 0.0f, 1.0f},
            40.0f,
            1
        }
    },
    // lightParam
    {
        {0.73f, 0.73f, 0.73f, 1.0f},
        {0.6f, 0.6f, 0.6f, 1.0f},
        {0.69999999f, 0.69999999f, 0.69999999f, 1.0f}
    },
    // rimParam
    {
        {0.3f, 0.3f, 0.3f, 1.0f},
        2.0f
    }
};
#endif

void Shader::draw_(const FFLDrawParam& draw_param)
{
    // Set the culling mode
    setCulling(draw_param.cullMode);

    // Set the modulation mode uniform
    mShader.setUniform(s32(draw_param.modulateParam.mode), u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_MODE]);

    // Handle different modulation modes
    switch (draw_param.modulateParam.mode)
    {
    case FFL_MODULATE_MODE_0:
    case FFL_MODULATE_MODE_3:
    case FFL_MODULATE_MODE_4:
    case FFL_MODULATE_MODE_5:
        mShader.setUniform(getColorUniform(*draw_param.modulateParam.pColorR), u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_CONST1]);
        break;
    case FFL_MODULATE_MODE_2:
        mShader.setUniform(getColorUniform(*draw_param.modulateParam.pColorR), u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_CONST1]);
        mShader.setUniform(getColorUniform(*draw_param.modulateParam.pColorG), u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_CONST2]);
        mShader.setUniform(getColorUniform(*draw_param.modulateParam.pColorB), u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_CONST3]);
        break;
    default:
        break;
    }


    // Set the modulation mode uniform
    mShader.setUniform(s32(draw_param.modulateParam.mode), u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_MODE]);

    #ifndef NO_MATERIAL
    // Map FFLModulateType to FFLDrawType
    FFLDrawType drawType = mapFFLModulateTypeToFFLDrawType(draw_param.modulateParam.type);

    // Get the corresponding material
    const FFLDefaultShaderMaterial& material = shaderParam.materialParam[drawType];
    // Set the material values
    mShader.setUniform(material.materialAmbient.r, material.materialAmbient.g, material.materialAmbient.b, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_MATERIAL_AMBIENT]);
    mShader.setUniform(material.materialDiffuse.r, material.materialDiffuse.g, material.materialDiffuse.b, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_MATERIAL_DIFFUSE]);
    mShader.setUniform(material.materialSpecular.r, material.materialSpecular.g, material.materialSpecular.b, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_MATERIAL_SPECULAR]);
    mShader.setUniform(material.materialSpecularPower, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_MATERIAL_SPECULAR_POWER]);
    mShader.setUniform(s32(material.materialSpecularMode), s32(-1), mPixelUniformLocation[PIXEL_UNIFORM_MATERIAL_SPECULAR_MODE]);
    #endif

    // Bind texture if available
    if (draw_param.modulateParam.pTexture2D != nullptr)
    {
        mSampler.linkTexture2D(draw_param.modulateParam.pTexture2D);
        mSampler.tryBindFS(mSamplerLocation, 0);
    }

    // Bind attribute buffers and draw elements if index buffer is available
    if (draw_param.primitiveParam.pIndexBuffer != nullptr)
    {
#if RIO_IS_CAFE
        for (int type = FFL_ATTRIBUTE_BUFFER_TYPE_POSITION; type <= FFL_ATTRIBUTE_BUFFER_TYPE_COLOR; ++type)
        {
            GX2SetAttribBuffer(
                type,
                draw_param.attributeBufferParam.attributeBuffers[type].size,
                draw_param.attributeBufferParam.attributeBuffers[type].stride,
                draw_param.attributeBufferParam.attributeBuffers[type].ptr
            );
        }
#elif RIO_IS_WIN
        for (int type = FFL_ATTRIBUTE_BUFFER_TYPE_POSITION; type <= FFL_ATTRIBUTE_BUFFER_TYPE_COLOR; ++type)
        {
            const FFLAttributeBuffer& buffer = draw_param.attributeBufferParam.attributeBuffers[type];
            s32 location = mAttributeLocation[type];
            void* ptr = buffer.ptr;

            if (ptr && location != -1)
            {
                u32 stride = buffer.stride;
                u32 vbo_handle = mVBOHandle[type];
                u32 size = buffer.size;

                if (stride == 0)
                {
                    // Directly set vertex attribute without buffer
                    switch (type)
                    {
                    case FFL_ATTRIBUTE_BUFFER_TYPE_POSITION:
                        RIO_GL_CALL(glVertexAttrib3fv(location, static_cast<f32*>(ptr)));
                        break;
                    case FFL_ATTRIBUTE_BUFFER_TYPE_TEXCOORD:
                        RIO_GL_CALL(glVertexAttrib2fv(location, static_cast<f32*>(ptr)));
                        break;
                    case FFL_ATTRIBUTE_BUFFER_TYPE_NORMAL:
                        RIO_GL_CALL(glVertexAttribP4ui(location, GL_INT_2_10_10_10_REV, true, *static_cast<u32*>(ptr)));
                        break;
                    case FFL_ATTRIBUTE_BUFFER_TYPE_TANGENT:
                        RIO_GL_CALL(glVertexAttrib4Nbv(location, static_cast<s8*>(ptr)));
                        break;
                    case FFL_ATTRIBUTE_BUFFER_TYPE_COLOR:
                        RIO_GL_CALL(glVertexAttrib4Nubv(location, static_cast<u8*>(ptr)));
                        break;
                    default:
                        break;
                    }
                }
                else
                {
                    // Bind buffer and set vertex attribute pointer
                    RIO_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo_handle));
                    RIO_GL_CALL(glBufferData(GL_ARRAY_BUFFER, size, ptr, GL_STATIC_DRAW));
                    RIO_GL_CALL(glEnableVertexAttribArray(location));

                    // Determine attribute pointer parameters based on buffer type
                    switch (type)
                    {
                    case FFL_ATTRIBUTE_BUFFER_TYPE_POSITION:
                        RIO_GL_CALL(glVertexAttribPointer(location, 3, GL_FLOAT, false, stride, nullptr));
                        break;
                    case FFL_ATTRIBUTE_BUFFER_TYPE_TEXCOORD:
                        #ifdef RIO_NO_CLIP_CONTROL
                        // accomodate OpenGL default clip plane by flipping texCoords
                        // NOTE: results in flipping face texture, RATHER THAN...
                        // ... leaving it upside down, and flipping it SOMEWHERE else (not in shader or here...???)
                        // NOTE: CAN ALSO PROBABLY BE ADDED TO FFL ITSELF, I just don't know where to put it
                        if (draw_param.modulateParam.type == FFL_MODULATE_TYPE_SHAPE_MASK ||
                            draw_param.modulateParam.type == FFL_MODULATE_TYPE_FACE_MAKE ||
                            draw_param.modulateParam.type == FFL_MODULATE_TYPE_FACE_LINE)
                        {
                            // Flip the texture coordinates within the buffer
                            const u32 numTexCoords = size / sizeof(f32);
                            f32 flippedTexCoords[numTexCoords];
                            for (u32 i = 0; i < numTexCoords; i += 2)
                            {
                                f32* originalTexCoord = static_cast<f32*>(ptr) + i;
                                flippedTexCoords[i] = originalTexCoord[0];
                                // flip Y coordinate
                                flippedTexCoords[i + 1] = 1.0f - originalTexCoord[1];
                            }
                            RIO_GL_CALL(glBufferData(GL_ARRAY_BUFFER, size, flippedTexCoords, GL_STATIC_DRAW));
                        }
                        else
                        {
                            RIO_GL_CALL(glBufferData(GL_ARRAY_BUFFER, size, ptr, GL_STATIC_DRAW));
                        }
                        #else
                        RIO_GL_CALL(glBufferData(GL_ARRAY_BUFFER, size, ptr, GL_STATIC_DRAW));
                        #endif
                        RIO_GL_CALL(glVertexAttribPointer(location, 2, GL_FLOAT, false, stride, nullptr));
                        break;
                    case FFL_ATTRIBUTE_BUFFER_TYPE_NORMAL:
                        RIO_GL_CALL(glVertexAttribPointer(location, 4, GL_INT_2_10_10_10_REV, true, stride, nullptr));
                        break;
                    case FFL_ATTRIBUTE_BUFFER_TYPE_TANGENT:
                        RIO_GL_CALL(glVertexAttribPointer(location, 4, GL_BYTE, true, stride, nullptr));
                        break;
                    case FFL_ATTRIBUTE_BUFFER_TYPE_COLOR:
                        RIO_GL_CALL(glVertexAttribPointer(location, 4, GL_UNSIGNED_BYTE, true, stride, nullptr));
                        break;
                    default:
                        break;
                    }
                }
            }
        }
#endif

        // Draw elements
        rio::Drawer::DrawElements(
            draw_param.primitiveParam.primitiveType,
            draw_param.primitiveParam.indexCount,
            (const u16*)draw_param.primitiveParam.pIndexBuffer
        );
    }
}


void Shader::drawCallback_(void* p_obj, const FFLDrawParam& draw_param)
{
    static_cast<Shader*>(p_obj)->draw_(draw_param);
}

void Shader::setMatrix_(const rio::BaseMtx44f& matrix)
{
    mShader.setUniformColumnMajor(rio::Matrix44f::ident, mVertexUniformLocation[VERTEX_UNIFORM_MV], u32(-1));

    static const rio::BaseMtx33f ident33 {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };

    mShader.setUniformColumnMajor(ident33, mVertexUniformLocation[VERTEX_UNIFORM_IT], u32(-1));

    mShader.setUniform(matrix, mVertexUniformLocation[VERTEX_UNIFORM_PROJ], u32(-1));
}

void Shader::setMatrixCallback_(void* p_obj, const rio::BaseMtx44f& matrix)
{
    static_cast<Shader*>(p_obj)->setMatrix_(matrix);
}
