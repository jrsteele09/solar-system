#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
uniform float animationTime;
uniform float coreRadius;
out vec4 finalColor;

void main() {
    vec2 p = fragTexCoord*2.0-1.0;
    float r = length(p);
    float angle = atan(p.y,p.x);
    float edge = 1.0-smoothstep(0.78,1.0,r);
    float pulse = 0.94+0.06*sin(animationTime*1.7);
    float corona = exp(-max(r-coreRadius,0.0)*7.0)*0.9;
    float glow = 0.42*exp(-r*r*4.0)+corona;
    // Narrow, gently changing optical rays give the bright source sparkle.
    float rays = pow(abs(cos(angle*4.0+0.025*sin(animationTime*0.6))),48.0);
    float fineRays = pow(abs(cos(angle*11.0+0.04*sin(animationTime*0.9))),90.0);
    float shimmer = 0.65+0.35*sin(animationTime*2.3+angle*9.0);
    glow += (rays*0.85+fineRays*shimmer*0.35)*exp(-r*2.2);
    vec3 color = mix(vec3(1.0,0.72,0.30),vec3(1.0,0.98,0.88),exp(-r*r*5.0));
    finalColor = vec4(color,clamp(glow*pulse*edge,0.0,1.0))*fragColor;
}
