/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef _GAME_APPLICATION_ORBIS_H_
#define _GAME_APPLICATION_ORBIS_H_

#include "../../common/redSystem/types.h"

#include "../../common/core/gameApplication.h"

class CGameApplicationOrbis : public CGameApplication
{
public:
	CGameApplicationOrbis();
	virtual ~CGameApplicationOrbis() override final;

protected:
	virtual void PumpEvents() override final;
};

#endif // _GAME_APPLICATION_ORBIS_H_
