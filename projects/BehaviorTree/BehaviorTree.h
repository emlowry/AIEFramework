#pragma once

#include "Application.h"
#include <glm/glm.hpp>

class Agent;
class Behavior;

// derived application class that wraps up all globals neatly
class BehaviorTree : public Application
{
public:

	BehaviorTree();
	virtual ~BehaviorTree();

protected:

	virtual bool onCreate(int a_argc, char* a_argv[]);
	virtual void onUpdate(float a_deltaTime);
	virtual void onDraw();
	virtual void onDestroy();

	Agent*		m_agent;
	Behavior*	m_behavior;

	glm::mat4	m_cameraMatrix;
	glm::mat4	m_projectionMatrix;
};