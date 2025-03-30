#include <Shader.h>

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


const rio::BaseVec4f& getColorUniform(const FFLColor& color)
{
    return reinterpret_cast<const rio::BaseVec4f&>(color.r);
}

void safeNormalizeVec3(rio::BaseVec3f* vec)
{
    float magnitude = std::sqrt(vec->x * vec->x + vec->y * vec->y + vec->z * vec->z);
    if (magnitude != 0.0f)
    {
        vec->x /= magnitude;
        vec->y /= magnitude;
        vec->z /= magnitude;
    }

    vec->x = std::fmax(std::fmin(vec->x, 1.0f), -1.0f);
    vec->y = std::fmax(std::fmin(vec->y, 1.0f), -1.0f);
    vec->z = std::fmax(std::fmin(vec->z, 1.0f), -1.0f);

    if (vec->x == 1.0f || vec->x == -1.0f)
    {
        vec->y = 0.0f;
        vec->z = 0.0f;
    }
    else if (vec->y == 1.0f || vec->y == -1.0f)
    {
        vec->x = 0.0f;
        vec->z = 0.0f;
    }
    else if (vec->z == 1.0f || vec->z == -1.0f)
    {
        vec->x = 0.0f;
        vec->y = 0.0f;
    }
}

void gramSchmidtOrthonormalizeMtx34(rio::BaseMtx34f* mat)
{
    // Extract and normalize the first column
    rio::BaseVec3f c0 = {
        mat->m[0][0],
        mat->m[1][0],
        mat->m[2][0]
    };
    rio::BaseVec3f c0Normalized = c0;
    safeNormalizeVec3(&c0Normalized);

    // Extract and normalize the second column
    rio::BaseVec3f c1 = {
        mat->m[0][1],
        mat->m[1][1],
        mat->m[2][1]
    };
    rio::BaseVec3f c1Normalized = c1;
    safeNormalizeVec3(&c1Normalized);

    // Compute the third column as the cross product of the first two normalized columns
    rio::BaseVec3f c2New = {
        c0Normalized.y * c1Normalized.z - c0Normalized.z * c1Normalized.y,
        c0Normalized.z * c1Normalized.x - c0Normalized.x * c1Normalized.z,
        c0Normalized.x * c1Normalized.y - c0Normalized.y * c1Normalized.x
    };

    // Compute the new second column as the cross product of the third column and the first normalized column
    rio::BaseVec3f c1New = {
        c2New.y * c0Normalized.z - c2New.z * c0Normalized.y,
        c2New.z * c0Normalized.x - c2New.x * c0Normalized.z,
        c2New.x * c0Normalized.y - c2New.y * c0Normalized.x
    };

    // Update the matrix with the new orthonormal columns
    mat->m[0][0] = c0Normalized.x;
    mat->m[1][0] = c0Normalized.y;
    mat->m[2][0] = c0Normalized.z;

    mat->m[0][1] = c1New.x;
    mat->m[1][1] = c1New.y;
    mat->m[2][1] = c1New.z;

    mat->m[0][2] = c2New.x;
    mat->m[1][2] = c2New.y;
    mat->m[2][2] = c2New.z;
}

struct FFLiDefaultShaderMaterial
{
    FFLColor ambient;
    FFLColor diffuse;
    FFLColor specular;
    f32 specularPower;
    FFLiDefaultShaderSpecularMode specularMode;
};

