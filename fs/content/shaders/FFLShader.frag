#version 410

uniform vec4 u_const1;
uniform vec4 u_const2;
uniform vec4 u_const3;
uniform vec4 u_light_ambient;
uniform vec4 u_light_diffuse;
uniform vec3 u_light_dir;
uniform bool u_light_enable;
uniform vec4 u_light_specular;
uniform vec4 u_material_ambient;
uniform vec4 u_material_diffuse;
uniform vec4 u_material_specular;
uniform int u_material_specular_mode;
uniform float u_material_specular_power;
uniform int u_mode;
uniform vec4 u_rim_color;
uniform float u_rim_power;

struct PS_PUSH_DATA
{
    uint alphaFunc;
    float alphaRef;
};

uniform PS_PUSH_DATA PS_PUSH;

uniform sampler2D s_texture;

layout(location = 0) in vec4 PARAM_0;
layout(location = 2) in vec4 PARAM_1;
layout(location = 1) in vec4 PARAM_2;
layout(location = 3) in vec4 PARAM_3;
layout(location = 4) in vec4 PARAM_4;
layout(location = 0) out vec4 PIXEL_0;
int stackIdxVar;
int stateVar;
vec4 RVar[128];
int stackVar[16];
bool predVar;
vec4 _pixelTmp;

