/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "extAnimCutsceneQuestEvent.h"
#include "questsSystem.h"
#include "itemIterator.h"
#include "../engine/clothComponent.h"
#include "../engine/animDangleComponent.h"

IMPLEMENT_ENGINE_CLASS( CExtAnimCutsceneQuestEvent );

CExtAnimCutsceneQuestEvent::CExtAnimCutsceneQuestEvent()
	: CExtAnimEvent()
	, m_cutsceneName( String() )
{

}

CExtAnimCutsceneQuestEvent::CExtAnimCutsceneQuestEvent( const CName& eventName,
   const CName& animationName, Float startTime, const String& trackName,
   const String& cutsceneName )
   : CExtAnimEvent( eventName, animationName, startTime, trackName )
   , m_cutsceneName( cutsceneName )
{
}

void CExtAnimCutsceneQuestEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	ASSERT( !component );

	if( GCommonGame != NULL && GCommonGame->GetSystem< CQuestsSystem >()!= NULL )
	{
		GCommonGame->GetSystem< CQuestsSystem >()->OnCutsceneEvent( m_cutsceneName, GetEventName() );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CExtAnimCutsceneResetClothAndDangleEvent );

CExtAnimCutsceneResetClothAndDangleEvent::CExtAnimCutsceneResetClothAndDangleEvent()
	: CExtAnimEvent()
{
	m_reportToScript = false;
}

CExtAnimCutsceneResetClothAndDangleEvent::CExtAnimCutsceneResetClothAndDangleEvent( const CName& eventName,
																				   const CName& animationName, Float startTime, const String& trackName )
																				   : CExtAnimEvent( eventName, animationName, startTime, trackName )
{
	m_reportToScript = false;
}

CExtAnimCutsceneResetClothAndDangleEvent::~CExtAnimCutsceneResetClothAndDangleEvent()
{

}

void CExtAnimCutsceneResetClothAndDangleEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	ASSERT( component );

	if ( CActor* a = Cast< CActor >( component->GetEntity() ) )
	{
		if ( m_forceRelaxedState )
		{
#ifdef USE_APEX
			for ( EntityWithItemsComponentIterator< CClothComponent > it( a ); it; ++it )
			{
				CClothComponent* c = *it;
				c->RequestTeleport();
			}
#endif

			for ( EntityWithItemsComponentIterator< CAnimDangleComponent > it( a ); it; ++it )
			{
				CAnimDangleComponent* c = *it;
				c->ForceResetWithRelaxedState();
			}
		}
		else
		{
#ifdef USE_APEX
			for ( EntityWithItemsComponentIterator< CClothComponent > it( a ); it; ++it )
			{
				CClothComponent* c = *it;
				c->RequestTeleport();
			}
#endif

			for ( EntityWithItemsComponentIterator< CAnimDangleComponent > it( a ); it; ++it )
			{
				CAnimDangleComponent* c = *it;
				c->ForceReset();
			}
		}
	}
	else if( CEntity* ent = component->GetEntity() )
	{
		if ( m_forceRelaxedState )
		{
#ifdef USE_APEX
			for ( ComponentIterator< CClothComponent > it( ent ); it; ++it )
			{
				CClothComponent* c = *it;
				c->RequestTeleport();
			}
#endif

			for ( ComponentIterator< CAnimDangleComponent > it( ent ); it; ++it )
			{
				CAnimDangleComponent* c = *it;
				c->ForceResetWithRelaxedState();
			}
		}
		else
		{
#ifdef USE_APEX
			for ( ComponentIterator< CClothComponent > it( ent ); it; ++it )
			{
				CClothComponent* c = *it;
				c->RequestTeleport();
			}
#endif

			for ( ComponentIterator< CAnimDangleComponent > it( ent ); it; ++it )
			{
				CAnimDangleComponent* c = *it;
				c->ForceReset();
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////
// CExtAnimCutsceneDisableClothEvent
///////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CExtAnimCutsceneDisableClothEvent );

CExtAnimCutsceneDisableClothEvent::CExtAnimCutsceneDisableClothEvent()
	: CExtAnimEvent()
	, m_blendTime( 1.f )
	, m_weight( 0.f )
{
	m_reportToScript = false;
}

CExtAnimCutsceneDisableClothEvent::CExtAnimCutsceneDisableClothEvent( const CName& eventName, const CName& animationName, Float startTime, const String& trackName )
	: CExtAnimEvent( eventName, animationName, startTime, trackName )
	, m_blendTime( 1.f )
	, m_weight( 0.f )
{
	m_reportToScript = false;
}

CExtAnimCutsceneDisableClothEvent::~CExtAnimCutsceneDisableClothEvent()
{
}

void CExtAnimCutsceneDisableClothEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	ASSERT( component );
	if ( CActor* a = Cast<CActor>( component->GetEntity() ) )
	{
#ifdef USE_APEX
		for ( EntityWithItemsComponentIterator< CClothComponent > it( a ); it; ++it )
		{
			CClothComponent* c = *it;
			c->SetMaxDistanceBlendTime( m_blendTime );
			c->SetMaxDistanceScale( 1.f-m_weight );
		}
#endif
	}
	else
	{
#ifdef USE_APEX
		for ( ComponentIterator< CClothComponent > it( component->GetEntity() ); it; ++it )
		{
			CClothComponent* c = *it;
			c->SetMaxDistanceBlendTime( m_blendTime );
			c->SetMaxDistanceScale( 1.f-m_weight );
		}
#endif
	}
}


IMPLEMENT_ENGINE_CLASS( CExtAnimCutsceneDisableDangleEvent );

void CExtAnimCutsceneDisableDangleEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	if ( CActor* actor = Cast<CActor>( component->GetEntity() ) )
	{
		for ( EntityWithItemsComponentIterator< CAnimDangleComponent > it( actor ); it; ++it )
		{
			CAnimDangleComponent* c = *it;
			c->SetBlendToAnimationWeight( m_weight );
		}
	}
}
