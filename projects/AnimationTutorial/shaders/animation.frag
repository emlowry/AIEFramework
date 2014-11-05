#version 330   

//-------------------------------------
// values sent from the vertex shader

in vec2 TexCoord;

//-------------------------------------

// output color
out vec4 outColor;

// textures
uniform sampler2D DiffuseTexture;

void main() 
{ 
	outColor = texture2D( DiffuseTexture, TexCoord.xy );
	outColor.a = 1;
}