const FFLiDefaultShaderMaterial cMaterialParam[CUSTOM_MATERIAL_PARAM_SIZE] = {
    { // ShapeFaceline
        { 0.85f, 0.75f, 0.75f, 1.0f }, // ambient
        { 0.75f, 0.75f, 0.75f, 1.0f }, // diffuse
        { 0.30f, 0.30f, 0.30f, 1.0f }, // specular
        1.2f, // specularPower
        FFL_SPECULAR_MODE_NORMAL // specularMode
    },
    { // ShapeBeard
        { 1.0f, 1.0f, 1.0f, 1.0f }, // ambient
        { 0.7f, 0.7f, 0.7f, 1.0f }, // diffuse
        { 0.0f, 0.0f, 0.0f, 1.0f }, // specular
        40.0f, // specularPower
        FFL_SPECULAR_MODE_NORMAL // specularMode
    },
    { // ShapeNose
        { 0.90f, 0.85f, 0.85f, 1.0f }, // ambient
        { 0.75f, 0.75f, 0.75f, 1.0f }, // diffuse
        { 0.22f, 0.22f, 0.22f, 1.0f }, // specular
        1.5f, // specularPower
        FFL_SPECULAR_MODE_NORMAL // specularMode
    },
    { // ShapeForehead
        { 0.85f, 0.75f, 0.75f, 1.0f }, // ambient
        { 0.75f, 0.75f, 0.75f, 1.0f }, // diffuse
        { 0.30f, 0.30f, 0.30f, 1.0f }, // specular
        1.2f, // specularPower
        FFL_SPECULAR_MODE_NORMAL // specularMode
    },
    { // ShapeHair
        { 1.00f, 1.00f, 1.00f, 1.0f }, // ambient
        { 0.70f, 0.70f, 0.70f, 1.0f }, // diffuse
        { 0.35f, 0.35f, 0.35f, 1.0f }, // specular
        10.0f, // specularPower
        FFL_SPECULAR_MODE_ANISOTROPIC // specularMode
    },
    { // ShapeCap
        { 0.75f, 0.75f, 0.75f, 1.0f }, // ambient
        { 0.72f, 0.72f, 0.72f, 1.0f }, // diffuse
        { 0.30f, 0.30f, 0.30f, 1.0f }, // specular
        1.5f, // specularPower
        FFL_SPECULAR_MODE_NORMAL // specularMode
    },
    { // ShapeMask
        { 1.0f, 1.0f, 1.0f, 1.0f }, // ambient
        { 0.7f, 0.7f, 0.7f, 1.0f }, // diffuse
        { 0.0f, 0.0f, 0.0f, 1.0f }, // specular
        40.0f, // specularPower
        FFL_SPECULAR_MODE_ANISOTROPIC // specularMode
    },
    { // ShapeNoseline
        { 1.0f, 1.0f, 1.0f, 1.0f }, // ambient
        { 0.7f, 0.7f, 0.7f, 1.0f }, // diffuse
        { 0.0f, 0.0f, 0.0f, 1.0f }, // specular
        40.0f, // specularPower
        FFL_SPECULAR_MODE_ANISOTROPIC // specularMode
    },
    { // ShapeGlass
        { 1.0f, 1.0f, 1.0f, 1.0f }, // ambient
        { 0.7f, 0.7f, 0.7f, 1.0f }, // diffuse
        { 0.0f, 0.0f, 0.0f, 1.0f }, // specular
        40.0f, // specularPower
        FFL_SPECULAR_MODE_ANISOTROPIC // specularMode
    },
    // HACK: THESE COLLIDE!!!
    // but it's with textures which have no lighting
    { // body
        { 0.95622f, 0.95622f, 0.95622f, 1.0f }, // 0.69804
        { 0.496733f, 0.496733f, 0.496733f, 1.0f }, // 0.29804
        { 0.2409f, 0.2409f, 0.2409f, 1.0f }, // 0.16863
        3.0f, // specularPower
        FFL_SPECULAR_MODE_NORMAL // specularMode
    },
    { // pants
        { 0.95622f, 0.95622f, 0.95622f, 1.0f }, // 0.69804
        { 1.084967f, 1.084967f, 1.084967f, 1.0f }, // 0.65098
        { 0.2409f, 0.2409f, 0.2409f, 1.0f }, // 0.16863
        3.0f, // specularPower
        FFL_SPECULAR_MODE_NORMAL // specularMode
    }
};

const FFLColor cLightAmbient  = { 0.73f, 0.73f, 0.73f, 1.0f };
const FFLColor cLightDiffuse  = { 0.60f, 0.60f, 0.60f, 1.0f };
const FFLColor cLightSpecular = { 0.70f, 0.70f, 0.70f, 1.0f };

const rio::BaseVec3f cLightDir = { -0.4531539381f, 0.4226179123f, 0.7848858833f }; // , 0.30943f

const FFLColor cRimColor = { 0.3f, 0.3f, 0.3f, 1.0f };
const FFLColor cRimColorBody = { 0.4f, 0.4f, 0.4f, 1.0f };

const f32 cRimPower = 2.0f;

}

Shader::Shader()
#if RIO_IS_CAFE
    : mAttribute()
    , mFetchShader()
