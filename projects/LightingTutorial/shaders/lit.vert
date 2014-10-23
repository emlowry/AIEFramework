in vec4 Position;
in vec4 Normal;

uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * Position;
}
