/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _GAMECONTRAINEDSTATE_H_
#define _GAMECONTRAINEDSTATE_H_

#include "../../common/core/gameApplication.h"
#include "../../common/core/types.h"

class CGameConstrainedState : public IGameState
{
public:
	virtual const Char* GetName() const override { return TXT("Constrained"); };

public:
	virtual Red::System::Bool OnEnterState() override; 
	virtual Red::System::Bool OnTick( IGameApplication & application ) override;
	virtual Red::System::Bool OnExitState() override;
};

#endif 
