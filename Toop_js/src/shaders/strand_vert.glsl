attribute vec2 particleUV;

uniform sampler2D uPositionTex;
uniform float     uTexSize;
uniform vec3      uLightDir;

varying float vStrandT;
varying float vDiffuse;

void main() {
    float texSize    = uTexSize;
    float px         = floor(particleUV.x * texSize - 0.5 + 0.5);
    float py         = floor(particleUV.y * texSize - 0.5 + 0.5);
    float particleIdx = py * texSize + px;

    float strandIdx  = floor(particleIdx / 6.0);
    float localIdx   = particleIdx - strandIdx * 6.0;
    vStrandT         = localIdx / 5.0;

    // current position
    vec4 data     = texture2D(uPositionTex, particleUV);
    vec3 worldPos = data.xyz;

    // adjacent particle for tangent
    float adjIdx = (localIdx < 5.0) ? particleIdx + 1.0 : particleIdx - 1.0;
    float adjPx  = mod(adjIdx, texSize);
    float adjPy  = floor(adjIdx / texSize);
    vec2  adjUV  = vec2((adjPx + 0.5) / texSize, (adjPy + 0.5) / texSize);
    vec3  adjPos = texture2D(uPositionTex, adjUV).xyz;

    // tangent along strand direction
    vec3 tangent = (localIdx < 5.0)
        ? normalize(adjPos - worldPos)
        : normalize(worldPos - adjPos);

    // Kajiya-Kay diffuse: sin of angle between tangent and light
    float cosT  = dot(tangent, normalize(uLightDir));
    float sinTL = sqrt(max(0.0, 1.0 - cosT * cosT));
    vDiffuse    = 0.3 + 0.4 * sinTL;

    gl_Position = projectionMatrix * modelViewMatrix * vec4(worldPos, 1.0);
}