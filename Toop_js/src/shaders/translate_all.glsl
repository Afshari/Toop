uniform sampler2D uPosition;
uniform vec3      uDelta;

void main() {
    vec2 uv  = gl_FragCoord.xy / resolution.xy;
    vec4 pos = texture2D(uPosition, uv);
    pos.xyz += uDelta;
    gl_FragColor = pos;
}