#version 330
 
in vec3 TexCoord1;

out vec4 outColor;

uniform samplerCube CubeMap;
 
void main (void) 
{
    outColor = texture(CubeMap, TexCoord1);
}
