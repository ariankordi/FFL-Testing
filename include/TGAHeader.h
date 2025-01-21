#pragma once

#include <Types.h> // just for PACKED()

// tga header as a prefix for render responses
// and used for ShaderMiitomo, HeadwearModel to use images
// most of this is unused though
PACKED(struct TGAHeader
{
    uint8_t idLength;        // unused (0)
    uint8_t colorMapType;    // always 0 for no color map
    uint8_t imageType;       // image_type_enum, 2 = uncomp_true_color
    int16_t colorMapOrigin;  // unused
    int16_t colorMapLength;  // unused (0)
    uint8_t colorMapDepth;   // unused
    int16_t originX;         // unused (0)
    int16_t originY;         // unused (0)
    int16_t width;
    int16_t height;
    uint8_t bitsPerPixel;
    uint8_t imageDescriptor; // ???
});
