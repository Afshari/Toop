attribute vec2 particleUV;
uniform sampler2D uPositionTex;

void main() {
    vec4 data    = texture2D(uPositionTex, particleUV);
    vec3 worldPos = data.xyz;
    gl_Position  = projectionMatrix * modelViewMatrix * vec4(worldPos, 1.0);
}