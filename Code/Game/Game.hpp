#pragma once
#include "Engine/Math/Vector2.hpp"
#include "Engine/Renderer/RHI/SpriteSheet2D.hpp"
#include "Engine/Audio/Audio.hpp"
#include "Game/Map.hpp"
#include "Game/Camera3D.hpp"

enum GameState
{
	STATE_MAINMENU,
	STATE_HOSTING,
	STATE_JOINING,
	STATE_PLAYING,
	STATE_WAITING,
	STATE_END_SCREEN
};

enum JoinState
{
	JOIN_STATE_SETUP,
	JOIN_STATE_CONNECTING,
	JOIN_STATE_GET_INFO,
	JOIN_STATE_NOT_JOINING
};

enum UIState
{
	STATE_MAP_SELECTION_MOVEMENT,
	STATE_MAP_SELECTION_TARGETING,
	STATE_MAP_SELECTION_VIEWING,
	STATE_COMMAND_LIST,
	STATE_ACTION_LIST,
	STATE_ABILITIES_LIST,
	STATE_STATUS_MENU,
	NUM_UI_STATES
};

enum CommandType
{
	COMMAND_ATTACK,
	COMMAND_MOVE,
	COMMAND_ABILITY,
	COMMAND_WAIT,

	NUM_COMMAND_TYPES
};
class Game;
class GameSession;

struct Command
{
	Command();
	Command(CommandType type, Character* actingCharacter, Tile* targettedTile, Character* targettedCharacter, AbilityDefinition* abilityToUse);
	void RunCommand(Game* game);

	void WriteToFile(FileBinaryStream* file);
	void ReadFromFile(FileBinaryStream* file);

	CommandType m_type;

	Character* m_actingCharacter;
	Tile* m_targettedTile = nullptr;
	Character* m_targettedCharacter = nullptr;
	AbilityDefinition* m_abilityToUse = nullptr;
};

class Game
{

public:
	Camera3D m_theCamera;
	CameraState m_northWestCamera;
	CameraState m_northEastCamera;
	CameraState m_southWestCamera;
	CameraState m_southEastCamera;

	Map* m_theMap;
	GameState m_currentGameState = STATE_MAINMENU;
	JoinState m_joinState = JOIN_STATE_NOT_JOINING;
	UIState m_currentUIState = STATE_COMMAND_LIST;
	bool m_isPlayingReplay = false;

	ShaderProgram* m_orthoShader;
	ShaderProgram* m_litShader;
	ShaderProgram* m_spriteShader;

	GameSession* m_session;

	std::string m_endScreenText;
	std::string m_joinAddress;

	Game();
	~Game();

	void Initialize();

	void Update(float deltaSeconds);
	void Render() const;

	void WaitUntilRelease();
	void ReleaseWait();

	void WaitCharacter(Character* characterToWait);
	void EndTurn(Character* characterToEnd, int remainingCT);

	//Character Actions
	void MoveCharacterToTile(Character* characterToMove, Tile* tileToMoveTo);
	void AttackCharacterWithCharacter(Character* attackingCharacter, Character* targettedCharacter);
	void UseAbilityWithCharacter(Character* actingCharacter, AbilityDefinition* abilityToUse, Tile* targettedTile, Character* targettedCharacter);

	void ProcessCommand(uint8_t commandType, uint8_t characterIndex, unsigned int tileIndex, uint8_t targettedCharacterIndex, uint8_t abilityIndex);

private:
	bool m_isGamePaused;
	int m_numWaits;

	std::queue<Command> m_commandQueue;
	std::queue<Command> m_commandHistory;

	int m_currentMenuSelection;
	SoundID m_menuConfirmSound;
	SoundID m_menuCancelSound;
	SoundID m_menuMoveSound;

	//Input Handling
	void HandleInputMapSelectionMovement(float deltaSeconds);
	void HandleInputMapSelectionTargetting(float deltaSeconds);
	void HandleCameraControl(float deltaSeconds);
	bool HandleMapMovement(float deltaSeconds);
	void HandleMenuControl(int numMenuOptions);

	void PushCommand(CommandType type, Character* actingCharacter, Tile* targettedTile = nullptr, Character* targettedCharacter = nullptr, AbilityDefinition* abilityToUse = nullptr);

	//State Updates
	void UpdateMainMenu(float deltaSeconds);
	void UpdateHosting(float deltaSeconds);
	void UpdateJoining(float deltaSeconds);
	void UpdatePlaying(float deltaSeconds);
	void UpdateWaiting(float deltaSeconds);
	void UpdateMapSelectionMovement(float deltaSeconds);
	void UpdateMapSelectionTargetting(float deltaSeconds);
	void UpdateCommandList(float deltaSeconds);
	void UpdateActionList(float deltaSeconds);
	void UpdateAbilitiesList(float deltaSeconds);
	void UpdateEndScreen(float deltaSeconds);

	//State Renders
	void RenderMainMenu() const;
	void RenderHosting() const;
	void RenderJoining() const;
	void RenderPlaying() const;
	void RenderWaiting() const;
	void RenderMapSelectionMovement() const;
	void RenderMapSelectionTargetting() const;
	void RenderCommandList() const;
	void RenderActionList() const;
	void RenderAbilitiesList() const;
	void RenderEndScreen() const;

	void DrawUI() const;
	void DrawPortraitMenu() const;


	//XML Loading
	void LoadTileDefinitions();
	void LoadCharacterBuilders();
	void LoadItemDefinitions();
	void LoadMapDefinitions();
	void LoadGameConstants();
	void LoadAbilityDefinitions();
	void LoadParticleEffects();
	void UnloadParticleEffects();

	bool CheckForVictoryOrDefeat();
	void WriteHistoryToFile();
	void ReadReplayFromFile();
};