/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once
#include "entity.h"

class CLightweightDecalEmitter
{
public:
	CLightweightDecalEmitter();

	void Spawn( CWorld* world, const Vector& position, const Vector& front, const Vector& up, CBitmapTexture* footStepDecalDiff, CBitmapTexture* footStepDecalNorm, Uint8 atlasScaleS, Uint8 atlasScaleT, Uint8 atlasTile );
};
