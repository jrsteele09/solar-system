#version 330

in vec3 worldPosition;
in vec3 worldNormal;
in vec2 texCoord;

uniform sampler2D texture0;
uniform sampler2D nightMap;
uniform sampler2D cloudMap;
uniform int cloudsEnabled;
uniform float cloudOpacityScale;
uniform int nightLightsEnabled;
uniform int shadowsEnabled;
uniform vec4 colDiffuse;
uniform vec3 sunPosition;

out vec4 finalColor;

void main() {
    vec3 normal = normalize(worldNormal);
    vec3 towardSun = normalize(sunPosition - worldPosition);
    float sunlight = max(dot(normal, towardSun), 0.0);
    vec4 surface = texture(texture0, texCoord) * colDiffuse;
    float cloud = cloudsEnabled != 0 ? texture(cloudMap, texCoord).r : 0.0;
    float cloudOpacity = (1.0 - exp(-cloud * cloud * 2.6)) * cloudOpacityScale;
    surface.rgb = mix(surface.rgb, vec3(0.92, 0.93, 0.94), cloudOpacity);
    if (shadowsEnabled == 0) {
        finalColor = surface;
        return;
    }
    vec3 linearColor = pow(max(surface.rgb, vec3(0.0)), vec3(2.2));
    vec3 litColor = linearColor * (0.007 + sunlight);
    if (nightLightsEnabled != 0) {
        float night = 1.0 - smoothstep(-0.16, 0.04, dot(normal, towardSun));
        vec3 nightSample = texture(nightMap, texCoord).rgb;
        vec3 nightColor = pow(nightSample, vec3(2.2));
        // Suppress blue-lit terrain while retaining dim warm and neutral lights.
        float redMinusBlue = nightSample.r - nightSample.b;
        float warmth = smoothstep(0.005, 0.035, redMinusBlue);
        float brightness = max(max(nightSample.r, nightSample.g), nightSample.b);
        float neutral = smoothstep(-0.025, 0.0, redMinusBlue) * smoothstep(0.18, 0.4, brightness);
        float city = max(warmth, neutral);
        litColor += nightColor * city * 1.8 * night * (1.0 - cloudOpacity);
    }
    finalColor = vec4(pow(max(litColor, vec3(0.0)), vec3(1.0 / 2.2)), surface.a);
}
