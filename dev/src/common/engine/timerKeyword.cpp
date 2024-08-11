#include "build.h"

#include "timerKeyword.h"

IMPLEMENT_ENGINE_CLASS( CTimerScriptKeyword );

CTimerScriptKeyword::CTimerScriptKeyword()
{
	SetTimer( 0.0f, 0.0f );
}

CTimerScriptKeyword::~CTimerScriptKeyword()
{
}
