#pragma once

#include "Application.h"
#include <glm/glm.hpp>

// derived application class that wraps up all globals neatly
class TesselationTutorial : public Application
{
public:

	TesselationTutorial();
	virtual ~TesselationTutorial();

protected:

	virtual bool onCreate(int a_argc, char* a_argv[]);
	virtual void onUpdate(float a_deltaTime);
	virtual void onDraw();
	virtual void onDestroy();

	glm::mat4	m_cameraMatrix;
	glm::mat4	m_projectionMatrix;

	unsigned int m_vao, m_vbo, m_ibo, m_texture, m_displacement, m_shader;
	float m_displacementScale;
	int m_tessLevelInner, m_tessLevelOuter;
};