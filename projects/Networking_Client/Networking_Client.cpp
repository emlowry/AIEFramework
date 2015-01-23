#include "Networking_Client.h"
#include "Gizmos.h"
#include "Utilities.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>
#include <conio.h>

#include "Bitstream.h"
#include "UpdateFormat.h"
#include <set>

#define DEFAULT_SCREENWIDTH 1280
#define DEFAULT_SCREENHEIGHT 720

Networking_Client::Networking_Client()
{

}

Networking_Client::~Networking_Client()
{

}

bool Networking_Client::onCreate(int a_argc, char* a_argv[]) 
{
	// initialise the Gizmos helper class
	Gizmos::create();

	// create a world-space matrix for a camera
	m_cameraMatrix = glm::inverse( glm::lookAt(glm::vec3(10,10,10),glm::vec3(0,0,0), glm::vec3(0,1,0)) );

	// get window dimensions to calculate aspect ratio
	int width = 0, height = 0;
	glfwGetWindowSize(m_window, &width, &height);

	// create a perspective projection matrix with a 90 degree field-of-view and widescreen aspect ratio
	m_projectionMatrix = glm::perspective(glm::pi<float>() * 0.25f, width / (float)height, 0.1f, 1000.0f);

	// set the clear colour and enable depth testing and backface culling
	glClearColor(0.25f,0.25f,0.25f,1);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	m_pInterface = RakNet::RakPeerInterface::GetInstance();

	RakNet::SocketDescriptor sd;
	m_pInterface->Startup(1, &sd, 1, 0);
	return Connect();
}

bool Networking_Client::Connect(float a_timeout)
{
	m_pInterface->Connect("127.0.0.1", SERVER_PORT, 0, 0);

	RakNet::Packet* pPacket = nullptr;

	while (Utility::getTotalTime() < a_timeout)
	{
		pPacket = m_pInterface->Receive();

		if (pPacket != nullptr)
		{
			if (pPacket->data[0] == ID_CONNECTION_REQUEST_ACCEPTED)
			{
				m_ServerAddress = pPacket->systemAddress;
				printf("Connection to server successful!\n");
				return true;
			}
		}
		Utility::tickTimer();
	}

	printf("Connection to server unsuccessful - timeout.\n");
	return false;
}

bool Networking_Client::Login(float a_timeout)
{
	m_loggedIn = false;
	if (RakNet::IS_CONNECTED != m_pInterface->GetConnectionState(m_ServerAddress))
	{
		if (!Connect())
			return false;
	}

	RakNet::BitStream outputStream;
	outputStream.Write((unsigned char)HEADER_CLIENT_LOGIN);
	m_pInterface->Send(&outputStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, m_ServerAddress, false);

	RakNet::Packet* pPacket = nullptr;
	while (Utility::getTotalTime() < a_timeout)
	{
		pPacket = m_pInterface->Receive();
		if (pPacket != nullptr)
		{
			if (pPacket->data[0] == HEADER_SERVER_LOGIN_FAILED ||
				pPacket->data[0] == HEADER_SERVER_LOGIN_ACCEPTED)
			{
				RakNet::BitStream inputStream(pPacket->data, pPacket->length, true);
				inputStream.IgnoreBytes(1);
				if (pPacket->data[0] == HEADER_SERVER_LOGIN_FAILED)
				{
					RakNet::RakString inputString;
					inputStream.Read(inputString);
					printf("Login unsuccessful - ");
					printf(inputString.C_String());
					return false;
				}
				if (pPacket->data[0] == HEADER_SERVER_LOGIN_ACCEPTED)
				{
					RakNet::uint24_t id;
					inputStream.Read(id);
					m_id = id.val;
					m_loggedIn = true;
					printf("Logged in as user #%u!\n", m_id);
					return true;
				}
			}
		}
		Utility::tickTimer();
	}

	printf("Login unsuccessful - timeout.\n");
	return false;
}

