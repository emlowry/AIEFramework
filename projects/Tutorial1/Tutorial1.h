#pragma once

#include "Application.h"
#include <glm/glm.hpp>

// derived application class that wraps up all globals neatly
class Tutorial1 : public Application
{
public:

	Tutorial1();
	virtual ~Tutorial1();

protected:

	virtual bool onCreate(int a_argc, char* a_argv[]);
	virtual void onUpdate(float a_deltaTime);
	virtual void onDraw();
	virtual void onDestroy();

	void moveObject(glm::mat4& a_transform, float a_deltaTime, float a_speed);

	glm::mat4	m_cameraMatrix;
	glm::mat4	m_projectionMatrix;
	glm::mat4	m_rotatingMatrix;
	glm::mat4	m_moveMatrix;
};