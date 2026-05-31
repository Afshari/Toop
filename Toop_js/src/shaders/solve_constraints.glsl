uniform sampler2D uPosition;
uniform float     uConstraintIndex;
uniform float     uRestLength;

vec2 indexToUV(float idx) {
    float px = mod(idx, resolution.x);
    float py = floor(idx / resolution.x);
    return vec2((px + 0.5) / resolution.x, (py + 0.5) / resolution.y);
}

void main() {
    vec2 uv  = gl_FragCoord.xy / resolution.xy;
    vec4 pos = texture2D(uPosition, uv);

    float idx        = floor(gl_FragCoord.x) + floor(gl_FragCoord.y) * resolution.x;
    float strandBase = floor(idx / 6.0) * 6.0;
    int   localIdx   = int(idx - strandBase + 0.5);
    int ci           = int(uConstraintIndex + 0.5);

    // only the two particles of this constraint do work
    if (localIdx != ci && localIdx != ci + 1) {
        gl_FragColor = pos;
        return;
    }

    vec2 uvA = indexToUV(strandBase + uConstraintIndex);
    vec2 uvB = indexToUV(strandBase + uConstraintIndex + 1.0);

    vec4 pA = texture2D(uPosition, uvA);
    vec4 pB = texture2D(uPosition, uvB);

    float w0   = pA.w;
    float w1   = pB.w;
    float wSum = w0 + w1;

    if (wSum < 1e-6) { gl_FragColor = pos; return; }

    vec3  d    = pB.xyz - pA.xyz;
    float dist = length(d);
    if (dist  < 1e-6) { gl_FragColor = pos; return; }

    float corr = (dist - uRestLength) / (dist * wSum);
    vec3  n    = d * corr;

    if (localIdx == ci) {
        gl_FragColor = vec4(pA.xyz + n * w0, pA.w);
    } else {
        gl_FragColor = vec4(pB.xyz - n * w1, pB.w);
    }
}