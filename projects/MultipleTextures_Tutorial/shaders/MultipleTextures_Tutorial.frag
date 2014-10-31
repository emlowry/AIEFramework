#version 330   

//-------------------------------------
// values sent from the vertex shader

in vec2 vUV;
in vec4 vColor;

//-------------------------------------

// output color
out vec4 outColor;

// textures
uniform sampler2D DiffuseTexture;
uniform sampler2D DecayTexture;
uniform sampler2D MetallicTexture;

// other uniforms
uniform float DecayValue;

void main() 
{ 
	if( texture2D( DecayTexture, vUV.xy ).r < DecayValue )
		outColor = texture2D( DiffuseTexture, vUV.xy ) * vColor;
	else
		outColor = texture2D( MetallicTexture, vUV.xy ) * vColor;
}
