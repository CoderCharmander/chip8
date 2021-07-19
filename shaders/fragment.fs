#version 330 core

out vec4 color;
in vec2 textCoord;
uniform sampler2D emuTexture;
void main() {
    color = texture(emuTexture, textCoord);
}