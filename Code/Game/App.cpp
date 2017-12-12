#include "Engine/Network/Net.hpp"
#include "Game/App.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Core/EngineConfig.hpp"
#include "Engine/Core/ConfigSystem.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include <time.h>
#include "Engine/Core/ProfileLogScope.hpp"
#include "Engine/Core/Memory.hpp"
#include "Engine/Core/Thread.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Core/Profiling.hpp"
#include "Engine/Core/JobSystem.hpp"
#include "Engine/Renderer/RHI/ComputeJob.hpp"
#include "Engine/Network/TCPSession.hpp"
#include "Engine/Network/NetMessage.hpp"
#include "Engine/Network/NetConnection.hpp"
#include "Engine/Network/RemoteCommandService.hpp"



bool CustomWinProc(UINT wmMessageCode, WPARAM wParam, LPARAM lParam)
{
	lParam;

	unsigned char asKey = (unsigned char)wParam;
	switch (wmMessageCode)
	{
	case WM_CLOSE:
	case WM_DESTROY:
	case WM_QUIT:
		if (g_theApp)
			g_theApp->SetIsQuitting(true);
		return false;

	case WM_KEYDOWN:
	{
		if (g_theApp)
		{
			if (!g_theConsole->m_isConsoleOpen)
				g_theApp->RegisterKeyDown(asKey);
			else
			{
				switch (asKey)
				{
				case KEYCODE_ENTER:
				case KEYCODE_ESCAPE:
					g_theApp->RegisterKeyDown(asKey);
				default:
					break;
				}
			}
		}
		break;
	}

	case WM_CHAR:
		if (g_theApp && g_theConsole->m_isConsoleOpen)
		{
			switch (asKey)
			{
			case KEYCODE_ENTER:
			case KEYCODE_ESCAPE:
			case KEYCODE_SHIFT:
				break;
			default:
				g_theConsole->RegisterKeyDown(asKey);
			}
		}
		break;

	case WM_KEYUP:
		if (g_theApp)
			g_theApp->RegisterKeyUp(asKey);
		break;

	case WM_LBUTTONDOWN:
		if (g_theApp)
			g_theApp->RegisterKeyDown(KEYCODE_LMB);
		break;

	case WM_RBUTTONDOWN:
		if (g_theApp)
			g_theApp->RegisterKeyDown(KEYCODE_RMB);
		break;

	case WM_LBUTTONUP:
		if (g_theApp)
			g_theApp->RegisterKeyUp(KEYCODE_LMB);
		break;

	case WM_RBUTTONUP:
		if (g_theApp)
			g_theApp->RegisterKeyUp(KEYCODE_RMB);
		break;

	case WM_SETFOCUS:
		if (g_theApp)
			g_theApp->OnGainedFocus();
		break;

	case WM_KILLFOCUS:
		if (g_theApp)
			g_theApp->OnLostFocus();
		break;
	}
	return true;
}

bool ConsoleQuit(std::string args)
{
	g_theApp->SetIsQuitting(true);
	return true;
}

App::App()
	: m_game(nullptr)
	, m_isQuitting(false)
	, m_shaders()
	, m_shaderIndex(0)
	, m_theSampler(nullptr)
	, m_consoleCaretBlinkTimer(0.f)
{
	g_theApp = this;

	g_theConsole = new ConsoleSystem();
	g_theConsole->RegisterCommand("quit", ConsoleQuit);

	g_theConfig = new ConfigSystem();
	g_theConfig->Initialize("Tactics.config");

	g_theInput = new InputSystem();
	g_theAudio = new AudioSystem();

	g_theRenderer = new SimpleRenderer();
	int windowWidth = WINDOW_DEFAULT_RESOLUTION_X;
	int windowHeight = WINDOW_DEFAULT_RESOLUTION_Y;

	g_theConfig->GetConfigInt(windowWidth, (std::string)"WINDOW_RES_X");
	g_theConfig->GetConfigInt(windowHeight, (std::string)"WINDOW_RES_Y");

	g_theRenderer->Setup(windowWidth, windowHeight);
	g_theRenderer->m_output->m_window->SetCustomWindowProcCallback(static_cast<WindowProcCB>(CustomWinProc));

	g_theConfig->GetConfigFloat(ORTHO_X_DIMENSION, "ORTHO_WIDTH");
	g_theConfig->GetConfigFloat(ORTHO_Y_DIMENSION, "ORTHO_HEIGHT");

	srand((unsigned int)time(NULL));

	m_game = new Game();

	ReloadShaders();

	m_theSampler = g_theRenderer->m_device->CreateSampler(FILTERMODE_LINEAR);

	m_consoleFont = g_theRenderer->CreateFontFromFilePath("Data/Fonts/centaur64.fnt");

	OnGainedFocus();
	m_game->Initialize();
}

