uniform sampler2D uPosition;
uniform float     uGroundY;

void main() {
    vec2 uv  = gl_FragCoord.xy / resolution.xy;
    vec4 pos = texture2D(uPosition, uv);

    if (pos.w == 0.0) { gl_FragColor = pos; return; }   // skip roots

    if (pos.y < uGroundY) pos.y = uGroundY;

    gl_FragColor = pos;
}