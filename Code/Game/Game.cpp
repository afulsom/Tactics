#include "Game/Game.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Math/Disc2D.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/Rgba.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Audio/Audio.hpp"
#include "Engine/Core/EngineConfig.hpp"
#include "Engine/Core/ConfigSystem.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include "Game/App.hpp"
#include "Game/MapDefinition.hpp"
#include "Game/CharacterBuilder.hpp"
#include "Game/AbilityDefinition.hpp"
#include "Engine/Core/ParticleEffectBuilder.hpp"
#include "Game/GameSession.hpp"
#include "Engine/Network/NetSession.hpp"
#include "Engine/Network/NetConnection.hpp"
#include "Engine/Network/NetMessage.hpp"
#include "Engine/Core/ConsoleSystem.hpp"
#include "Engine/Core/EngineConfig.hpp"

bool ConsolePrintCT(std::string args)
{
	for (Character* character : g_theApp->m_game->m_theMap->m_characters)
	{
		g_theConsole->ConsolePrintf("%s", std::to_string(character->m_currentCT).c_str());
	}
	return true;
}

bool ConsoleSetJoinAddress(std::string args)
{
	if (g_theApp->m_game->m_currentGameState == STATE_JOINING)
	{
		g_theApp->m_game->m_joinAddress = args;
		return true;
	}

	return false;
}

Game::Game()
	: m_isGamePaused(false)
	, m_theMap(nullptr)
	, m_northEastCamera(Vector3(-2.5f, 2.5f, -2.5f), 45.1f, 25.f, 0.f)
	, m_northWestCamera(Vector3(22.5f, 2.5f, -2.5f), -45.1f, 25.f, 0.f)
	, m_southEastCamera(Vector3(-2.5f, 2.5f, 22.5f), 135.1f, 25.f, 0.f)
	, m_southWestCamera(Vector3(22.5f, 2.5f, 22.5f), -135.1f, 25.f, 0.f)
	, m_currentMenuSelection(0)
	, m_numWaits(0)
	, m_commandQueue()
	, m_commandHistory()
{
	m_theCamera.m_yaw = m_northWestCamera.yaw;
	m_theCamera.m_pitch = m_northWestCamera.pitch;
	m_theCamera.m_roll = m_northWestCamera.roll;

	m_orthoShader = g_theRenderer->CreateShaderFromHLSLFile("Data/HLSL/ortho_textured.hlsl");
	m_litShader = g_theRenderer->CreateShaderFromHLSLFile("Data/HLSL/lit.hlsl");
	m_spriteShader = g_theRenderer->CreateShaderFromHLSLFile("Data/HLSL/sprite.hlsl");
	m_session = new GameSession();

	m_joinAddress = NetAddressToString(GetMyAddress(GAME_PORT));

	g_theConsole->RegisterCommand("ct", ConsolePrintCT);
	g_theConsole->RegisterCommand("set_join_address", ConsoleSetJoinAddress);
}


Game::~Game()
{
	UnloadParticleEffects();

	delete m_orthoShader;
	delete m_litShader;

	delete m_theMap;
}


void Game::Initialize()
{
	m_menuCancelSound = g_theAudio->CreateOrGetSound("Data/Audio/Cancel.mp3");
	m_menuConfirmSound = g_theAudio->CreateOrGetSound("Data/Audio/Confirm.mp3");
	m_menuMoveSound = g_theAudio->CreateOrGetSound("Data/Audio/Move.mp3");

	LoadGameConstants();
	LoadTileDefinitions();
	LoadAbilityDefinitions();
	LoadCharacterBuilders();
	LoadItemDefinitions();
	LoadMapDefinitions();
	LoadParticleEffects();

	g_theAudio->PlaySoundAtVolume(g_theAudio->CreateOrGetSound("Data/Audio/Theme.mp3"));
}

void Game::Update(float deltaSeconds)
{
	if(m_session)
	{
		m_session->Update();
	}
	else
	{
		ERROR_AND_DIE("No running session.");
	}

	if(!m_isPlayingReplay)
	{
		TCPConnection* hostConnection = (TCPConnection*)m_session->m_session.m_hostConnection;
		if ((m_currentGameState == STATE_PLAYING || m_currentGameState == STATE_WAITING) && (!m_session->m_session.IsRunning() || hostConnection == nullptr || hostConnection->IsDisconnected()))
		{
			m_session->m_session.Leave();
			m_currentGameState = STATE_MAINMENU;
		}
	}

	switch (m_currentGameState)
	{
	case STATE_MAINMENU:
		UpdateMainMenu(deltaSeconds);
		break;
	case STATE_HOSTING:
		UpdateHosting(deltaSeconds);
		break;
	case STATE_JOINING:
		UpdateJoining(deltaSeconds);
		break;
	case STATE_PLAYING:
		UpdatePlaying(deltaSeconds);
		break;
	case STATE_WAITING:
		UpdateWaiting(deltaSeconds);
		break;
	case STATE_END_SCREEN:
		UpdateEndScreen(deltaSeconds);
	}
}

void Game::Render() const
{
	switch (m_currentGameState)
	{
	case STATE_MAINMENU:
		RenderMainMenu();
		break;
	case STATE_HOSTING:
		RenderHosting();
		break;
	case STATE_JOINING:
		RenderJoining();
		break;
	case STATE_PLAYING:
		RenderPlaying();
		break;
	case STATE_WAITING:
		RenderWaiting();
		break;
	case STATE_END_SCREEN:
		RenderEndScreen();
		break;
	}
}

void Game::WaitUntilRelease()
{
	m_currentGameState = STATE_WAITING;
	m_numWaits++;
}

void Game::ReleaseWait()
{
	m_numWaits--;
	if (m_numWaits <= 0)
	{
		m_numWaits = 0;
		m_currentGameState = STATE_PLAYING;
	}
}

void Game::UpdateMainMenu(float deltaSeconds)
{
	if (g_theInput->WasKeyJustPressed('1'))
	{
		g_theAudio->PlaySoundAtVolume(m_menuConfirmSound);

		m_session->m_session.Host(GAME_PORT);
		m_session->m_session.StartListening();
		m_session->m_players[0] = new Player();
		m_session->m_players[0]->m_connectionIndex = 0;
		m_session->m_currentNumPlayers++;

		m_session->m_seed = rand();

		m_currentGameState = STATE_HOSTING;

	}
	else if (g_theInput->WasKeyJustPressed('2'))
	{
		g_theAudio->PlaySoundAtVolume(m_menuConfirmSound);
		m_currentGameState = STATE_JOINING;
		m_joinState = JOIN_STATE_SETUP;
	}
	else if (g_theInput->WasKeyJustPressed('3'))
	{
		g_theAudio->PlaySoundAtVolume(m_menuConfirmSound);
		ReadReplayFromFile();
		m_currentGameState = STATE_PLAYING;
		m_isPlayingReplay = true;
		UpdateWaiting(deltaSeconds);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESCAPE))
		g_theApp->SetIsQuitting(true);
}

void Game::UpdateHosting(float deltaSeconds)
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESCAPE))
	{
		m_session->m_session.Leave();
		m_currentGameState = STATE_MAINMENU;
	}

	if(m_session->m_currentNumPlayers > 1)
	{
		g_random.Seed(m_session->m_seed);
		m_theMap = new Map("test");
		m_currentGameState = STATE_PLAYING;
		UpdateWaiting(deltaSeconds);
	}
}

void Game::UpdateJoining(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	switch (m_joinState)
	{
	case JOIN_STATE_NOT_JOINING:
		m_currentGameState = STATE_MAINMENU;
		break;
	case JOIN_STATE_GET_INFO:
	{
		uint8_t connectionIndex = m_session->m_session.m_myConnection->m_connectionIndex;
		if (m_session->m_players.size() > connectionIndex && m_session->m_players[connectionIndex] != nullptr)
		{
			g_random.Seed(m_session->m_seed);
			m_joinState = JOIN_STATE_NOT_JOINING;
			m_theMap = new Map("test");
			m_currentGameState = STATE_PLAYING;
			UpdateWaiting(deltaSeconds);
		}
		else if (!m_session->m_session.IsReady())
		{
			m_joinState = JOIN_STATE_SETUP;
		}
	}
	break;
	case JOIN_STATE_CONNECTING:
		if (m_session->m_session.IsReady())
		{
			m_joinState = JOIN_STATE_GET_INFO;
			m_session->SendJoinRequest();
		}
		break;
	case JOIN_STATE_SETUP:
	{
		if (g_theInput->WasKeyJustPressed(KEYCODE_ENTER))
		{
			if (m_session->Join(StringToNetAddress(m_joinAddress)))
			{
				m_joinState = JOIN_STATE_CONNECTING;
			}
			else
			{
				g_theConsole->ConsolePrintf("Join failed.");
			}
		}
	}
	break;
	default:
		break;
	}
}

