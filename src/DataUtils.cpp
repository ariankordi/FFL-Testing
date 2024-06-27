#include <nn/ffl.h>

#include <misc/rio_MemUtil.h>

#include <mii_ext_MiiPort.h>

// for markCommonColor
#include <nn/ffl/FFLiColor.h>

// NOTE: TAKEN FROM MiiPort/include/convert_mii.h
// only conversion tables we need from MiiPort
const u8 ToVer3GlassTypeTable[20] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 1, 3, 7, 7, 6, 7, 8, 7, 7};

const u8 Ver3MouthColorTable[5] = {19, 20, 21, 22, 23};
// shortened to 24 values
const u8 ToVer3MouthColorTable[24] = {4, 4, 4, 4, 4, 4, 4, 3, 4, 4, 4, 4, 4, 4, 4, 1, 4, 4, 4, 0, 1, 2, 3, 4};


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
    // NOTE ON MOUTH COLOR: we DO NOT KNOW UPPER LIP COLOR...
    // SO if the mouth color is a ver3 mouth color then map it
    // is the mouth color in the mouth color table??
    bool mouthColorInVer3Table = false;
    for (int i = 0; i < sizeof(Ver3MouthColorTable); ++i) {
        if(Ver3MouthColorTable[i] == src->mouth_color) {
            mouthColorInVer3Table = true;
        }
    }
    if (mouthColorInVer3Table)
        dest->parts.mouthColor = ToVer3MouthColorTable[src->mouth_color];
    else
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





// NOTE: NOT USED AS OF 2024-04-24, INTEGRATED INTO FFLiColor.cpp

