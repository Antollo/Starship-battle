#version 330 compatibility
in vec2 coords;
uniform sampler2D tex;
uniform vec2 dir;

void main()
{
    vec2 res = textureSize(tex, 0);
    float pixel = 0.0;
    vec2 off1 = vec2(1.3846153846) * dir;
    vec2 off2 = vec2(3.2307692308) * dir;
    pixel += texture2D(tex, gl_TexCoord[0].xy).r * 0.2270270270;
    pixel += texture2D(tex, gl_TexCoord[0].xy + (off1 / res)).r * 0.3162162162;
    pixel += texture2D(tex, gl_TexCoord[0].xy - (off1 / res)).r * 0.3162162162;
    pixel += texture2D(tex, gl_TexCoord[0].xy + (off2 / res)).r * 0.0702702703;
    pixel += texture2D(tex, gl_TexCoord[0].xy - (off2 / res)).r * 0.0702702703;

    /*vec2 off1 = vec2(1.411764705882353) * dir;
    vec2 off2 = vec2(3.2941176470588234) * dir;
    vec2 off3 = vec2(5.176470588235294) * dir;
    pixel += texture2D(tex, gl_TexCoord[0].xy) * 0.1964825501511404;
    pixel += texture2D(tex, gl_TexCoord[0].xy + (off1 / res)) * 0.2969069646728344;
    pixel += texture2D(tex, gl_TexCoord[0].xy - (off1 / res)) * 0.2969069646728344;
    pixel += texture2D(tex, gl_TexCoord[0].xy + (off2 / res)) * 0.09447039785044732;
    pixel += texture2D(tex, gl_TexCoord[0].xy - (off2 / res)) * 0.09447039785044732;
    pixel += texture2D(tex, gl_TexCoord[0].xy + (off3 / res)) * 0.010381362401148057;
    pixel += texture2D(tex, gl_TexCoord[0].xy - (off3 / res)) * 0.010381362401148057;*/
    //gl_FragColor = gl_Color * pixel;
    gl_FragColor.a = gl_Color.r * pow(pixel, 0.6) * 0.6;
    gl_FragColor.rgb = vec3(1);
}