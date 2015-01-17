#pragma once

#include "Application.h"
#include <glm/glm.hpp>

#include "RakPeerInterface.h"

// derived application class that wraps up all globals neatly
class Networking_Server : public Application
{
public:

	Networking_Server();
	virtual ~Networking_Server();

protected:

	virtual bool onCreate(int a_argc, char* a_argv[]);
	virtual void onUpdate(float a_deltaTime);
	virtual void onDraw();
	virtual void onDestroy();

	glm::mat4	m_cameraMatrix;
	glm::mat4	m_projectionMatrix;

	RakNet::RakPeerInterface* m_pInterface;
};