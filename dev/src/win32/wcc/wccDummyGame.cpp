/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "wccDummyGame.h"

IMPLEMENT_ENGINE_CLASS( CWccDummyGame );

void CWccDummyGame::Init()
{
	TBaseClass::Init();

	switch( m_game )
	{
	case RG_R4:
		InitializeR4GameSystems( this );
		break;
	//case RG_R6:
	//	InitializeR6GameSystems( this );
	//	break;
	}
}

void CWccDummyGame::ShutDown()
{
	switch( m_game )
	{
	case RG_R4:
		ShutdownR4GameSystems( this );
		break;
	//case RG_R6:
	//	ShutdownR6GameSystems( this );
	//	break;
	}

	TBaseClass::ShutDown();
}