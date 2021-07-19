#version 330 core

layout(location = 0) in vec3 vertexPosition_modelSpace;
layout(location = 1) in vec2 iTextCoord;

out vec2 textCoord;

void main() {
    gl_Position.xyz = vertexPosition_modelSpace;
    gl_Position.w = 1.0;
    textCoord = iTextCoord;
}