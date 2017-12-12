#pragma once
#include "Engine/Network/TCPSession.hpp"
#include "Engine/Math/Vector2.hpp"


class NetConnection;

enum eGameMessageTypes
{
	SEND_GAME_JOIN_RESPONSE = 17,
	SEND_GAME_JOIN_REQUEST = 18,
	SEND_GAME_ALERT_TURN = 19,
	SEND_GAME_COMMAND = 20,

	NUM_GAME_MESSAGE_TYPES
};

constexpr uint16_t GAME_PORT = 54321;

struct Player
{
	uint8_t m_connectionIndex;
};


class GameSession
{
public:
	GameSession();
	~GameSession();

	void SetupMessageDefinitions();

	void Update();

	bool Join(NetAddress address);

	void OnJoinRequest(NetMessage* msg);
	void OnJoinResponse(NetMessage* msg);
	void OnTurnAlert(NetMessage* msg);
	void OnCommand(NetMessage* msg);

	void SendJoinRequest();
	void SendJoinResponse(uint8_t connectionIndex);
	void SendTurnAlert(uint8_t connectionIndex, uint8_t characterIndex);
	void SendCommand(uint8_t commandType, uint8_t characterIndex, unsigned int targettedTileIndex, uint8_t targettedCharacterIndex, uint8_t abilityIndex);

	TCPSession m_session;
	std::vector<Player*> m_players;
	uint8_t m_maxNumPlayers;
	uint8_t m_currentNumPlayers;

	uint_fast32_t m_seed;
};
