#version 330 core

#define AGX_FEATURE_ALBEDO_TEXTURE
/**
 * @file    LUT.vsh
 * @brief   LUT
 * @since   2014/10/02
 *
 * Copyright (c)2014 Nintendo Co., Ltd. All rights reserved.
 */

// シェーダーの種類毎に設定されるマクロリスト
// AGX_FEATURE_VERTEX_COLOR         頂点カラーが有効
// AGX_FEATURE_ALBEDO_TEXTURE       アルベドテクスチャーが有効
// AGX_FEATURE_BUMP_TEXTURE         バンプテクスチャーが有効
// AGX_FEATURE_MASK_TEXTURE         マスクテクスチャーが有効
// AGX_FEATURE_ALPHA_TEXTURE        アルファテクスチャーが有効
// AGX_FEATURE_SPHERE_MAP_TEXTURE   スフィア環境マップが有効
// AGX_FEATURE_SKIN_MASK            肌マスクが有効（uColor0）
// AGX_FEATURE_HAIR_MASK            髪マスクが有効（uColor1）
// AGX_FEATURE_ALPHA_TEST           アルファテストが有効
// AGX_FEATURE_FADE_OUT_COLOR       フェードアウトカラーが有効（uColor2）
// AGX_FEATURE_DISABLE_LIGHT        ライトが無効
// AGX_FEATURE_ALPHA_COLOR_FILTER   アルベドアルファによる色替えが有効
// AGX_FEATURE_ALBEDO_ALPHA         アルベドのアルファをカラーのアルファに適用
// AGX_FEATURE_PREMULTIPLY_ALPHA    プレマルチプライアルファな描画
// AGX_FEATURE_MII                  Miiを描画する
// AGX_FEATURE_MII_CONSTANT         Miiを描画する：Constant
// AGX_FEATURE_MII_TEXTURE_DIRECT   Miiを描画する：Texture Direct
// AGX_FEATURE_MII_RGB_LAYERED      Miiを描画する：RGB Layered
// AGX_FEATURE_MII_ALPHA            Miiを描画する：Alpha
// AGX_FEATURE_MII_LUMINANCE_ALPHA  Miiを描画する：Luminance Alpha
// AGX_FEATURE_MII_ALPHA_OPA        Miiを描画する：Alpha Opa
//
// AGX_BONE_MAX     ボーンの最大数

#ifdef GL_ES
precision highp float;
#else
#   define lowp
#   define mediump
#   define highp
#endif

#ifndef AGX_BONE_MAX
#   define AGX_BONE_MAX 15
#endif
#ifndef AGX_DIR_LIGHT_MAX
#   define AGX_DIR_LIGHT_MAX 2
#endif

// ----------------------------------------
// 頂点シェーダーに入力される attribute 変数
layout( location = 0 ) in highp   vec3 aPosition;   //!< 入力:[ 1 : 1 ] 位置情報
#if defined(AGX_FEATURE_ALBEDO_TEXTURE) || defined(AGX_FEATURE_BUMP_TEXTURE) || defined(AGX_FEATURE_MASK_TEXTURE) || defined(AGX_FEATURE_ALPHA_TEXTURE)
layout( location = 1 ) in mediump vec2 aTexcoord0;  //!< 入力:[ 1 : 5 ] テクスチャー座標
#endif
layout( location = 2 ) in mediump vec3 aNormal;     //!< 入力:[ 1 : 2 ] 法線ベクトル
layout( location = 3 ) in mediump vec4 aBoneIndex;  //!< 入力:[ 1 : 3 ] ボーンのインデックス（最大4つ）
layout( location = 4 ) in mediump vec4 aBoneWeight; //!< 入力:[ 1 : 4 ] ボーンの影響度（最大4つ）
#if defined(AGX_FEATURE_VERTEX_COLOR)
layout( location = 5 ) in lowp    vec4 aColor;      //!< 入力:[ 1 : 6 ] 頂点カラー
#endif
#if defined(AGX_FEATURE_BUMP_TEXTURE)
layout( location = 6 ) in mediump vec3 aTangent;    //!< 入力:[ 1 : 7 ] 接線ベクトル
#endif