void Game::UpdatePlaying(float deltaSeconds)
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESCAPE))
	{
		g_theAudio->PlaySoundAtVolume(m_menuCancelSound);
		m_currentGameState = STATE_MAINMENU;
		m_session->m_session.Leave();
	}

	if (CheckForVictoryOrDefeat())
	{
		return;
	}

	if (!m_commandQueue.empty())
	{
		if (m_isPlayingReplay)
		{
			if (m_commandQueue.front().m_actingCharacter->m_currentCT >= 100)
			{
				m_commandQueue.front().RunCommand(this);
				m_commandHistory.push(m_commandQueue.front());
				m_commandQueue.pop();
			}
		}
		else
		{
			m_commandQueue.front().RunCommand(this);
			m_commandHistory.push(m_commandQueue.front());
			m_commandQueue.pop();
		}
// 		m_theMap->m_isWaitingForInput = false;
	}

	if (nullptr != m_theMap->m_selectedCharacter)
	{
		switch (m_currentUIState)
		{
		case STATE_MAP_SELECTION_MOVEMENT:
			UpdateMapSelectionMovement(deltaSeconds);
			break;
		case STATE_MAP_SELECTION_TARGETING:
			UpdateMapSelectionTargetting(deltaSeconds);
			break;
		case STATE_COMMAND_LIST:
			UpdateCommandList(deltaSeconds);
			break;
		case STATE_ACTION_LIST:
			UpdateActionList(deltaSeconds);
			break;
		case STATE_ABILITIES_LIST:
			UpdateAbilitiesList(deltaSeconds);
			break;
		default:
			break;
		}
	}
	else if (nullptr != m_theMap->m_activeCharacter)
	{
		m_theCamera.m_position = m_theMap->m_activeCharacter->m_currentPosition;
		m_theCamera.m_position -= (m_theCamera.GetForwardXYZ() * 10.f);
	}

	m_theMap->Update(deltaSeconds);
}

void Game::UpdateWaiting(float deltaSeconds)
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESCAPE))
	{
		g_theAudio->PlaySoundAtVolume(m_menuCancelSound);
		m_currentGameState = STATE_MAINMENU;
	}

	if(m_theMap->m_selectedCharacter)
	{
		m_theCamera.m_position = m_theMap->m_selectedCharacter->m_currentPosition;
		m_theCamera.m_position -= (m_theCamera.GetForwardXYZ() * 10.f);
	}
	else if (nullptr != m_theMap->m_activeCharacter)
	{
		m_theCamera.m_position = m_theMap->m_activeCharacter->m_currentPosition;
		m_theCamera.m_position -= (m_theCamera.GetForwardXYZ() * 10.f);
	}

	g_theRenderer->SetEyePosition(Vector4(m_theCamera.m_position.x, m_theCamera.m_position.y, m_theCamera.m_position.z, 0.f));

	m_theMap->Update(deltaSeconds);

}

void Game::UpdateMapSelectionMovement(float deltaSeconds)
{
	HandleInputMapSelectionMovement(deltaSeconds);

	if (m_theMap->m_selectedTile != nullptr)
	{
		m_theCamera.m_position = Vector3(m_theMap->m_selectedTile->m_tileCoords.x + 0.5f, m_theMap->m_selectedTile->GetDisplayHeight(), m_theMap->m_selectedTile->m_tileCoords.y + 0.5f);
		m_theCamera.m_position -= (m_theCamera.GetForwardXYZ() * 10.f);
	}

	g_theRenderer->SetEyePosition(Vector4(m_theCamera.m_position.x, m_theCamera.m_position.y, m_theCamera.m_position.z, 0.f));
}

void Game::UpdateMapSelectionTargetting(float deltaSeconds)
{
	HandleInputMapSelectionTargetting(deltaSeconds);

	if (m_theMap->m_selectedTile != nullptr)
	{
		m_theCamera.m_position = Vector3(m_theMap->m_selectedTile->m_tileCoords.x + 0.5f, m_theMap->m_selectedTile->GetDisplayHeight(), m_theMap->m_selectedTile->m_tileCoords.y + 0.5f);
		m_theCamera.m_position -= (m_theCamera.GetForwardXYZ() * 10.f);
	}

	g_theRenderer->SetEyePosition(Vector4(m_theCamera.m_position.x, m_theCamera.m_position.y, m_theCamera.m_position.z, 0.f));
}

void Game::RenderMainMenu() const
{
	g_theRenderer->SetTexture(nullptr);
	g_theRenderer->SetShader(m_orthoShader);
	g_theRenderer->ClearColor(Rgba::BLACK);

	float orthoWidth = ORTHO_X_DIMENSION;
	float orthoHeight = ORTHO_Y_DIMENSION;

	g_theConfig->GetConfigFloat(orthoWidth, "ORTHO_WIDTH");
	g_theConfig->GetConfigFloat(orthoHeight, "ORTHO_HEIGHT");

	g_theRenderer->SetProjectionMatrix(g_theRenderer->CreateOrthoProjectionMatrix(Vector2(ORTHO_X_OFFSET, ORTHO_Y_OFFSET), Vector2(orthoWidth + ORTHO_X_OFFSET, orthoHeight + ORTHO_Y_OFFSET)));
	g_theRenderer->SetViewMatrix(Matrix4::CreateTranslation(Vector3::ZERO));

	g_theRenderer->DrawCenteredText2D(Vector2(ORTHO_X_DIMENSION * 0.5f, ORTHO_Y_DIMENSION - 3.f), g_theRenderer->m_defaultFont, "Tactics", Rgba::WHITE, 3.f);
	g_theRenderer->DrawCenteredText2D(Vector2(ORTHO_X_DIMENSION * 0.5f, 3.f), g_theRenderer->m_defaultFont, "1. Host", Rgba::WHITE, 1.f);
	g_theRenderer->DrawCenteredText2D(Vector2(ORTHO_X_DIMENSION * 0.5f, 2.f), g_theRenderer->m_defaultFont, "2. Join.", Rgba::WHITE, 1.f);
	g_theRenderer->DrawCenteredText2D(Vector2(ORTHO_X_DIMENSION * 0.5f, 1.f), g_theRenderer->m_defaultFont, "3. Play Replay.", Rgba::WHITE, 1.f);
}

void Game::RenderHosting() const
{
	g_theRenderer->SetTexture(nullptr);
	g_theRenderer->SetShader(m_orthoShader);
	g_theRenderer->ClearColor(Rgba::BLACK);

	float orthoWidth = ORTHO_X_DIMENSION;
	float orthoHeight = ORTHO_Y_DIMENSION;

	g_theConfig->GetConfigFloat(orthoWidth, "ORTHO_WIDTH");
	g_theConfig->GetConfigFloat(orthoHeight, "ORTHO_HEIGHT");

	g_theRenderer->SetProjectionMatrix(g_theRenderer->CreateOrthoProjectionMatrix(Vector2(ORTHO_X_OFFSET, ORTHO_Y_OFFSET), Vector2(orthoWidth + ORTHO_X_OFFSET, orthoHeight + ORTHO_Y_OFFSET)));
	g_theRenderer->SetViewMatrix(Matrix4::CreateTranslation(Vector3::ZERO));

	g_theRenderer->DrawCenteredText2D(Vector2(ORTHO_X_DIMENSION * 0.5f, ORTHO_Y_DIMENSION * 0.5f), g_theRenderer->m_defaultFont, "Waiting for client.", Rgba::WHITE, 1.f);
	g_theRenderer->DrawCenteredText2D(Vector2(ORTHO_X_DIMENSION * 0.5f, 3.f), g_theRenderer->m_defaultFont, "Hosting on: " + NetAddressToString(m_session->m_session.m_hostConnection->m_address), Rgba::WHITE, 1.f);
}

