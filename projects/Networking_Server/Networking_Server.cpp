#include "Networking_Server.h"
#include "Gizmos.h"
#include "Utilities.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>

#include "Bitstream.h"
#include "UpdateFormat.h"

#define DEFAULT_SCREENWIDTH 1280
#define DEFAULT_SCREENHEIGHT 720

Networking_Server::Networking_Server()
{

}

Networking_Server::~Networking_Server()
{

}

bool Networking_Server::onCreate(int a_argc, char* a_argv[]) 
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

	RakNet::SocketDescriptor sd(SERVER_PORT, 0);
	m_pInterface->Startup(10, &sd, 1, 0);
	m_pInterface->SetMaximumIncomingConnections(10);

	return true;
}

void Networking_Server::onUpdate(float a_deltaTime) 
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

	ProcessMessages();

	// update and draw player positions
	RakNet::Time time = RakNet::GetTime();
	for (auto player : m_players)
	{
		float delta = (float)(time - player->timeStamp) / 1000;
		player->position += player->velocity * delta;
		player->position = glm::clamp(player->position, glm::vec2(-10), glm::vec2(10));
		player->timeStamp = time;
		Gizmos::addSphere(glm::vec3(player->position.x, 0.5, player->position.y), 0.5f, 8, 16,
			glm::vec4(player->color, 1.0f - (float)(time - player->clientTimestamp) / (TIMEOUT_INTERVAL * 1000)));
	}

	// if it's been long enough since the last update, send out the user list
	if ((float)(time - m_lastUserListTimestamp) / 1000 >= UPDATE_INTERVAL)
	{
		UserList list(m_players);
		RakNet::BitStream outputStream;
		list.Encode(outputStream);
		m_pInterface->Send(&outputStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
		printf("Updated user list broadcast.\n");
		m_lastUserListTimestamp = time;
	}

	// quit our application when escape is pressed
	if (glfwGetKey(m_window,GLFW_KEY_ESCAPE) == GLFW_PRESS)
		quit();
}

void Networking_Server::ProcessMessages()
{
	RakNet::Packet* pPacket = nullptr;
	for (pPacket = m_pInterface->Receive(); pPacket != nullptr; m_pInterface->DeallocatePacket(pPacket), pPacket = m_pInterface->Receive())
	{
		RakNet::BitStream inputStream(pPacket->data, pPacket->length, true);
		unsigned char messageType = pPacket->data[0];
		if (ID_TIMESTAMP == messageType)
			messageType = pPacket->data[sizeof(unsigned char)+sizeof(RakNet::Time)];
		switch (messageType)
		{
		case HEADER_CLIENT_LOGIN:
		{
			// create a new player
			RakNet::BitStream outputStream;
			outputStream.Write((unsigned char)HEADER_SERVER_LOGIN_ACCEPTED);
			auto id = newId();
			outputStream.Write(id);
			m_pInterface->Send(&outputStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, pPacket->systemAddress, false);
			printf("Player #%u logged in.\n", id);

			// tell all the other players about the new player
			outputStream.Reset();
			m_players[id]->Encode(outputStream);
			m_pInterface->Send(&outputStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, pPacket->systemAddress, true);
			printf("Player #%u login broadcast.\n", id);
		}
		case HEADER_UPDATE_REQUEST:
		{
			// return information about all the active players
			RakNet::BitStream outputStream;
			UserList list(m_players);
			list.Encode(outputStream);
			m_pInterface->Send(&outputStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, pPacket->systemAddress, false);
			printf("Updated user list sent.\n");
			break;
		}
		case HEADER_CLIENT_LOGOFF:
		{
			// get the id of the player that's logging off
			inputStream.IgnoreBytes(1);
			RakNet::uint24_t id;
			inputStream.Read(id);

			// remove player from list
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

			// pass message on to other players
			m_pInterface->Send(&inputStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, pPacket->systemAddress, true);
			printf("Player #%u logoff broadcast.\n", id);
			break;
		}
		case HEADER_CLIENT_UPDATE:
		{
			ClientUpdate update(inputStream);
			RakNet::BitStream outputStream;

			// if the player is valid, apply and pass on the update
			if (m_players.size() > update.id && nullptr != m_players[update.id] &&
				m_players[update.id]->clientTimestamp < update.timeStamp)
			{
				*m_players[update.id] = update;
				m_players[update.id]->Encode(outputStream);
				m_pInterface->Send(&outputStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, pPacket->systemAddress, true);
				printf("Player #%u update broadcast.\n", update.id);
			}
			// otherwise, tell the client that the player isn't valid
			else
			{
				outputStream.Write((unsigned char)HEADER_CLIENT_LOGOFF);
				outputStream.Write(update.id);
				m_pInterface->Send(&outputStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, pPacket->systemAddress, false);
				printf("Player #%u doesn't exist!\n", update.id);
			}
		}
		} // end of switch statement
	} // end of for loop
}

RakNet::uint24_t Networking_Server::newId()
{
	for (unsigned int i = 0; i < m_players.size(); ++i)
	{
		if (nullptr == m_players[i])
		{
			m_players[i] = new ServerUpdate(i, glm::vec2(0), glm::vec2(0), RandomColor());
			return i;
		}
	}
	m_players.push_back(new ServerUpdate(m_players.size(), glm::vec2(0), glm::vec2(0), RandomColor()));
	return m_players.back()->id;
}

void Networking_Server::onDraw() 
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

void Networking_Server::DestroyData()
{
	for (auto data : m_players)
	{
		delete data;
	}
	m_players.clear();
}

void Networking_Server::onDestroy()
{
	// clean up anything we created
	Gizmos::destroy();

	RakNet::BitStream outputStream;
	outputStream.Write((unsigned char)HEADER_SERVER_DOWN);
	m_pInterface->Send(&outputStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
	printf("Logging off server.\n");
	DestroyData();
	m_pInterface->Shutdown((unsigned int)(TIMEOUT_INTERVAL * 1000));
}

// main that controls the creation/destruction of an application
int main(int argc, char* argv[])
{
	// explicitly control the creation of our application
	Application* app = new Networking_Server();
	
	if (app->create("AIE - Networking_Server",DEFAULT_SCREENWIDTH,DEFAULT_SCREENHEIGHT,argc,argv) == true)
		app->run();
		
	// explicitly control the destruction of our application
	delete app;

	return 0;
}