#elif RIO_IS_WIN
    : mVBOHandle()
    , mVAOHandle()
#endif
    , mCallback()
    , mpCharInfo(nullptr)
    , mSpecularMode(FFL_SPECULAR_MODE_ANISOTROPIC)
    // default values
    , mDefaultLightDir(cLightDir)
    , mDefaultLightAmbient(cLightAmbient)
    , mDefaultLightDiffuse(cLightDiffuse)
    , mDefaultLightSpecular(cLightSpecular)
    // current values
    , mLightDir(mDefaultLightDir)
    , mLightAmbient(mDefaultLightAmbient)
    , mLightDiffuse(mDefaultLightDiffuse)
    , mLightSpecular(mDefaultLightSpecular)
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

void Shader::resetUniformsToDefault()
{
    mLightDir = mDefaultLightDir;
    mLightAmbient = mDefaultLightAmbient;
    mLightDiffuse = mDefaultLightDiffuse;
    mLightSpecular = mDefaultLightSpecular;
}

void Shader::initialize()
{
    mShader.load("FFLShader", rio::Shader::MODE_UNIFORM_REGISTER);

    mVertexUniformLocation[VERTEX_UNIFORM_IT]   = mShader.getVertexUniformLocation("u_it");
    mVertexUniformLocation[VERTEX_UNIFORM_MV]   = mShader.getVertexUniformLocation("u_mv");
    mVertexUniformLocation[VERTEX_UNIFORM_PROJ] = mShader.getVertexUniformLocation("u_proj");

    mVertexUniformLocation[VERTEX_UNIFORM_MTX_PALETTE] = mShader.getVertexUniformLocation("u_mtxPalette");
    mVertexUniformLocation[VERTEX_UNIFORM_SKIN_COUNT] = mShader.getVertexUniformLocation("u_vtxSkinCount");

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
    // Custom uniform to set values previously handled by v_color if the color attribute is constant (stride = 0).
    mPixelUniformLocation[PIXEL_UNIFORM_PARAMETER_MODE]             = mShader.getFragmentUniformLocation("u_parameter_mode");

    mSamplerLocation = mShader.getFragmentSamplerLocation("s_texture");

    mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_POSITION]  = mShader.getVertexAttribLocation("a_position");
    // 2D planes (faceline/mask) do not have anything but position and texcoord
    mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_NORMAL]    = mShader.getVertexAttribLocation("a_normal");
    // hair does not have texcoord
    mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_TEXCOORD] = mShader.getVertexAttribLocation("a_texCoord");
    /* NOTE: "color" is not meant as an actual color... it is
     * only here as parameters to control the anisotropic effect
     * color.r - aniso specular blend, param 1 to calculateSpecularBlend
                 (switch SampleShader: "specularMix" in vertex shader)
     * color.g - aniso specular strength, param 4 to calculateSpecularColor
                 (for blinn they force it to 1.0)
     * color.b - unused and set to 0
     * color.a - rim lighting width "rimWidth", param 3 into calculateRimColor
                 (when rim lighting is not active, usually the reason is that A is 0 here)

     * color's stride is 0 and size is 4 on everything else, suggesting it is constant

     * in the FFL resource on Wii U, all shapes where color's stride is 0 use the value: <1, 1, 0, 1>
     * ... EXCEPT for where R is 0 in: hair type 34/beanie, and hair
     *              for hat meshes: 18, 23, 34, 39, 45, 57, 114, 127
     */
    mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_COLOR]     = mShader.getVertexAttribLocation("a_color");
    // tangent is only set on hair, size and stride are 0 on everything else
    mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_TANGENT]   = mShader.getVertexAttribLocation("a_tangent");

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
    RIO_GL_CALL(glGenBuffers(FFL_ATTRIBUTE_BUFFER_TYPE_MAX, mVBOHandle));
    RIO_GL_CALL(glGenVertexArrays(1, &mVAOHandle));
    RIO_ASSERT(mVAOHandle != GL_NONE);
