#version 330
 
in vec3 Position;

out vec3 TexCoord1;

uniform mat4 ModelViewProjection;
 
void main()
{
    gl_Position = ModelViewProjection * vec4(Position, 1.0);
    TexCoord1 = vec3(-Position.x, Position.y, Position.z);
}
