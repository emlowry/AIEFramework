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

	void UseAI(bool a_use = true) { m_AI = a_use; }

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

	bool m_AI = false;
	float m_directionOfAI = 0;	// 0-1=(1,0), 1-2=(1,1), 2-3=(0,1), 3-4=(-1,1), 4-5=(-1,0), 5-6=(-1,-1), 6-7=(0,-1), 7-8=(1,-1)
	float m_speedOfAI = 0;	// 0-1=0, 1-3=1
};