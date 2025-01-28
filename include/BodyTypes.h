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
