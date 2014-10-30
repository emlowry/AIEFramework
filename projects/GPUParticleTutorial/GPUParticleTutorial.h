#pragma once

#include "Application.h"
#include "GPUParticleSystem.h"
#include <glm/glm.hpp>

// derived application class that wraps up all globals neatly
class GPUParticleTutorial : public Application
{
public:

	GPUParticleTutorial(const char* a_titleFormat);
	virtual ~GPUParticleTutorial();

protected:

	virtual bool onCreate(int a_argc, char* a_argv[]);
	virtual void onUpdate(float a_deltaTime);
	virtual void onDraw();
	virtual void onDestroy();

	unsigned int m_frameCount;
	float m_timeSinceLastFPSUpdate;
	float m_FPSUpdateInterval = 1.0f;
	char m_titleFormat[256];

	glm::mat4	m_cameraMatrix;
	glm::mat4	m_projectionMatrix;

	GPUParticleEmitter* m_emitter = nullptr;
};