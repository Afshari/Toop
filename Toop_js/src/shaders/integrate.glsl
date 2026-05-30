uniform sampler2D uPosition;
uniform sampler2D uVelocity;
uniform sampler2D uRootOffset;
uniform vec3  uSphereCenter;
uniform vec4  uSphereQuat;
uniform float uGravity;
uniform float uDt;
uniform float uDamping;
uniform vec3 uWind;
uniform float uTime;

// rotate vector v by quaternion q (xyzw)
vec3 quatRotate(vec4 q, vec3 v) {
    vec3 t = 2.0 * cross(q.xyz, v);
    return v + q.w * t + cross(q.xyz, t);
}

void main() {
    vec2 uv  = gl_FragCoord.xy / resolution.xy;
    vec4 pos = texture2D(uPosition, uv);
    vec4 vel = texture2D(uVelocity, uv);

    float invMass = pos.w;

    // root particle - pin to sphere surface
    if (invMass == 0.0) {
        vec3 offset     = texture2D(uRootOffset, uv).xyz;
        vec3 rotated    = quatRotate(uSphereQuat, offset);
        gl_FragColor    = vec4(uSphereCenter + rotated, 0.0);
        return;
    }

    // non-root - integrate
    vec3 v       = vel.xyz;
    float phase = gl_FragCoord.x * 0.11 + gl_FragCoord.y * 0.17;
    float noise = sin(phase + uTime * 2.1) * cos(phase * 0.53 + uTime * 1.3);
    vec3  wind  = uWind * (1.0 + 12.0 * noise);
    v = (v + (vec3(0.0, uGravity, 0.0) + wind) * uDt) * uDamping;
    vec3 newPos  = pos.xyz + v * uDt;

    gl_FragColor = vec4(newPos, invMass);
}