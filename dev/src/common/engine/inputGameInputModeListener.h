/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class IGameInputModeListener
{
protected:
											IGameInputModeListener() {}
	virtual									~IGameInputModeListener() {}

public:
	virtual void							OnGameInputMode( Bool ) {}
};