void Networking_Client::DestroyData()
{
	for (auto data : m_players)
	{
		delete data;
	}
	m_players.clear();
}

void Networking_Client::onUpdate(float a_deltaTime) 
{
	// update our camera matrix using the keyboard/mouse
	Utility::freeMovement( m_cameraMatrix, a_deltaTime, 10 );

	// clear all gizmos from last frame
	Gizmos::clear();
	
	// add an identity matrix gizmo
	Gizmos::addTransform( glm::mat4(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1) );

	// add a 20x20 grid on the XZ-plane
	for ( int i = 0 ; i < 21 ; ++i )
	{
		Gizmos::addLine( glm::vec3(-10 + i, 0, 10), glm::vec3(-10 + i, 0, -10), 
						 i == 10 ? glm::vec4(1,1,1,1) : glm::vec4(0,0,0,1) );
		
		Gizmos::addLine( glm::vec3(10, 0, -10 + i), glm::vec3(-10, 0, -10 + i), 
						 i == 10 ? glm::vec4(1,1,1,1) : glm::vec4(0,0,0,1) );
	}

	if (!m_loggedIn || RakNet::IS_CONNECTED != m_pInterface->GetConnectionState(m_ServerAddress))
	{
		DestroyData();
		Login();
	}

	bool sendUpdate = false;
	if (m_loggedIn)
	{
		// if ProcessMessages returns true, then the server is requesting an update
		sendUpdate = ProcessMessages();
	}

	if (m_loggedIn)
	{
		if (nullptr == m_players[m_id])
			m_players[m_id] = new ServerUpdate(m_id, glm::vec2(0), glm::vec2(0), glm::vec3(0));

		// if it's been long enough since the last update was sent, send an update
		RakNet::Time time = RakNet::GetTime();
		if (UPDATE_INTERVAL <= (float)(time - m_players[m_id]->clientTimestamp) / 1000)
		{
			sendUpdate = true;
			m_players[m_id]->clientTimestamp = time;
		}

		// update and draw player positions
		for (auto player : m_players)
		{
			float delta = (float)(time - player->timeStamp) / 1000;
			player->position += player->velocity * delta;
			player->position = glm::clamp(player->position, glm::vec2(-10), glm::vec2(10));
			player->timeStamp = time;
			Gizmos::addSphere(glm::vec3(player->position.x, 0, player->position.y), 0.5f, 8, 16,
							  glm::vec4(player->color, (float)(time - player->clientTimestamp) / (TIMEOUT_INTERVAL * 1000)));
		}

		// highlight player character
		Gizmos::addRing(glm::vec3(m_players[m_id]->position.x, 1, m_players[m_id]->position.y),
						0.45f, 0.5f, 16, glm::vec4(1.0f, 0.875f, 0.5f, 1.0f));

		// if player velocity changes, send an update
		glm::vec2 velocity((glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS ? 1 : 0) -
						   (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS ? 1 : 0),
						   (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS ? 1 : 0) -
						   (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS ? 1 : 0));
		if (velocity != m_players[m_id]->velocity)
		{
			sendUpdate = true;
			m_players[m_id]->velocity = velocity;
		}

		// if neccessary, send an update
		if (sendUpdate)
		{
			RakNet::BitStream outputStream;
			ClientUpdate update(*m_players[m_id]);
			update.Encode(outputStream);
			m_pInterface->Send(&outputStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, m_ServerAddress, false);
		}
	}

	// quit our application when escape is pressed
	if (glfwGetKey(m_window,GLFW_KEY_ESCAPE) == GLFW_PRESS)
		quit();
}

