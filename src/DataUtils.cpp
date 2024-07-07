#include <nn/ffl.h>

#include <misc/rio_MemUtil.h>

#include <mii_ext_MiiPort.h>

// for markCommonColor
#include <nn/ffl/FFLiColor.h>

// NOTE: TAKEN FROM MiiPort/include/convert_mii.h
// only conversion table we need from MiiPort
const u8 ToVer3GlassTypeTable[20] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 1, 3, 7, 7, 6, 7, 8, 7, 7};

void convertCharInfoNXToFFLiCharInfo(FFLiCharInfo* dest, const charInfo* src) {
    // Initialize the destination structure
    rio::MemUtil::set(dest, 0, sizeof(FFLiCharInfo));

    // Copy and convert values
    dest->parts.faceType = src->faceline_type;
    dest->parts.facelineColor = markCommonColor(src->faceline_color);
    dest->parts.faceLine = src->faceline_wrinkle;
    dest->parts.faceMakeup = src->faceline_make;
    dest->parts.hairType = src->hair_type;
    dest->parts.hairColor = markCommonColor(src->hair_color);
    dest->parts.hairDir = src->hair_flip;
    dest->parts.eyeType = src->eye_type;
    dest->parts.eyeColor = markCommonColor(src->eye_color);
    dest->parts.eyeScale = src->eye_scale;
    dest->parts.eyeScaleY = src->eye_aspect;
    dest->parts.eyeRotate = src->eye_rotate;
    dest->parts.eyeSpacingX = src->eye_x;
    dest->parts.eyePositionY = src->eye_y;
    dest->parts.eyebrowType = src->eyebrow_type;
    dest->parts.eyebrowColor = markCommonColor(src->eyebrow_color);
    dest->parts.eyebrowScale = src->eyebrow_scale;
    dest->parts.eyebrowScaleY = src->eyebrow_aspect;
    dest->parts.eyebrowRotate = src->eyebrow_rotate;
    dest->parts.eyebrowSpacingX = src->eyebrow_x;
    dest->parts.eyebrowPositionY = src->eyebrow_y;
    dest->parts.noseType = src->nose_type;
    dest->parts.noseScale = src->nose_scale;
    dest->parts.nosePositionY = src->nose_y;
    dest->parts.mouthType = src->mouth_type;
    dest->parts.mouthColor = markCommonColor(src->mouth_color);
    dest->parts.mouthScale = src->mouth_scale;
    dest->parts.mouthScaleY = src->mouth_aspect;
    dest->parts.mouthPositionY = src->mouth_y;
    dest->parts.mustacheType = src->mustache_type;
    dest->parts.beardType = src->beard_type;
    dest->parts.beardColor = markCommonColor(src->beard_color);
    dest->parts.mustacheScale = src->mustache_scale;
    dest->parts.mustachePositionY = src->mustache_y;

    /*dest->parts.glassType = src->glass_type;
    // Glass type special case
    // NOTE: LOOK INTO THIS AGAIN TO SEE IF YOU CAN DO OPAQUE GLASSES
    if (dest->parts.glassType > 8) {
        dest->parts.glassType = dest->parts.glassType - 5;
    }*/
    dest->parts.glassType = ToVer3GlassTypeTable[src->glass_type];

    dest->parts.glassColor = markCommonColor(src->glass_color);
    dest->parts.glassScale = src->glass_scale;
    dest->parts.glassPositionY = src->glass_y;

    dest->parts.moleType = src->mole_type;
    dest->parts.moleScale = src->mole_scale;
    dest->parts.molePositionX = src->mole_x;
    dest->parts.molePositionY = src->mole_y;

    dest->height = src->height;
    dest->build = src->build;
    rio::MemUtil::copy(dest->name, src->nickname, sizeof(src->nickname));
    dest->gender = (FFLGender)src->gender;
    dest->favoriteColor = src->favorite_color;
    dest->favoriteMii = src->type;
    dest->regionMove = src->region_move;
    //dest->fontRegion = src->font_region;
}


void convertStudioToCharInfoNX(charInfo *dest, const charInfoStudio *src) {
    // Initialize charInfo struct with zeros
    rio::MemUtil::set(dest, 0, sizeof(charInfo));

    // Mapping the fields from charInfoStudio to charInfo
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
