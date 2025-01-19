#pragma once

#include <nn/ffl.h> // FFLColor

enum PantsColor
{
    PANTS_COLOR_DEFAULT_FOR_SHADER = -1,
    PANTS_COLOR_GRAY               = 0,
    PANTS_COLOR_BLUE               = 1,
    PANTS_COLOR_RED                = 2,
    PANTS_COLOR_GOLD               = 3,
    PANTS_COLOR_COUNT              = PANTS_COLOR_GOLD + 1,
    PANTS_COLOR_SAME_AS_BODY       = 4,
    PANTS_COLOR_NO_DRAW_PANTS      = 5,
    PANTS_COLOR_MAX                = 6
};

const FFLColor cPantsColors[PANTS_COLOR_COUNT] =
{
    // gray - one of your own miis
    // nn::mii::detail::PantsNormalColor
    { 0.2509804f, 0.2745099f, 0.30588239f, 1.0f },

    // blue - foreign mii from other console
    // nn::mii::detail::PantsPresentColor
    { 0.1568628f, 0.2509804f, 0.4705883f, 1.0f },

    // red - favorite/account mii
    // nn::mii::detail::PantsRegularColor
    { 0.43921569f, 0.1254902f, 0.0627451f, 1.0f },

    // gold - special mii created officially
    // nn::mii::detail::PantsSpecialColor
    { 0.75294119f, 0.627451f, 0.1882353f, 1.0f }
};
