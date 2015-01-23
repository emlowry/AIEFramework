#pragma once

#include "Application.h"
#include <glm/glm.hpp>

#include "RakPeerInterface.h"
#include "UpdateFormat.h"
#include <vector>

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

	void ProcessMessages();
	void DestroyData();

	RakNet::uint24_t newId();

	glm::mat4	m_cameraMatrix;
	glm::mat4	m_projectionMatrix;

	RakNet::RakPeerInterface* m_pInterface;

	std::vector<ServerUpdate*> m_players;
	RakNet::Time m_lastUserListTimestamp = 0;
};