void Game::RenderJoining() const
{
	g_theRenderer->SetTexture(nullptr);
	g_theRenderer->SetShader(m_orthoShader);
	g_theRenderer->ClearColor(Rgba::BLACK);

	g_theRenderer->SetProjectionMatrix(g_theRenderer->CreateOrthoProjectionMatrix(Vector2(-16.f, -9.f), Vector2(16.f, 9.f)));

	switch (m_joinState)
	{
	case JOIN_STATE_NOT_JOINING:
		break;
	case JOIN_STATE_GET_INFO:
		g_theRenderer->DrawCenteredText2D(Vector2::ZERO, g_theRenderer->m_defaultFont, "Waiting for host response.");
		break;
	case JOIN_STATE_CONNECTING:
		g_theRenderer->DrawCenteredText2D(Vector2::ZERO, g_theRenderer->m_defaultFont, "Joining host.");
		break;
	case JOIN_STATE_SETUP:
		g_theRenderer->DrawCenteredText2D(Vector2::ZERO, g_theRenderer->m_defaultFont, "JOIN");
		g_theRenderer->DrawCenteredText2D(Vector2(0.f, -4.f), g_theRenderer->m_defaultFont, "Address: " + m_joinAddress);
		g_theRenderer->DrawCenteredText2D(Vector2(0.f, -6.f), g_theRenderer->m_defaultFont, "Enter: Join");
		break;
	}
}

void Game::RenderPlaying() const
{
	switch (m_currentUIState)
	{
	case STATE_MAP_SELECTION_MOVEMENT:
		RenderMapSelectionMovement();
		break;
	case STATE_MAP_SELECTION_TARGETING:
		RenderMapSelectionTargetting();
		break;
	case STATE_COMMAND_LIST:
		RenderCommandList();
		break;
	case STATE_ACTION_LIST:
		RenderActionList();
		break;
	case STATE_ABILITIES_LIST:
		RenderAbilitiesList();
		break;
	default:
		break;
	}
}

void Game::RenderWaiting() const
{
	int windowWidth = WINDOW_DEFAULT_RESOLUTION_X;
	int windowHeight = WINDOW_DEFAULT_RESOLUTION_Y;
	g_theConfig->GetConfigInt(windowWidth, (std::string)"WINDOW_RES_X");
	g_theConfig->GetConfigInt(windowHeight, (std::string)"WINDOW_RES_Y");

	float orthoHeight = 10.f;
	float orthWidth = orthoHeight * ((float)windowWidth / (float)windowHeight);
	g_theRenderer->SetProjectionMatrix(g_theRenderer->CreateOrthoProjectionMatrix(Vector2(-(orthWidth * 0.5f), -(orthoHeight * 0.5f)), Vector2((orthWidth * 0.5f), (orthoHeight * 0.5f)), -10.f, 30.f));
	g_theRenderer->SetViewMatrix(m_theCamera.GetViewMatrix());
	g_theRenderer->SetDirectionalLight(Vector3(0.5f, -0.7f, 0.f), Rgba::WHITE, 1.f);
	g_theRenderer->SetShader(m_litShader);

	g_theRenderer->EnableDepthTest(true);
	g_theRenderer->EnableDepthWrite(true);

	if (m_theMap)
	{
		m_theMap->Render();
		DrawUI();
	}
}

void Game::RenderMapSelectionMovement() const
{
	int windowWidth = WINDOW_DEFAULT_RESOLUTION_X;
	int windowHeight = WINDOW_DEFAULT_RESOLUTION_Y;
	g_theConfig->GetConfigInt(windowWidth, (std::string)"WINDOW_RES_X");
	g_theConfig->GetConfigInt(windowHeight, (std::string)"WINDOW_RES_Y");

	float orthoHeight = 10.f;
	float orthWidth = orthoHeight * ((float)windowWidth / (float)windowHeight);
	g_theRenderer->SetProjectionMatrix(g_theRenderer->CreateOrthoProjectionMatrix(Vector2(-(orthWidth * 0.5f), -(orthoHeight * 0.5f)), Vector2((orthWidth * 0.5f), (orthoHeight * 0.5f)), -10.f, 30.f));
	g_theRenderer->SetViewMatrix(m_theCamera.GetViewMatrix());
	g_theRenderer->SetDirectionalLight(Vector3(0.5f, -0.7f, 0.f), Rgba::WHITE, 1.f);
	g_theRenderer->SetShader(m_litShader);

	g_theRenderer->EnableDepthTest(true);
	g_theRenderer->EnableDepthWrite(true);

	if (m_theMap)
	{
		m_theMap->Render();
		m_theMap->DrawTraversableTiles(m_theMap->m_traversableTiles);
		m_theMap->DrawSelectedTile();
		DrawUI();
	}
}

void Game::RenderMapSelectionTargetting() const
{
	int windowWidth = WINDOW_DEFAULT_RESOLUTION_X;
	int windowHeight = WINDOW_DEFAULT_RESOLUTION_Y;
	g_theConfig->GetConfigInt(windowWidth, (std::string)"WINDOW_RES_X");
	g_theConfig->GetConfigInt(windowHeight, (std::string)"WINDOW_RES_Y");

	float orthoHeight = 10.f;
	float orthWidth = orthoHeight * ((float)windowWidth / (float)windowHeight);
	g_theRenderer->SetProjectionMatrix(g_theRenderer->CreateOrthoProjectionMatrix(Vector2(-(orthWidth * 0.5f), -(orthoHeight * 0.5f)), Vector2((orthWidth * 0.5f), (orthoHeight * 0.5f)), -10.f, 30.f));
	g_theRenderer->SetViewMatrix(m_theCamera.GetViewMatrix());
	g_theRenderer->SetDirectionalLight(Vector3(0.5f, -0.7f, 0.f), Rgba::WHITE, 1.f);
	g_theRenderer->SetShader(m_litShader);

	g_theRenderer->EnableDepthTest(true);
	g_theRenderer->EnableDepthWrite(true);

	if (m_theMap)
	{
		m_theMap->Render();
		m_theMap->DrawTargettableTiles();
		m_theMap->DrawSelectedTile();
		if(m_theMap->IsTileInTargettableTiles(m_theMap->m_selectedTile))
			m_theMap->DrawAoETiles();
		DrawUI();
	}
}

void Game::HandleInputMapSelectionMovement(float deltaSeconds)
{
	HandleCameraControl(deltaSeconds);
	HandleMapMovement(deltaSeconds);

	if (g_theInput->WasKeyJustPressed('X'))
	{
		PushCommand(COMMAND_MOVE, m_theMap->m_selectedCharacter, m_theMap->m_selectedTile);
// 		MoveCharacterToTile(m_theMap->m_selectedCharacter, m_theMap->m_selectedTile);
		g_theAudio->PlaySoundAtVolume(m_menuConfirmSound);
		m_currentUIState = STATE_COMMAND_LIST;
	}	
	else if (g_theInput->WasKeyJustPressed('Z'))
	{
		g_theAudio->PlaySoundAtVolume(m_menuCancelSound);
		m_currentUIState = STATE_COMMAND_LIST;
		m_theMap->m_selectedTile = m_theMap->m_selectedCharacter->m_currentTile;
	}

}

void Game::HandleCameraControl(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	if (g_theApp->HasFocus())
	{
		IntVector2 mouseScreenPos = g_theInput->GetCursorScreenPos();
		IntVector2 screenCenter = g_theInput->GetScreenCenter();

		IntVector2 mouseDeltaMove = mouseScreenPos - screenCenter;

		g_theInput->SetCursorScreenPos(screenCenter);

		float mouseMovedX = (float)mouseDeltaMove.x;
		float mouseMovedY = (float)mouseDeltaMove.y;
		float mouseSensitivityY = 0.03f;
		float mouseSensitivityX = 0.03f;
		m_theCamera.m_yaw += mouseMovedX * mouseSensitivityX;
		m_theCamera.m_pitch += mouseMovedY * mouseSensitivityY;

		m_theCamera.m_pitch = ClampFloat(m_theCamera.m_pitch, -89.99f, 89.99f);
	}

	if (g_theInput->WasKeyJustPressed('1'))
	{
		m_theCamera.m_yaw = m_northWestCamera.yaw;
		m_theCamera.m_pitch = m_northWestCamera.pitch;
		m_theCamera.m_roll = m_northWestCamera.roll;
	}
	if (g_theInput->WasKeyJustPressed('2'))
	{
		m_theCamera.m_yaw = m_northEastCamera.yaw;
		m_theCamera.m_pitch = m_northEastCamera.pitch;
		m_theCamera.m_roll = m_northEastCamera.roll;
	}
	if (g_theInput->WasKeyJustPressed('3'))
	{
		m_theCamera.m_yaw = m_southEastCamera.yaw;
		m_theCamera.m_pitch = m_southEastCamera.pitch;
		m_theCamera.m_roll = m_southEastCamera.roll;
	}
	if (g_theInput->WasKeyJustPressed('4'))
	{
		m_theCamera.m_yaw = m_southWestCamera.yaw;
		m_theCamera.m_pitch = m_southWestCamera.pitch;
		m_theCamera.m_roll = m_southWestCamera.roll;
	}
}

