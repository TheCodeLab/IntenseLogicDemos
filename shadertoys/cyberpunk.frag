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

float weighted_sin(float a)
{
    return (sin(a)+1) * (sin(a)+1) - 1;
}

void main()
{
    const float pi = 3.14159265;
    vec2 uv = gl_FragCoord.xy / iResolution.xy;
    uv = uv * 2 - vec2(1);
    vec2 xy = vec2(0, 0);
    float f = cos(uv.x * (25 + weighted_sin(iGlobalTime/5)*25)) / 10;
    f += sin(uv.y * (50 + weighted_sin(iGlobalTime/5)*50)) / 100;
    mat3 mat = ident()
        * rotate(iGlobalTime)
        * shear(vec3(0, f - .04, .01))
        * translate(vec3(xy.x, xy.y, 1.0))
        * scale(vec3(1,0.5,1));
    vec3 tuv = mat * vec3(uv-xy, 1);
    uv = tuv.xy / tuv.z;
    float r = mod(uv.x, 50) < 0.5? 1 : .5; //mod(uv.x, 50) / 50;
    float g = mod(uv.y, 50) < 0.5? 1 : .5; //mod(uv.y, 50) / 50;
    float b = 10 / length(uv);
    out_Color = vec3(r-b, g-b, b);
}
