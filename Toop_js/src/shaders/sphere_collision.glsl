uniform sampler2D uPosition;
uniform vec3      uSphereCenter;
uniform float     uSphereRadius;
uniform float     uComplianceSph;
uniform float     uSubDt;

void main() {
    vec2 uv  = gl_FragCoord.xy / resolution.xy;
    vec4 pos = texture2D(uPosition, uv);

    if (pos.w == 0.0) { gl_FragColor = pos; return; }

    vec3  d    = pos.xyz - uSphereCenter;
    float dist = length(d);

    if (dist >= uSphereRadius || dist < 1e-6) {
        gl_FragColor = pos;
        return;
    }

    float C          = dist - uSphereRadius;
    float alphaTilde = uComplianceSph / (uSubDt * uSubDt);
    float dLambda    = -C / (pos.w + alphaTilde);

    pos.xyz += pos.w * dLambda * (d / dist);
    gl_FragColor = pos;
}