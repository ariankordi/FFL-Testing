#pragma once

#include <Types.h> // for PACKED()

// Structure representing a render request, derived from
// request query parameters by caller (web server).
PACKED(struct RenderRequest
{
    uint8_t  data[96];       // just a buffer that accounts for maximum size
    uint16_t dataLength;     // determines the mii data format
    uint8_t  modelFlag;      // FFLModelType + nose flatten @ bit 4
    // completely changes the response type:
    uint8_t  responseFormat; // indicates if response is gltf or tga

    // note that arbitrary resolutions CRASH THE BACKEND
    uint16_t resolution;     // resolution for render buffer
    // texture resolution controls mipmap enable (1 << 30)
    int16_t  texResolution;  // FFLResolution/u32, negative = mipmap
    uint8_t  viewType;       // camera view (setViewTypeParams)
    int8_t  resourceType;    // FFLResourceType (default high/-1)
    uint8_t  shaderType;     // custom ShaderType
    uint8_t  expression;     // used if expressionFlag is all zeroes
    uint32_t expressionFlag[3];  // casted to FFLAllExpressionFlag
                                 // used for multiple expressions
    // expressionFlag will only apply in gltf mode for now
    int16_t  cameraRotate[3];    // converted deg2rad to vector
    int16_t  modelRotate[3];     // same as above
    uint8_t  backgroundColor[4]; // passed to clearcolor

    // controls scaling/anti-aliasing mode:
    uint8_t  aaMethod;       // TODO: to be implemented perfectly
    uint8_t  drawStageMode;  // custom DrawStageMode: opa, xlu, all
    bool     verifyCharInfo; // for FFLiVerifyCharInfoWithReason
    bool     verifyCRC16;    // passed to pickupCharInfoFromData
    bool     lightEnable;    // passed to IShader::bind()
    int8_t   clothesColor;   // favorite color, -1 for default
    int8_t   pantsColor;     // PantsColor, -1 = default shader
    int8_t   bodyType;       // BodyType, -1 = default for shader

    uint8_t  instanceCount;  // for instanceCountNewRender loop
    uint8_t  instanceRotationMode; // model, camera, TODO
    int16_t  lightDirection[3];    // unset if all negative, TODO
    uint8_t  splitMode;      // none (default), front, back, both
});
