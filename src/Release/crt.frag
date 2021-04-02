#version 330 compatibility
#define PI 3.1415926538

uniform sampler2D tex;
 
vec2 remap(vec2 uv)
{
    uv = uv * 2.0 - 1.0;
    vec2 offset = abs(uv.yx) / 4.5;
    return (uv + uv * pow(offset, vec2(2.2))) * 0.5 + 0.5;
}

vec4 scanlines(vec2 uv, vec2 res, float opacity)
{
    res.y /= 1.4;
    vec2 intensity = sin(uv * res * PI * 2.0);
    intensity = ((0.5 * intensity) + 0.5) * 0.9 + 0.1;
    return vec4(vec3(pow(intensity.x, opacity) * pow(intensity.y, opacity)), 1.0);
}

void main(void) 
{
    vec2 off = 1.0 / textureSize(tex, 0) * 0.8;
    vec2 uv = remap(gl_TexCoord[0].xy);
    if (uv.x < 0.0 || uv.y < 0.0 || uv.x > 1.0 || uv.y > 1.0){
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        gl_FragColor.a = 1;
        gl_FragColor.r =  texture2D(tex, uv + off).r;
        gl_FragColor.g =  texture2D(tex, uv).g;
        gl_FragColor.b =  texture2D(tex, uv + -off).b;
        gl_FragColor += vec4(0.09, 0.09, 0.09, 0);

        gl_FragColor *= scanlines(uv, textureSize(tex, 0) / 4.0, 0.1);

        float intensity = clamp(uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y) * 3000, 0, 1);
        gl_FragColor.rgb *= intensity;
    }
}