#version 150

in vec4 Position;
in vec4 Normal;

out vec3 position;
out vec3 normal;

uniform mat4 view;
uniform mat4 projection;

void main()
{
	position = Position.xyz;
	normal = Normal.xyz;
	gl_Position = projection * view * Position;
}
