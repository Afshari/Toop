uniform sampler2D uPosition;
uniform vec3      uDelta;
uniform float     uDragSpeed;

void main() {
    vec2 uv  = gl_FragCoord.xy / resolution.xy;
    vec4 pos = texture2D(uPosition, uv);

    pos.xyz += uDelta;

    // perturbation for non-root particles based on drag speed
    if (pos.w != 0.0) {
        float idx   = floor(gl_FragCoord.x) + floor(gl_FragCoord.y) * resolution.x;
        float noise = sin(idx * 127.1 + uDragSpeed) * uDragSpeed * 0.002;
        pos.x += noise;
        pos.z += noise;
    }

    gl_FragColor = pos;
}