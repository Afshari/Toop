uniform vec3 uColor;

varying float vStrandT;
varying float vDiffuse;

void main() {
    // very subtle root darkening only
    float root  = 0.7 + 0.3 * vStrandT;
    vec3  color = uColor * root;

    // gentle tip fade - don't go fully transparent
    float alpha = 0.95 - 0.35 * vStrandT * vStrandT;

    gl_FragColor = vec4(color, alpha);
}