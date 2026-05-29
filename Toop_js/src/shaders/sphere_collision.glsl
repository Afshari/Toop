uniform sampler2D uPosition;
uniform vec3      uSphereCenter;
uniform float     uSphereRadius;

void main() {
    vec2 uv  = gl_FragCoord.xy / resolution.xy;
    vec4 pos = texture2D(uPosition, uv);

    if (pos.w == 0.0) { gl_FragColor = pos; return; }   // skip roots

    vec3  d    = pos.xyz - uSphereCenter;
    float dist = length(d);

    if (dist < uSphereRadius && dist > 1e-6)
        pos.xyz = uSphereCenter + normalize(d) * uSphereRadius;

    gl_FragColor = pos;
}