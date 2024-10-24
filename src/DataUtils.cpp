#include <nn/ffl.h>
#include <nn/ffl/FFLBirthPlatform.h>
#include <nn/ffl/detail/FFLiCharInfo.h>

#include <misc/rio_MemUtil.h>

#include <mii_ext_MiiPort.h>

// for markCommonColor
#include <nn/ffl/FFLiColor.h>

void charInfoNXToFFLiCharInfo(FFLiCharInfo* dest, const charInfo* src) {
    // Initialize charInfo struct with zeros
    rio::MemUtil::set(dest, 0, sizeof(FFLiCharInfo));

    dest->parts.faceType = src->faceline_type;
    // NOTE: ver3 common colors ARE compatible with switch table
    // ASSUMES THAT YOUR FFL IS USING SWITCH FACELINE COLOR TABLE!!!!
    dest->parts.facelineColor = src->faceline_color;
    dest->parts.faceLine = src->faceline_wrinkle;
    dest->parts.faceMakeup = src->faceline_make;
    dest->parts.hairType = src->hair_type;
    dest->parts.hairColor = src->hair_color | FFLI_NN_MII_COMMON_COLOR_ENABLE_MASK;
    dest->parts.hairDir = src->hair_flip;
    dest->parts.eyeType = src->eye_type;
    dest->parts.eyeColor = src->eye_color | FFLI_NN_MII_COMMON_COLOR_ENABLE_MASK;
    dest->parts.eyeScale = src->eye_scale;
    dest->parts.eyeScaleY = src->eye_aspect;
    dest->parts.eyeRotate = src->eye_rotate;
    dest->parts.eyeSpacingX = src->eye_x;
    dest->parts.eyePositionY = src->eye_y;
    dest->parts.eyebrowType = src->eyebrow_type;
    dest->parts.eyebrowColor = src->eyebrow_color | FFLI_NN_MII_COMMON_COLOR_ENABLE_MASK;
    dest->parts.eyebrowScale = src->eyebrow_scale;
    dest->parts.eyebrowScaleY = src->eyebrow_aspect;
    dest->parts.eyebrowRotate = src->eyebrow_rotate;
    dest->parts.eyebrowSpacingX = src->eyebrow_x;
    dest->parts.eyebrowPositionY = src->eyebrow_y;
    dest->parts.noseType = src->nose_type;
    dest->parts.noseScale = src->nose_scale;
    dest->parts.nosePositionY = src->nose_y;
    dest->parts.mouthType = src->mouth_type;
    dest->parts.mouthColor = src->mouth_color | FFLI_NN_MII_COMMON_COLOR_ENABLE_MASK;
    dest->parts.mouthScale = src->mouth_scale;
    dest->parts.mouthScaleY = src->mouth_aspect;
    dest->parts.mouthPositionY = src->mouth_y;
    dest->parts.mustacheType = src->mustache_type;
    dest->parts.beardType = src->beard_type;
    dest->parts.beardColor = src->beard_color | FFLI_NN_MII_COMMON_COLOR_ENABLE_MASK;
    dest->parts.mustacheScale = src->mustache_scale;
    dest->parts.mustachePositionY = src->mustache_y;

    dest->parts.glassType = src->glass_type;
    // Glass type special case
    //dest->parts.glassType = ToVer3GlassTypeTable[src->glass_type];
    // NOTE: glass type mapping has been moved to FFLiResourceLoader.cpp

    dest->parts.glassColor = src->glass_color | FFLI_NN_MII_COMMON_COLOR_ENABLE_MASK;
    dest->parts.glassScale = src->glass_scale;
    dest->parts.glassPositionY = src->glass_y;

    dest->parts.moleType = src->mole_type;
    dest->parts.moleScale = src->mole_scale;
    dest->parts.molePositionX = src->mole_x;
    dest->parts.molePositionY = src->mole_y;

    dest->height = src->height;
    dest->build = src->build;
    rio::MemUtil::copy(dest->name, src->nickname, sizeof(src->nickname));
    dest->gender = static_cast<FFLGender>(u32(src->gender));
    dest->favoriteColor = src->favorite_color;
    // no equivalent for favoriteMii
    dest->regionMove = src->region_move;
    // MiiPort: "The switch sets this to 4, but the 3DS rejects it if set to >3"
    // ^ actually above they may be talking about the mii version I don't know
    dest->birthPlatform = FFL_BIRTH_PLATFORM_CTR;
    //dest->fontRegion = src->font_region;
}


