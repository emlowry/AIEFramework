#version 150

in vec4 Position;
in vec4 Colour;
in vec2 TextureCoordinate;

out vec4 colour;
out vec2 textureCoordinate;

uniform mat4 view;
uniform mat4 projection;

void main()
{
	colour = Colour;
	textureCoordinate = TextureCoordinate;
	gl_Position = projection * view * Position;
}
