#version 150

in vec4 Position;
in vec4 Normals;
in vec3 TexCoord;
out vec4 normals;
out vec2 texCoord;
out vec4 shadowCoord;
uniform mat4 projectionView;
uniform mat4 world;

uniform mat4 lightProjectionViewWorld;

const mat4 SHADOW_BIAS_MATRIX = mat4(
					0.5, 0.0, 0.0, 0.0,
					0.0, 0.5, 0.0, 0.0,
					0.0, 0.0, 0.5, 0.0,
					0.5, 0.5, 0.5, 1.0
					);
void main()
{
	normals = world * Normals;
	texCoord = TexCoord.xy;
	shadowCoord = SHADOW_BIAS_MATRIX * lightProjectionViewWorld * Position;
	gl_Position = projectionView * world * Position;
}
