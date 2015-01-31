#pragma once

#include "Application.h"
#include "Scene.h"
#include <glm/glm.hpp>

// derived application class that wraps up all globals neatly
class Physics2D : public Application
{
public:

	Physics2D();
	virtual ~Physics2D();

protected:

	virtual bool onCreate(int a_argc, char* a_argv[]);
	virtual void onUpdate(float a_deltaTime);
	virtual void onDraw();
	virtual void onDestroy();

	glm::mat4	m_cameraMatrix;
	glm::mat4	m_projectionMatrix;

	Scene* m_scene;
};