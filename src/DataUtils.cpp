#include <nn/ffl.h>
#include <nn/ffl/FFLBirthPlatform.h>
#include <nn/ffl/detail/FFLiCharInfo.h>

#include <misc/rio_MemUtil.h>
#include <nn/ffl/FFLiColor.h>

#include <nn/ffl/FFLiMiiData.h>
#include <nn/ffl/FFLiMiiDataCore.h>
#include <nn/ffl/detail/FFLiCrc.h>
#include <Types.h>

#include <mii_ext_MiiPort.h>


void charInfoNXToFFLiCharInfo(FFLiCharInfo* dest, const charInfo* src)
{
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
    dest->gender = FFLGender(u32(src->gender));
    dest->favoriteColor = FFLFavoriteColor(src->favorite_color);
    // no equivalent for favoriteMii
    dest->regionMove = src->region_move;
    // MiiPort: "The switch sets this to 4, but the 3DS rejects it if set to >3"
    // ^ actually above they may be talking about the mii version I don't know
    dest->birthPlatform = FFL_BIRTH_PLATFORM_CTR;
    //dest->fontRegion = src->font_region;
}


void studioToCharInfoNX(charInfo* dest, const charInfoStudio* src)
{
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

void studioURLObfuscationDecode(char* data)
{
    // The first byte is the random seed used in encoding
    unsigned char random = data[0];
    unsigned char previous = random;

    // Reverse the encoding process
    // NOTE: 47 = length of obfuscated studio data
    for (int i = 1; i < STUDIO_DATA_ENCODED_LENGTH; i++)
    {
        // Reverse the modulation and XOR to find the original byte
        unsigned char encodedByte = data[i];
        unsigned char original = (encodedByte - 7 + 256) % 256; // reverse the addition of 7
        original ^= previous; // reverse the XOR with the previous encoded byte
        data[i - 1] = original;
        previous = encodedByte; // update previous to the current encoded byte for next iteration
    }
}

void coreDataToCharInfoNX(charInfo* dest, const coreData* src)
{
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


FFLResult pickupCharInfoFromData(FFLiCharInfo* pCharInfo, const void* data, u32 dataLength, bool verifyCRC16)
{
    MiiDataInputType inputType;

    switch (dataLength)
    {
        case 76: // RFLStoreData, FFLiStoreDataRFL ....????? (idk if this exists)
            inputType = INPUT_TYPE_RFL_STOREDATA;
            break;
        case 74: // RFLCharData, FFLiMiiDataOfficialRFL
            inputType = INPUT_TYPE_RFL_CHARDATA;
            break;
        case sizeof(charInfo): // nx char info
            inputType = INPUT_TYPE_NX_CHARINFO;
            break;
        case sizeof(coreData):
        case 68://sizeof(storeData):
            inputType = INPUT_TYPE_NX_COREDATA;
            break;
        case sizeof(charInfoStudio): // studio raw
            inputType = INPUT_TYPE_STUDIO_RAW;
            break;
        case STUDIO_DATA_ENCODED_LENGTH: // studio encoded i think
            inputType = INPUT_TYPE_STUDIO_ENCODED;
            break;
        case sizeof(FFLiMiiDataCore):
        case sizeof(FFLiMiiDataOfficial): // creator name unused
            inputType = INPUT_TYPE_FFL_MIIDATACORE;
            break;
        case sizeof(FFLStoreData):
            inputType = INPUT_TYPE_FFL_STOREDATA;
            break;
        default:
            // uh oh, we can't detect it
            RIO_LOG("pickupCharInfoFromData: Unknown type for data size %d, returning error.\n", dataLength);
            return FFL_RESULT_ERROR;
            break;
    }

    // create temporary charInfoNX for studio, coredata
    charInfo charInfoNX;

    switch (inputType)
    {
        case INPUT_TYPE_RFL_STOREDATA:
            // 76 = sizeof RFLStoreData
            if (verifyCRC16 && !FFLiIsValidCRC16(data, 76))
                return FFL_RESULT_FILE_INVALID;
            [[fallthrough]];
        case INPUT_TYPE_RFL_CHARDATA:
        {
            FFLiMiiDataCoreRFL charDataRFL;
            rio::MemUtil::copy(&charDataRFL, data, sizeof(FFLiMiiDataCoreRFL));
            // look at create id to run FFLiIsNTRMiiID
            // NOTE: FFLiMiiDataCoreRFL2CharInfo is SUPPOSED to check
            // whether it is an NTR mii id or not and store
            // the result as pCharInfo->birthPlatform = FFL_BIRTH_PLATFORM_NTR
            // HOWEVER, it clears out the create ID and runs the compare anyway
            // the create id is actually not the same size either

            // NOTE: NFLCharData for DS can be little endian tho!!!!!!
#if __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
            charDataRFL.SwapEndian();
#endif // __BYTE_ORDER__

            FFLiMiiDataCoreRFL2CharInfo(pCharInfo,
                charDataRFL,
                NULL, false
                //reinterpret_cast<u16*>(&buf->data[0x36]), true // creator name
                // name offset: https://github.com/SMGCommunity/Petari/blob/53fd4ff9db54cb1c91a96534dcae9f2c2ea426d1/libs/RVLFaceLib/include/RFLi_Types.h#L342
            );
            break;
        }
        case INPUT_TYPE_STUDIO_ENCODED:
        {
            // mii studio url data format is obfuscated
            // decodes it in a buffer
            char decodedData[STUDIO_DATA_ENCODED_LENGTH];
            rio::MemUtil::copy(decodedData, data, STUDIO_DATA_ENCODED_LENGTH);
            studioURLObfuscationDecode(decodedData);
            studioToCharInfoNX(&charInfoNX, reinterpret_cast<::charInfoStudio*>(decodedData));
            charInfoNXToFFLiCharInfo(pCharInfo, &charInfoNX);
            break;
        }
        case INPUT_TYPE_STUDIO_RAW:
            // we may not need this if we decode from and to data but that's confusing
            studioToCharInfoNX(&charInfoNX, reinterpret_cast<const ::charInfoStudio*>(data));
            charInfoNXToFFLiCharInfo(pCharInfo, &charInfoNX);
            break;
        case INPUT_TYPE_NX_CHARINFO:
            charInfoNXToFFLiCharInfo(pCharInfo, reinterpret_cast<const ::charInfo*>(data));
            break;
        case INPUT_TYPE_NX_COREDATA:
            coreDataToCharInfoNX(&charInfoNX, reinterpret_cast<const ::coreData*>(data));
            charInfoNXToFFLiCharInfo(pCharInfo, &charInfoNX);
            break;
        case INPUT_TYPE_FFL_STOREDATA:
            // only verify CRC16, then fall through
            if (verifyCRC16 && !FFLiIsValidCRC16(data, sizeof(FFLiStoreDataCFL)))
                return FFL_RESULT_FILE_INVALID;
            [[fallthrough]];
        case INPUT_TYPE_FFL_MIIDATACORE:
        {
            FFLiMiiDataCore miiDataCore;
            rio::MemUtil::copy(&miiDataCore, data, sizeof(FFLiMiiDataCore));
            // NOTE: FFLiMiiDataOfficial from CFL/FFL databases
            // are both in big endian, not sure how to detect that
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
            miiDataCore.SwapEndian();
#endif // __BYTE_ORDER__
            FFLiMiiDataCore2CharInfo(pCharInfo, miiDataCore,
            // const u16* pCreatorName, bool resetBirthday
            NULL, false);
            break;
        }
        default: // unknown
            return FFL_RESULT_ERROR;
            break;
    }
    return FFL_RESULT_OK;
}
