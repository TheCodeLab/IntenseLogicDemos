#version 140
uniform vec2      iResolution;           // viewport resolution (in pixels)
uniform float     iGlobalTime;           // shader playback time (in seconds)

out vec3 out_Color;

mat3 rotate(float a)
{
    return mat3(sin(a), -cos(a), 0,
                cos(a),  sin(a), 0,
                0,       0,      1);
}

mat3 scale(vec3 v)
{
    return mat3(v.x, 0,   0,
                0,   v.y, 0,
                0,   0,   v.z);
}

mat3 translate(vec3 v)
{
    return mat3(1,   0,   0,
                0,   1,   0,
                v.x, v.y, v.z);
}

mat3 shear(vec3 v)
{
    return mat3(1, 0, v.x,
                0, 1, v.y,
                0, 0, v.z);
}

mat3 ident()
{
    return mat3(1, 0, 0,
                0, 1, 0,
                0, 0, 1);
}

void main()
{
    vec2 uv = gl_FragCoord.xy / iResolution.xy;
    uv = uv * 2 - vec2(1);
    vec2 xy = vec2(0, 0);
    float f = cos(uv.x * 10100) / 10;
    mat3 mat = ident()
        * rotate(iGlobalTime)
        * shear(vec3(0, f - .01, .01))
        * translate(vec3(xy.x, xy.y, 1.0));
    vec3 tuv = mat * vec3(uv-xy, 1);
    uv = tuv.xy / tuv.z;
    float r = mod(uv.x, 10) / 10;
    float g = mod(uv.y, 10) / 10;
    float b = 20 / length(uv);
    out_Color = vec3(r, g, b);
}