void studioToCharInfoNX(charInfo* dest, const charInfoStudio* src) {
    // Initialize charInfo struct with zeros
    rio::MemUtil::set(dest, 0, sizeof(charInfo));

    dest->beard_color = src->beard_color;
    dest->mustache_type = src->mustache_type;
    dest->build = src->build;
    dest->eye_aspect = src->eye_aspect;
    dest->eye_color = src->eye_color;
    dest->eye_rotate = src->eye_rotate;
    dest->eye_scale = src->eye_scale;
    dest->eye_type = src->eye_type;
    dest->eye_x = src->eye_x;
    dest->eye_y = src->eye_y;
    dest->eyebrow_aspect = src->eyebrow_aspect;
    dest->eyebrow_color = src->eyebrow_color;
    dest->eyebrow_rotate = src->eyebrow_rotate;
    dest->eyebrow_scale = src->eyebrow_scale;
    dest->eyebrow_type = src->eyebrow_type;
    dest->eyebrow_x = src->eyebrow_x;
    dest->eyebrow_y = src->eyebrow_y;
    dest->faceline_color = src->faceline_color;
    dest->faceline_make = src->faceline_make;
    dest->faceline_type = src->faceline_type;
    dest->faceline_wrinkle = src->faceline_wrinkle;
    dest->favorite_color = src->favorite_color;
    dest->gender = src->gender;
    dest->glass_color = src->glass_color;
    dest->glass_scale = src->glass_scale;
    dest->glass_type = src->glass_type;
    dest->glass_y = src->glass_y;
    dest->hair_color = src->hair_color;
    dest->hair_flip = src->hair_flip;
    dest->hair_type = src->hair_type;
    dest->height = src->height;
    dest->mole_scale = src->mole_scale;
    dest->mole_type = src->mole_type;
    dest->mole_x = src->mole_x;
    dest->mole_y = src->mole_y;
    dest->mouth_aspect = src->mouth_aspect;
    dest->mouth_color = src->mouth_color;
    dest->mouth_scale = src->mouth_scale;
    dest->mouth_type = src->mouth_type;
    dest->mouth_y = src->mouth_y;
    dest->mustache_scale = src->mustache_scale;
    dest->beard_type = src->beard_type;
    dest->mustache_y = src->mustache_y;
    dest->nose_scale = src->nose_scale;
    dest->nose_type = src->nose_type;
    dest->nose_y = src->nose_y;

    // Other fields of charInfo will remain zero-initialized.
}

void studioURLObfuscationDecode(char* data) {
    // The first byte is the random seed used in encoding
    unsigned char random = data[0];
    unsigned char previous = random;

    // Reverse the encoding process
    // NOTE: 47 = length of obfuscated studio data
    for (int i = 1; i < STUDIO_DATA_ENCODED_LENGTH; i++) {
        // Reverse the modulation and XOR to find the original byte
        unsigned char encodedByte = data[i];
        unsigned char original = (encodedByte - 7 + 256) % 256; // reverse the addition of 7
        original ^= previous; // reverse the XOR with the previous encoded byte
        data[i - 1] = original;
        previous = encodedByte; // update previous to the current encoded byte for next iteration
    }
}

void coreDataToCharInfoNX(charInfo* dest, const coreData* src) {
    // Initialize charInfo struct with zeros
    rio::MemUtil::set(dest, 0, sizeof(charInfo));

    dest->font_region = src->font_region;
    dest->favorite_color = src->favorite_color;
    dest->gender = src->gender;
    dest->height = src->height;
    dest->build = src->build;
    dest->type = src->type;
    dest->region_move = src->region_move;
    dest->faceline_type = src->faceline_type;
    dest->faceline_color = src->faceline_color;
    dest->faceline_wrinkle = src->faceline_wrinkle;
    dest->faceline_make = src->faceline_make;
    dest->hair_type = src->hair_type;
    dest->hair_color = src->hair_color;
    dest->hair_flip = src->hair_flip;
    dest->eye_type = src->eye_type;
    dest->eye_color = src->eye_color;
    dest->eye_scale = src->eye_scale;
    dest->eye_aspect = src->eye_aspect;
    dest->eye_rotate = src->eye_rotate;
    dest->eye_x = src->eye_x;
    dest->eye_y = src->eye_y;
    dest->eyebrow_type = src->eyebrow_type;
    dest->eyebrow_color = src->eyebrow_color;
    dest->eyebrow_scale = src->eyebrow_scale;
    dest->eyebrow_aspect = src->eyebrow_aspect;
    dest->eyebrow_rotate = src->eyebrow_rotate;
    dest->eyebrow_x = src->eyebrow_x;
    dest->eyebrow_y = src->eyebrow_y + 3; // Adjusted value
    dest->nose_type = src->nose_type;
    dest->nose_scale = src->nose_scale;
    dest->nose_y = src->nose_y;
    dest->mouth_type = src->mouth_type;
    dest->mouth_color = src->mouth_color;
    dest->mouth_scale = src->mouth_scale;
    dest->mouth_aspect = src->mouth_aspect;
    dest->mouth_y = src->mouth_y;
    dest->beard_color = src->beard_color;
    dest->beard_type = src->beard_type;
    dest->mustache_type = src->mustache_type;
    dest->mustache_scale = src->mustache_scale;
    dest->mustache_y = src->mustache_y;
    dest->glass_type = src->glass_type;
    dest->glass_color = src->glass_color;
    dest->glass_scale = src->glass_scale;
    dest->glass_y = src->glass_y;
    dest->mole_type = src->mole_type;
    dest->mole_scale = src->mole_scale;
    dest->mole_x = src->mole_x;
    dest->mole_y = src->mole_y;

    // Copy nickname
    memcpy(dest->nickname, src->nickname, sizeof(src->nickname));

    // Other fields of charInfo will remain zero-initialized.
}