bool Game::HandleMapMovement(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	//Find direction most similar to camera forward, starting with north
	IntVector2 mostSimilarForwardDirection = IntVector2(0, 1);
	float mostSimilarForward = DotProduct(g_theApp->m_game->m_theCamera.GetForwardXZ(), Vector3(0.f, 0.f, 1.f));

	float southForwardSimilarity = DotProduct(g_theApp->m_game->m_theCamera.GetForwardXZ(), Vector3(0.f, 0.f, -1.f));
	if (southForwardSimilarity > mostSimilarForward)
	{
		mostSimilarForward = southForwardSimilarity;
		mostSimilarForwardDirection = IntVector2(0, -1);
	}

	float eastForwardSimilarity = DotProduct(g_theApp->m_game->m_theCamera.GetForwardXZ(), Vector3(1.f, 0.f, 0.f));
	if (eastForwardSimilarity > mostSimilarForward)
	{
		mostSimilarForward = eastForwardSimilarity;
		mostSimilarForwardDirection = IntVector2(1, 0);
	}

	float westForwardSimilarity = DotProduct(g_theApp->m_game->m_theCamera.GetForwardXZ(), Vector3(-1.f, 0.f, 0.f));
	if (westForwardSimilarity > mostSimilarForward)
	{
		mostSimilarForward = westForwardSimilarity;
		mostSimilarForwardDirection = IntVector2(-1, 0);
	}

	//Find direction most similar to camera left, starting with north
	IntVector2 mostSimilarLeftDirection = IntVector2(0, 1);
	float mostSimilarLeft = DotProduct(g_theApp->m_game->m_theCamera.GetLeftXZ(), Vector3(0.f, 0.f, 1.f));

	float southLeftSimilarity = DotProduct(g_theApp->m_game->m_theCamera.GetLeftXZ(), Vector3(0.f, 0.f, -1.f));
	if (southLeftSimilarity > mostSimilarLeft)
	{
		mostSimilarLeft = southLeftSimilarity;
		mostSimilarLeftDirection = IntVector2(0, -1);
	}

	float eastLeftSimilarity = DotProduct(g_theApp->m_game->m_theCamera.GetLeftXZ(), Vector3(1.f, 0.f, 0.f));
	if (eastLeftSimilarity > mostSimilarLeft)
	{
		mostSimilarLeft = eastLeftSimilarity;
		mostSimilarLeftDirection = IntVector2(1, 0);
	}

	float westLeftSimilarity = DotProduct(g_theApp->m_game->m_theCamera.GetLeftXZ(), Vector3(-1.f, 0.f, 0.f));
	if (westLeftSimilarity > mostSimilarLeft)
	{
		mostSimilarLeft = westLeftSimilarity;
		mostSimilarLeftDirection = IntVector2(-1, 0);
	}
	
	bool didPlayerMoveCursor = false;
	if (g_theInput->WasKeyJustPressed(KEYCODE_UP))
	{
		if (m_theMap->GetTileAtTileCoords(m_theMap->m_selectedTile->m_tileCoords + mostSimilarForwardDirection))
		{
			m_theMap->m_selectedTile = m_theMap->GetTileAtTileCoords(m_theMap->m_selectedTile->m_tileCoords + mostSimilarForwardDirection);
			didPlayerMoveCursor = true;
		}
	}
	else if (g_theInput->WasKeyJustPressed(KEYCODE_DOWN))
	{
		if (m_theMap->GetTileAtTileCoords(m_theMap->m_selectedTile->m_tileCoords - mostSimilarForwardDirection))
		{
			m_theMap->m_selectedTile = m_theMap->GetTileAtTileCoords(m_theMap->m_selectedTile->m_tileCoords - mostSimilarForwardDirection);
			didPlayerMoveCursor = true;
		}
	}
	else if (g_theInput->WasKeyJustPressed(KEYCODE_LEFT))
	{
		if (m_theMap->GetTileAtTileCoords(m_theMap->m_selectedTile->m_tileCoords + mostSimilarLeftDirection))
		{
			m_theMap->m_selectedTile = m_theMap->GetTileAtTileCoords(m_theMap->m_selectedTile->m_tileCoords + mostSimilarLeftDirection);
			didPlayerMoveCursor = true;
		}
	}
	else if (g_theInput->WasKeyJustPressed(KEYCODE_RIGHT))
	{
		if (m_theMap->GetTileAtTileCoords(m_theMap->m_selectedTile->m_tileCoords - mostSimilarLeftDirection))
		{
			m_theMap->m_selectedTile = m_theMap->GetTileAtTileCoords(m_theMap->m_selectedTile->m_tileCoords - mostSimilarLeftDirection);
			didPlayerMoveCursor = true;
		}
	}

	if (didPlayerMoveCursor)
	{
		g_theAudio->PlaySoundAtVolume(m_menuMoveSound);
	}

	return didPlayerMoveCursor;
}

void Game::MoveCharacterToTile(Character* characterToMove, Tile* tileToMoveTo)
{
	m_theMap->m_traversableTiles = m_theMap->GetTraversableTilesInRangeOfCharacter(characterToMove);
	if (m_theMap->IsTileInTraversableTiles(tileToMoveTo) && tileToMoveTo->m_occupyingCharacter == nullptr)
	{
		m_theMap->StartMovingCharacterToTile(characterToMove, tileToMoveTo);
	}
}

void Game::AttackCharacterWithCharacter(Character* attackingCharacter, Character* targettedCharacter)
{
	attackingCharacter->StartAttack(targettedCharacter);
	m_currentUIState = STATE_COMMAND_LIST;
}

void Game::UseAbilityWithCharacter(Character* actingCharacter, AbilityDefinition* abilityToUse, Tile* targettedTile, Character* targettedCharacter)
{
	actingCharacter->m_targettedCharacter = targettedCharacter;
	actingCharacter->m_targettedTile = targettedTile;
	actingCharacter->m_currentAbility = abilityToUse;

	if (abilityToUse->m_speed >= 100)
	{
		actingCharacter->StartAbility();
		m_currentUIState = STATE_COMMAND_LIST;
	}
	else
	{
		m_currentUIState = STATE_COMMAND_LIST;
		EndTurn(actingCharacter, 0);
	}
}

void Game::ProcessCommand(uint8_t type, uint8_t characterIndex, unsigned int tileIndex, uint8_t targettedCharacterIndex, uint8_t abilityIndex)
{
	CommandType commandType;
	switch (type)
	{
	case 0:
		commandType = COMMAND_ATTACK;
		break;
	case 1:
		commandType = COMMAND_MOVE;
		break;
	case 2:
		commandType = COMMAND_ABILITY;
		break;
	case 3:
		commandType = COMMAND_WAIT;
		break;
	default:
		ERROR_AND_DIE("Invalid command type.");
	}

	Character* actingCharacter = nullptr;
	for (Character* character : m_theMap->m_characters)
	{
		if (character->m_characterIndex == characterIndex)
		{
			actingCharacter = character;
			break;
		}
	}

	ASSERT_OR_DIE(actingCharacter != nullptr, "Invalid character index sent.");

	Tile* targettedTile = m_theMap->GetTileAtTileIndex(tileIndex);

	Character* targettedCharacter = nullptr;
	for (Character* character : m_theMap->m_characters)
	{
		if (character->m_characterIndex == targettedCharacterIndex)
		{
			targettedCharacter = character;
			break;
		}
	}

	AbilityDefinition* abilityToUse = nullptr;
	if (abilityIndex < actingCharacter->m_abilities.size())
	{
		abilityToUse = actingCharacter->m_abilities[abilityIndex];
	}

	m_commandQueue.push(Command(commandType, actingCharacter, targettedTile, targettedCharacter, abilityToUse));
}

void Game::UpdateCommandList(float deltaSeconds)
{
	HandleCameraControl(deltaSeconds);

	if(m_theMap->m_selectedTile != nullptr)
	{
		m_theCamera.m_position = Vector3(m_theMap->m_selectedTile->m_tileCoords.x + 0.5f, m_theMap->m_selectedTile->GetDisplayHeight(), m_theMap->m_selectedTile->m_tileCoords.y + 0.5f);
		m_theCamera.m_position -= (m_theCamera.GetForwardXYZ() * 10.f);
	}

	g_theRenderer->SetEyePosition(Vector4(m_theCamera.m_position.x, m_theCamera.m_position.y, m_theCamera.m_position.z, 0.f));

	HandleMenuControl(3);

	if (g_theInput->WasKeyJustPressed('X'))
	{
		switch (m_currentMenuSelection)
		{
		case 0:
			m_currentUIState = STATE_MAP_SELECTION_MOVEMENT;
			break;
		case 1:
			m_currentUIState = STATE_ACTION_LIST;
			m_currentMenuSelection = 0;
			break;
		case 2:
			PushCommand(COMMAND_WAIT, m_theMap->m_selectedCharacter);
// 			WaitCharacter(m_theMap->m_selectedCharacter);
			break;
		default:
			break;
		}
		g_theAudio->PlaySoundAtVolume(m_menuConfirmSound);
	}
}

