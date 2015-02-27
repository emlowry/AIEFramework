#version 150

in vec4 Position;
in vec4 Normals;
in vec2 TexCoord;
out vec4 normals;
out vec2 texCoord;
uniform mat4 projectionView;
uniform mat4 world;

void main()
{
	normals = world * Normals;
	texCoord = TexCoord;
	gl_Position = projectionView * world * Position;
}
