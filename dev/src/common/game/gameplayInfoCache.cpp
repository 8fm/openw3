/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "gameplayInfoCache.h"
#include "gameplayEntity.h"
#include "interactionComponent.h"
#include "../../common/engine/drawableComponent.h"

IMPLEMENT_RTTI_ENUM( EGameplayInfoCacheType );

Bool SGameplayInfoCache::EvaluateInternal( const CGameplayEntity *entity, EGameplayInfoCacheType type, Bool& res ) const
{
	if ( type == GICT_IsInteractive )
	{
		res = ( entity->FindComponent< CInteractionComponent >() != nullptr );
		return true;
	}
	else if ( type == GICT_HasDrawableComponents )
	{
		res = ( entity->FindComponent< CDrawableComponent >() != nullptr );
		return true;
	}
	return false;
}