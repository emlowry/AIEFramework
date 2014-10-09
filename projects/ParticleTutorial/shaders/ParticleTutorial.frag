#version 150

in vec4 colour;
in vec2 textureCoordinate;

uniform sampler2D textureMap;
uniform int useTexture;

void main()
{
	if (useTexture)
	{
		gl_FragColor = colour * texture( textureMap, textureCoordinate );
	}
	else
	{
		gl_FragColor = colour;
	}
}
