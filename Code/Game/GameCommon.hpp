#pragma once
#include "Engine/Renderer/RHI/SimpleRenderer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/Audio.hpp"
#include "Engine/Core/ConsoleSystem.hpp"
#include "Engine/Math/Random.hpp"

class App;

extern InputSystem* g_theInput;
extern AudioSystem* g_theAudio;
extern App* g_theApp;

constexpr unsigned int WINDOW_DEFAULT_RESOLUTION_X = 1280;
constexpr unsigned int WINDOW_DEFAULT_RESOLUTION_Y = 720;

extern float ORTHO_X_DIMENSION;
extern float ORTHO_Y_DIMENSION;
extern float ORTHO_X_OFFSET;
extern float ORTHO_Y_OFFSET;

extern float BASE_CHANCE_TO_HIT;
extern float CHANCE_TO_HIT_PER_AGILITY;
extern float BASE_CRITICAL_CHANCE;
extern float CRITICAL_CHANCE_PER_LUCK;
extern float CRITICAL_MULTIPLIER;

extern RandomGenerator g_random;