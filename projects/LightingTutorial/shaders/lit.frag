#version 150

// struct describing a light source
struct Light
{
	vec3 color;
	vec3 direction;	// zero vector = point light
	vec3 position;
	float power;	// usually 1 if there's no attenuation

	// 0 = no attenuation, otherwise intensity = power / ((distance)^(2 * attenuation))
	float attenuation;

	// only used for spot lights:
	float angle;	// angle between axis and edge of spot light cone, 0 = directional light
	float blur;		// 0 = sharp cutoff, 1 = radial gradient
};

// maximum number of lights this shader can handle
const int MAX_LIGHTS = 10;

in vec3 position;
in vec3 normal;

uniform vec3 lightAmbient = vec3(0, 0, 0);

uniform vec3 lightDirection;
uniform vec3 lightColour;

uniform Light lights[MAX_LIGHTS];
uniform int lightCount = 0;

uniform int hasMaterial = 0;
uniform vec4 materialAmbient;
uniform vec4 materialDiffuse;
uniform vec4 materialSpecular;

uniform vec3 cameraPosition;

// return a vector containing the normalized light direction as the first three
// elements and the light intensity as the fourth, given a point light source
vec4 pointLight(in Light light)
{
	vec3 displacement = position - light.position;
	float intensity = light.power;

	// attenuation is based on distance from point light location
	float squareDistance = dot(displacement, displacement);
	if (light.attenuation > 0 && squareDistance > 0)
	{
		intensity /= pow(squareDistance, light.attenuation);
	}

	return vec4(normalize(displacement), intensity);
}

// return a vector containing the normalized light direction as the first three
// elements and the light intensity as the fourth, given a directional light
// source
vec4 directionalLight(in Light light)
{
	float intensity = light.power;
	vec3 direction = normalize(light.direction);

	if (light.attenuation > 0)
	{
		// attenuation is based on distance from plane containing light location
		// and perpendicular to light direction
		vec3 displacement = position - light.position;
		float distance = dot(direction, displacement);
		if (distance <= 0)
		{
			// no light arrives from behind the directional light if attenuated
			intensity = 0;
		}
		else
		{
			intensity /= pow(distance*distance, light.attenuation);
		}
	}

	return vec4(direction, intensity);
}

// return a vector containing the normalized light direction as the first three
// elements and the light intensity as the fourth, given a spot light source
vec4 spotLight(in Light light)
{
	float intensity = light.power;
	vec3 displacement = position - light.position;
	vec3 direction = normalize(displacement);

	// no light arrives if outside the spot light cone
	float angle = degrees(acos(dot(direction, light.direction)));
	if (angle > light.angle)
	{
		intensity = 0;
	}
	else
	{
		// attenuation is based on distance from light location
		if (light.attenuation > 0)
		{
			float squareDistance = dot(displacement, displacement);
			if (squareDistance > 0)
			{
				intensity /= pow(squareDistance, light.attenuation);
			}
		}

		// blurring is based on angular distance from cone edge, proportional
		// to angle from edge to axis
		if (light.blur > 0)
		{
			float inFromEdge = (light.angle - angle) / light.angle;
			if (inFromEdge < light.blur)
			{
				intensity *= inFromEdge / light.blur;
			}
		}
	}

	return vec4(direction, intensity);
}

// calculates contributions of a given light source to diffuse and specular
// lighting of an object
void calculateLightContributions(in vec3 N, in vec3 E, in Light light,
								 inout vec4 diffuse, inout vec4 specular)
{
	// calculate direction and intensity of light from the given source at this
	// fragment
	vec4 directionAndIntensity =
		(vec3(0, 0, 0) == light.direction) ? pointLight(light) :
		(0 >= light.angle) ? directionalLight(light) : spotLight(light);

	if (0 >= directionAndIntensity.w)
	{
		// no light arriving from this source
		diffuse = vec4(0, 0, 0, diffuse.a);
		specular = vec4(light.color, 0);
	}
	else
	{
		// adjust light intensity
		vec3 color = light.color * directionAndIntensity.w;

		// diffuse
		vec3 L = -directionAndIntensity.xyz;
		float d = max( 0, dot( N, L ) );
		diffuse = vec4(diffuse.rgb * color * d, diffuse.a);

		// specular
		vec3 R = reflect( -L, N );
		float s = pow( max( 0, dot( E, R ) ), 128 * specular.a );
		specular = vec4(specular.rgb * color, s);
	}
}

void main()
{
	// starting values
	vec4 diffuse = vec4(1,1,1,1);
	vec4 ambient = vec4(0,0,0,1);
	vec4 specular = vec4(1,1,1,1);
	if (bool(hasMaterial))
	{
		diffuse = materialDiffuse;
		ambient = materialAmbient;
		specular = materialSpecular;
	}

	// ambient light
	vec3 matte = ambient.rgb * ambient.a * lightAmbient;

	// calculations
	vec3 N = normalize( normal );
	vec3 E = normalize( cameraPosition - position );
	
	// sum individual diffuse and specular light sources
	Light light = Light(lightColour, lightDirection, vec3(0,0,0), 1, 0, 0, 0);
	vec4 currentDiffuse;
	vec4 currentSpecular;
	vec3 gloss = vec3(0, 0, 0);
	for(int i = 0; i < MAX_LIGHTS && i < lightCount; ++i)
	{
		currentDiffuse = diffuse;
		currentSpecular = specular;
		calculateLightContributions(N, E, light, currentDiffuse, currentSpecular);
		matte += currentDiffuse.rgb;
		gloss += currentSpecular.rgb * currentSpecular.a;
	}

	// combine ambient/diffuse with specular
	float glossAlpha = max(gloss.r, gloss.g, gloss.b);
	if (diffuse.a >= 1)
	{
		gl_FragColor = vec4(matte + gloss, 1);
	}
	else if (diffuse.a <=0)
	{
		if (glossAlpha >= 1)
		{
			gl_FragColor = vec4(gloss, 1);
		}
		else if (glossAlpha > 0)
		{
			gl_FragColor = vec4(gloss / glossAlpha, glossAlpha);
		}
		else
		{
			gl_FragColor = vec4(matte, 0);
		}
	}
	else if (glossAlpha >= 1)
	{
		gl_FragColor = vec4((matte / diffuse.a) + gloss, 1);
	}
	else if (glossAlpha > 0)
	{
		
	}
	else
	{
		gl_FragColor = vec4(matte, diffuse.a);
	}

	float alpha = max(diffuse.a, specular.a);
	if (alpha > 0)
	{
		diffuse = vec4(diffuse.rgb * diffuse.a / alpha, alpha);
		specular = vec4(specular.rgb * specular.a / alpha, 0);
	}
	gl_FragColor = ambient + diffuse + specular;
}