void Game::UpdateActionList(float deltaSeconds)
{
	HandleCameraControl(deltaSeconds);

	if (m_theMap->m_selectedTile != nullptr)
	{
		m_theCamera.m_position = Vector3(m_theMap->m_selectedTile->m_tileCoords.x + 0.5f, m_theMap->m_selectedTile->GetDisplayHeight(), m_theMap->m_selectedTile->m_tileCoords.y + 0.5f);
		m_theCamera.m_position -= (m_theCamera.GetForwardXYZ() * 10.f);
	}

	g_theRenderer->SetEyePosition(Vector4(m_theCamera.m_position.x, m_theCamera.m_position.y, m_theCamera.m_position.z, 0.f));

	int numMenuOptions = 2;
	if (m_theMap->m_selectedCharacter->m_abilities.empty())
	{
		numMenuOptions -= 1;
	}

	HandleMenuControl(numMenuOptions);

	if (g_theInput->WasKeyJustPressed('X'))
	{
		switch (m_currentMenuSelection)
		{
		//Attack
		case 0:
			m_currentUIState = STATE_MAP_SELECTION_TARGETING;
			m_theMap->m_targettableTiles = m_theMap->GetTargettableTiles(m_theMap->m_selectedCharacter->m_currentTile->m_tileCoords, m_theMap->m_selectedCharacter->m_attackRange, m_theMap->m_selectedCharacter->m_maxAttackHeightDifference);
			m_theMap->m_AoETiles = m_theMap->GetAoETiles(m_theMap->m_selectedCharacter->m_currentTile->m_tileCoords, 0, 0);
			break;
			
		//Ability
		case 1:
			m_currentUIState = STATE_ABILITIES_LIST;
			m_currentMenuSelection = 0;
			break;
		}

		g_theAudio->PlaySoundAtVolume(m_menuConfirmSound);
	}

	if (g_theInput->WasKeyJustPressed('Z'))
	{
		g_theAudio->PlaySoundAtVolume(m_menuCancelSound);
		m_currentUIState = STATE_COMMAND_LIST;
	}
}

void Game::UpdateAbilitiesList(float deltaSeconds)
{
	HandleCameraControl(deltaSeconds);

	if (m_theMap->m_selectedTile != nullptr)
	{
		m_theCamera.m_position = Vector3(m_theMap->m_selectedTile->m_tileCoords.x + 0.5f, m_theMap->m_selectedTile->GetDisplayHeight(), m_theMap->m_selectedTile->m_tileCoords.y + 0.5f);
		m_theCamera.m_position -= (m_theCamera.GetForwardXYZ() * 10.f);
	}

	g_theRenderer->SetEyePosition(Vector4(m_theCamera.m_position.x, m_theCamera.m_position.y, m_theCamera.m_position.z, 0.f));

	HandleMenuControl((int)m_theMap->m_selectedCharacter->m_abilities.size());

	if (g_theInput->WasKeyJustPressed('X'))
	{
		m_currentUIState = STATE_MAP_SELECTION_TARGETING;
		m_theMap->m_targettableTiles = m_theMap->GetTargettableTiles(m_theMap->m_selectedCharacter->m_currentTile->m_tileCoords, m_theMap->m_selectedCharacter->m_abilities[m_currentMenuSelection]->m_range, m_theMap->m_selectedCharacter->m_abilities[m_currentMenuSelection]->m_maxHeightDifference);
		m_theMap->m_AoETiles = m_theMap->GetAoETiles(m_theMap->m_selectedTile->m_tileCoords, m_theMap->m_selectedCharacter->m_abilities[m_currentMenuSelection]->m_radius, m_theMap->m_selectedCharacter->m_abilities[m_currentMenuSelection]->m_areaMaxHeightDifference);
		m_theMap->m_selectedCharacter->m_currentAbility = m_theMap->m_selectedCharacter->m_abilities[m_currentMenuSelection];

		g_theAudio->PlaySoundAtVolume(m_menuConfirmSound);
	}

	if (g_theInput->WasKeyJustPressed('Z'))
	{
		g_theAudio->PlaySoundAtVolume(m_menuCancelSound);
		m_currentUIState = STATE_ACTION_LIST;
	}
}

void Game::RenderCommandList() const
{
	int windowWidth = WINDOW_DEFAULT_RESOLUTION_X;
	int windowHeight = WINDOW_DEFAULT_RESOLUTION_Y;
	g_theConfig->GetConfigInt(windowWidth, (std::string)"WINDOW_RES_X");
	g_theConfig->GetConfigInt(windowHeight, (std::string)"WINDOW_RES_Y");

	float orthoHeight = 10.f;
	float orthWidth = orthoHeight * ((float)windowWidth / (float)windowHeight);
	g_theRenderer->SetProjectionMatrix(g_theRenderer->CreateOrthoProjectionMatrix(Vector2(-(orthWidth * 0.5f), -(orthoHeight * 0.5f)), Vector2((orthWidth * 0.5f), (orthoHeight * 0.5f)), -10.f, 30.f));
	g_theRenderer->SetViewMatrix(m_theCamera.GetViewMatrix());
	g_theRenderer->SetDirectionalLight(Vector3(0.5f, -0.7f, 0.f), Rgba::WHITE, 1.f);
	g_theRenderer->SetShader(m_litShader);

	g_theRenderer->EnableDepthTest(true);
	g_theRenderer->EnableDepthWrite(true);

	if (m_theMap)
	{
		m_theMap->Render();
		DrawUI();
	}


	g_theRenderer->SetProjectionMatrix(g_theRenderer->CreateOrthoProjectionMatrix(Vector2::ZERO, Vector2((float)windowWidth, (float)windowHeight)));
	g_theRenderer->SetViewMatrix(Matrix4::CreateIdentity());
	g_theRenderer->SetShader(m_orthoShader);
	g_theRenderer->EnableBlend(BLEND_SRC_ALPHA, BLEND_INV_SRC_ALPHA);
	g_theRenderer->EnableDepthTest(false);
	g_theRenderer->EnableDepthWrite(false);


	g_theRenderer->SetTexture(nullptr);

	if(m_theMap->m_selectedCharacter != nullptr && m_theMap->m_selectedCharacter->m_controller != CONTROLLER_AI)
	{
		g_theRenderer->DrawBorderedQuad2D(AABB2(400.f, 200.f, 600.f, 600.f), 2.f, Rgba(241, 231, 205), Rgba::BLACK);

		g_theRenderer->DrawText2D(Vector2(425.f, 550.f), g_theRenderer->m_defaultFont, "Move", Rgba::BLACK, 30.f);
		g_theRenderer->DrawText2D(Vector2(425.f, 500.f), g_theRenderer->m_defaultFont, "Act", Rgba::BLACK, 30.f);
		g_theRenderer->DrawText2D(Vector2(425.f, 450.f), g_theRenderer->m_defaultFont, "Wait", Rgba::BLACK, 30.f);

		g_theRenderer->SetTexture(nullptr);
		switch (m_currentMenuSelection)
		{
		case 0:
			g_theRenderer->DrawBorderedDisc2D(Vector2(412.5f, 525.f), 10.f, 0.5f, 64, Rgba::RED, Rgba::RED);
			break;
		case 1:
			g_theRenderer->DrawBorderedDisc2D(Vector2(412.5f, 475.f), 10.f, 0.5f, 64, Rgba::RED, Rgba::RED);
			break;
		case 2:
			g_theRenderer->DrawBorderedDisc2D(Vector2(412.5f, 425.f), 10.f, 0.5f, 64, Rgba::RED, Rgba::RED);
			break;
		}
	}
}