#endif

    mSampler.setWrap(rio::TEX_WRAP_MODE_MIRROR, rio::TEX_WRAP_MODE_MIRROR, rio::TEX_WRAP_MODE_MIRROR);

    mSampler.setFilter(rio::TEX_XY_FILTER_MODE_LINEAR, rio::TEX_XY_FILTER_MODE_LINEAR, rio::TEX_MIP_FILTER_MODE_LINEAR, rio::TEX_ANISO_1_TO_1);

    mCallback.pObj = this;
    mCallback.pApplyAlphaTestFunc = &Shader::applyAlphaTestCallback_;
    mCallback.pDrawFunc = &Shader::drawCallback_;
    mCallback.pSetMatrixFunc = &Shader::setMatrixCallback_;
    setShaderCallback_();
}

void Shader::setShaderCallback_()
{
    FFLSetShaderCallback(&mCallback);
}

// think this needs to be in radians
void Shader::setLightDirection(const rio::Vector3f direction)
{
    rio::BaseVec3f newLightDirection = cLightDir;
    // only set each axis if it is non-negative
    if (direction.x > -0.01)
        newLightDirection.x = direction.x;
    if (direction.y > -0.01)
        newLightDirection.y = direction.y;
    if (direction.z > -0.01)
        newLightDirection.z = direction.z;
    mLightDir = newLightDirection;
}

#ifdef FFL_USE_ADJUST_MTX
rio::Matrix34f g_MV = rio::Matrix34f::ident;
#endif

void Shader::bind(bool light_enable, FFLiCharInfo* pCharInfo)
{
    mpCharInfo = pCharInfo;
    mShader.bind();
    setShaderCallback_();
#if RIO_IS_CAFE
    GX2SetFetchShader(&mFetchShader);
#elif RIO_IS_WIN
    RIO_GL_CALL(glBindVertexArray(mVAOHandle));
    for (u32 i = 0; i < FFL_ATTRIBUTE_BUFFER_TYPE_MAX; i++)
        RIO_GL_CALL(glDisableVertexAttribArray(i));
#endif

    mShader.setUniform(mLightDir, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_LIGHT_DIR]);
    mShader.setUniform(light_enable, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_LIGHT_ENABLE]);
    mShader.setUniform(getColorUniform(mLightAmbient), u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_LIGHT_AMBIENT]);
    mShader.setUniform(getColorUniform(mLightDiffuse), u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_LIGHT_DIFFUSE]);
    mShader.setUniform(getColorUniform(mLightSpecular), u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_LIGHT_SPECULAR]);

    mShader.setUniform(getColorUniform(cRimColor), u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_RIM_COLOR]);
    mShader.setUniform(cRimPower, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_RIM_POWER]);

    // reset specular mode, parameter mode
    mShader.setUniform(FFL_SPECULAR_MODE_NORMAL, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_MATERIAL_SPECULAR_MODE]);
    mShader.setUniform(1, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_PARAMETER_MODE]); // FFL_PARAMETER_MODE_DEFAULT_1

    mShader.setUniform(0, mVertexUniformLocation[VERTEX_UNIFORM_SKIN_COUNT], u32(-1));
#ifdef FFL_USE_ADJUST_MTX
    g_MV = rio::Matrix34f::ident;
#endif
}

void Shader::setBoneMatrix(rio::Matrix34f* mtx, s32 boneCount)
{
    mShader.setUniform(1, mVertexUniformLocation[VERTEX_UNIFORM_SKIN_COUNT], u32(-1));

    rio::BaseVec4f mtxPalette[3 * SHADER_MAX_BONE_COUNT];
    for (s32 i = 0; i < boneCount; i++)
    {
        const s32 mtxIndex = i * 3;
        for (s32 j = 0; j < 3; j++)
            mtxPalette[mtxIndex + j] = mtx[i].v[j];
    }
    mShader.setUniformArray(boneCount * 3, mtxPalette, mVertexUniformLocation[VERTEX_UNIFORM_MTX_PALETTE], u32(-1));
}

