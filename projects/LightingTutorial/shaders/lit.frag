#version 150

in vec3 position;
in vec3 normal;

uniform vec3 lightAmbient;

uniform vec3 lightDirection;
uniform vec3 lightColour;

uniform int hasMaterial;
uniform vec4 materialAmbient;
uniform vec4 materialDiffuse;
uniform vec4 materialSpecular;

uniform vec3 cameraPosition;

void main()
{
	vec3 N = normalize( normal );
	vec3 L = normalize( -lightDirection );

	vec3 R = reflect( -L, N );
	vec3 E = normalize( cameraPosition - position );
	
	float d = max( 0, dot( N, L ) );
	float a = 1;

	// diffuse
	vec3 diffuse = lightColour * d;

	// ambient
	vec3 ambient = lightAmbient;

	// specular
	vec3 specular = lightColour;

	if (!(bool(hasMaterial)))
	{
		// specular
		float s = pow( max( 0, dot( E, R ) ), 128 );
		specular = lightColour * s;
	}
	else
	{
		// diffuse
		diffuse *= materialDiffuse.rgb;

		// ambient
		ambient *= materialAmbient.rgb * materialAmbient.a;

		// specular
		float s = pow( max( 0, dot( E, R ) ), 128 * materialSpecular.a );
		specular *= materialSpecular.rgb * s;

		// final alpha correction
		a = max(materialDiffuse.a, s);
		if (a > 0)
		{
			diffuse *= materialDiffuse.a / a;
			specular /= a;
		}
	}

	gl_FragColor = vec4( ambient + diffuse + specular, a );
}
