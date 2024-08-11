/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptStackFrame.h"
#include "scriptThread.h"

class CObject;
extern Bool GLatentFunctionStart;

namespace
{
	void funcSleep( CObject*, CScriptStackFrame& stack, void* result )
	{
		GET_PARAMETER( Float, time, 0.0f );
		FINISH_PARAMETERS;

		// Yield if not waited long enough
		ASSERT( stack.m_thread );

		if( GLatentFunctionStart )
		{
			stack.m_thread->ForceYield();
		}
		else
		{
			const Float waitedTime = stack.m_thread->GetCurrentLatentTime();
			if ( waitedTime < time )
			{
				stack.m_thread->ForceYield();
			}
		}

	RETURN_VOID();
	}

	void funcSleepOneFrame( CObject*, CScriptStackFrame& stack, void* result )
	{
		FINISH_PARAMETERS;

		if( GLatentFunctionStart )
		{
			stack.m_thread->ForceYield();
		}

		RETURN_VOID();
	}

	void funcKillThread( CObject*, CScriptStackFrame& stack, void* result )
	{
		FINISH_PARAMETERS;
		ASSERT( stack.m_thread );
		stack.m_thread->ForceKill();

		RETURN_VOID();
	}
}

void ExportCoreLatentFunctions()
{
	NATIVE_GLOBAL_FUNCTION( "Sleep", funcSleep );
	NATIVE_GLOBAL_FUNCTION( "SleepOneFrame", funcSleepOneFrame );
	NATIVE_GLOBAL_FUNCTION( "KillThread", funcKillThread );
}