App::~App()
{
	delete m_game;
	m_game = nullptr;

	delete g_theInput;
	g_theInput = nullptr;

	delete g_theRenderer;
	g_theRenderer = nullptr;

	delete g_theConfig;
	g_theConfig = nullptr;

	for (size_t shaderIndex = 0; shaderIndex < m_shaders.size(); shaderIndex++)
	{
		delete m_shaders[shaderIndex];
		m_shaders[shaderIndex] = nullptr;
	}
	m_shaders.clear();

	delete m_theSampler;
	m_theSampler = nullptr;

	delete m_consoleFont;
	m_consoleFont = nullptr;
}

void App::BeginFrame()
{
	MemoryProfilerRunFrame();

	if (g_theInput)
		g_theInput->BeginFrame();

	if (g_theAudio)
		g_theAudio->BeginFrame();
}

void App::EndFrame()
{
	if (g_theInput)
		g_theInput->EndFrame();
	
}

void App::RunFrame()
{
	g_theRenderer->m_output->m_window->ProcessMessages();

	BeginFrame();

	static double timeOfLastRunFrame = GetCurrentTimeSeconds();

	double timeNow = GetCurrentTimeSeconds();
	double deltaSeconds = timeNow - timeOfLastRunFrame;
	timeOfLastRunFrame = timeNow;

	if (deltaSeconds > 0.25)
	{
		deltaSeconds = 0.25;
	}

	Update((float)deltaSeconds);			
	Render();		

	EndFrame();
}


bool App::IsQuitting() const
{
	return m_isQuitting;
}

void App::SetIsQuitting(bool isQuitting)
{
	m_isQuitting = isQuitting;
}

void App::RegisterKeyDown(unsigned char keyCode)
{
	if (keyCode == KEYCODE_F1)
	{
		g_theConsole->m_isConsoleOpen = !g_theConsole->m_isConsoleOpen;
		g_theInput->RegisterKeyDown(keyCode);
		if (g_theConsole->m_isConsoleOpen)
			OnGainedFocus();
		else
			OnLostFocus();
	}
	else
	{
		if (g_theConsole->m_isConsoleOpen)
			g_theConsole->RegisterKeyDown(keyCode);
		else
		{
			g_theInput->RegisterKeyDown(keyCode);
		}
	}
}

void App::RegisterKeyUp(unsigned char keyCode)
{
	g_theInput->RegisterKeyUp(keyCode);
}

void App::Update(float deltaSeconds)
{
	m_mainConsumer.JobConsumeForMS(5.0);

	if (m_game)
		m_game->Update(deltaSeconds);

	if (g_theRenderer)
		g_theRenderer->Update(deltaSeconds);

	m_consoleCaretBlinkTimer += deltaSeconds;
	if (m_consoleCaretBlinkTimer >= 1.f)
		m_consoleCaretBlinkTimer = 0.f;
}


