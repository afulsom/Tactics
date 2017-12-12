#include "Game/GameCommon.hpp"


InputSystem* g_theInput = nullptr;
AudioSystem* g_theAudio = nullptr;
App* g_theApp = nullptr;


float ORTHO_X_DIMENSION = 16.f;
float ORTHO_Y_DIMENSION = 9.f;
float ORTHO_X_OFFSET = 0.f;
float ORTHO_Y_OFFSET = 0.f;

float BASE_CHANCE_TO_HIT = 0.f;
float CHANCE_TO_HIT_PER_AGILITY = 0.f;
float BASE_CRITICAL_CHANCE = 0.f;
float CRITICAL_CHANCE_PER_LUCK = 0.f;
float CRITICAL_MULTIPLIER = 1.f;

RandomGenerator g_random;