#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D highResTexture;
uniform int ssaaFactor;


/*
void main() {
    vec2 texSize = textureSize(highResTexture, 0);
    vec2 ssaaTexelSize = vec2(1.0 / texSize.x, 1.0 / texSize.y);

    vec4 color = vec4(0.0);
    float totalAlpha = 0.0;

    for (int x = 0; x < ssaaFactor; ++x) {
        for (int y = 0; y < ssaaFactor; ++y) {
            vec2 offset = vec2(x, y) * ssaaTexelSize;
            vec4 sample = texture(highResTexture, TexCoord + offset / float(ssaaFactor));
            color += sample * sample.a;
            totalAlpha += sample.a;
        }
    }

    if (totalAlpha > 0.0) {
        color /= totalAlpha; // Average the color based on alpha
    }
    color.a = totalAlpha / float(ssaaFactor * ssaaFactor); // Average the alpha

    FragColor = color;
}
*/
/*
//uniform float lanczosA;
const float lanczosA = 2.0;

float sinc(float x) {
    if (x == 0.0) {
        return 1.0;
    }
    x *= 3.14159265358979323846; // PI
    return sin(x) / x;
}

float lanczos(float x, float a) {
    if (abs(x) < 1e-5) {
        return 1.0;
    }
    if (abs(x) > a) {
        return 0.0;
    }
    return sinc(x) * sinc(x / a);
}*/
/*
void main() {
    vec2 texSize = textureSize(highResTexture, 0);
    vec2 ssaaTexelSize = vec2(1.0 / texSize.x, 1.0 / texSize.y);

    vec4 color = vec4(0.0);
    float totalWeight = 0.0;
    float a = lanczosA;
    int kernelRadius = int(a);

    for (int x = -kernelRadius; x <= kernelRadius; ++x) {
        for (int y = -kernelRadius; y <= kernelRadius; ++y) {
            vec2 offset = vec2(x, y) * ssaaTexelSize / float(ssaaFactor);
            vec4 sample = texture(highResTexture, TexCoord + offset);
            float weight = lanczos(length(vec2(x, y)) / float(ssaaFactor), a);
            color += sample * weight;
            totalWeight += weight;
        }
    }
    color /= totalWeight;
    FragColor = color;
}
*/

/*
void main() {
    FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Output solid red color
}
*/

void main() {
    FragColor = texture(highResTexture, TexCoord);
}
/*
uniform vec2 resolution;

#define FXAA_REDUCE_MIN   (1.0/128.0)
#define FXAA_REDUCE_MUL   (1.0/8.0)
#define FXAA_SPAN_MAX     8.0

void main() {
    vec4 rgbaNW = texture(highResTexture, TexCoord + (vec2(-1.0, -1.0) / resolution));
    vec4 rgbaNE = texture(highResTexture, TexCoord + (vec2(1.0, -1.0) / resolution));
    vec4 rgbaSW = texture(highResTexture, TexCoord + (vec2(-1.0, 1.0) / resolution));
    vec4 rgbaSE = texture(highResTexture, TexCoord + (vec2(1.0, 1.0) / resolution));
    vec4 rgbaM  = texture(highResTexture, TexCoord);

    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbaNW.rgb, luma);
    float lumaNE = dot(rgbaNE.rgb, luma);
    float lumaSW = dot(rgbaSW.rgb, luma);
    float lumaSE = dot(rgbaSE.rgb, luma);
    float lumaM  = dot(rgbaM.rgb, luma);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);

    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX),
              max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
              dir * rcpDirMin)) / resolution;

    vec4 rgbaA = 0.5 * (
        texture(highResTexture, TexCoord + dir * (1.0 / 3.0 - 0.5)) +
        texture(highResTexture, TexCoord + dir * (2.0 / 3.0 - 0.5)));
    vec4 rgbaB = rgbaA * 0.5 + 0.25 * (
        texture(highResTexture, TexCoord + dir * -0.5) +
        texture(highResTexture, TexCoord + dir * 0.5));

    float lumaB = dot(rgbaB.rgb, luma);
    if ((lumaB < lumaMin) || (lumaB > lumaMax)) {
        FragColor = vec4(rgbaA.rgb, rgbaM.a);
    } else {
        FragColor = vec4(rgbaB.rgb, rgbaM.a);
    }
}*/
