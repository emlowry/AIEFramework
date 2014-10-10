#pragma once

#include "Application.h"
#include <glm/glm.hpp>

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

	glm::mat4	m_cameraMatrix;
	glm::mat4	m_projectionMatrix;
};