void Game::RenderActionList() const
{
	int windowWidth = WINDOW_DEFAULT_RESOLUTION_X;
	int windowHeight = WINDOW_DEFAULT_RESOLUTION_Y;
	g_theConfig->GetConfigInt(windowWidth, (std::string)"WINDOW_RES_X");
	g_theConfig->GetConfigInt(windowHeight, (std::string)"WINDOW_RES_Y");

	float orthoHeight = 10.f;
	float orthWidth = orthoHeight * ((float)windowWidth / (float)windowHeight);
	g_theRenderer->SetProjectionMatrix(g_theRenderer->CreateOrthoProjectionMatrix(Vector2(-(orthWidth * 0.5f), -(orthoHeight * 0.5f)), Vector2((orthWidth * 0.5f), (orthoHeight * 0.5f)), -10.f, 30.f));
	g_theRenderer->SetViewMatrix(m_theCamera.GetViewMatrix());
	g_theRenderer->SetDirectionalLight(Vector3(0.5f, -0.7f, 0.f), Rgba::WHITE, 1.f);
	g_theRenderer->SetShader(m_litShader);

	g_theRenderer->EnableDepthTest(true);
	g_theRenderer->EnableDepthWrite(true);

	if (m_theMap)
	{
		m_theMap->Render();
		DrawUI();
	}

	windowWidth = WINDOW_DEFAULT_RESOLUTION_X;
	windowHeight = WINDOW_DEFAULT_RESOLUTION_Y;
	g_theConfig->GetConfigInt(windowWidth, (std::string)"WINDOW_RES_X");
	g_theConfig->GetConfigInt(windowHeight, (std::string)"WINDOW_RES_Y");

	g_theRenderer->SetProjectionMatrix(g_theRenderer->CreateOrthoProjectionMatrix(Vector2::ZERO, Vector2((float)windowWidth, (float)windowHeight)));
	g_theRenderer->SetViewMatrix(Matrix4::CreateIdentity());
	g_theRenderer->SetShader(m_orthoShader);
	g_theRenderer->EnableBlend(BLEND_SRC_ALPHA, BLEND_INV_SRC_ALPHA);
	g_theRenderer->EnableDepthTest(false);
	g_theRenderer->EnableDepthWrite(false);


	g_theRenderer->SetTexture(nullptr);
	g_theRenderer->DrawBorderedQuad2D(AABB2(400.f, 200.f, 600.f, 600.f), 2.f, Rgba(241, 231, 205), Rgba::BLACK);

	g_theRenderer->DrawText2D(Vector2(425.f, 550.f), g_theRenderer->m_defaultFont, "Attack", Rgba::BLACK, 30.f);

	if(!m_theMap->m_selectedCharacter->m_abilities.empty())
	{
		g_theRenderer->DrawText2D(Vector2(425.f, 500.f), g_theRenderer->m_defaultFont, "Ability", Rgba::BLACK, 30.f);
	}

	g_theRenderer->SetTexture(nullptr);
	switch (m_currentMenuSelection)
	{
	case 0:
		g_theRenderer->DrawBorderedDisc2D(Vector2(412.5f, 525.f), 10.f, 0.5f, 64, Rgba::RED, Rgba::RED);
		break;
	case 1:
		g_theRenderer->DrawBorderedDisc2D(Vector2(412.5f, 475.f), 10.f, 0.5f, 64, Rgba::RED, Rgba::RED);
		break;
	}
}

void Game::RenderAbilitiesList() const
{
	int windowWidth = WINDOW_DEFAULT_RESOLUTION_X;
	int windowHeight = WINDOW_DEFAULT_RESOLUTION_Y;
	g_theConfig->GetConfigInt(windowWidth, (std::string)"WINDOW_RES_X");
	g_theConfig->GetConfigInt(windowHeight, (std::string)"WINDOW_RES_Y");

	float orthoHeight = 10.f;
	float orthWidth = orthoHeight * ((float)windowWidth / (float)windowHeight);
	g_theRenderer->SetProjectionMatrix(g_theRenderer->CreateOrthoProjectionMatrix(Vector2(-(orthWidth * 0.5f), -(orthoHeight * 0.5f)), Vector2((orthWidth * 0.5f), (orthoHeight * 0.5f)), -10.f, 30.f));
	g_theRenderer->SetViewMatrix(m_theCamera.GetViewMatrix());
	g_theRenderer->SetDirectionalLight(Vector3(0.5f, -0.7f, 0.f), Rgba::WHITE, 1.f);
	g_theRenderer->SetShader(m_litShader);

	g_theRenderer->EnableDepthTest(true);
	g_theRenderer->EnableDepthWrite(true);

	if (m_theMap)
	{
		m_theMap->Render();
		DrawUI();
	}

	windowWidth = WINDOW_DEFAULT_RESOLUTION_X;
	windowHeight = WINDOW_DEFAULT_RESOLUTION_Y;
	g_theConfig->GetConfigInt(windowWidth, (std::string)"WINDOW_RES_X");
	g_theConfig->GetConfigInt(windowHeight, (std::string)"WINDOW_RES_Y");

	g_theRenderer->SetProjectionMatrix(g_theRenderer->CreateOrthoProjectionMatrix(Vector2::ZERO, Vector2((float)windowWidth, (float)windowHeight)));
	g_theRenderer->SetViewMatrix(Matrix4::CreateIdentity());
	g_theRenderer->SetShader(m_orthoShader);
	g_theRenderer->EnableBlend(BLEND_SRC_ALPHA, BLEND_INV_SRC_ALPHA);
	g_theRenderer->EnableDepthTest(false);
	g_theRenderer->EnableDepthWrite(false);


	g_theRenderer->SetTexture(nullptr);
	g_theRenderer->DrawBorderedQuad2D(AABB2(400.f, 200.f, 600.f, 600.f), 2.f, Rgba(241, 231, 205), Rgba::BLACK);

	Vector2 drawPosition(425.f, 550.f);

	for (AbilityDefinition* ability : m_theMap->m_selectedCharacter->m_abilities)
	{
		g_theRenderer->DrawText2D(drawPosition, g_theRenderer->m_defaultFont, ability->m_name, Rgba::BLACK, 30.f);
		drawPosition.y -= 50.f;

	}

	drawPosition = Vector2(412.5f, 525.f - (m_currentMenuSelection * 50.f));

	g_theRenderer->SetTexture(nullptr);
	g_theRenderer->DrawBorderedDisc2D(drawPosition, 10.f, 0.5f, 64, Rgba::RED, Rgba::RED);
}

void Game::DrawUI() const
{
	if (m_theMap->m_selectedTile->m_occupyingCharacter)
	{
		DrawPortraitMenu();
	}
}

void Game::LoadTileDefinitions()
{
	std::string tileDefinitionFileName = "Data/Gameplay/Tiles.xml";
	g_theConfig->GetConfigString(tileDefinitionFileName, "TileDefinitionFileName");

	XMLNode tileDefinitionsHead = XMLNode::parseFile(tileDefinitionFileName.c_str(), "TileDefinitions");
	for (int tileDefIndex = 0; tileDefIndex < tileDefinitionsHead.nChildNode("TileDefinition"); tileDefIndex++)
	{
		XMLNode tileDef = tileDefinitionsHead.getChildNode("TileDefinition", tileDefIndex);
		new TileDefinition(tileDef);
	}
}

void Game::LoadMapDefinitions()
{
	std::string mapFileName = "Data/Gameplay/Maps.xml";

	XMLNode mapsHead = XMLNode::parseFile(mapFileName.c_str(), "MapDefinitions");
	for (int mapsIndex = 0; mapsIndex < mapsHead.nChildNode("MapDefinition"); mapsIndex++)
	{
		XMLNode mapDefinition = mapsHead.getChildNode("MapDefinition", mapsIndex);
		new MapDefinition(mapDefinition);
	}
}

void Game::LoadCharacterBuilders()
{
	std::string charactersFileName = "Data/Gameplay/Characters.xml";
	g_theConfig->GetConfigString(charactersFileName, "CharactersFileName");

	XMLNode charactersHead = XMLNode::parseFile(charactersFileName.c_str(), "Characters");
	for (int characterIndex = 0; characterIndex < charactersHead.nChildNode("Character"); characterIndex++)
	{
		XMLNode characterBuilder = charactersHead.getChildNode("Character", characterIndex);
		new CharacterBuilder(characterBuilder);
	}
}

void Game::LoadItemDefinitions()
{
	std::string itemsFileName = "Data/Gameplay/Items.xml";
	g_theConfig->GetConfigString(itemsFileName, "ItemsFileName");

	XMLNode itemsHead = XMLNode::parseFile(itemsFileName.c_str(), "ItemDefinitions");
	for (int itemIndex = 0; itemIndex < itemsHead.nChildNode("ItemDefinition"); itemIndex++)
	{
		XMLNode itemDefinition = itemsHead.getChildNode("ItemDefinition", itemIndex);
		new ItemDefinition(itemDefinition);
	}
}

