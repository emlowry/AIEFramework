#pragma once

#include "Application.h"
#include "ParticleSystem.h"
#include <glm/glm.hpp>

// derived application class that wraps up all globals neatly
class ParticleTutorial : public Application
{
public:

	ParticleTutorial();
	virtual ~ParticleTutorial();

protected:

	virtual bool onCreate(int a_argc, char* a_argv[]);
	virtual void onUpdate(float a_deltaTime);
	virtual void onDraw();
	virtual void onDestroy();

	glm::mat4	m_cameraMatrix;
	glm::mat4	m_projectionMatrix;

	unsigned int m_vertShader;
	unsigned int m_fragShader;
	unsigned int m_shader;

	ParticleEmitter* m_emitter;
};