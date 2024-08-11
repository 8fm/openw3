/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _GAMERUNNINGSTATE_H_
#define _GAMERUNNINGSTATE_H_

#include "../../common/core/gameApplication.h"
#include "../../common/core/types.h"

class CGameRunningState : public IGameState
{
public:
	virtual const Char* GetName() const override { return TXT("Running"); };

public:
	virtual Red::System::Bool OnEnterState() override; 
	virtual Red::System::Bool OnTick( IGameApplication & application ) override;
};

#endif 
