uniform sampler2D uPosition;
uniform sampler2D uPrevPos;
uniform float uDt;

void main() {
    vec2 uv  = gl_FragCoord.xy / resolution.xy;
    vec4 pos     = texture2D(uPosition, uv);
    vec4 prevPos = texture2D(uPrevPos,  uv);

    vec3 vel = (pos.xyz - prevPos.xyz) / uDt;

    gl_FragColor = vec4(vel, 0.0);
}