void Shader::setViewUniform(const rio::BaseMtx34f& model_mtx, const rio::BaseMtx34f& view_mtx, const rio::BaseMtx44f& proj_mtx) const
{
    mShader.setUniform(proj_mtx, mVertexUniformLocation[VERTEX_UNIFORM_PROJ], u32(-1));

    rio::Matrix34f mv;
    mv.setMul(static_cast<const rio::Matrix34f&>(view_mtx), static_cast<const rio::Matrix34f&>(model_mtx));
#ifdef FFL_USE_ADJUST_MTX
    g_MV = mv;
#endif
    rio::Matrix44f mv44;
    mv44.fromMatrix34(mv);
    mShader.setUniform(mv44, mVertexUniformLocation[VERTEX_UNIFORM_MV], u32(-1));

    rio::Matrix34f it34 = mv;
    it34.setInverseTranspose(mv);
    gramSchmidtOrthonormalizeMtx34(&it34);
    rio::BaseMtx33f it {
        it34.m[0][0], it34.m[1][0], it34.m[2][0],
        it34.m[0][1], it34.m[1][1], it34.m[2][1],
        it34.m[0][2], it34.m[1][2], it34.m[2][2]
    };
    mShader.setUniformColumnMajor(it, mVertexUniformLocation[VERTEX_UNIFORM_IT], u32(-1));
}

void Shader::applyAlphaTest(bool enable, rio::Graphics::CompareFunc func, f32 ref) const
{
#if RIO_IS_CAFE
    GX2SetAlphaTest(enable, GX2CompareFunction(func), ref);
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

void Shader::bindTexture_(const FFLModulateParam& modulateParam)
{
    if (modulateParam.pTexture2D != nullptr)
    {
        mSampler.linkTexture2D(reinterpret_cast<const rio::Texture2D*>(modulateParam.pTexture2D));
        mSampler.tryBindFS(mSamplerLocation, 0);
    }
}

void Shader::setConstColor_(u32 ps_loc, const FFLColor& color)
{
    mShader.setUniform(getColorUniform(color), u32(-1), ps_loc);
}

void Shader::setModulateMode_(FFLModulateMode mode)
{
    mShader.setUniform(s32(mode), u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_MODE]);
}

void Shader::setModulate_(const FFLModulateParam& modulateParam)
{
    setModulateMode_(modulateParam.mode); // Modulate mode
    setMaterial_(modulateParam.type); // Material shader uniforms

    // if you want to change colors based on modulateParam.type
    // FFL_MODULATE_TYPE_SHAPE_HAIR
    // hair color: pColorR/const1
    // FFL_MODULATE_TYPE_EYEBROW
    // eyebrow color: pColorB/const2
    // NOTE, also need to color: FFL_MODULATE_TYPE_SHAPE_BEARD, FFL_MODULATE_TYPE_MUSTACHE, FFL_MODULATE_TYPE_FACE_BEARD

    switch (modulateParam.mode)
    {
    case FFL_MODULATE_MODE_CONSTANT:
    case FFL_MODULATE_MODE_ALPHA:
    case FFL_MODULATE_MODE_LUMINANCE_ALPHA:
    case FFL_MODULATE_MODE_ALPHA_OPA:
        RIO_ASSERT(modulateParam.pColorR != nullptr);
        setConstColor_(mPixelUniformLocation[PIXEL_UNIFORM_CONST1], *modulateParam.pColorR);
        break;
    case FFL_MODULATE_MODE_RGB_LAYERED:
        RIO_ASSERT(modulateParam.pColorR != nullptr);
        setConstColor_(mPixelUniformLocation[PIXEL_UNIFORM_CONST1], *modulateParam.pColorR);
        setConstColor_(mPixelUniformLocation[PIXEL_UNIFORM_CONST2], *modulateParam.pColorG);
        setConstColor_(mPixelUniformLocation[PIXEL_UNIFORM_CONST3], *modulateParam.pColorB);
        break;
    default:
        break;
    }

    bindTexture_(modulateParam);
}

void Shader::setModulatePantsMaterial(PantsColor pantsColor)
{
    const FFLModulateParam modulateParam = {
        FFL_MODULATE_MODE_CONSTANT,
        static_cast<FFLModulateType>(CUSTOM_MATERIAL_PARAM_PANTS),
        &cPantsColors[pantsColor],
        nullptr, // no color G
        nullptr, // no color B
        nullptr  // no texture
    };
    setModulate_(modulateParam);
}