void App::Render() const
{
	g_theRenderer->SetRenderTarget(nullptr, nullptr);
	Rgba clearColor(127, 127, 230, 255);
	g_theRenderer->ClearColor(clearColor);
	g_theRenderer->ClearDepth();

	int windowWidth = WINDOW_DEFAULT_RESOLUTION_X;
	int windowHeight = WINDOW_DEFAULT_RESOLUTION_Y;

	g_theConfig->GetConfigInt(windowWidth, (std::string)"WINDOW_RES_X");
	g_theConfig->GetConfigInt(windowHeight, (std::string)"WINDOW_RES_Y");

	g_theRenderer->SetViewport(0, 0, windowWidth, windowHeight);
	g_theRenderer->SetAmbientLight(Rgba::WHITE, 0.25f);
	g_theRenderer->SetSpecularConstants(16.f, 1.f);
	g_theRenderer->SetShader(m_shaders[m_shaderIndex]);

	g_theRenderer->SetSampler(m_theSampler);

	g_theRenderer->EnableBlend(BLEND_SRC_ALPHA, BLEND_INV_SRC_ALPHA);
	g_theRenderer->EnableDepthTest(false);
	g_theRenderer->EnableDepthWrite(false);
	g_theRenderer->SetTexture(nullptr);

	if (m_game)
		m_game->Render();

	g_theRenderer->EnableBlend(BLEND_SRC_ALPHA, BLEND_INV_SRC_ALPHA);
	g_theRenderer->EnableDepthTest(false);
	g_theRenderer->EnableDepthWrite(false);
	g_theRenderer->SetTexture(nullptr);

	if (g_theConsole->m_isConsoleOpen)
		DrawConsoleLog();

	g_theRenderer->DisableBlend();

	g_theRenderer->Present();
}

bool App::HasFocus() const
{
	return m_hasFocus;
}

void App::OnGainedFocus()
{
	m_hasFocus = true;
	g_theInput->ShowMouseCursor(false);
}

void App::OnLostFocus()
{
	m_hasFocus = false;
	g_theInput->ShowMouseCursor(true);
}

void App::ReloadShaders()
{
	if (!m_shaders.empty())
	{
		for (size_t shaderIndex = 0; shaderIndex < m_shaders.size(); shaderIndex++)
		{
			delete m_shaders[shaderIndex];
			m_shaders[shaderIndex] = nullptr;
		}
		m_shaders.clear();
	}
	m_shaders.push_back(g_theRenderer->CreateShaderFromHLSLFile("Data/HLSL/ortho_textured.hlsl"));
}

void App::DrawConsoleLog() const
{
	int windowWidth = WINDOW_DEFAULT_RESOLUTION_X;
	int windowHeight = WINDOW_DEFAULT_RESOLUTION_Y;
	g_theConfig->GetConfigInt(windowWidth, (std::string)"WINDOW_RES_X");
	g_theConfig->GetConfigInt(windowHeight, (std::string)"WINDOW_RES_Y");

	g_theRenderer->SetProjectionMatrix(g_theRenderer->CreateOrthoProjectionMatrix(Vector2::ZERO, Vector2((float)windowWidth, (float)windowHeight)));
	g_theRenderer->SetViewMatrix(Matrix4::CreateTranslation(Vector3::ZERO));

	g_theRenderer->SetShader(m_shaders[0]);
	g_theRenderer->SetTexture(nullptr);
	g_theRenderer->DrawQuad2D(0.f, 0.f, (float)windowWidth, (float)windowHeight, Rgba(30, 30, 30, 100));

	Vector2 drawPosition(50.f, 50.f);
	float textScale = 30.f;

	std::string buffer = g_theConsole->GetCurrentBuffer();
	if (m_consoleCaretBlinkTimer > 0.5f)
		buffer += '|';

	g_theRenderer->DrawText2D(drawPosition, m_consoleFont, buffer, Rgba::WHITE, textScale);
	std::vector<ConsoleMessage> messageLog = g_theConsole->GetMessageLog();
	for (int messageIndex = (int)(messageLog.size() - 1) - g_theConsole->m_currentLogLineOffset; messageIndex >= 0; messageIndex--)
	{
		drawPosition += Vector2(0.f, m_consoleFont->CalculateTextHeight(messageLog[messageIndex].text, messageLog[messageIndex].scale));

		if (drawPosition.y > windowHeight)
			break;
		g_theRenderer->DrawText2D(drawPosition, m_consoleFont, messageLog[messageIndex].text, messageLog[messageIndex].color, messageLog[messageIndex].scale);
	}
}