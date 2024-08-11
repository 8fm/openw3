/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/core/messagePump.h"
#include "../../common/core/gameApplication.h"

class CGameApplication;

CGameApplication * CreateGameApplication( const wchar_t * commandLine );

void DestroyGameApplication( CGameApplication * gameApplication );

class CGameApplicationDurango : public CGameApplication
{
protected:
	virtual void PumpEvents() override final;
};
