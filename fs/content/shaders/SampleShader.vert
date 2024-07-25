#version 330 core

#define VARYING_QUALIFIER out
#define VARYING_INSTANCE Out

VARYING_QUALIFIER Varying
{
    vec2 texCoord;
    vec3 normal;
    float specularMix;
} VARYING_INSTANCE;

/// ================================================================
/// 頂点シェーダーの実装
/// ================================================================

out gl_PerVertex
{
    vec4 gl_Position;
};

layout( location = 0 ) in vec4 i_Position;
layout( location = 1 ) in vec3 i_Normal;
layout( location = 2 ) in vec2 i_TexCoord;
layout( location = 3 ) in vec4 i_Parameter;

/*
layout(std140) uniform u_Matrix
{
    vec4 mv[3];
    vec4 proj[4];
};


// vec4[3] * vec4(vec3, 1)
#define TRANSFORM_POS(mtx, pos) vec3( \
    dot(mtx[0].xyzw, pos.xyzw), \
    dot(mtx[1].xyzw, pos.xyzw), \
    dot(mtx[2].xyzw, pos.xyzw))

// vec3[3] * vec3
#define TRANSFORM_VEC(mtx, vec) vec3( \
    dot(mtx[0].xyz, vec.xyz), \
    dot(mtx[1].xyz, vec.xyz), \
    dot(mtx[2].xyz, vec.xyz))

// vec4[4] * vec4
#define PROJECT(mtx, pos) vec4( \
    dot(mtx[0].xyzw, pos.xyzw), \
    dot(mtx[1].xyzw, pos.xyzw), \
    dot(mtx[2].xyzw, pos.xyzw), \
    dot(mtx[3].xyzw, pos.xyzw))
*/

uniform mat4 mv;
uniform mat4 proj;

void main()
{
    /// ビュー行列に変換
    /*vec3 vPos = TRANSFORM_POS(mv,i_Position);
    gl_Position = PROJECT(proj,vec4(vPos,1.0f));
    */
    gl_Position = proj * mv * i_Position;
    Out.normal = mat3(mv) * i_Normal.xyz;

    Out.texCoord = i_TexCoord.xy;
    //Out.normal = TRANSFORM_VEC(mv,i_Normal);
    Out.specularMix = i_Parameter.r;


#if defined(USE_DEBUG)
    gl_Position = i_Position;
#endif
    
}