bool Networking_Client::ProcessMessages()
{
	bool sendUpdate = false;
	RakNet::Packet* pPacket = nullptr;
	for (pPacket = m_pInterface->Receive(); pPacket != nullptr; m_pInterface->DeallocatePacket(pPacket), pPacket = m_pInterface->Receive())
	{
		RakNet::BitStream inputStream(pPacket->data, pPacket->length, true);
		unsigned char messageType = pPacket->data[0];
		if (messageType = ID_TIMESTAMP)
			messageType = pPacket->data[sizeof(unsigned char)+sizeof(RakNet::Time)];
		switch (messageType)
		{
		case HEADER_UPDATE_REQUEST:
		{
			sendUpdate = true;
			break;
		}
		case HEADER_CLIENT_LOGOFF:
		{
			inputStream.IgnoreBytes(1);
			RakNet::uint24_t id;
			inputStream.Read(id);
			if (m_players.size() > id)
			{
				if (nullptr != m_players[id])
				{
					delete m_players[id];
					m_players[id] = nullptr;
				}
				while (nullptr == m_players.back())
					m_players.pop_back();
			}
			if (id != m_id)
				break;
		}
		case HEADER_SERVER_DOWN:
		{
			m_loggedIn = false;
			printf("Logged off by server!\n");
			break;
		}
		case HEADER_SERVER_UPDATE:
		{
			ServerUpdate* update = new ServerUpdate(inputStream);
			while (m_players.size() <= update->id)
				m_players.push_back(nullptr);
			if (nullptr == m_players[update->id])
				m_players[update->id] = update;
			else
			{
				if (m_players[update->id]->clientTimestamp < update->timeStamp &&
					m_players[update->id]->clientTimestamp <= update->clientTimestamp)
					*m_players[update->id] = *update;
				delete update;
			}
			break;
		}
		case HEADER_SERVER_USER_LIST:
		{
			UserList list(inputStream);
			if (list.timeStamp > m_lastUserListTimestamp)
			{
				m_lastUserListTimestamp = list.timeStamp;
				std::set<RakNet::uint24_t> ids;
				for (auto player : list.players)
				{
					ids.insert(player.id);
					while (m_players.size() <= player.id)
						m_players.push_back(nullptr);
					if (nullptr == m_players[player.id])
						m_players[player.id] = new ServerUpdate(player);
					else if (m_players[player.id]->clientTimestamp < player.timeStamp &&
							 m_players[player.id]->clientTimestamp <= player.clientTimestamp)
						*m_players[player.id] = player;
				}
				for (unsigned int i = 0; i < m_players.size(); ++i)
				{
					if (0 == ids.count(i) && nullptr != m_players[i])
					{
						delete m_players[i];
						m_players[i] = nullptr;
					}
				}
				while (nullptr == m_players.back())
					m_players.pop_back();
			}
		}
		}// end of switch statement
	}// end of for loop
	return sendUpdate;
}

void Networking_Client::onDraw() 
{
	// clear the backbuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// get the view matrix from the world-space camera matrix
	glm::mat4 viewMatrix = glm::inverse( m_cameraMatrix );
	
	// draw the gizmos from this frame
	Gizmos::draw(m_projectionMatrix, viewMatrix);

	// get window dimensions for 2D orthographic projection
	int width = 0, height = 0;
	glfwGetWindowSize(m_window, &width, &height);
	Gizmos::draw2D(glm::ortho<float>(0.0f, (float)width, 0.0f, (float)height, -1.0f, 1.0f));
}

void Networking_Client::onDestroy()
{
	// clean up anything we created
	Gizmos::destroy();

	RakNet::BitStream outputStream;
	outputStream.Write((unsigned char)HEADER_CLIENT_LOGOFF);
	outputStream.Write(m_id);
	m_pInterface->Send(&outputStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, m_ServerAddress, false);
	printf("Logging off server.\n");
	DestroyData();
}

// main that controls the creation/destruction of an application
int main(int argc, char* argv[])
{
	// explicitly control the creation of our application
	Application* app = new Networking_Client();
	
	if (app->create("AIE - Networking_Client",DEFAULT_SCREENWIDTH,DEFAULT_SCREENHEIGHT,argc,argv) == true)
		app->run();
		
	// explicitly control the destruction of our application
	delete app;

	return 0;
}