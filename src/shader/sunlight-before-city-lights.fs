#version 330

in vec3 worldPosition;
in vec3 worldNormal;
in vec2 texCoord;

uniform sampler2D texture0;
uniform sampler2D nightMap;
uniform int nightLightsEnabled;
uniform vec4 colDiffuse;
uniform vec3 sunPosition;

out vec4 finalColor;

void main() {
    vec3 normal = normalize(worldNormal);
    vec3 towardSun = normalize(sunPosition - worldPosition);
    float sunlight = max(dot(normal, towardSun), 0.0);
    vec4 surface = texture(texture0, texCoord) * colDiffuse;
    vec3 linearColor = pow(max(surface.rgb, vec3(0.0)), vec3(2.2));
    vec3 litColor = linearColor * (0.007 + sunlight);
    if (nightLightsEnabled != 0) {
        float night = 1.0 - smoothstep(-0.16, 0.04, dot(normal, towardSun));
        vec3 nightColor = pow(texture(nightMap, texCoord).rgb, vec3(2.2));
        litColor = mix(litColor, nightColor * 1.8, night);
    }
    finalColor = vec4(pow(max(litColor, vec3(0.0)), vec3(1.0 / 2.2)), surface.a);
}
