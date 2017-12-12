#include "Engine/Network/Net.hpp"
#include "Engine/Network/NetAddress.hpp"
#include "Engine/Network/TCPSocket.hpp"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <math.h>
#include <cassert>
#include <crtdbg.h>
#include "Engine/Math/Vector2.hpp"
#include "Game/App.hpp"
#include "Game/GameCommon.hpp"
#include <time.h>
#include "Engine/Core/Memory.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Core/Callstack.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Core/Profiling.hpp"
#include "Engine/Core/JobSystem.hpp"

//-----------------------------------------------------------------------------------------------
//#define UNUSED(x) (void)(x);



//-----------------------------------------------------------------------------------------------
void Initialize( HINSTANCE applicationInstanceHandle )
{
	UNUSED(applicationInstanceHandle);

	SetHasMainBeenEntered(true);
	JobSystemStartup(JOB_TYPE_COUNT);
#if (PROFILE_MEMORY == PROFILE_MEMORY_VERBOSE)
	CallstackSystemInit();
#endif
	SetProcessDPIAware();
	LogStartup("log.log");
	ProfilerSystemStartup();

	NetSystemStartup();
	g_theApp = new App();
}


//-----------------------------------------------------------------------------------------------
void Shutdown()
{
	delete g_theApp;
	g_theApp = nullptr;

	NetSystemShutdown();
	ProfilerSystemShutdown();
#if (PROFILE_MEMORY == PROFILE_MEMORY_VERBOSE)
	MemoryProfilerLogActiveAllocations();
	CallstackSystemDeinit();
#endif
	LogShutdown();
	SetHasMainBeenEntered(false);
	JobSystemShutdown();
}


//-----------------------------------------------------------------------------------------------
int WINAPI WinMain( HINSTANCE applicationInstanceHandle, HINSTANCE, LPSTR commandLineString, int )
{
	UNUSED( commandLineString );
	Initialize( applicationInstanceHandle );

	while( !g_theApp->IsQuitting() )
	{
		Sleep(1);
		g_theApp->RunFrame();
	}

	Shutdown();
	return 0;
}


