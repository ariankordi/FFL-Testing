#version 330 core
/******************************************************//**
 * @file    sample.vsh
 * @brief   Vertex Shader
 * Copyright (c) 2014 Nintendo Co., Ltd. All rights reserved.
 **********************************************************/

#ifdef GL_ES
precision highp float;
#else
#   define lowp
#   define mediump
#   define highp
#endif

// 頂点シェーダーに入力される attribute 変数
in vec4 a_position;   //!< 入力: 位置情報
in vec2 a_texCoord;   //!< 入力: テクスチャー座標
// NOTE: ^^ texCoord is used as matrix index
in vec3 a_normal;     //!< 入力: 法線ベクトル
in vec4 a_color;      //!< 入力: 頂点の色
in vec3 a_tangent;    //!< 入力: 異方位

// フラグメントシェーダーへの入力
out   vec4 v_color;      //!< 出力: 頂点の色
out   vec4 v_position;   //!< 出力: 位置情報
out   vec3 v_normal;     //!< 出力: 法線ベクトル
out   vec3 v_tangent;    //!< 出力: 異方位
out   vec2 v_texCoord;   //!< 出力: テクスチャー座標

// ユニフォーム
uniform   mat3 u_it;         //!< ユニフォーム: モデルの法線用行列
uniform   mat4 u_mv;         //!< ユニフォーム: プロジェクション行列
uniform   mat4 u_proj;       //!< ユニフォーム: モデル行列

#define SHADER_MAX_BONE_COUNT 65
// TODO: RENAME/RESIZE (miitomo = 65):
uniform   vec4 u_mtxPalette[3 * SHADER_MAX_BONE_COUNT];
uniform   int  u_vtxSkinCount; // TODO: RENAME

void main()
{
//#ifdef FFL_COORDINATE_MODE_NORMAL
    // 頂点座標を変換

    if (u_vtxSkinCount > 0)
    {
        int mtxIndex = int(a_texCoord.x) * 3; // cast int
        vec4 pos_w = vec4(0, 0, 0, 1);
        vec3 nrm_w = vec3(0, 0, 0);
        vec3 tan_w = vec3(0, 0, 0);

        pos_w.x = dot(u_mtxPalette[mtxIndex + 0], a_position);
        pos_w.y = dot(u_mtxPalette[mtxIndex + 1], a_position);
        pos_w.z = dot(u_mtxPalette[mtxIndex + 2], a_position);

        nrm_w.x = dot(u_mtxPalette[mtxIndex + 0].xyz, a_normal);
        nrm_w.y = dot(u_mtxPalette[mtxIndex + 1].xyz, a_normal);
        nrm_w.z = dot(u_mtxPalette[mtxIndex + 2].xyz, a_normal);
        tan_w.x = dot(u_mtxPalette[mtxIndex + 0].xyz, a_tangent);
        tan_w.y = dot(u_mtxPalette[mtxIndex + 1].xyz, a_tangent);
        tan_w.z = dot(u_mtxPalette[mtxIndex + 2].xyz, a_tangent);

        v_position = u_mv * pos_w;

        gl_Position = u_proj * v_position;

        v_normal = normalize(u_it * nrm_w.xyz);
        v_tangent = normalize(u_it * tan_w.xyz);
        v_texCoord = vec2(0.0, 0.0);
    }
    else
    {
        v_position = u_mv * a_position;
        gl_Position = u_proj * v_position;
        v_normal = normalize(u_it * a_normal);
        v_tangent = normalize(u_it * a_tangent);
        v_texCoord = a_texCoord;
    }

    // 法線も変換
//#elif defined(FFL_COORDINATE_MODE_NONE)
//    // 頂点座標を変換
//    gl_Position = vec4(a_position.x, a_position.y * -1.0, a_position.z, a_position.w);
//    v_position = a_position;
//
//    v_normal = a_normal;
//#endif

     // その他の情報も書き出す
    //v_tangent = mat3(u_mv) * a_tangent;
    v_color = a_color;
}
