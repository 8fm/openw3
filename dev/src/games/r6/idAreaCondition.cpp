/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "idAreaCondition.h"
#include "idInterlocutor.h"
#include "idInstance.h"

#include "../../common/game/gameWorld.h"
#include "../../common/engine/tagManager.h"
#include "../../common/engine/areaComponent.h"

IMPLEMENT_ENGINE_CLASS( CIDAreaContition )
IMPLEMENT_ENGINE_CLASS( CIDInterlocutorInAreaContition )
IMPLEMENT_ENGINE_CLASS( CIDTaggedEntityInAreaContition )

Bool CIDAreaContition::IsInArea( const Box& aabb ) const
{
	const CEntity* areaEntity = m_area.Get();
	if ( areaEntity )
	{
		for ( ComponentIterator< const CAreaComponent > it( areaEntity ); it; ++it )
		{
			if ( ( *it )->TestBoxOverlap( aabb ) )
			{
				return !m_checkOutside;
			}
		}
	}

	return m_checkOutside;
}

Bool CIDInterlocutorInAreaContition::IsFulfilled( Uint32 dialogId ) const
{
	CInteractiveDialogInstance* instance = GetDialogInstance( dialogId );
	if ( instance )
	{
		CIDInterlocutorComponent* component = instance->GetInterlocutor( m_interlocutor );
		if ( component )
		{
			return IsInArea( component->GetEntity()->CalcBoundingBox() );
		}
	}

	return false;
}

Bool CIDTaggedEntityInAreaContition::IsFulfilled( Uint32 dialogId ) const
{
	CGameWorld* world = GCommonGame->GetActiveWorld();
	if ( world )
	{
		CEntity* entity = world->GetTagManager()->GetTaggedEntity( m_tagToMatch );
		if ( entity )
		{
			return IsInArea( entity->CalcBoundingBox() );
		}
	}

	return false;
}
