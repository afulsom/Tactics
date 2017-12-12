#include "Game/GameSession.hpp"
#include "Engine/Core/EngineConfig.hpp"
#include "Engine/Core/ConsoleSystem.hpp"
#include "Engine/Network/NetMessage.hpp"
#include "Engine/Network/NetConnection.hpp"
#include "Engine/Network/NetMessageDefinition.hpp"
#include "Engine/Network/TCPSocket.hpp"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"


bool ConsoleSetSeed(std::string args)
{
	g_theApp->m_game->m_session->m_seed = atoi(args.c_str());
	return true;
}

GameSession::GameSession()
	: m_maxNumPlayers(2)
	, m_currentNumPlayers(0)
{
	m_players.resize(m_maxNumPlayers, nullptr);

	SetupMessageDefinitions();
	g_theConsole->RegisterCommand("set_seed", ConsoleSetSeed);
}

GameSession::~GameSession()
{

}

void GameSession::SetupMessageDefinitions()
{
	m_session.m_messageDefinitions.resize(NUM_GAME_MESSAGE_TYPES);

	//  Send/Receive commands
	//  Send message
	std::function<void(NetMessage*)> onJoinRequestHandler = [=](NetMessage* msg)
	{
		this->OnJoinRequest(msg);
	};
	NetMessageDefinition* requestDef = new NetMessageDefinition();
	requestDef->m_handler = onJoinRequestHandler;
	requestDef->m_messageTypeIndex = SEND_GAME_JOIN_REQUEST;
	m_session.RegisterMessageDefinition(SEND_GAME_JOIN_REQUEST, requestDef);

	std::function<void(NetMessage*)> onJoinResponseHandler = [=](NetMessage* msg)
	{
		this->OnJoinResponse(msg);
	};
	NetMessageDefinition* responseDef = new NetMessageDefinition();
	responseDef->m_handler = onJoinResponseHandler;
	responseDef->m_messageTypeIndex = SEND_GAME_JOIN_RESPONSE;
	m_session.RegisterMessageDefinition(SEND_GAME_JOIN_RESPONSE, responseDef);

	std::function<void(NetMessage*)> onTurnAlertHandler = [=](NetMessage* msg)
	{
		this->OnTurnAlert(msg);
	};
	NetMessageDefinition* turnDef = new NetMessageDefinition();
	turnDef->m_handler = onTurnAlertHandler;
	turnDef->m_messageTypeIndex = SEND_GAME_ALERT_TURN;
	m_session.RegisterMessageDefinition(SEND_GAME_ALERT_TURN, turnDef);

	std::function<void(NetMessage*)> onCommandHandler = [=](NetMessage* msg)
	{
		this->OnCommand(msg);
	};
	NetMessageDefinition* commandDef = new NetMessageDefinition();
	commandDef->m_handler = onCommandHandler;
	commandDef->m_messageTypeIndex = SEND_GAME_COMMAND;
	m_session.RegisterMessageDefinition(SEND_GAME_COMMAND, commandDef);

}

void GameSession::Update()
{
	if (m_session.IsRunning())
	{
		m_session.Update();
	}
}

bool GameSession::Join(NetAddress address)
{
	m_session.Leave();
	bool success = m_session.Join(address);
	if (success)
	{
		((TCPConnection*)m_session.m_hostConnection)->m_socket->SetBlocking(false);
	}
	return success;
}

void GameSession::OnJoinRequest(NetMessage* msg)
{
	m_players[msg->m_sender->m_connectionIndex] = new Player();
	m_players[msg->m_sender->m_connectionIndex]->m_connectionIndex = msg->m_sender->m_connectionIndex;

	m_currentNumPlayers++;

	Game* game = g_theApp->m_game;
	SendJoinResponse(msg->m_sender->m_connectionIndex);
}

void GameSession::OnJoinResponse(NetMessage* msg)
{
	Game* game = g_theApp->m_game;
	msg->Read(m_maxNumPlayers);
	m_players.resize(m_maxNumPlayers, nullptr);
	msg->Read(m_currentNumPlayers);
	msg->Read(m_seed);

	for (uint8_t playerIndex = 0; playerIndex < m_currentNumPlayers; playerIndex++)
	{
		Player* newPlayer = new Player();
		msg->Read(newPlayer->m_connectionIndex);
		m_players[newPlayer->m_connectionIndex] = newPlayer;
	}

	game->Initialize();
}

void GameSession::OnTurnAlert(NetMessage* msg)
{
	uint8_t characterIndex;
	msg->Read(characterIndex);

	g_theApp->m_game->m_currentGameState = STATE_PLAYING;
	g_theApp->m_game->m_currentUIState = STATE_COMMAND_LIST;

	for(Character* character : g_theApp->m_game->m_theMap->m_characters)
	{
		if(character->m_characterIndex == characterIndex)
		{
			g_theApp->m_game->m_theMap->m_selectedCharacter = character;
			g_theApp->m_game->m_theMap->m_selectedTile = character->m_currentTile;
			break;
		}
	}
}

void GameSession::OnCommand(NetMessage* msg)
{
	uint8_t commandType;
	uint8_t characterIndex;
	unsigned int tileIndex;
	uint8_t targettedCharacterIndex;
	uint8_t abilityIndex;

	msg->Read(commandType);
	msg->Read(characterIndex);
	msg->Read(tileIndex);
	msg->Read(targettedCharacterIndex);
	msg->Read(abilityIndex);

	g_theApp->m_game->ProcessCommand(commandType, characterIndex, tileIndex, targettedCharacterIndex, abilityIndex);
}

void GameSession::SendJoinRequest()
{
	NetMessage* msg = new NetMessage(SEND_GAME_JOIN_REQUEST);

	m_session.m_hostConnection->Send(msg);
}

void GameSession::SendJoinResponse(uint8_t connectionIndex)
{
	NetMessage* msg = new NetMessage(SEND_GAME_JOIN_RESPONSE);
	msg->Write(m_maxNumPlayers);
	msg->Write(m_currentNumPlayers);
	msg->Write(m_seed);

	for (Player* player : m_players)
	{
		if (nullptr != player)
		{
			msg->Write(player->m_connectionIndex);
		}
	}


	m_session.GetConnection(connectionIndex)->Send(msg);
}

void GameSession::SendTurnAlert(uint8_t connectionIndex, uint8_t characterIndex)
{
	NetMessage* msg = new NetMessage(SEND_GAME_ALERT_TURN);
	msg->Write(characterIndex);

	m_session.GetConnection(connectionIndex)->Send(msg);
}

void GameSession::SendCommand(uint8_t commandType, uint8_t characterIndex, unsigned int targettedTileIndex, uint8_t targettedCharacterIndex, uint8_t abilityIndex)
{
	NetMessage* msg = new NetMessage(SEND_GAME_COMMAND);
	msg->Write(commandType);
	msg->Write(characterIndex);
	msg->Write(targettedTileIndex);
	msg->Write(targettedCharacterIndex);
	msg->Write(abilityIndex);

	m_session.SendMessageToOthers(*msg);
}
