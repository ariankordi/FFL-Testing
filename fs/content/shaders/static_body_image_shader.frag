#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D headTexture;
uniform sampler2D bodyTexture;
uniform sampler2D depthTexture;
uniform float threshold;

void main() {
    vec4 headColor = texture(headTexture, TexCoord);
    vec4 bodyColor = texture(bodyTexture, TexCoord);
    float depthValue = texture(depthTexture, TexCoord).r;

    if (depthValue > threshold) {
        // Blend the body texture over the head texture considering alpha
        FragColor = mix(headColor, bodyColor, bodyColor.a);
    } else {
        //FragColor = headColor;
        FragColor = mix(bodyColor, headColor, headColor.a);
    }
}
