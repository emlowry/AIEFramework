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
	Login();
	return true;
}

bool Networking_Client::Connect(float a_timeout)
{
	m_pInterface->Connect("127.0.0.1", SERVER_PORT, 0, 0);

	RakNet::Packet* pPacket = nullptr;

	float time = Utility::getTotalTime();
	while (Utility::getTotalTime() - time < a_timeout)
	{
		printf("\rConnecting - %f.3 seconds until timeout...", a_timeout - (Utility::getTotalTime() - time));
		pPacket = m_pInterface->Receive();

		if (pPacket != nullptr)
		{
			if (pPacket->data[0] == ID_CONNECTION_REQUEST_ACCEPTED)
			{
				m_ServerAddress = pPacket->systemAddress;
				m_pInterface->SetTimeoutTime((RakNet::TimeMS)(TIMEOUT_INTERVAL * 1000), m_ServerAddress);
				printf("\rConnection to server successful!                                               \n");
				return true;
			}
		}
		Utility::tickTimer();
	}

	printf("\rConnection to server unsuccessful - timeout.                                   \n");
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
	float time = Utility::getTotalTime();
	while (Utility::getTotalTime() - time < a_timeout)
	{
		printf("\rLogging in - %f.3 seconds until timeout...", a_timeout - (Utility::getTotalTime() - time));
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
					printf("\rLogin unsuccessful - ");
					printf(inputString.C_String());
					for (unsigned int i = 0; i < 58 - inputString.GetLength(); ++i)
						printf(" ");
					printf("\n");
					return false;
				}
				if (pPacket->data[0] == HEADER_SERVER_LOGIN_ACCEPTED)
				{
					RakNet::uint24_t id;
					inputStream.Read(id);
					m_id = id.val;
					m_loggedIn = true;
					printf("\rLogged in as user #%u!                                                          \n", m_id);
					return true;
				}
			}
		}
		Utility::tickTimer();
	}

	printf("\rLogin unsuccessful - timeout.                                                  \n");
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

	// quit our application when escape is pressed
	if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		quit();

	if (m_loggedIn && RakNet::IS_CONNECTED != m_pInterface->GetConnectionState(m_ServerAddress))
	{
		m_loggedIn = false;
		DestroyData();
		return;
	}

	if (!m_loggedIn)
	{
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
		}

		// update and draw player positions
		for (auto player : m_players)
		{
			float delta = (float)(time - player->timeStamp) / 1000;
			player->position += player->velocity * delta;
			player->position = glm::clamp(player->position, glm::vec2(-10), glm::vec2(10));
			player->timeStamp = time;
			Gizmos::addSphere(glm::vec3(player->position.x, 0.5, player->position.y), 0.5f, 8, 16,
							  glm::vec4(player->color, 1.0f - (float)(time - player->clientTimestamp) / (TIMEOUT_INTERVAL * 1000)));
		}

		// highlight player character
		Gizmos::addRing(glm::vec3(m_players[m_id]->position.x, 1, m_players[m_id]->position.y),
						0.45f, 0.5f, 16, glm::vec4(1.0f, 0.875f, 0.5f, 1.0f));

		// AI-controlled players wander
		glm::vec2 velocity;
		if (m_AI)
		{
			m_directionOfAI += RandomFloat(16.0, -16.0) * a_deltaTime;
			while (0.0f > m_directionOfAI)
			{
				m_directionOfAI += 8.0f;
			}
			while (8.0f <= m_directionOfAI)
			{
				m_directionOfAI -= 8.0f;
			}
			m_speedOfAI += RandomFloat(3.0, -3.0) * a_deltaTime;
			while (0.0f > m_speedOfAI)
			{
				m_speedOfAI += 3.0f;
			}
			while (3.0f <= m_speedOfAI)
			{
				m_speedOfAI -= 3.0f;
			}
			if (1.0f > m_speedOfAI)
			{
				switch ((int)m_directionOfAI)
				{
				case 0: velocity = glm::vec2(1, 0); break;
				case 1: velocity = glm::vec2(1, 1); break;
				case 2: velocity = glm::vec2(0, 1); break;
				case 3: velocity = glm::vec2(-1, 1); break;
				case 4: velocity = glm::vec2(-1, 0); break;
				case 5: velocity = glm::vec2(-1, -1); break;
				case 6: velocity = glm::vec2(0, -1); break;
				case 7: velocity = glm::vec2(1, -1); break;
				default: velocity = m_players[m_id]->velocity; break;
				}
			}

			velocity -= m_players[m_id]->position / 20.0f;
			velocity.x = (0.5f < velocity.x ? 1.0f : -0.5f > velocity.x ? -1.0f : 0.0f);
			velocity.y = (0.5f < velocity.y ? 1.0f : -0.5f > velocity.y ? -1.0f : 0.0f);

			m_directionOfAI -= (int)m_directionOfAI;
			m_directionOfAI += (glm::vec2(1, 0) == velocity ? 0 :
								glm::vec2(1, 1) == velocity ? 1 :
								glm::vec2(0, 1) == velocity ? 2 :
								glm::vec2(-1, 1) == velocity ? 3 :
								glm::vec2(-1, 0) == velocity ? 4 :
								glm::vec2(-1, -1) == velocity ? 5 :
								glm::vec2(0, -1) == velocity ? 6 :
								glm::vec2(1, -1) == velocity ? 7 : 0);
		}
		// human-controlled players respond to keys
		else
		{
			velocity.x = (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS ? 1.0f : 0.0f) -
						 (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS ? 1.0f : 0.0f);
			velocity.y = (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS ? 1.0f : 0.0f) -
						 (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS ? 1.0f : 0.0f);
		}

		// if velocity has changed, send update
		if (velocity != m_players[m_id]->velocity)
		{
			sendUpdate = true;
			m_players[m_id]->velocity = velocity;
		}

		// if neccessary, send an update
		if (sendUpdate)
		{
			m_players[m_id]->clientTimestamp = time;
			RakNet::BitStream outputStream;
			ClientUpdate update(*m_players[m_id]);
			update.Encode(outputStream);
			m_pInterface->Send(&outputStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, m_ServerAddress, false);
			printf("Sent update to server.\n");
		}
	}
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
			printf("Received update request.\n");
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
				while (!m_players.empty() && nullptr == m_players.back())
					m_players.pop_back();
			}
			if (id == m_id)
			{
				m_loggedIn = false;
			}
			printf("User #%u logged off.\n", id);
			break;
		}
		case HEADER_SERVER_DOWN:
		{
			m_loggedIn = false;
			printf("Server shut down!\n");
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
			printf("Update to player #%u recieved.\n", update->id);
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
					if (nullptr != m_players[i] && 0 == ids.count(i))
					{
						delete m_players[i];
						m_players[i] = nullptr;
					}
				}
				while (!m_players.empty() && nullptr == m_players.back())
					m_players.pop_back();
				printf("Updated user list recieved.\n");
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
	m_pInterface->Shutdown((unsigned int)(TIMEOUT_INTERVAL * 1000));
}

// main that controls the creation/destruction of an application
int main(int argc, char* argv[])
{
	// explicitly control the creation of our application
	Application* app = new Networking_Client();

	for (int i = 0; i < argc; ++i)
	{
		if (0 == strcmp(argv[i], "ai") || 0 == strcmp(argv[i], "AI") ||
			0 == strcmp(argv[i], "aI") || 0 == strcmp(argv[i], "Ai") ||
			0 == strcmp(argv[i], "-ai") || 0 == strcmp(argv[i], "-AI") ||
			0 == strcmp(argv[i], "-aI") || 0 == strcmp(argv[i], "-Ai") ||
			0 == strcmp(argv[i], "--ai") || 0 == strcmp(argv[i], "--AI") ||
			0 == strcmp(argv[i], "--aI") || 0 == strcmp(argv[i], "--Ai"))
		{
			((Networking_Client*)app)->UseAI(true);
		}
	}
	
	if (app->create("AIE - Networking_Client",DEFAULT_SCREENWIDTH,DEFAULT_SCREENHEIGHT,argc,argv) == true)
		app->run();
		
	// explicitly control the destruction of our application
	delete app;

	return 0;
}