#pragma once
#include <PantsColor.h>

// Enums included in RenderRequest.

enum MiiDataInputType
{
    INPUT_TYPE_FFL_MIIDATACORE,
    INPUT_TYPE_FFL_STOREDATA,
    INPUT_TYPE_RFL_CHARDATA,
    INPUT_TYPE_RFL_STOREDATA,
    INPUT_TYPE_NX_CHARINFO,
    INPUT_TYPE_STUDIO_ENCODED,
    // mii studio site decoded URL format/LocalStorage format
    INPUT_TYPE_STUDIO_RAW,
    INPUT_TYPE_NX_COREDATA,
    INPUT_TYPE_NX_STOREDATA,
    //INPUT_TYPE_RFL_CHARDATA_LE
};

enum ShaderType
{
    SHADER_TYPE_WIIU,
    SHADER_TYPE_SWITCH,
    SHADER_TYPE_MIITOMO,
    /* FUTURE OPTIONS:
     * 3ds
       - potentially downscaling
     * wii...????????
       - it would have to be some TEV to GLSL sheeee
     */
    SHADER_TYPE_WIIU_BLINN, // same as wiiu but with mSpecularMode
    SHADER_TYPE_WIIU_FFLICONWITHBODY, // wiiu but "nwf lighting"
    SHADER_TYPE_MAX,
};

enum BodyType
{
    BODY_TYPE_DEFAULT_FOR_SHADER = -1, // Will be replaced
    BODY_TYPE_WIIU_MIIBODYMIDDLE = 0,
    BODY_TYPE_SWITCH_MIIBODYHIGH = 1,
    BODY_TYPE_MIITOMO            = 2,  // Female body is custom
    BODY_TYPE_FFLBODYRES         = 3,  // AKA body in nwf.mii
    BODY_TYPE_3DS                = 4,
    BODY_TYPE_MAX                = 5
};

// Map shader types to a default body type:
const BodyType cShaderTypeDefaultBodyType[SHADER_TYPE_MAX] = {
    BODY_TYPE_WIIU_MIIBODYMIDDLE, // SHADER_TYPE_WIIU
    BODY_TYPE_SWITCH_MIIBODYHIGH, // SHADER_TYPE_SWITCH
    BODY_TYPE_MIITOMO,            // SHADER_TYPE_MIITOMO
    BODY_TYPE_WIIU_MIIBODYMIDDLE, // SHADER_TYPE_WIIU_BLINN
    BODY_TYPE_FFLBODYRES,         // SHADER_TYPE_WIIU_FFLICONWITHBODY
};

// Same as above but for pants type.
const PantsColor cShaderTypeDefaultPantsType[SHADER_TYPE_MAX] = {
    PANTS_COLOR_RED,          // SHADER_TYPE_WIIU
    PANTS_COLOR_GRAY,         // SHADER_TYPE_SWITCH
    PANTS_COLOR_GRAY,         // SHADER_TYPE_MIITOMO
    PANTS_COLOR_RED,          // SHADER_TYPE_WIIU_BLINN
    PANTS_COLOR_SAME_AS_BODY, // SHADER_TYPE_WIIU_FFLICONWITHBODY
};

enum InstanceRotationMode
{
    INSTANCE_ROTATION_MODE_MODEL,
    INSTANCE_ROTATION_MODE_CAMERA,
    INSTANCE_ROTATION_MODE_EXPRESSION, // neewwww
    INSTANCE_ROATATION_MODE_MAX,
};

enum SplitMode
{
    SPLIT_MODE_NONE,
    SPLIT_MODE_FRONT,
    SPLIT_MODE_BACK,
    SPLIT_MODE_BOTH,
    SPLIT_MODE_MAX
};

enum ViewType
{
    VIEW_TYPE_FACE,  // head with body
    VIEW_TYPE_FACE_ONLY,  // head only
    VIEW_TYPE_ALL_BODY,
    VIEW_TYPE_FACE_ONLY_FFLMAKEICON,
    VIEW_TYPE_FFLICONWITHBODY,
    // nn::mii::VariableIconBody::StoreCameraMatrix
    VIEW_TYPE_NNMII_VARIABLEICONBODY,
    VIEW_TYPE_ALL_BODY_SUGAR,
};

enum DrawStageMode
{
    DRAW_STAGE_MODE_ALL,
    DRAW_STAGE_MODE_OPA_ONLY,
    DRAW_STAGE_MODE_XLU_ONLY,
    DRAW_STAGE_MODE_MASK_ONLY,
    DRAW_STAGE_MODE_XLU_DEPTH_MASK,
};

enum ResponseFormat
{
    RESPONSE_FORMAT_TGA_DEFAULT,     // with tga header
    RESPONSE_FORMAT_GLTF_MODEL,      // glTF model export
    RESPONSE_FORMAT_TGA_BGRA_FLIP_Y, // RIO_IS_WIN only
    //RESPONSE_FORMAT_JPEG // planned, libjpeg-turbo? ffmpeg?
};
