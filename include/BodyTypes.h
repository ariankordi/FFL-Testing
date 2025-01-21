#pragma once
#include <Types.h>
#include <nn/ffl/FFLGender.h>

inline const char* cBodyGenderStrings[FFL_GENDER_MAX] = {
    "male",  // FFL_GENDER_MALE
    "female" // FFL_GENDER_FEMALE
};

inline const char* cStaticBodyFileNameFormat = "body/mii_static_body_%s_%s";
      // body type_gender
inline const char* cBodyFileNameFormat = "body/body_%s_%s";
inline const char* cBodySkeletonFileNameFormat = "models/body/body_%s_skeleton.csv";


// Scale, head rotation and translation for all body types:

static const f32 cMiiBodyMiddleHeadYTranslation
                                       = (6.6766f // skl_root
                                        + 4.1f); // head

static const rio::Vector3f cMiiBodyMiddleBodyHeadRotation
    = { (0.002f + -0.005f), 0.000005f, -0.001f };
    // ^^ MiiBodyMiddle "head" rotation

static const f32 cMiiBodyMiddleScale = 7.0f;

// Tables for properties of all body types supported.
static const rio::Vector3f cBodyTypeHeadRotation[BODY_TYPE_MAX] = {
    cMiiBodyMiddleBodyHeadRotation, // BODY_TYPE_WIIU_MIIBODYMIDDLE
    { 0.0f, 0.0f, 0.0f },   // BODY_TYPE_SWITCH_MIIBODYHIGH
    { 0.012f, 0.0f, 0.0f }, // BODY_TYPE_MIITOMO
    { 0.0f, 0.0f, 0.0f },   // BODY_TYPE_FFLBODYRES
    cMiiBodyMiddleBodyHeadRotation // BODY_TYPE_3DS
};

static const f32 cBodyTypeScaleFactors[BODY_TYPE_MAX] = {
    cMiiBodyMiddleScale, // BODY_TYPE_WIIU_MIIBODYMIDDLE
    cMiiBodyMiddleScale, // BODY_TYPE_SWITCH_MIIBODYHIGH
    cMiiBodyMiddleScale, // BODY_TYPE_MIITOMO
    8.715f,              // BODY_TYPE_FFLBODYRES
    cMiiBodyMiddleScale  // BODY_TYPE_3DS
};

static const f32 cBodyTypeHeadTranslation[BODY_TYPE_MAX] = {
    cMiiBodyMiddleHeadYTranslation, // BODY_TYPE_WIIU_MIIBODYMIDDLE
    6.7143f + 4.1f,                 // BODY_TYPE_SWITCH_MIIBODYHIGH
    6.5f + 4.1f,                    // BODY_TYPE_MIITOMO
    cMiiBodyMiddleHeadYTranslation, // BODY_TYPE_FFLBODYRES
    cMiiBodyMiddleHeadYTranslation  // BODY_TYPE_3DS
};