void Shader::setMaterial_(const FFLModulateType modulateType)
{
    if (modulateType >= CUSTOM_MATERIAL_PARAM_SIZE)
        return;

    mShader.setUniform(getColorUniform(cMaterialParam[modulateType].ambient), u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_MATERIAL_AMBIENT]);
    mShader.setUniform(getColorUniform(cMaterialParam[modulateType].diffuse), u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_MATERIAL_DIFFUSE]);
    mShader.setUniform(getColorUniform(cMaterialParam[modulateType].specular), u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_MATERIAL_SPECULAR]);
    mShader.setUniform(cMaterialParam[modulateType].specularPower, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_MATERIAL_SPECULAR_POWER]);

    if (modulateType == CUSTOM_MATERIAL_PARAM_BODY
        || modulateType == CUSTOM_MATERIAL_PARAM_PANTS)
    {
        // body uses different rim color
        mShader.setUniform(getColorUniform(cRimColorBody), u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_RIM_COLOR]);
        // set parameter/basically rim light enable
        mShader.setUniform(1, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_PARAMETER_MODE]); // FFL_PARAMETER_MODE_DEFAULT_1
    }
    else
        // set default rim color otherwise
        mShader.setUniform(getColorUniform(cRimColor), u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_RIM_COLOR]);
}

void Shader::draw_(const FFLDrawParam& draw_param)
{
    setCulling(draw_param.cullMode);
    setModulate_(draw_param.modulateParam);
    const FFLModulateType modulateType = draw_param.modulateParam.type;
    setMaterial_(modulateType);

    // moved from setMaterial_ to here, set material specular mode
    if (modulateType < CUSTOM_MATERIAL_PARAM_SIZE)
    {
        // blinn as default...
        FFLiDefaultShaderSpecularMode materialSpecularMode = FFL_SPECULAR_MODE_NORMAL;
        if (mSpecularMode != FFL_SPECULAR_MODE_NORMAL) // if the default is not blinn,
            materialSpecularMode = cMaterialParam[modulateType].specularMode; // set it

        // if there's no tangent then leave it as blinn
        if (draw_param.attributeBufferParam.attributeBuffers[FFL_ATTRIBUTE_BUFFER_TYPE_TANGENT].ptr == nullptr)
            materialSpecularMode = FFL_SPECULAR_MODE_NORMAL;

        mShader.setUniform(materialSpecularMode, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_MATERIAL_SPECULAR_MODE]);
    }

#ifdef FFL_USE_ADJUST_MTX
    if (draw_param.primitiveParam.pAdjustMatrix != nullptr)
    {
        rio::Matrix34f mv;
        mv.setMul(g_MV, *draw_param.primitiveParam.pAdjustMatrix);
        rio::Matrix44f mv44;
        mv44.fromMatrix34(mv);
        mShader.setUniform(mv44, mVertexUniformLocation[VERTEX_UNIFORM_MV], u32(-1));

/*
        rio::Matrix34f it34 = mv;
        it34.setInverseTranspose(mv);
        gramSchmidtOrthonormalizeMtx34(&it34);
        rio::BaseMtx33f it {
            it34.m[0][0], it34.m[1][0], it34.m[2][0],
            it34.m[0][1], it34.m[1][1], it34.m[2][1],
            it34.m[0][2], it34.m[1][2], it34.m[2][2]
        };
        mShader.setUniformColumnMajor(it, mVertexUniformLocation[VERTEX_UNIFORM_IT], u32(-1));
*/
    }
/* NOTE:
 * this should work for the most part for shapes but...
 * having to multiply on every frame isn't ideal at all
 * u_it is broken rn and g_MV global is unacceptable
 * recommend making it a uniform so the shader can factor it into the pre-existing mv, as well as removing u_it
 */