// ----------------------------------------
// 頂点シェーダーに入力される uniform 変数
uniform highp   mat4 uMVPMatrix;                            //!< 入力:[ 4      /  4 :   4 ] モデルの合成行列
uniform highp   mat4 uViewMatrix;                           //!< 入力:[ 4      /  4 :   8 ] モデルのビュー行列
uniform mediump mat3 uNormalMatrix;                         //!< 入力:[ 3      /  3 :  11 ] モデルの法線用行列
uniform highp   mat4 uModelMatrix;                          //!< 入力:[ 4      /  4 :  15 ] モデルのワールド変換行列
uniform lowp    int  uBoneCount;                            //!< 入力:[ 1      /  1 :  16 ] ボーンの個数
//uniform highp   mat4 uBoneMatrices[AGX_BONE_MAX];           //!< 入力:[ 4 x 15 / 60 :  76 ] ボーンの行列配列
//uniform mediump mat3 uBoneNormalMatrices[AGX_BONE_MAX];     //!< 入力:[ 3 x 15 / 45 : 121 ] ボーンの法線行列配列
#define SHADER_MAX_BONE_COUNT 65
// TODO: RENAME/RESIZE (miitomo = 65):
uniform highp   vec4 uBoneMatrices[3 * SHADER_MAX_BONE_COUNT];

uniform lowp    int  uDirLightCount;                        //!< 入力:[ 1      /  1 : 122 ] 方向ライトの数
uniform mediump vec4 uDirLightDirAndType0;//!< 入力:[ 1 x  2 /  2 : 124 ] 平行ライトの向く方向
uniform mediump vec4 uDirLightDirAndType1;//!< 入力:[ 1 x  2 /  2 : 124 ] 平行ライトの向く方向
uniform mediump vec3 uDirLightColor0;     //!< 入力:[ 1 x  2 /  2 : 126 ] 平行ライトのカラー
uniform mediump vec3 uDirLightColor1;     //!< 入力:[ 1 x  2 /  2 : 126 ] 平行ライトのカラー
uniform mediump vec3 uHSLightSkyColor;                      //!< 入力:[ 1      /  1 : 127 ] 半球ライトのスカイカラー
uniform mediump vec3 uHSLightGroundColor;                   //!< 入力:[ 1      /  1 : 128 ] 半球ライトのグラウンドカラー
uniform mediump vec3 uEyePt;                                //!< 入力:[ 1      /  1 : 129 ] カメラの位置
uniform mediump float uAlpha;                               //!< 入力:[ 1      /  1 : 130 ] アルファ値

// ----------------------------------------
// フラグメントシェーダーに渡される varying 変数
out lowp    vec4    vModelColor;                            //!< 出力:[ 1 : 1 ] モデルの色
#if !defined(AGX_FEATURE_BUMP_TEXTURE)
out mediump vec3    vNormal;                                //!< 出力:[ 1 : 2 ] モデルの法線
#endif
#if defined(AGX_FEATURE_ALBEDO_TEXTURE) || defined(AGX_FEATURE_BUMP_TEXTURE) || defined(AGX_FEATURE_MASK_TEXTURE) || defined(AGX_FEATURE_ALPHA_TEXTURE)
out mediump vec2    vTexcoord0;                             //!< 出力:[ 1 : 3 ] テクスチャーUV
#endif
// camera
out mediump vec3    vEyeVecWorldOrTangent;                  //!< 出力:[ 1 : 4 ] 視線ベクトル
#if !defined(AGX_FEATURE_DISABLE_LIGHT)
// punctual light
out mediump vec3    vPunctualLightDirWorldOrTangent;        //!< 出力:[ 1 : 5 ] ライトの方向
out mediump vec3    vPunctualLightHalfVecWorldOrTangent;    //!< 出力:[ 1 : 6 ] カメラとライトのハーフベクトル
// GI
out mediump vec3    vGISpecularLightColor;                  //!< 出力:[ 1 : 7 ] GIフレネルで使用するカラー
// Lighting Result
out mediump vec3    vDiffuseColor;                          //!< 出力:[ 1 : 8 ] ディフューズライティング結果
#endif
// Reflect
#if defined(AGX_FEATURE_SPHERE_MAP_TEXTURE)
out lowp    vec3    vReflectDir;                            //!< 出力:[ 1 : 9 ] 環境マップの反射ベクトル
#endif

