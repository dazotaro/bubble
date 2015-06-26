#version 420

layout(location = 0) in vec3 VertexPosition;
layout(location = 1) in vec2 VertexTexture;
layout(location = 2) in vec3 VertexNormal;

out vec3 Position_eye;
out vec3 Normal_eye;
out vec2 TexCoord;

uniform mat4 ModelViewMatrix;
uniform mat3 NormalMatrix;
uniform mat4 ProjectionMatrix;
uniform mat4 MVP;

void main()
{
    Normal_eye = normalize(NormalMatrix * VertexNormal);
    Position_eye = vec3(ModelViewMatrix * vec4(VertexPosition,1.0));
    TexCoord = VertexTexture;

    gl_Position = MVP * vec4(VertexPosition,1.0);
}