void Game::LoadGameConstants()
{
	std::string constantsFileName = "Data/Gameplay/GameConstants.xml";
	g_theConfig->GetConfigString(constantsFileName, "ConstantsFileName");

	XMLNode constantsHead = XMLNode::parseFile(constantsFileName.c_str(), "Constants");

	XMLNode baseChanceToHitNode = constantsHead.getChildNode("BaseChanceToHit");
	BASE_CHANCE_TO_HIT = ParseXMLAttributeFloat(baseChanceToHitNode, "chance", 0.f);

	XMLNode chanceToHitPerAgilityNode = constantsHead.getChildNode("ChanceToHitPerAgility");
	CHANCE_TO_HIT_PER_AGILITY = ParseXMLAttributeFloat(chanceToHitPerAgilityNode, "chance", 0.f);

	XMLNode baseCriticalChanceNode = constantsHead.getChildNode("BaseCriticalChance");
	BASE_CRITICAL_CHANCE = ParseXMLAttributeFloat(baseCriticalChanceNode, "chance", 0.f);

	XMLNode criticalChancePerLuckNode = constantsHead.getChildNode("CriticalChancePerLuck");
	CRITICAL_CHANCE_PER_LUCK = ParseXMLAttributeFloat(criticalChancePerLuckNode, "chance", 0.f);

	XMLNode criticalMultiplierNode = constantsHead.getChildNode("CriticalMultiplier");
	CRITICAL_MULTIPLIER = ParseXMLAttributeFloat(criticalMultiplierNode, "multiplier", 1.f);
}

void Game::LoadAbilityDefinitions()
{
	std::string abilitiesFileName = "Data/Gameplay/Abilities.xml";
	g_theConfig->GetConfigString(abilitiesFileName, "AbilitiesFileName");

	XMLNode abilitiesHead = XMLNode::parseFile(abilitiesFileName.c_str(), "AbilityDefinitions");
	for (int abilityIndex = 0; abilityIndex < abilitiesHead.nChildNode("AbilityDefinition"); abilityIndex++)
	{
		XMLNode abilityDefinition = abilitiesHead.getChildNode("AbilityDefinition", abilityIndex);
		new AbilityDefinition(abilityDefinition);
	}
}

void Game::LoadParticleEffects()
{
	std::string particlesFileName = "Data/Gameplay/Particles.xml";

	XMLNode particlesHead = XMLNode::parseFile(particlesFileName.c_str(), "ParticleEffects");
	for (int particlesIndex = 0; particlesIndex < particlesHead.nChildNode("ParticleEffect"); particlesIndex++)
	{
		XMLNode particleBuilder = particlesHead.getChildNode("ParticleEffect", particlesIndex);
		new ParticleEffectBuilder(particleBuilder);
	}
}

void Game::UnloadParticleEffects()
{
	ParticleEffectBuilder::ClearRegistry();
}

bool Game::CheckForVictoryOrDefeat()
{
	std::set<uint8_t> survivingPlayers;
	for (Character* character : m_theMap->m_characters)
	{
		if (!character->m_isDead)
		{
			survivingPlayers.insert(character->m_owningPlayer);
		}
	}

	if (survivingPlayers.size() == 1)
	{
		if (m_isPlayingReplay)
		{
			m_endScreenText = "Replay over.";
		}
		else
		{
			if (*survivingPlayers.begin() == m_session->m_session.m_myConnection->m_connectionIndex)
			{
				m_endScreenText = "Victory!";
			}
			else
			{
				m_endScreenText = "Defeat.";
			}
			WriteHistoryToFile();
		}
		m_currentGameState = STATE_END_SCREEN;

		m_session->m_session.Leave();
		return true;
	}
	return false;
}

void Game::WriteHistoryToFile()
{
	FileBinaryStream file;
	file.OpenForWrite("replay.sav");
	file.Write(m_session->m_seed);
	file.Write(m_commandHistory.size());
	while (!m_commandHistory.empty())
	{
		m_commandHistory.front().WriteToFile(&file);
		m_commandHistory.pop();
	}
	file.Close();
}

void Game::ReadReplayFromFile()
{
	FileBinaryStream file;
	file.OpenForRead("replay.sav");
	uint_fast32_t seed;
	file.Read(seed);
	g_random.Seed(seed);

	m_theMap = new Map("test");

	size_t numCommands;
	file.Read(numCommands);
	for(size_t commandIndex = 0; commandIndex < numCommands; commandIndex++)
	{
		Command newCommand;
		newCommand.ReadFromFile(&file);
		m_commandQueue.push(newCommand);
	}
	file.Close();
}

void Game::UpdateEndScreen(float deltaSeconds)
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESCAPE) || g_theInput->WasKeyJustPressed(KEYCODE_ENTER))
	{
		g_theAudio->PlaySoundAtVolume(m_menuCancelSound);
		m_currentGameState = STATE_MAINMENU;
	}
}

void Game::RenderEndScreen() const
{
	g_theRenderer->SetTexture(nullptr);
	g_theRenderer->SetShader(m_orthoShader);
	g_theRenderer->ClearColor(Rgba::BLACK);

	float orthoWidth = ORTHO_X_DIMENSION;
	float orthoHeight = ORTHO_Y_DIMENSION;

	g_theConfig->GetConfigFloat(orthoWidth, "ORTHO_WIDTH");
	g_theConfig->GetConfigFloat(orthoHeight, "ORTHO_HEIGHT");

	g_theRenderer->SetProjectionMatrix(g_theRenderer->CreateOrthoProjectionMatrix(Vector2(ORTHO_X_OFFSET, ORTHO_Y_OFFSET), Vector2(orthoWidth + ORTHO_X_OFFSET, orthoHeight + ORTHO_Y_OFFSET)));
	g_theRenderer->SetViewMatrix(Matrix4::CreateTranslation(Vector3::ZERO));

	g_theRenderer->DrawCenteredText2D(Vector2(ORTHO_X_DIMENSION * 0.5f, ORTHO_Y_DIMENSION - 3.f), g_theRenderer->m_defaultFont, m_endScreenText, Rgba::WHITE, 3.f);
	g_theRenderer->DrawCenteredText2D(Vector2(ORTHO_X_DIMENSION * 0.5f, 2.f), g_theRenderer->m_defaultFont, "Press enter to return to main menu.", Rgba::WHITE, 1.f);
}

void Game::EndTurn(Character* characterToEnd, int remainingCT)
{
	m_currentMenuSelection = 0;
	characterToEnd->m_currentCT = remainingCT;
	characterToEnd->DecrementEffectDurations();
	m_theMap->m_selectedCharacter = nullptr;
	m_theMap->m_isWaitingForInput = false;
}

void Game::DrawPortraitMenu() const
{
	int windowWidth = WINDOW_DEFAULT_RESOLUTION_X;
	int windowHeight = WINDOW_DEFAULT_RESOLUTION_Y;
	g_theConfig->GetConfigInt(windowWidth, (std::string)"WINDOW_RES_X");
	g_theConfig->GetConfigInt(windowHeight, (std::string)"WINDOW_RES_Y");

	g_theRenderer->SetProjectionMatrix(g_theRenderer->CreateOrthoProjectionMatrix(Vector2::ZERO, Vector2((float)windowWidth, (float)windowHeight)));
	g_theRenderer->SetViewMatrix(Matrix4::CreateIdentity());
	g_theRenderer->SetShader(m_orthoShader);
	g_theRenderer->EnableBlend(BLEND_SRC_ALPHA, BLEND_INV_SRC_ALPHA);
	g_theRenderer->EnableDepthTest(false);
	g_theRenderer->EnableDepthWrite(false);

	g_theRenderer->SetTexture(nullptr);
	g_theRenderer->DrawBorderedQuad2D(AABB2(50.f, 50.f, 325.f, 200.f), 2.f, Rgba(241, 231, 205), Rgba::BLACK);

	g_theRenderer->SetTexture(m_theMap->m_selectedTile->m_occupyingCharacter->m_portrait);
	g_theRenderer->DrawQuad2D(75.f, 60.f, 84.f, 116.f, Rgba::WHITE);

	g_theRenderer->DrawText2D(Vector2(200.f, 200.f), g_theRenderer->m_defaultFont, "HP: " + std::to_string(m_theMap->m_selectedTile->m_occupyingCharacter->m_currentHP) + "/" + std::to_string(m_theMap->m_selectedTile->m_occupyingCharacter->m_stats[STAT_MAX_HP]), Rgba::BLACK, 30.f);
	g_theRenderer->DrawText2D(Vector2(200.f, 170.f), g_theRenderer->m_defaultFont, "CT: " + std::to_string(m_theMap->m_selectedTile->m_occupyingCharacter->m_currentCT), Rgba::BLACK, 30.f);
}