/*
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} Color;

const Color commonColorTable[] = {
    { 45, 40, 40, 1 },  // id: 0
    { 64, 32, 16, 1 },  // id: 1
    { 92, 24, 10, 1 },  // id: 2
    { 124, 58, 20, 1 },  // id: 3
    { 120, 120, 128, 1 },  // id: 4
    { 78, 62, 16, 1 },  // id: 5
    { 136, 88, 24, 1 },  // id: 6
    { 208, 160, 74, 1 },  // id: 7
    { 0, 0, 0, 1 },  // id: 8
    { 108, 112, 112, 1 },  // id: 9
    { 102, 60, 44, 1 },  // id: 10
    { 96, 94, 48, 1 },  // id: 11
    { 70, 84, 168, 1 },  // id: 12
    { 56, 112, 88, 1 },  // id: 13
    { 96, 56, 16, 1 },  // id: 14
    { 168, 16, 8, 1 },  // id: 15
    { 32, 48, 104, 1 },  // id: 16
    { 168, 96, 0, 1 },  // id: 17
    { 120, 112, 104, 1 },  // id: 18
    { 216, 82, 8, 1 },  // id: 19
    { 240, 12, 8, 1 },  // id: 20
    { 245, 72, 72, 1 },  // id: 21
    { 240, 154, 116, 1 },  // id: 22
    { 140, 80, 64, 1 },  // id: 23
    { 132, 38, 38, 1 },  // id: 24
    { 255, 115, 102, 1 },  // id: 25
    { 255, 166, 166, 1 },  // id: 26
    { 255, 192, 186, 1 },  // id: 27
    { 115, 46, 59, 1 },  // id: 28
    { 153, 31, 61, 1 },  // id: 29
    { 138, 23, 62, 1 },  // id: 30
    { 181, 62, 66, 1 },  // id: 31
    { 199, 30, 86, 1 },  // id: 32
    { 176, 83, 129, 1 },  // id: 33
    { 199, 84, 110, 1 },  // id: 34
    { 250, 117, 151, 1 },  // id: 35
    { 252, 172, 201, 1 },  // id: 36
    { 255, 201, 216, 1 },  // id: 37
    { 49, 28, 64, 1 },  // id: 38
    { 55, 40, 61, 1 },  // id: 39
    { 76, 24, 77, 1 },  // id: 40
    { 111, 66, 179, 1 },  // id: 41
    { 133, 92, 184, 1 },  // id: 42
    { 192, 131, 204, 1 },  // id: 43
    { 168, 147, 201, 1 },  // id: 44
    { 197, 172, 230, 1 },  // id: 45
    { 238, 190, 250, 1 },  // id: 46
    { 210, 197, 237, 1 },  // id: 47
    { 25, 31, 64, 1 },  // id: 48
    { 18, 63, 102, 1 },  // id: 49
    { 42, 130, 212, 1 },  // id: 50
    { 87, 180, 242, 1 },  // id: 51
    { 122, 197, 222, 1 },  // id: 52
    { 137, 166, 250, 1 },  // id: 53
    { 132, 189, 250, 1 },  // id: 54
    { 161, 227, 255, 1 },  // id: 55
    { 11, 46, 54, 1 },  // id: 56
    { 1, 61, 59, 1 },  // id: 57
    { 13, 79, 89, 1 },  // id: 58
    { 35, 102, 99, 1 },  // id: 59
    { 48, 126, 140, 1 },  // id: 60
    { 79, 174, 176, 1 },  // id: 61
    { 122, 196, 158, 1 },  // id: 62
    { 127, 212, 192, 1 },  // id: 63
    { 135, 229, 182, 1 },  // id: 64
    { 10, 74, 53, 1 },  // id: 65
    { 67, 122, 0, 1 },  // id: 66
    { 2, 117, 98, 1 },  // id: 67
    { 54, 153, 112, 1 },  // id: 68
    { 75, 173, 26, 1 },  // id: 69
    { 146, 191, 10, 1 },  // id: 70
    { 99, 199, 136, 1 },  // id: 71
    { 158, 224, 66, 1 },  // id: 72
    { 150, 222, 126, 1 },  // id: 73
    { 187, 242, 170, 1 },  // id: 74
    { 153, 147, 43, 1 },  // id: 75
    { 166, 149, 99, 1 },  // id: 76
    { 204, 192, 57, 1 },  // id: 77
    { 204, 185, 135, 1 },  // id: 78
    { 217, 204, 130, 1 },  // id: 79
    { 213, 217, 111, 1 },  // id: 80
    { 213, 230, 131, 1 },  // id: 81
    { 216, 250, 157, 1 },  // id: 82
    { 125, 69, 0, 1 },  // id: 83
    { 230, 187, 122, 1 },  // id: 84
    { 254, 226, 74, 1 },  // id: 85
    { 250, 222, 130, 1 },  // id: 86
    { 247, 234, 156, 1 },  // id: 87
    { 250, 248, 155, 1 },  // id: 88
    { 166, 77, 30, 1 },  // id: 89
    { 255, 150, 13, 1 },  // id: 90
    { 209, 155, 105, 1 },  // id: 91
    { 255, 178, 102, 1 },  // id: 92
    { 255, 194, 140, 1 },  // id: 93
    { 229, 207, 177, 1 },  // id: 94
    { 65, 65, 65, 1 },  // id: 95
    { 155, 155, 155, 1 },  // id: 96
    { 190, 190, 190, 1 },  // id: 97
    { 220, 215, 205, 1 },  // id: 98
    { 255, 255, 255, 1 }  // id: 99
};

const int commonColorTableSize = sizeof(commonColorTable) / sizeof(commonColorTable[0]);

const Color facelineColorTable[] = {
    { 255, 211, 173, 1 },  // id: 0
    { 255, 182, 107, 1 },  // id: 1
    { 222, 121, 66, 1 },  // id: 2
    { 255, 170, 140, 1 },  // id: 3
    { 173, 81, 41, 1 },  // id: 4
    { 99, 44, 24, 1 },  // id: 5
    { 255, 190, 165, 1 },  // id: 6
    { 255, 197, 143, 1 },  // id: 7
    { 140, 60, 35, 1 },  // id: 8
    { 60, 45, 35, 1 }  // id: 9
};

const int facelineColorTableSize = sizeof(facelineColorTable) / sizeof(facelineColorTable[0]);

/*
s32 convertCommonColorToS32(u8 color) {
    return (color << 8) | 0x01;
}

s32 convertFacelineColorToS32(u8 color) {
    return (color << 8) | 0x01;
}
*/
