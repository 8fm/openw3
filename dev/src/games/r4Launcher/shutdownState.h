/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _SHUTDOWNSTATE_H_
#define _SHUTDOWNSTATE_H_

#include "../../common/core/gameApplication.h"
#include "../../common/core/types.h"

class CShutdownState : public IGameState
{
public:
	virtual const Char* GetName() const { return TXT("Shutdown"); };

public:
	virtual Red::System::Bool OnTick( IGameApplication & application ) override;
};

#endif 
