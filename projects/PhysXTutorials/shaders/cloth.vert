#version 330

in vec3 position;
in vec2 texCoords;

out vec2 textureCoords;

uniform mat4 projectionView;

void main()
{
	textureCoords = texCoords;
	gl_Position = projectionView * vec4(position,1);
}
