#version 330 core

uniform vec4 lightDir; // location = 0
uniform mat4 proj;     // location = 4
uniform mat4 view;     // location = 20
uniform mat4 world;    // location = 36

out vec4 v_lightDir;
out vec4 v_normal;
out vec4 v_position;

layout(location = 0) in vec3 i_position0;
//layout(location = 1) in vec2 i_texCoord0; // UNUSED but provided by RIO
layout(location = 2) in vec3 i_normal0;

int stackIdxVar;
int stateVar;
vec4 RVar[128];

void fs_main()
{
    if (stateVar == 0)
    {
        RVar[1u] = vec4(i_normal0, 0.0);
        RVar[2u] = vec4(i_position0, 0.0);
        //RVar[3u] = vec4(i_texCoord0, 0.0, 0.0); // UNUSED
    }
    if (stateVar == 0)
    {
        return;
    }
}

void main()
{
    stackIdxVar = 0;
    stateVar = 0;
    RVar[0u] = vec4(intBitsToFloat(gl_VertexID), intBitsToFloat(gl_InstanceID), 0.0, 0.0);
    fs_main();
    if (stateVar == 0)
    {
        float _81 = RVar[2u].z;
        float _86 = (_81 * world[2].z) + (world[3].z * uintBitsToFloat(1065353216u));
        float _88 = RVar[2u].z;
        float _92 = (_88 * world[2].y) + (world[3].y * uintBitsToFloat(1065353216u));
        float _94 = RVar[2u].z;
        float _98 = (_94 * world[2].x) + (world[3].x * uintBitsToFloat(1065353216u));
        float _100 = RVar[2u].z;
        float _104 = (_100 * world[2].w) + (world[3].w * uintBitsToFloat(1065353216u));
        RVar[123u].x = _86;
        RVar[123u].y = _92;
        RVar[123u].z = _98;
        RVar[123u].w = _104;
        float _111 = RVar[2u].y;
        float _116 = (_111 * world[1].z) + _86;
        float _118 = RVar[2u].y;
        float _122 = (_118 * world[1].y) + _92;
        float _124 = RVar[2u].y;
        float _128 = (_124 * world[1].x) + _98;
        float _130 = RVar[2u].y;
        float _134 = (_130 * world[1].w) + _104;
        RVar[123u].x = _116;
        RVar[123u].y = _122;
        RVar[123u].z = _128;
        RVar[123u].w = _134;
        float _147 = RVar[2u].x;
        float _153 = RVar[2u].x;
        float _159 = RVar[2u].x;
        float _163 = (_159 * world[0].w) + _134;
        RVar[127u].x = (RVar[2u].x * world[0].z) + _116;
        RVar[127u].y = (_147 * world[0].y) + _122;
        RVar[127u].z = (_153 * world[0].x) + _128;
        RVar[123u].w = _163;
        float _183 = RVar[127u].x;
        float _188 = (_183 * view[2].z) + (_163 * view[3].z);
        float _190 = RVar[127u].x;
        float _194 = (_190 * view[2].y) + (_163 * view[3].y);
        float _196 = RVar[127u].x;
        float _200 = (_196 * view[2].x) + (_163 * view[3].x);
        float _202 = RVar[127u].x;
        float _206 = (_202 * view[2].w) + (_163 * view[3].w);
        RVar[123u].x = _188;
        RVar[123u].y = _194;
        RVar[123u].z = _200;
        RVar[123u].w = _206;
        float _212 = RVar[127u].y;
        float _217 = (_212 * view[1].z) + _188;
        float _219 = RVar[127u].y;
        float _223 = (_219 * view[1].y) + _194;
        float _225 = RVar[127u].y;
        float _229 = (_225 * view[1].x) + _200;
        float _231 = RVar[127u].y;
        float _235 = (_231 * view[1].w) + _206;
        RVar[123u].x = _217;
        RVar[123u].y = _223;
        RVar[123u].z = _229;
        RVar[123u].w = _235;
        float _248 = RVar[127u].z;
        float _254 = RVar[127u].z;
        float _258 = (_254 * view[0].x) + _229;
        float _260 = RVar[127u].z;
        float _264 = (_260 * view[0].w) + _235;
        RVar[127u].x = (RVar[127u].z * view[0].z) + _217;
        RVar[127u].y = (_248 * view[0].y) + _223;
        RVar[127u].z = _258;
        RVar[123u].w = _264;
        RVar[3u].x = _258;
        float _284 = RVar[127u].x;
        float _289 = (_284 * proj[2].y) + (_264 * proj[3].y);
        float _291 = RVar[127u].x;
        float _295 = (_291 * proj[2].x) + (_264 * proj[3].x);
        float _297 = RVar[127u].x;
        float _301 = (_297 * proj[2].w) + (_264 * proj[3].w);
        float _303 = RVar[127u].x;
        float _307 = (_303 * proj[2].z) + (_264 * proj[3].z);
        float _309 = RVar[127u].y;
        RVar[123u].x = _289;
        RVar[123u].y = _295;
        RVar[123u].z = _301;
        RVar[123u].w = _307;
        RVar[3u].y = _309;
        float _316 = RVar[127u].y;
        float _320 = (_316 * proj[1].y) + _289;
        float _322 = RVar[127u].y;
        float _326 = (_322 * proj[1].x) + _295;
        float _328 = RVar[127u].y;
        float _332 = (_328 * proj[1].w) + _301;
        float _334 = RVar[127u].y;
        float _338 = (_334 * proj[1].z) + _307;
        float _340 = RVar[127u].x;
        RVar[123u].x = _320;
        RVar[123u].y = _326;
        RVar[123u].z = _332;
        RVar[123u].w = _338;
        RVar[3u].z = _340;
        float _353 = RVar[127u].z;
        float _359 = RVar[127u].z;
        float _365 = RVar[127u].z;
        RVar[0u].x = (RVar[127u].z * proj[0].x) + _326;
        RVar[0u].y = (_353 * proj[0].y) + _320;
        RVar[0u].z = (_359 * proj[0].z) + _338;
        RVar[0u].w = (_365 * proj[0].w) + _332;
        RVar[127u].x = world[2].x * view[1].z;
        RVar[127u].w = world[2].y * view[1].z;
        RVar[127u].z = world[2].z * view[1].z;
        RVar[126u].z = world[2].y * view[2].z;
        RVar[126u].w = world[2].x * view[2].z;
        RVar[127u].y = world[2].z * view[2].z;
        float _424 = RVar[127u].x;
        RVar[127u].x = (view[1].y * world[1].y) + RVar[127u].w;
        RVar[126u].y = (view[1].y * world[1].x) + _424;
        RVar[127u].w = (view[1].y * world[1].z) + RVar[127u].z;
        float _451 = RVar[126u].z;
        RVar[126u].x = (view[2].y * world[1].x) + RVar[126u].w;
        RVar[126u].w = (view[2].y * world[1].y) + _451;
        RVar[126u].z = (view[2].y * world[1].z) + RVar[127u].y;
        RVar[127u].y = world[2].y * view[0].z;
        RVar[127u].z = world[2].x * view[0].z;
        RVar[125u].x = world[2].z * view[0].z;
        float _497 = RVar[126u].y;
        RVar[125u].y = (view[1].x * world[0].y) + RVar[127u].x;
        RVar[125u].z = (view[1].x * world[0].x) + _497;
        RVar[127u].x = (view[1].x * world[0].z) + RVar[127u].w;
        float _516 = RVar[126u].w;
        float _518 = (view[2].x * world[0].y) + _516;
        float _524 = RVar[126u].x;
        float _526 = (view[2].x * world[0].x) + _524;
        RVar[2u].x = _518;
        RVar[126u].y = _526;
        float _530 = RVar[125u].y;
        float _533 = RVar[127u].x;
        float _540 = RVar[126u].z;
        float _542 = (view[2].x * world[0].z) + _540;
        RVar[127u].w = _542;
        float _550 = RVar[125u].z;
        float _557 = RVar[127u].y;
        float _565 = RVar[127u].z;
        float _569 = RVar[125u].z;
        float _571 = RVar[2u].x;
        float _574 = (_569 * _571) + (-(_530 * _526));
        RVar[4u].x = (RVar[125u].y * _542) + (-(_533 * _518));
        RVar[127u].z = (view[0].y * world[1].y) + _557;
        RVar[126u].w = (view[0].y * world[1].x) + _565;
        RVar[126u].z = _574;
        float _581 = RVar[127u].x;
        float _583 = RVar[126u].y;
        float _586 = (_581 * _583) + (-(_550 * _542));
        float _592 = RVar[125u].x;
        RVar[125u].x = _586;
        RVar[127u].y = (view[0].y * world[1].z) + _592;
        RVar[125u].w = _574;
        float _603 = RVar[126u].w;
        float _605 = (view[0].x * world[0].x) + _603;
        float _611 = RVar[127u].z;
        float _613 = (view[0].x * world[0].y) + _611;
        RVar[124u].x = _605;
        RVar[124u].y = _613;
        RVar[2u].z = _586;
        float _622 = RVar[125u].y;
        float _629 = RVar[127u].y;
        float _631 = (world[1][0] * view[1][2]) + _629;
        float _633 = RVar[127u].w;
        float _636 = RVar[126u].y;
        RVar[126u].x = _613 * RVar[127u].x;
        RVar[127u].y = _605 * _622;
        RVar[127u].z = _631;
        RVar[126u].w = _605 * _633;
        RVar[124u].z = _613 * _636;
        float _644 = RVar[124u].x;
        float _646 = RVar[124u].y;
        float _651 = RVar[4u].x;
        float _653 = RVar[125u].x;
        float _655 = RVar[126u].z;
        float _660 = RVar[125u].z;
        float _663 = RVar[124u].x;
        float _665 = RVar[127u].x;
        float _668 = (_663 * (-_665)) + (_631 * _660);
        float _670 = RVar[127u].z;
        float _672 = RVar[2u].x;
        float _675 = RVar[127u].z;
        float _677 = RVar[126u].y;
        float _680 = RVar[126u].w;
        float _684 = RVar[124u].y;
        float _686 = RVar[125u].z;
        float _689 = RVar[127u].y;
        float _691 = (_684 * (-_686)) + _689;
        float _693 = 1.0 / dot(vec4(_644, _646, _631, uintBitsToFloat(2147483648u)), vec4(_651, _653, _655, uintBitsToFloat(0u)));
        RVar[123u].x = _668;
        RVar[125u].z = (_675 * (-_677)) + _680;
        RVar[123u].w = _691;
        RVar[6u].y = _693;
        float _700 = RVar[124u].x;
        float _702 = RVar[2u].x;
        float _705 = RVar[124u].z;
        float _707 = (_700 * (-_702)) + _705;
        float _709 = RVar[127u].z;
        float _711 = RVar[125u].y;
        float _714 = RVar[126u].x;
        float _716 = (_709 * (-_711)) + _714;
        float _719 = RVar[124u].y;
        float _721 = RVar[127u].w;
        float _724 = (_719 * (-_721)) + (_670 * _672);
        RVar[123u].x = _707;
        RVar[123u].y = _716;
        RVar[123u].w = _724;
        RVar[126u].x = _691 * _693;
        float _736 = RVar[6u].y;
        float _739 = RVar[6u].y;
        float _742 = RVar[6u].y;
        RVar[124u].x = RVar[125u].z * RVar[6u].y;
        RVar[125u].y = _707 * _736;
        RVar[124u].y = _668 * _693;
        float _748 = RVar[126u].x;
        float _755 = RVar[125u].w;
        float _757 = RVar[6u].y;
        RVar[7u].y = RVar[2u].z * RVar[6u].y;
        RVar[1u].w = _755 * _757;
        RVar[2u].y = _724 * _742;
        float _766 = RVar[1u].z;
        float _769 = RVar[1u].z;
        float _771 = RVar[124u].y;
        float _774 = RVar[125u].y;
        float _776 = RVar[1u].z;
        RVar[2u].x = RVar[124u].x;
        RVar[4u].y = _766 * (_716 * _739);
        RVar[2u].z = _769 * _771;
        RVar[2u].w = _774;
        RVar[5u].y = _776 * _748;
    }
    vec4 _799 = RVar[0u];
    gl_Position = _799;
    v_position = RVar[3u];
    if (stateVar == 0)
    {
        float _834 = RVar[1u].w;
        float _836 = RVar[1u].y;
        float _838 = RVar[2u].x;
        float _840 = RVar[2u].z;
        RVar[124u].x = (RVar[1u].y * RVar[2u].y) + RVar[4u].y;
        RVar[125u].y = view[2].x * (-lightDir.z);
        RVar[125u].z = _834;
        RVar[125u].w = (_836 * _838) + _840;
        RVar[127u].z = view[2].y * (-lightDir.z);
        float _855 = RVar[4u].x;
        float _857 = RVar[6u].y;
        float _866 = RVar[1u].y;
        float _868 = RVar[2u].w;
        float _870 = RVar[5u].y;
        float _872 = (_866 * _868) + _870;
        RVar[124u].y = view[2].z * (-lightDir.z);
        RVar[123u].z = _872;
        float _887 = RVar[125u].y;
        float _891 = RVar[1u].x;
        float _893 = RVar[125u].z;
        float _897 = RVar[7u].y;
        float _904 = RVar[127u].z;
        RVar[124u].x = (RVar[1u].x * (_855 * _857)) + RVar[124u].x;
        RVar[125u].y = ((-lightDir.y) * view[1].x) + _887;
        RVar[125u].z = (_891 * _893) + _872;
        RVar[127u].w = ((-lightDir.y) * view[1].y) + _904;
        float _912 = RVar[1u].x;
        float _914 = RVar[125u].w;
        float _916 = (_912 * _897) + _914;
        float _923 = RVar[124u].y;
        RVar[124u].y = _916;
        RVar[127u].z = ((-lightDir.y) * view[1].z) + _923;
        float _929 = RVar[124u].x;
        float _931 = RVar[125u].z;
        float _935 = RVar[124u].x;
        float _937 = RVar[125u].z;
        RVar[1u].x = ((-lightDir.x) * view[0].x) + RVar[125u].y;
        float _961 = inversesqrt(dot(vec4(_929, _916, _931, uintBitsToFloat(2147483648u)), vec4(_935, _916, _937, uintBitsToFloat(0u))));
        RVar[1u].y = ((-lightDir.x) * view[0].y) + RVar[127u].w;
        float _967 = RVar[124u].y;
        float _975 = RVar[127u].z;
        float _979 = RVar[125u].z;
        RVar[7u].x = RVar[124u].x * _961;
        RVar[7u].y = _967 * _961;
        RVar[1u].z = ((-lightDir.x) * view[0].z) + _975;
        RVar[7u].w = _979 * _961;
    }
    // un-negative the light directions
    vec4 lightDirTmp = vec4(-RVar[1u].x, -RVar[1u].y, -RVar[1u].z, -RVar[1u].w);
    v_lightDir = lightDirTmp;
    v_normal = vec4(RVar[7u].x, RVar[7u].y, RVar[7u].w, RVar[7u].w);
    if (stateVar == 0)
    {
    }
}
