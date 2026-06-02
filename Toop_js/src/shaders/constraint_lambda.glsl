uniform sampler2D uPosition;
uniform sampler2D uLambda;
uniform float     uConstraintIndex;
uniform float     uRestLength;
uniform float     uCompliance;
uniform float     uSubDt;

vec2 indexToUV(float idx) {
    float px = mod(idx, resolution.x);
    float py = floor(idx / resolution.x);
    return vec2((px + 0.5) / resolution.x, (py + 0.5) / resolution.y);
}

void main() {
    vec2 uv = gl_FragCoord.xy / resolution.xy;

    float idx        = floor(gl_FragCoord.x) + floor(gl_FragCoord.y) * resolution.x;
    float strandBase = floor(idx / 6.0) * 6.0;
    int   localIdx   = int(idx - strandBase + 0.5);
    int   ci         = int(uConstraintIndex + 0.5);

    // only the parent particle stores lambda
    if (localIdx != ci) {
        gl_FragColor = texture2D(uLambda, uv);
        return;
    }

    vec2 uvA = indexToUV(strandBase + uConstraintIndex);
    vec2 uvB = indexToUV(strandBase + uConstraintIndex + 1.0);

    vec4 pA = texture2D(uPosition, uvA);
    vec4 pB = texture2D(uPosition, uvB);

    float w0 = pA.w, w1 = pB.w, wSum = w0 + w1;
    if (wSum < 1e-6) { gl_FragColor = texture2D(uLambda, uv); return; }

    vec3  d    = pB.xyz - pA.xyz;
    float dist = length(d);
    if (dist  < 1e-6) { gl_FragColor = texture2D(uLambda, uv); return; }

    float alphaTilde = uCompliance / (uSubDt * uSubDt);
    float C          = dist - uRestLength;
    float lambda     = texture2D(uLambda, uv).x;
    float dLambda    = (-C - alphaTilde * lambda) / (wSum + alphaTilde);

    gl_FragColor = vec4(lambda + dLambda, 0.0, 0.0, 0.0);
}