#pragma once
#include "Game/Game.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Renderer/RHI/RHI.hpp"
#include <vector>
#include "Engine/Core/JobSystem.hpp"

class Font;

class App
{
private:
	bool m_isQuitting;
	float m_consoleCaretBlinkTimer;
	bool m_hasFocus;

	std::vector<ShaderProgram*> m_shaders;
	unsigned int m_shaderIndex;
	Sampler* m_theSampler;
	Font* m_consoleFont;

	JobConsumer m_mainConsumer;

private:
	void Update(float deltaSeconds);
	void Render() const;

	void ReloadShaders();

	void DrawConsoleLog() const;

public:
	App();
	~App();
	void BeginFrame();
	void EndFrame();
	void RunFrame();
	bool IsQuitting() const;
	void SetIsQuitting(bool isQuitting);
	void RegisterKeyDown(unsigned char keyCode);
	void RegisterKeyUp(unsigned char keyCode);
	bool HasFocus() const;
	void OnGainedFocus();
	void OnLostFocus();

	Game* m_game;
};