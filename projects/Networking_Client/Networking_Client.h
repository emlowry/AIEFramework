#pragma once

#include "Application.h"
#include <glm/glm.hpp>

#include "RakPeerInterface.h"
#include "UpdateFormat.h"
#include <vector>

// derived application class that wraps up all globals neatly
class Networking_Client : public Application
{
public:

	Networking_Client();
	virtual ~Networking_Client();

protected:

	virtual bool onCreate(int a_argc, char* a_argv[]);
	virtual void onUpdate(float a_deltaTime);
	virtual void onDraw();
	virtual void onDestroy();

	bool Connect(float a_timeout = TIMEOUT_INTERVAL);
	bool Login(float a_timeout = TIMEOUT_INTERVAL);
	bool ProcessMessages();
	void DestroyData();

	glm::mat4	m_cameraMatrix;
	glm::mat4	m_projectionMatrix;

	RakNet::RakPeerInterface* m_pInterface;
	RakNet::SystemAddress m_ServerAddress;

	std::vector<ServerUpdate*> m_players;
	RakNet::uint24_t m_id;
	bool m_loggedIn = false;
	RakNet::Time m_lastUserListTimestamp = 0;
};