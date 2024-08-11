/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _INITIALIZESTATE_H_
#define _INITIALIZESTATE_H_

#include "../../common/core/gameApplication.h"
#include "../../common/core/types.h"

class CInitializationSate : public IGameState
{
public:
	virtual const Char* GetName() const override { return TXT("Initialization"); };

public:

	virtual Red::System::Bool OnTick( IGameApplication & application ) override;
};

#endif
