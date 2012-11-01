uniform sampler2D entryTex_;
uniform sampler2D exitTex_;
uniform sampler3D volume_;
uniform vec2 dimension_;

// set reference sampling interval for opacity correction
#define REF_SAMPLING_INTERVAL 150.0
// set threshold for early ray termination
#define ERT_THRESHOLD 0.9

vec4 getVoxel(sampler3D volume, vec3 samplePos) {
    return texture3D(volume_, samplePos);
}

vec4 applyTF(vec4 voxel) {
    if (voxel.a < 0.05) return vec4(0.0);
    return vec4(1.0, 1.0, 1.0, 1.0)*voxel.a*5.0;
}

vec4 rayTraversal(vec3 entryPoint, vec3 exitPoint) {
    vec4 result = vec4(0.0);
    float t = 0.0;
    float tIncr = 0.0005;
    vec3 rayDirection = exitPoint - entryPoint;
    float tEnd = length(rayDirection);
    rayDirection = normalize(rayDirection);
    while (t < tEnd) {
        vec3 samplePos = entryPoint + t * rayDirection;
        vec4 voxel = getVoxel(volume_, samplePos);
        vec4 color = applyTF(voxel);
        
        // opacity correction
        color.a = 1.0 - pow(1.0 - color.a, tIncr * REF_SAMPLING_INTERVAL);
        result.rgb = result.rgb + (1.0 - result.a) * color.a * color.rgb;
        result.a = result.a + (1.0 -result.a) * color.a;

        // early ray termination
        if (result.a > ERT_THRESHOLD) t = tEnd;
        else t += tIncr;
    }
    return result;
}

void main() {
    vec2 texCoords = gl_FragCoord.xy * dimension_; //TODO: replace vec2(512.0) by screenDimRCP
    vec3 entryPoint = texture2D(entryTex_, texCoords).rgb;
    vec3 exitPoint = texture2D(exitTex_, texCoords).rgb;
    vec4 color = rayTraversal(entryPoint, exitPoint);
    gl_FragColor = color;
    gl_FragDepth = 0.0;
}