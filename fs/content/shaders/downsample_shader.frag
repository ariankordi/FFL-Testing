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
            vec2 offset = vec2(x, y) * ssaaTexelSize / float(ssaaFactor);
            vec4 sample = texture(highResTexture, TexCoord + offset);
            color.rgb += sample.rgb * sample.a;
            totalAlpha += sample.a;
        }
    }
    color.rgb /= max(totalAlpha, 1e-5); // Prevent division by zero
    color.a = totalAlpha / float(ssaaFactor * ssaaFactor);
    FragColor = color;
}
*/
/*
void main() {
    vec2 texSize = textureSize(highResTexture, 0);
    vec2 ssaaTexelSize = vec2(1.0 / texSize.x, 1.0 / texSize.y);

    vec4 color = vec4(0.0);
    float totalAlpha = 0.0;

    for (int x = 0; x < ssaaFactor; ++x) {
        for (int y = 0; y < ssaaFactor; ++y) {
            vec2 offset = vec2(x, y) * ssaaTexelSize / float(ssaaFactor);
            vec4 sample = texture(highResTexture, TexCoord + offset);
            color.rgb += sample.rgb * sample.a;
            totalAlpha += sample.a;
        }
    }

    if (totalAlpha > 0.0) {
        color.rgb /= totalAlpha; // Average the color by the accumulated alpha
    }
    color.a = totalAlpha / float(ssaaFactor * ssaaFactor); // Average the alpha

    FragColor = color;
}
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
