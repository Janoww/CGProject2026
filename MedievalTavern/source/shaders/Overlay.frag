#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform OverlayUniformBuffer { float visible; } ubo;
layout(set = 0, binding = 1) uniform sampler2D tex;

void main() {
    if (ubo.visible < 0.5) discard;   // hidden when not on the menu
    outColor = texture(tex, fragUV);
}