void main()
{
    stackIdxVar = 0;
    stateVar = 0;
    RVar[0u] = PARAM_0;
    RVar[1u] = PARAM_1;
    RVar[2u] = PARAM_2;
    RVar[3u] = PARAM_3;
    RVar[4u] = PARAM_4;
    stackVar[stackIdxVar] = stateVar;
    stackIdxVar++;
    if (stateVar == 0)
    {
        RVar[1u].w = intBitsToFloat((u_mode != (0)) ? (-1) : 0);
        bool _75 = floatBitsToInt(RVar[1u].w) == (0);
        predVar = _75;
        stateVar = _75 ? 0 : 1;
    }
    if (stateVar == 0)
    {
        RVar[5u].x = u_const1.x;
        RVar[5u].y = u_const1.y;
        RVar[5u].z = u_const1.z;
        RVar[5u].w = uintBitsToFloat(1065353216u);
    }
    if (stackVar[stackIdxVar - 1] == 0)
    {
        stateVar = int(stateVar == 0);
    }
    stackVar[stackIdxVar] = stateVar;
    stackIdxVar++;
    if (stateVar == 0)
    {
        RVar[1u].w = intBitsToFloat((u_mode != (1)) ? (-1) : 0);
        bool _128 = floatBitsToInt(RVar[1u].w) == (0);
        predVar = _128;
        stateVar = _128 ? 0 : 1;
    }
    if (stateVar == 0)
    {
        RVar[5u] = texture(s_texture, vec4(RVar[4u].xy, 0.0, RVar[4u].x).xy);
    }
    if (stackVar[stackIdxVar - 1] == 0)
    {
        stateVar = int(stateVar == 0);
    }
    stackVar[stackIdxVar] = stateVar;
    stackIdxVar++;
    if (stateVar == 0)
    {
        RVar[1u].w = intBitsToFloat((u_mode != (2)) ? (-1) : 0);
        bool _188 = floatBitsToInt(RVar[1u].w) == (0);
        predVar = _188;
        stateVar = _188 ? 0 : 1;
    }
    if (stateVar == 0)
    {
        RVar[4u] = texture(s_texture, vec4(RVar[4u].xy, 0.0, RVar[4u].x).xy);
    }
    if (stateVar == 0)
    {
        float _212 = RVar[4u].x;
        float _217 = RVar[4u].x;
        float _222 = RVar[4u].x;
        float _227 = RVar[4u].y;
        float _231 = (_227 * u_const2.z) + (_222 * u_const1.z);
        float _233 = RVar[4u].y;
        float _237 = (_233 * u_const2.y) + (_212 * u_const1.y);
        float _239 = RVar[4u].y;
        float _243 = (_239 * u_const2.x) + (_217 * u_const1.x);
        RVar[123u].x = _231;
        RVar[123u].y = _237;
        RVar[123u].z = _243;
        float _255 = RVar[4u].z;
        float _261 = RVar[4u].z;
        RVar[4u].x = (RVar[4u].z * u_const3.x) + _243;
        RVar[4u].y = (_255 * u_const3.y) + _237;
        RVar[0u].z = (_261 * u_const3.z) + _231;
        float _272 = RVar[4u].y;
        float _274 = RVar[0u].z;
        float _276 = RVar[4u].w;
        RVar[5u].x = RVar[4u].x;
        RVar[5u].y = _272;
        RVar[5u].z = _274;
        RVar[5u].w = _276;
    }
    if (stackVar[stackIdxVar - 1] == 0)
    {
        stateVar = int(stateVar == 0);
    }
    stackVar[stackIdxVar] = stateVar;
    stackIdxVar++;
    if (stateVar == 0)
    {
        RVar[1u].w = intBitsToFloat((u_mode != (3)) ? (-1) : 0);
        bool _312 = floatBitsToInt(RVar[1u].w) == (0);
        predVar = _312;
        stateVar = _312 ? 0 : 1;
    }
    if (stateVar == 0)
    {
        RVar[5u] = vec4(RVar[5u].x, RVar[5u].y, RVar[5u].z, texture(s_texture, vec4(RVar[4u].xy, 0.0, RVar[4u].x).xy).x);
    }
    if (stateVar == 0)
    {
        RVar[5u].x = u_const1.x;
        RVar[5u].y = u_const1.y;
        RVar[5u].z = u_const1.z;
    }
    if (stackVar[stackIdxVar - 1] == 0)
    {
        stateVar = int(stateVar == 0);
    }
    stackVar[stackIdxVar] = stateVar;
    stackIdxVar++;
    if (stateVar == 0)
    {
        RVar[1u].w = intBitsToFloat((u_mode != (4)) ? (-1) : 0);
        bool _377 = floatBitsToInt(RVar[1u].w) == (0);
        predVar = _377;
        stateVar = _377 ? 0 : 1;
    }
    if (stateVar == 0)
    {
        vec4 _394 = texture(s_texture, vec4(RVar[4u].xy, 0.0, RVar[4u].x).xy);
        RVar[4u] = vec4(_394.x, _394.y, RVar[4u].z, RVar[4u].w);
    }
    if (stateVar == 0)
    {
        float _408 = RVar[4u].y;
        float _413 = RVar[4u].y;
        float _418 = RVar[4u].x;
        RVar[5u].x = RVar[4u].y * u_const1.x;
        RVar[5u].y = _408 * u_const1.y;
        RVar[5u].z = _413 * u_const1.z;
        RVar[5u].w = _418;
    }
    if (stackVar[stackIdxVar - 1] == 0)
    {
        stateVar = int(stateVar == 0);
    }
    stackVar[stackIdxVar] = stateVar;
    stackIdxVar++;
    if (stateVar == 0)
    {
        RVar[1u].w = intBitsToFloat((u_mode != (5)) ? (-1) : 0);
        bool _454 = floatBitsToInt(RVar[1u].w) == (0);
        predVar = _454;
        stateVar = _454 ? 0 : 1;
    }
    if (stateVar == 0)
    {
        vec4 _471 = texture(s_texture, vec4(RVar[4u].xy, 0.0, RVar[4u].x).xy);
        RVar[4u] = vec4(_471.x, _471.x, _471.x, RVar[4u].w);
    }
    if (stateVar == 0)
    {
        float _485 = RVar[4u].y;
        float _490 = RVar[4u].z;
        RVar[5u].x = RVar[4u].x * u_const1.x;
        RVar[5u].y = _485 * u_const1.y;
        RVar[5u].z = _490 * u_const1.z;
        RVar[5u].w = uintBitsToFloat(1065353216u);
    }
    int stackIdx = stackIdxVar;
    int newStackIdx = stackIdx - 2;
    stackIdxVar = newStackIdx;
    stateVar = stackVar[newStackIdx];
    int stackIdx_1 = stackIdxVar;
    int newStackIdx_1 = stackIdx_1 - 2;
    stackIdxVar = newStackIdx_1;
    stateVar = stackVar[newStackIdx_1];
    int stackIdx_2 = stackIdxVar;
    int newStackIdx_2 = stackIdx_2 - 2;
    stackIdxVar = newStackIdx_2;
    stateVar = stackVar[newStackIdx_2];
    stackVar[stackIdxVar] = stateVar;
    stackIdxVar++;
    if (stateVar == 0)
    {
        bool _524 = u_light_enable;
        predVar = _524;
        stateVar = _524 ? 0 : 1;
    }
    stackVar[stackIdxVar] = stateVar;
    stackIdxVar++;
    if (stateVar == 0)
    {
        float _537 = RVar[1u].x;
        float _539 = RVar[1u].y;
        float _541 = RVar[1u].z;
        float _546 = RVar[1u].x;
        float _548 = RVar[1u].y;
        float _550 = RVar[1u].z;
        RVar[4u].x = uintBitsToFloat(0u);
        float _559 = inversesqrt(dot(vec4(_537, _539, _541, uintBitsToFloat(2147483648u)), vec4(_546, _548, _550, uintBitsToFloat(0u))));
        RVar[4u].y = uintBitsToFloat(0u);
        RVar[4u].z = uintBitsToFloat(0u);
        float _563 = RVar[1u].x;
        float _564 = _563 * _559;
        float _566 = RVar[1u].y;
        float _567 = _566 * _559;
        float _569 = RVar[1u].z;
        float _570 = _569 * _559;
        RVar[1u].x = _564;
        RVar[1u].y = _567;
        RVar[0u].z = _570;
        RVar[1u].w = max(dot(vec4(u_light_dir.x, u_light_dir.y, u_light_dir.z, uintBitsToFloat(2147483648u)), vec4(_564, _567, _570, uintBitsToFloat(0u))), uintBitsToFloat(1036831949u));
        bool _592 = RVar[1u].w >= uintBitsToFloat(0u);
        predVar = _592;
        stateVar = _592 ? 0 : 1;
    }
    stackVar[stackIdxVar] = stateVar;
    stackIdxVar++;
    if (stateVar == 0)
    {
        float _604 = RVar[1u].w;
        float _609 = RVar[1u].w;
        float _614 = RVar[1u].w;
        RVar[1u].w = uintBitsToFloat(0u);
        RVar[6u].x = (_614 * u_light_diffuse.x) * u_material_diffuse.x;
        RVar[6u].y = (_609 * u_light_diffuse.y) * u_material_diffuse.y;
        RVar[6u].z = (_604 * u_light_diffuse.z) * u_material_diffuse.z;
        bool _639 = u_material_specular_mode == (0);
        predVar = _639;
        stateVar = _639 ? 0 : 1;
    }
    if (stateVar == 0)
    {
        float _648 = RVar[1u].x;
        float _650 = RVar[1u].y;
        float _652 = RVar[0u].z;
        float _666 = dot(vec4(_648, _650, _652, uintBitsToFloat(2147483648u)), vec4(-u_light_dir.x, -u_light_dir.y, -u_light_dir.z, uintBitsToFloat(0u)));
        RVar[4u].x = uintBitsToFloat(1065353216u);
        float _670 = RVar[2u].x;
        float _673 = RVar[2u].y;
        float _676 = RVar[2u].z;
        float _681 = RVar[2u].x;
        float _684 = RVar[2u].y;
        float _687 = RVar[2u].z;
        float _692 = _666 + _666;
        float _703 = RVar[1u].y;
        float _711 = RVar[0u].z;
        float _717 = inversesqrt(dot(vec4(-_670, -_673, -_676, uintBitsToFloat(2147483648u)), vec4(-_681, -_684, -_687, uintBitsToFloat(0u))));
        RVar[127u].x = ((-_692) * RVar[1u].x) + (-u_light_dir.x);
        RVar[127u].y = ((-_692) * _703) + (-u_light_dir.y);
        RVar[127u].z = ((-_692) * _711) + (-u_light_dir.z);
        RVar[2u].w = exp(log(max(dot(vec4(RVar[127u].x, RVar[127u].y, RVar[127u].z, uintBitsToFloat(2147483648u)), vec4((-RVar[2u].x) * _717, (-RVar[2u].y) * _717, (-RVar[2u].z) * _717, uintBitsToFloat(0u))), uintBitsToFloat(0u))) * u_material_specular_power);
        RVar[0u].x = RVar[4u].x;
    }
    if (stackVar[stackIdxVar - 1] == 0)
    {
        stateVar = int(stateVar == 0);
    }
    if (stateVar == 0)
    {
        float _772 = RVar[2u].x;
        float _775 = RVar[2u].y;
        float _778 = RVar[2u].z;
        float _783 = RVar[2u].x;
        float _786 = RVar[2u].y;
        float _789 = RVar[2u].z;
        RVar[4u].x = RVar[0u].y;
        float _798 = RVar[3u].x;
        float _800 = RVar[3u].y;
        float _802 = RVar[3u].z;
        float _813 = dot(vec4(_798, _800, _802, uintBitsToFloat(2147483648u)), vec4(u_light_dir.x, u_light_dir.y, u_light_dir.z, uintBitsToFloat(0u)));
        float _814 = inversesqrt(dot(vec4(-_772, -_775, -_778, uintBitsToFloat(2147483648u)), vec4(-_783, -_786, -_789, uintBitsToFloat(0u))));
        RVar[126u].z = _813;
        float _818 = RVar[2u].x;
        float _820 = (-_818) * _814;
        float _822 = RVar[2u].y;
        float _824 = (-_822) * _814;
        float _826 = RVar[2u].z;
        float _828 = (-_826) * _814;
        float _834 = RVar[0u].z;
        RVar[127u].x = _820;
        RVar[127u].y = _824;
        RVar[127u].z = _828;
        RVar[127u].w = ((-_813) * _813) + uintBitsToFloat(1065353216u);
        float _844 = RVar[3u].x;
        float _846 = RVar[3u].y;
        float _848 = RVar[3u].z;
        float _853 = dot(vec4(_844, _846, _848, uintBitsToFloat(2147483648u)), vec4(_820, _824, _828, uintBitsToFloat(0u)));
        float _855 = RVar[1u].y;
        float _860 = (_855 * (-u_light_dir.y)) + (_834 * (-u_light_dir.z));
        RVar[126u].y = _853;
        RVar[122u].x = _860;
        float _867 = ((-_853) * _853) + uintBitsToFloat(1065353216u);
        float _869 = RVar[1u].x;
        float _874 = (_869 * (-u_light_dir.x)) + _860;
        float _876 = RVar[127u].w;
        RVar[123u].x = _867;
        RVar[123u].z = _874;
        RVar[127u].w = sqrt(_876);
        float _881 = _874 + _874;
        float _885 = RVar[1u].x;
        float _890 = ((-_881) * _885) + (-u_light_dir.x);
        float _893 = RVar[1u].y;
        float _898 = ((-_881) * _893) + (-u_light_dir.y);
        float _901 = RVar[0u].z;
        float _906 = ((-_881) * _901) + (-u_light_dir.z);
        float _908 = RVar[127u].w;
        RVar[123u].x = _890;
        RVar[123u].y = _898;
        RVar[123u].z = _906;
        float _916 = RVar[127u].x;
        float _918 = RVar[127u].y;
        float _920 = RVar[127u].z;
        float _925 = RVar[126u].z;
        float _928 = RVar[126u].y;
        float _930 = ((-_925) * _928) + (_908 * sqrt(_867));
        RVar[122u].x = _930;
        RVar[127u].x = max(_930, uintBitsToFloat(0u));
        float _942 = RVar[127u].x;
        RVar[2u].w = exp(log(max(dot(vec4(_890, _898, _906, uintBitsToFloat(2147483648u)), vec4(_916, _918, _920, uintBitsToFloat(0u))), uintBitsToFloat(0u))) * u_material_specular_power);
        RVar[1u].w = exp(log(_942) * u_material_specular_power);
    }
    int stackIdx_3 = stackIdxVar;
    int newStackIdx_3 = stackIdx_3 - 1;
    stackIdxVar = newStackIdx_3;
    stateVar = stackVar[newStackIdx_3];
    if (stateVar == 0)
    {
        float _960 = RVar[0u].x;
        float _965 = RVar[1u].w;
        float _968 = RVar[2u].w;
        float _970 = RVar[0u].x;
        float _972 = (_968 * _970) + (_965 * ((-_960) + uintBitsToFloat(1065353216u)));
        RVar[123u].y = _972;
        float _998 = RVar[4u].x;
        float _1001 = RVar[4u].x;
        RVar[4u].x *= ((_972 * u_light_specular.x) * u_material_specular.x);
        RVar[4u].y = _998 * ((_972 * u_light_specular.y) * u_material_specular.y);
        RVar[4u].z = _1001 * ((_972 * u_light_specular.z) * u_material_specular.z);
    }
    if (stackVar[stackIdxVar - 1] == 0)
    {
        stateVar = int(stateVar == 0);
    }
    if (stateVar == 0)
    {
        float _1023 = RVar[4u].y;
        float _1025 = RVar[4u].z;
        RVar[6u].x = RVar[4u].x;
        RVar[6u].y = _1023;
        RVar[6u].z = _1025;
    }
    int stackIdx_4 = stackIdxVar;
    int newStackIdx_4 = stackIdx_4 - 1;
    stackIdxVar = newStackIdx_4;
    stateVar = stackVar[newStackIdx_4];
    if (stateVar == 0)
    {
        float _1043 = RVar[6u].x;
        float _1045 = (u_light_ambient.x * u_material_ambient.x) + _1043;
        float _1047 = RVar[0u].z;
        float _1049 = RVar[0u].z;
        float _1057 = RVar[6u].y;
        float _1059 = (u_light_ambient.y * u_material_ambient.y) + _1057;
        RVar[123u].x = _1045;
        RVar[123u].w = _1059;
        float _1076 = RVar[6u].z;
        float _1078 = (u_light_ambient.z * u_material_ambient.z) + _1076;
        float _1080 = RVar[5u].y;
        float _1082 = RVar[4u].y;
        RVar[127u].x = (RVar[5u].x * _1045) + RVar[4u].x;
        RVar[123u].z = _1078;
        RVar[127u].w = (_1080 * _1059) + _1082;
        float _1089 = RVar[0u].w;
        RVar[126u].w = (RVar[5u].z * _1078) + RVar[4u].z;
        float _1103 = exp(log(_1089 * ((-max(_1047, -_1049)) + uintBitsToFloat(1065353216u))) * u_rim_power);
        float _1114 = RVar[127u].w;
        float _1120 = RVar[126u].w;
        RVar[5u].x = (_1103 * u_rim_color.x) + RVar[127u].x;
        RVar[5u].y = (_1103 * u_rim_color.y) + _1114;
        RVar[5u].z = (_1103 * u_rim_color.z) + _1120;
    }
    int stackIdx_5 = stackIdxVar;
    int newStackIdx_5 = stackIdx_5 - 1;
    stackIdxVar = newStackIdx_5;
    stateVar = stackVar[newStackIdx_5];
    switch (PS_PUSH.alphaFunc & 255u)
    {
        case 0u:
        {
            discard;
        }
        case 1u:
        {
            if (!(RVar[5u].w < PS_PUSH.alphaRef))
            {
                discard;
            }
            break;
        }
        case 2u:
        {
            if (abs(RVar[5u].w - PS_PUSH.alphaRef) > 9.9999997473787516355514526367188e-05)
            {
                discard;
            }
            break;
        }
        case 3u:
        {
            if (!(RVar[5u].w <= PS_PUSH.alphaRef))
            {
                discard;
            }
            break;
        }
        case 4u:
        {
            if (!(RVar[5u].w > PS_PUSH.alphaRef))
            {
                discard;
            }
            break;
        }
        case 5u:
        {
            if (abs(RVar[5u].w - PS_PUSH.alphaRef) <= 9.9999997473787516355514526367188e-05)
            {
                discard;
            }
            break;
        }
        case 6u:
        {
            if (!(RVar[5u].w >= PS_PUSH.alphaRef))
            {
                discard;
            }
            break;
        }
    }
    _pixelTmp = RVar[5u];
    uint _1201 = PS_PUSH.alphaFunc >> 8u;
    if (_1201 == 1u)
    {
        _pixelTmp = vec4(1.0);
    }
    else
    {
        if (_1201 == 2u)
        {
            _pixelTmp = vec4(0.0);
        }
    }
    PIXEL_0 = _pixelTmp;
}