#endif

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

        GLuint indexBufferHandle;
        RIO_GL_CALL(glGenBuffers(1, &indexBufferHandle));  // Generate a new buffer

        for (int type = FFL_ATTRIBUTE_BUFFER_TYPE_POSITION; type <= FFL_ATTRIBUTE_BUFFER_TYPE_COLOR; ++type)
        {

            const FFLAttributeBuffer& buffer = draw_param.attributeBufferParam.attributeBuffers[type];
            s32 location = mAttributeLocation[type];
            void* ptr = buffer.ptr;

            if (ptr && location != -1) // only color's stride is 0
            {

                u32 stride = buffer.stride;
                u32 vbo_handle = mVBOHandle[type];
                u32 size = buffer.size;

                // Bind buffer and set vertex attribute pointer
                RIO_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo_handle));
                RIO_GL_CALL(glBufferData(GL_ARRAY_BUFFER, size, ptr, GL_STATIC_DRAW));
                RIO_GL_CALL(glEnableVertexAttribArray(location));

                // Determine attribute pointer parameters based on buffer type
                switch (type)
                {
                case FFL_ATTRIBUTE_BUFFER_TYPE_POSITION:
                    // for some reason cullMode is 3 for 2D planes
                    //if (draw_param.cullMode != FFL_CULL_MODE_MAX)
                    if (stride > 0 && stride < 12)
                        RIO_GL_CALL(glVertexAttribPointer(location, 3, GL_HALF_FLOAT, false, stride, nullptr));
                    else
                        RIO_GL_CALL(glVertexAttribPointer(location, 3, GL_FLOAT, false, stride, nullptr));
                    break;
                case FFL_ATTRIBUTE_BUFFER_TYPE_TEXCOORD:
                    if (stride > 0 && stride < 8)
                    //if (draw_param.cullMode != FFL_CULL_MODE_MAX)
                        RIO_GL_CALL(glVertexAttribPointer(location, 2, GL_HALF_FLOAT, false, stride, nullptr));
                    else
                        RIO_GL_CALL(glVertexAttribPointer(location, 2, GL_FLOAT, false, stride, nullptr));
                    break;
                case FFL_ATTRIBUTE_BUFFER_TYPE_NORMAL:
                    //if (draw_param.cullMode != FFL_CULL_MODE_MAX)
                        // use FFLiSnorm8_8_8_8
                    //    RIO_GL_CALL(glVertexAttribPointer(location, 4, GL_BYTE, true, 4, nullptr));
                    //else
                        RIO_GL_CALL(glVertexAttribPointer(location, 4, GL_INT_2_10_10_10_REV, true, stride, nullptr));
                    break;
                case FFL_ATTRIBUTE_BUFFER_TYPE_TANGENT:
                    RIO_GL_CALL(glVertexAttribPointer(location, 4, GL_BYTE, true, stride, nullptr));
                    break;
                case FFL_ATTRIBUTE_BUFFER_TYPE_COLOR:
                    if (stride == 0 && size == 4) // If color's stride is 0, its value is constant.
                    {
                        RIO_GL_CALL(glDisableVertexAttribArray(location)); // Attribute will not be used.
                        // Dereference the first value as u8.
                        const u8* color = reinterpret_cast<u8*>(ptr);
                        if (color[0] == 0) // Only possibilities for first component are 0 or 1 (read above).
                            mShader.setUniform(2, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_PARAMETER_MODE]); // FFL_PARAMETER_MODE_DEFAULT_2
                        else
                            mShader.setUniform(1, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_PARAMETER_MODE]); // FFL_PARAMETER_MODE_DEFAULT_1
                        break; // Break out of the switch case.
                    }
                    mShader.setUniform(0, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_PARAMETER_MODE]); // FFL_PARAMETER_MODE_COLOR
                    RIO_GL_CALL(glVertexAttribPointer(location, 4, GL_UNSIGNED_BYTE, true, stride, nullptr));
                    break;
                default:
                    break;
                }
            }

            else if (location != -1) // Disable attributes without corresponding VS inputs.
                RIO_GL_CALL(glDisableVertexAttribArray(location));
        }
#endif // RIO_IS_WIN

        // Draw elements
        rio::Drawer::DrawElements(
            draw_param.primitiveParam.primitiveType,
            draw_param.primitiveParam.indexCount,
            (const u16*)draw_param.primitiveParam.pIndexBuffer
        );
    }
}


void Shader::drawCallback_(void* p_obj, const FFLDrawParam* draw_param)
{
    static_cast<Shader*>(p_obj)->draw_(*draw_param);
}

void Shader::setMatrix_(const rio::BaseMtx44f& matrix)
{
    mShader.setUniform(matrix, mVertexUniformLocation[VERTEX_UNIFORM_PROJ], u32(-1));
    mShader.setUniformColumnMajor(rio::Matrix44f::ident, mVertexUniformLocation[VERTEX_UNIFORM_MV], u32(-1));

    static const rio::BaseMtx33f ident33 = {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };
    mShader.setUniformColumnMajor(ident33, mVertexUniformLocation[VERTEX_UNIFORM_IT], u32(-1));
}

void Shader::setMatrixCallback_(void* p_obj, const rio::BaseMtx44f* matrix)
{
    static_cast<Shader*>(p_obj)->setMatrix_(*matrix);
}
