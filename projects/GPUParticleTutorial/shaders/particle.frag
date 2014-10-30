#version 150

in vec4 Colour;
in vec2 textureCoordinate;

uniform sampler2D textureMap;
uniform int useTexture;

void main()
{
	if (useTexture)
	{
		gl_FragColor = Colour * texture( textureMap, textureCoordinate );
	}
	else
	{
		gl_FragColor = Colour;
	}
}
