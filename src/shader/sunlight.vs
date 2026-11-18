#version 330

in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 vertexTexCoord;

uniform mat4 mvp;
uniform mat4 matModel;

out vec3 worldPosition;
out vec3 worldNormal;
out vec2 texCoord;

void main() {
    worldPosition = (matModel * vec4(vertexPosition, 1.0)).xyz;
    worldNormal = mat3(matModel) * vertexNormal;
    texCoord = vertexTexCoord;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