// ------------------------------------------------------------
// 頂点シェーダーのエントリーポイント
// ------------------------------------------------------------
void main()
{
    // ------------------------------------------------------------
    // 頂点変換用の処理
    // ------------------------------------------------------------
    highp   vec4 position;  //!< 最終的な頂点
    mediump vec3 normal;    //!< 最終的な法線
    mediump vec3 tangent;   //!< 最終的な接線
    highp   vec4 positionWorld; //!< ワールド空間上での頂点
    
    // ----------------------------------------
    if (uBoneCount >= 1)
    {
        lowp    int boneIndex  = int(aTexcoord0.x) * 3;
        //lowp    ivec4 boneIndex  = ivec4(aBoneIndex);   //!< ボーンのインデックス
        //mediump vec4  boneWeight = aBoneWeight;         //!< ボーンの影響度
        mediump vec4  boneWeight = vec4(1.0, 1.0, 1.0, 1.0);

        // ボーンの行列を取得する
        highp mat4 boneMatrix = mat4(
            vec4(uBoneMatrices[boneIndex + 0].x, uBoneMatrices[boneIndex + 1].x, uBoneMatrices[boneIndex + 2].x, 0.0),
            vec4(uBoneMatrices[boneIndex + 0].y, uBoneMatrices[boneIndex + 1].y, uBoneMatrices[boneIndex + 2].y, 0.0),
            vec4(uBoneMatrices[boneIndex + 0].z, uBoneMatrices[boneIndex + 1].z, uBoneMatrices[boneIndex + 2].z, 0.0),
            vec4(uBoneMatrices[boneIndex + 0].w, uBoneMatrices[boneIndex + 1].w, uBoneMatrices[boneIndex + 2].w, 1.0)
        );
        //highp   mat4 boneMatrix = uBoneMatrices[boneIndex.x];
        //mediump mat3 boneNormMatrix = uBoneNormalMatrices[boneIndex.x];

        // 位置と法線をあらかじめ計算しておく
        position = boneMatrix * vec4(aPosition, 1.0) * boneWeight.x;
        normal   = mat3(boneMatrix) * aNormal * boneWeight.x;
        //normal   = boneNormMatrix * (aNormal * boneWeight.x);
#if defined(AGX_FEATURE_BUMP_TEXTURE)
        tangent  = mat3(boneMatrix) * aTangent * boneWeight.x;
        //tangent  = boneNormMatrix * (aTangent * boneWeight.x);
#endif
        
/*
        // 他の影響するボーンの行列を取得し、計算していく
        int iBone = 1;
        for (; iBone < uBoneCount; ++ iBone)
        {
            // ボーンの参照を変更する
            boneIndex  = boneIndex.yzwx;
            boneWeight = boneWeight.yzwx;
            
            // 新しいボーンの行列を取得
            boneMatrix = uBoneMatrices[boneIndex.x];
            boneNormMatrix = uBoneNormalMatrices[boneIndex.x];
            
            // 位置と法線を加算していく
            position += boneMatrix * vec4(aPosition, 1.0) * boneWeight.x;
            normal   += boneNormMatrix * (aNormal * boneWeight.x);
#if defined(AGX_FEATURE_BUMP_TEXTURE)
            tangent  += boneNormMatrix * (aTangent * boneWeight.x);
#endif
        }
*/
    }
    else
    {
        // ボーンが存在しない場合は位置と法線に手を加えない
        position = vec4(aPosition, 1.0);
        //position = vec4(aPosition.x*0.1, aPosition.y*0.1, aPosition.z*0.1, 1.0);
        normal   = aNormal;
#if defined(AGX_FEATURE_BUMP_TEXTURE)
        tangent  = aTangent;
#endif
    }
    
    // ----------------------------------------
    // ワールド上での位置を求める
    positionWorld = uModelMatrix * position;
    // 最終結果を行う
    position = uMVPMatrix * position;
    normal   = normalize(uNormalMatrix * normal);
#if defined(AGX_FEATURE_BUMP_TEXTURE)
    tangent  = normalize(uNormalMatrix * tangent);
#endif
    
    // ----------------------------------------
    // 計算結果を保持させる
    gl_Position = position;
#if !defined(AGX_FEATURE_BUMP_TEXTURE)
    vNormal     = normal;
#endif
#if defined(AGX_FEATURE_ALBEDO_TEXTURE) || defined(AGX_FEATURE_BUMP_TEXTURE) || defined(AGX_FEATURE_MASK_TEXTURE) || defined(AGX_FEATURE_ALPHA_TEXTURE)
    // テクスチャー座標を設定する
    vTexcoord0 = aTexcoord0;
#endif
    // モデルの色を指定する
#if defined(AGX_FEATURE_VERTEX_COLOR)
    lowp vec4 modelColor = aColor;
    
#else
    lowp vec4 modelColor = vec4(1.0, 1.0, 1.0, 1.0);
#endif
    
    // プリマルチプライドアルファ
#if defined(AGX_FEATURE_PREMULTIPLY_ALPHA)
    modelColor *= uAlpha;
#else
    modelColor.a *= uAlpha;
#endif
    
    
    // ------------------------------------------------------------
    // ライト用の処理
    // ------------------------------------------------------------
    mediump vec3 eyeVecWorld;   //!< ワールド状態での視線ベクトル
    mediump vec3 eyeVec;        //!< 最終的にフラグメントシェーダーに渡す視線ベクトル（バンプの有無によって、ワールド座標系になったり、タンジェント座標系になったりする）
    
    // 視線ベクトルを取得する
    eyeVecWorld = normalize(uEyePt - positionWorld.xyz);
    eyeVec = eyeVecWorld;
    
    lowp vec3 diffuseColor = vec3(0.0); // バーテックスシェーダーで計算できるディフューズの色をここに格納する
    
#   if defined(AGX_FEATURE_BUMP_TEXTURE)
    // Normal, Binormal, Tangent を取得する
    mediump vec3 n = normal;
    mediump vec3 t = tangent;
    mediump vec3 b = cross(n, t);
    // 接空間からローカルへ変換する行列を設定する（mat3(N, T, B)の逆行列）
    mediump mat3 tangentMatrix = mat3(t.x, b.x, n.x, t.y, b.y, n.y, t.z, b.z, n.z);
    // 視線ベクトルを接空間へ
    vEyeVecWorldOrTangent.xyz = tangentMatrix * eyeVec;
#else
    vEyeVecWorldOrTangent.xyz = eyeVec;
#endif
    
#if !defined(AGX_FEATURE_DISABLE_LIGHT)
    // punctual lightの設定
    if (uDirLightCount > 0)
    {
        mediump vec3 lightDir;
        
        // 方向ライト
        if (uDirLightDirAndType0.w < 0.0) { lightDir = uDirLightDirAndType0.xyz; }
        // 点光源ライト
        else                                { lightDir = uDirLightDirAndType0.xyz - positionWorld.xyz; }
        lightDir = normalize(lightDir);
        
#   if defined(AGX_FEATURE_BUMP_TEXTURE)
        // ライトを接空間へ
        vPunctualLightDirWorldOrTangent.xyz = tangentMatrix * lightDir;
#   else
        vPunctualLightDirWorldOrTangent.xyz = lightDir;
#   endif
        
        // Halfベクトルを求める
        vPunctualLightHalfVecWorldOrTangent.xyz = normalize(vPunctualLightDirWorldOrTangent.xyz + vEyeVecWorldOrTangent.xyz);
        
        // Diffuse計算
        diffuseColor += (uDirLightColor0.rgb * clamp(dot(lightDir, normal), 0.0, 1.0));
    }
    if (uDirLightCount > 1)
    {
        mediump vec3 lightDir;
        
        // 方向ライト
        if (uDirLightDirAndType1.w < 0.0) { lightDir = uDirLightDirAndType1.xyz; }
        // 点光源ライト
        else                                { lightDir = uDirLightDirAndType1.xyz - positionWorld.xyz; }
        lightDir = normalize(lightDir);
        
        diffuseColor += max(dot(lightDir, normal), 0.0) * uDirLightColor1;
    }
    // ライトは1.0を超えないように
    diffuseColor = min(diffuseColor, 1.0);
#endif
        
#if defined(AGX_FEATURE_SPHERE_MAP_TEXTURE)
    {
        // キューブ環境マップ用の反射ベクトルを求める
//        vReflectDir = reflect(normalize(positionWorld.xyz - uEyePt), normal);

        // スフィア環境マップ用の反射ベクトルを求める
//        vReflectDir = normalize((uViewMatrix * vec4(normal, 0.0)).xyz) * 0.5 + 0.5;
        
        // ビュー座標系での位置と法線を取得
        mediump vec3 viewNormal   = normalize(mat3(uViewMatrix) * normal);
        mediump vec4 viewPosition = uViewMatrix * positionWorld;
        viewPosition = viewPosition / viewPosition.w;
        // ビュー座標系での頂点ベクトルを取得
        viewPosition.z = 1.0 - viewPosition.z;
        mediump vec3 viewPositionVec = normalize(viewPosition.xyz);
        // ビュー座標系での反射ベクトルを求める
        mediump vec3 viewReflect  = viewPositionVec - 2.0 * dot(viewPositionVec, viewNormal) * viewNormal;
        // 両面スフィア環境マップではないので、反射ベクトルを調整
        viewReflect = normalize(viewReflect - vec3(0.0, 0.0, 1.5));
        // 反射ベクトルをテクスチャー座標系へ
        vReflectDir = viewReflect * 0.5 + 0.5;
        
        // 公式
//        mediump vec3  viewPositionVec = normalize(vec3(uViewMatrix * positionWorld));
//        mediump vec3  viewReflectVec = viewPositionVec - 2.0 * dot(viewPositionVec, normal) * normal;
//        mediump float m = 2.0 * sqrt(viewReflectVec.x * viewReflectVec.x +
//                                     viewReflectVec.y * viewReflectVec.y +
//                                     (viewReflectVec.z + 1.0) * (viewReflectVec.z * 1.0));
//        vReflectDir = viewReflectVec / m + 0.5;
        
        // 別版
//        mediump vec3 posW = positionWorld.xyz;
//        mediump vec3 dir  = normalize(mat3(uViewMatrix) * normal);
//        
//        mediump float radius     = 75.0;
//        mediump vec3  posWDir    = dot(dir, posW) * dir;
//        mediump vec3  posWDirV   = posW - posWDir;
//        mediump float lengthDir  = sqrt(radius * radius - dot(posWDirV, posWDirV)) - length(posWDir);
//        vReflectDir = normalize(posW + dir * lengthDir) * 0.5 + 0.5;
    }
#endif
    
#if !defined(AGX_FEATURE_DISABLE_LIGHT)
    // GIの計算
    {
        mediump vec3 hemiColor;
        mediump vec3 sky = uHSLightSkyColor;
        mediump vec3 ground = uHSLightGroundColor;
        
        {
            mediump float skyRatio = (normal.y + 1.0) * 0.5;
            hemiColor =  (sky * skyRatio + ground * (1.0 - skyRatio));
            diffuseColor += hemiColor;
        }
        
        {
//            mediump vec3 reflectDir = -reflect(normal, eyeVecWorld); // おそらくコレで良いはず
            mediump vec3 reflectDir = 2.0 * dot(eyeVecWorld, normal) * normal - eyeVecWorld; // 多少冗長でも、正しい計算で行なう
            
            mediump float skyRatio = (reflectDir.y + 1.0) * 0.5;
            hemiColor =  (sky * skyRatio + ground * (1.0 - skyRatio));
            vGISpecularLightColor.rgb = hemiColor;
        }
    }
#endif
    
    // モデルの色を設定
    vModelColor = modelColor;
#if !defined(AGX_FEATURE_DISABLE_LIGHT)
    vDiffuseColor.rgb = diffuseColor;
#endif
}
