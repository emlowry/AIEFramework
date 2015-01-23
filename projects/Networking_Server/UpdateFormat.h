#pragma once

#include "Bitstream.h"
#include "GetTime.h"
#include "MessageIdentifiers.h"
#include <glm/glm.hpp>
#include <vector>

#define SERVER_PORT 12001

#define UPDATE_INTERVAL 2.0
#define TIMEOUT_INTERVAL 3.0

enum MESSAGE_HEADER
{
	HEADER_CLIENT_LOGIN = ID_USER_PACKET_ENUM + 1,
	HEADER_SERVER_LOGIN_FAILED,
	HEADER_SERVER_LOGIN_ACCEPTED,
	HEADER_CLIENT_LOGOFF,
	HEADER_SERVER_DOWN,
	HEADER_CLIENT_UPDATE,
	HEADER_SERVER_UPDATE,
	HEADER_UPDATE_REQUEST,
	HEADER_SERVER_USER_LIST
};

#pragma pack(push, 1)
struct ClientUpdate
{
	unsigned char useTimeStamp = ID_TIMESTAMP;
	RakNet::Time timeStamp;
	unsigned char typeId = HEADER_CLIENT_UPDATE;
	RakNet::uint24_t id;
	glm::vec2 position;
	glm::vec2 velocity;

	ClientUpdate() : timeStamp(RakNet::GetTime()) {}
	ClientUpdate(unsigned int a_id, const glm::vec2& a_position,
				 const glm::vec2& a_velocity)
				 : timeStamp(RakNet::GetTime()), id(a_id),
				   position(a_position), velocity(a_velocity) {}
	ClientUpdate(RakNet::BitStream& a_input) { Decode(a_input); }
	
	virtual void Encode(RakNet::BitStream& a_output)
	{
		a_output.Write(useTimeStamp);
		a_output.Write(timeStamp);
		a_output.Write(typeId);
		a_output.Write(id);
		if (glm::vec2(0, 0) == position)
		{
			a_output.Write(false);
		}
		else
		{
			a_output.Write(true);
			a_output.Write(position.x);
			a_output.Write(position.y);
		}
		if (glm::vec2(0, 0) == velocity)
		{
			a_output.Write(false);
		}
		else
		{
			a_output.Write(true);
			a_output.Write(velocity.x);
			a_output.Write(velocity.y);
		}
	}
	virtual void Decode(RakNet::BitStream& a_input)
	{
		unsigned char type;
		a_input.Read(type);
		if (ID_TIMESTAMP == type)
		{
			a_input.Read(timeStamp);
			a_input.Read(typeId);
		}
		a_input.Read(id);
		bool nonZero = false;
		a_input.Read(nonZero);
		if (nonZero)
		{
			a_input.Read(position.x);
			a_input.Read(position.y);
		}
		else
		{
			position.x = 0;
			position.y = 0;
		}
		a_input.Read(nonZero);
		if (nonZero)
		{
			a_input.Read(velocity.x);
			a_input.Read(velocity.y);
		}
		else
		{
			velocity.x = 0;
			velocity.y = 0;
		}
	}
};
#pragma pack(pop)

float RandomFloat(float max = 1.0f, float min = 0.0f)
{
	return ((max - min) * (float)rand() / (float)RAND_MAX) + min;
}

glm::vec3 RandomColor(bool allowWhite = false, bool allowBlack = false)
{
	glm::vec3 color(0);
	do
	{
		color = glm::vec3(RandomFloat(), RandomFloat(), RandomFloat());
		if (color != glm::vec3(0))
			color /= fmax(color.x, fmax(color.y, color.z));
	} while ((!allowBlack && glm::vec3(0) == color) ||
			 (!allowWhite && glm::vec3(1) == color));
	return color;
}

#pragma pack(push, 1)
struct ServerUpdate : public ClientUpdate
{
	glm::vec3 color;
	RakNet::Time clientTimestamp;	// time of last update from associated client, if position had to be extrapolated

