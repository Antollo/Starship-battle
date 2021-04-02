#version 330 compatibility
out vec2 coords;

void main()
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    gl_Position = gl_Position / gl_Position.w;
    coords = gl_Position.xy;
    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
    gl_FrontColor = gl_Color;
}