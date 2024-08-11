/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "effectDummyComponent.h"
#include "../../common/core/gatheredResource.h"
#include "bitmapTexture.h"

IMPLEMENT_ENGINE_CLASS( CEffectDummyComponent );

CGatheredResource resEffectDummy( TXT("engine\\textures\\icons\\waypointicon.xbm"), RGF_NotCooked );

CEffectDummyComponent::CEffectDummyComponent()
{
}

CBitmapTexture* CEffectDummyComponent::GetSpriteIcon() const
{
	return resEffectDummy.LoadAndGet< CBitmapTexture >();
}

Uint32 CEffectDummyComponent::GetMinimumStreamingDistance() const
{
	return 16; // No idea...
}