	ServerUpdate() { typeId = HEADER_SERVER_UPDATE; }
	ServerUpdate(unsigned int a_id, const glm::vec2& a_position,
				 const glm::vec2& a_velocity, const glm::vec3& a_color)
		: ClientUpdate(a_id, a_position, a_velocity), color(a_color)
	{
		typeId = HEADER_SERVER_UPDATE;
		clientTimestamp = timeStamp;
	}
	ServerUpdate(unsigned int a_id, const glm::vec2& a_position,
				 const glm::vec2& a_velocity, const glm::vec3& a_color,
				 RakNet::Time a_clientTimestamp)
		: ClientUpdate(a_id, a_position, a_velocity),
		  color(a_color), clientTimestamp(a_clientTimestamp)
	{
		typeId = HEADER_SERVER_UPDATE;
	}
	ServerUpdate(const ServerUpdate& a_update)
		: ClientUpdate(a_update), color(a_update.color),
		  clientTimestamp(a_update.clientTimestamp)
	{
		typeId = HEADER_SERVER_UPDATE;
	}
	ServerUpdate(const ClientUpdate& a_update) : ClientUpdate(a_update)
	{
		typeId = HEADER_SERVER_UPDATE;
		clientTimestamp = timeStamp;
	}
	ServerUpdate(const ClientUpdate& a_update, const glm::vec3& a_color)
		: ClientUpdate(a_update), color(a_color)
	{
		typeId = HEADER_SERVER_UPDATE;
		clientTimestamp = timeStamp;
	}
	ServerUpdate(RakNet::BitStream& a_input) { Decode(a_input); }

	ServerUpdate& operator=(const ServerUpdate& a_update)
	{
		timeStamp = a_update.timeStamp;
		position = a_update.position;
		velocity = a_update.velocity;
		color = a_update.color;
		clientTimestamp = a_update.clientTimestamp;
		return *this;
	}
	ServerUpdate& operator=(const ClientUpdate& a_update)
	{
		timeStamp = a_update.timeStamp;
		position = a_update.position;
		velocity = a_update.velocity;
		clientTimestamp = a_update.timeStamp;
		return *this;
	}

	virtual void Encode(RakNet::BitStream& a_output)
	{
		ClientUpdate::Encode(a_output);
		if (glm::vec3(0, 0, 0) == color)
		{
			a_output.Write(false);
		}
		else
		{
			a_output.Write(true);
			a_output.Write(color.x);
			a_output.Write(color.y);
			a_output.Write(color.z);
		}
		a_output.Write(clientTimestamp);
	}
	virtual void Decode(RakNet::BitStream& a_input)
	{
		ClientUpdate::Decode(a_input);
		bool nonZero = false;
		a_input.Read(nonZero);
		if (nonZero)
		{
			a_input.Read(color.x);
			a_input.Read(color.y);
			a_input.Read(color.z);
		}
		else
		{
			color.x = 0;
			color.y = 0;
			color.z = 0;
		}
		a_input.Read(clientTimestamp);
	}
};
#pragma pack(pop)

#pragma pack(push, 1)
struct UserList
{
	unsigned char useTimeStamp = ID_TIMESTAMP;
	RakNet::Time timeStamp;
	unsigned char typeId = HEADER_SERVER_USER_LIST;
	std::vector<ServerUpdate> players;

	UserList() : timeStamp(RakNet::GetTime()) {}
	UserList(const std::vector<ServerUpdate*>& a_players)
		: timeStamp(RakNet::GetTime())
	{
		for (auto player : a_players)
		{
			if (nullptr != player)
				players.push_back(*player);
		}
	}
	UserList(RakNet::BitStream& a_input) { Decode(a_input); }

	void Encode(RakNet::BitStream& a_output)
	{
		a_output.Write(useTimeStamp);
		a_output.Write(timeStamp);
		a_output.Write(typeId);
		RakNet::uint24_t size = players.size();
		a_output.Write(size);
		for (auto player : players)
			player.Encode(a_output);
	}

	void Decode(RakNet::BitStream& a_input)
	{
		unsigned char type;
		a_input.Read(type);
		if (ID_TIMESTAMP == type)
		{
			a_input.Read(timeStamp);
			a_input.Read(typeId);
		}
		RakNet::uint24_t size = 0;
		a_input.Read(size);
		for (unsigned int i = 0; i < size; ++i)
		{
			ServerUpdate update;
			update.Decode(a_input);
			players.push_back(update);
		}
	}
};
#pragma pack(pop)
