#version 150

out vec4 FragColor;
in vec4 normals;
in vec2 texCoord;
uniform vec3 ambientLight;
uniform vec4 lightDir;
uniform sampler2D diffuseMap;
const float SHADOW_BIAS = 0.002f;

void main()
{
	// sample diffuse texture and perform lambert lighting
	float NdL = max( 0, dot( normalize(normals), normalize(-lightDir) ));
	vec4 texColour = texture( diffuseMap, texCoord );

	// combine diffuse + ambient
	FragColor.rgb = texColour.rgb * NdL + texColour.rgb * ambientLight;
	FragColor.a = texColour.a;
}
