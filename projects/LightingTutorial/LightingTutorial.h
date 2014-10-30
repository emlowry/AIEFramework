#pragma once

#include "Application.h"
#include "FBXFile.h"
#include <glm/glm.hpp>

// struct describing a light source
struct Light
{
	glm::vec3 color = glm::vec3(1, 1, 1);
	glm::vec3 direction = glm::vec3(0, 0, 0);	// zero vector = point light
	glm::vec3 position = glm::vec3(0, 0, 0);

	// intensity = power / ((distance)^(2 * attenuation))
	float power = 1;		// usually 1 if there's no attenuation
	float attenuation = 0;	// 0 means no attenuation

	// only used for spot lights:
	float angle = 0;	// angle between axis and edge of spot light cone, 0 = directional light
	float blur = 0;		// 0 = sharp cutoff, 1 = radial gradient
};

// derived application class that wraps up all globals neatly
class LightingTutorial : public Application
{
public:

	LightingTutorial();
	virtual ~LightingTutorial();

protected:

	virtual bool onCreate(int a_argc, char* a_argv[]);
	virtual void onUpdate(float a_deltaTime);
	virtual void onDraw();
	virtual void onDestroy();

	void createOpenGLBuffers(FBXFile* a_fbx);
	void cleanupOpenGLBuffers(FBXFile* a_fbx);

	glm::mat4	m_cameraMatrix;
	glm::mat4	m_projectionMatrix;

	unsigned int m_vertShader;
	unsigned int m_fragShader;
	unsigned int m_shader;

	glm::vec3	m_lightAmbient;

	static const int MAX_LIGHTS = 10;
	Light m_lights[MAX_LIGHTS];
	int m_lightCount = 0;

	FBXFile* m_fbx;
};