void Game::HandleMenuControl(int numMenuOptions)
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_UP))
	{
		g_theAudio->PlaySoundAtVolume(m_menuMoveSound);
		m_currentMenuSelection--;
	}
	else if (g_theInput->WasKeyJustPressed(KEYCODE_DOWN))
	{
		g_theAudio->PlaySoundAtVolume(m_menuMoveSound);
		m_currentMenuSelection++;
	}

	if (m_currentMenuSelection < 0)
		m_currentMenuSelection = 0;
	else if (m_currentMenuSelection >= numMenuOptions)
		m_currentMenuSelection = numMenuOptions - 1;
}

void Game::PushCommand(CommandType type, Character* actingCharacter, Tile* targettedTile /*= nullptr*/, Character* targettedCharacter /*= nullptr*/, AbilityDefinition* abilityToUse /*= nullptr*/)
{
	uint8_t commandType = type;
	uint8_t characterIndex = actingCharacter->m_characterIndex;
	unsigned int tileIndex = 0;
	if (nullptr != targettedTile)
	{
		tileIndex = m_theMap->CalculateTileIndexFromTileCoords(targettedTile->m_tileCoords);
	}
	uint8_t targettedCharacterIndex = 0;
	if (nullptr != targettedCharacter)
	{
		targettedCharacterIndex = targettedCharacter->m_characterIndex;
	}
	uint8_t abilityIndex = 0;
	if(abilityToUse != nullptr)
	{
		for (; abilityIndex < actingCharacter->m_abilities.size(); abilityIndex++)
		{
			if (actingCharacter->m_abilities[abilityIndex] == abilityToUse)
			{
				break;
			}
		}
	}

	m_session->SendCommand(type, characterIndex, tileIndex, targettedCharacterIndex, abilityIndex);
	m_commandQueue.push(Command(type, actingCharacter, targettedTile, targettedCharacter, abilityToUse));
}

void Game::WaitCharacter(Character* characterToWait)
{
	EndTurn(characterToWait, 20);
	//ReleaseWait();
}

void Game::HandleInputMapSelectionTargetting(float deltaSeconds)
{
	HandleCameraControl(deltaSeconds);
	bool didPlayerMoveCursor = HandleMapMovement(deltaSeconds);

	if (didPlayerMoveCursor)
	{
		int radius = 0;
		int maxAreaHeightDifference = 0;

		if (m_theMap->m_selectedCharacter->m_currentAbility)
		{
			radius = m_theMap->m_selectedCharacter->m_currentAbility->m_radius;
			maxAreaHeightDifference = m_theMap->m_selectedCharacter->m_currentAbility->m_areaMaxHeightDifference;
		}

		m_theMap->m_AoETiles = m_theMap->GetAoETiles(m_theMap->m_selectedTile->m_tileCoords, radius, maxAreaHeightDifference);
	}

	if (g_theInput->WasKeyJustPressed('X'))
	{
		g_theAudio->PlaySoundAtVolume(m_menuConfirmSound);
		if (m_theMap->m_selectedCharacter->m_currentAbility)
		{
			Character* targettedCharacter = nullptr;
			Tile* targettedTile = nullptr;
			if (nullptr != m_theMap->m_selectedTile->m_occupyingCharacter)
			{
				targettedCharacter = m_theMap->m_selectedTile->m_occupyingCharacter;
			}
			else
			{
				targettedTile = m_theMap->m_selectedTile;
			}

			PushCommand(COMMAND_ABILITY, m_theMap->m_selectedCharacter, targettedTile, targettedCharacter, m_theMap->m_selectedCharacter->m_currentAbility);
			//UseAbilityWithCharacter(m_theMap->m_selectedCharacter, m_theMap->m_selectedCharacter->m_currentAbility, targettedTile, targettedCharacter);
		}
		else
		{
			//handle attack
			if (m_theMap->m_selectedTile->m_occupyingCharacter && m_theMap->m_selectedTile->m_occupyingCharacter != m_theMap->m_selectedCharacter)
			{
				PushCommand(COMMAND_ATTACK, m_theMap->m_selectedCharacter, nullptr, m_theMap->m_selectedTile->m_occupyingCharacter);
// 				AttackCharacterWithCharacter(m_theMap->m_selectedCharacter, m_theMap->m_selectedTile->m_occupyingCharacter);
			}
		}
		
	}
	else if (g_theInput->WasKeyJustPressed('Z'))
	{
		g_theAudio->PlaySoundAtVolume(m_menuCancelSound);
		m_currentUIState = STATE_ACTION_LIST;
		m_theMap->m_selectedCharacter->m_currentAbility = nullptr;
		m_theMap->m_selectedTile = m_theMap->m_selectedCharacter->m_currentTile;
		m_theMap->m_AoETiles.clear();
	}
	
}

Command::Command(CommandType type, Character* actingCharacter, Tile* targettedTile, Character* targettedCharacter, AbilityDefinition* abilityToUse)
	: m_type(type)
	, m_actingCharacter(actingCharacter)
	, m_targettedTile(targettedTile)
	, m_targettedCharacter(targettedCharacter)
	, m_abilityToUse(abilityToUse)
{

}

Command::Command()
	: m_type(COMMAND_WAIT)
	, m_actingCharacter(nullptr)
	, m_targettedCharacter(nullptr)
	, m_targettedTile(nullptr)
	, m_abilityToUse(nullptr)
{

}

void Command::RunCommand(Game* game)
{
	switch (m_type)
	{
	case COMMAND_MOVE:
		game->MoveCharacterToTile(m_actingCharacter, m_targettedTile);
		break;
	case COMMAND_ATTACK:
		game->AttackCharacterWithCharacter(m_actingCharacter, m_targettedCharacter);
		break;
	case COMMAND_ABILITY:
		game->UseAbilityWithCharacter(m_actingCharacter, m_abilityToUse, m_targettedTile, m_targettedCharacter);
		break;
	case COMMAND_WAIT:
		game->WaitCharacter(m_actingCharacter);
	}
}

void Command::WriteToFile(FileBinaryStream* file)
{
	switch (m_type)
	{
	case COMMAND_ATTACK:
		file->Write(0);
		break;
	case COMMAND_MOVE:
		file->Write(1);
		break;
	case COMMAND_ABILITY:
		file->Write(2);
		break;
	case COMMAND_WAIT:
		file->Write(3);
		break;
	}
	file->Write(m_actingCharacter->m_characterIndex);
	if(m_targettedTile)
	{
		file->Write(m_targettedTile->m_tileCoords.x);
		file->Write(m_targettedTile->m_tileCoords.y);
	}
	else
	{
		file->Write(0);
		file->Write(0);
	}

	if(m_targettedCharacter)
	{
		file->Write(m_targettedCharacter->m_characterIndex);
	}
	else
	{
		file->Write((uint8_t)0);
	}

	uint8_t abilityIndex = 0;
	if (m_abilityToUse != nullptr)
	{
		for (; abilityIndex < m_actingCharacter->m_abilities.size(); abilityIndex++)
		{
			if (m_actingCharacter->m_abilities[abilityIndex] == m_abilityToUse)
			{
				break;
			}
		}
	}
	file->Write(abilityIndex);
}

void Command::ReadFromFile(FileBinaryStream* file)
{
	int type;
	file->Read(type);
	switch (type)
	{
	case 0:
		m_type = COMMAND_ATTACK;
		break;
	case 1:
		m_type = COMMAND_MOVE;
		break;
	case 2:
		m_type = COMMAND_ABILITY;
		break;
	case 3:
		m_type = COMMAND_WAIT;
		break;
	default:
		ERROR_AND_DIE("Invalid command type.");
	}

	uint8_t actingCharacterIndex;
	file->Read(actingCharacterIndex);
	for (Character* character : g_theApp->m_game->m_theMap->m_characters)
	{
		if (character->m_characterIndex == actingCharacterIndex)
		{
			m_actingCharacter = character;
			break;
		}
	}
	ASSERT_OR_DIE(m_actingCharacter != nullptr, "Invalid character index.");

	IntVector2 tileCoords;
	file->Read(tileCoords.x);
	file->Read(tileCoords.y);
	m_targettedTile = g_theApp->m_game->m_theMap->GetTileAtTileCoords(tileCoords);

	uint8_t targettedCharacterIndex;
	file->Read(targettedCharacterIndex);
	m_targettedCharacter = nullptr;
	for (Character* character : g_theApp->m_game->m_theMap->m_characters)
	{
		if (character->m_characterIndex == targettedCharacterIndex)
		{
			m_targettedCharacter = character;
			break;
		}
	}

	uint8_t abilityIndex;
	file->Read(abilityIndex);
	if(abilityIndex < m_actingCharacter->m_abilities.size())
	{
		m_abilityToUse = m_actingCharacter->m_abilities[abilityIndex];
	}
	else
	{
		m_abilityToUse = false;
	}
}

