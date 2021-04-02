#version 330 compatibility
in vec2 coords;
uniform sampler2D tex;

void main()
{
    vec4 pixel = texture2D(tex, gl_TexCoord[0].xy);
    gl_FragColor = gl_Color * pixel * vec4(clamp(0.7 - coords.y, 0, 1));
}