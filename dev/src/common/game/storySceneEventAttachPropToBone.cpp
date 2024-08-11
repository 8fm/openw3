#include "build.h"
#include "storySceneEventAttachPropToBone.h"
#include "storyScenePlayer.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneEventAttachPropToSlot )

CStorySceneEventAttachPropToSlot::CStorySceneEventAttachPropToSlot() 
	: m_activate( true )
	, m_showHide( true )
	, m_snapAtStart( true )	
{
}

CStorySceneEventAttachPropToSlot::CStorySceneEventAttachPropToSlot( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName id, const String& trackName )
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName ), m_actorName( id ), m_activate( true )
	, m_showHide( true )
	, m_snapAtStart( true )
{
	if( CStorySceneSection* section = sceneElement->GetSection() )
	{
		if( CStoryScene* scene =  section->GetScene() )
		{
			if( scene->GetPropDefinition( id ) )
			{
				m_propId = id;
			}
		}
	}	
}

CStorySceneEventAttachPropToSlot* CStorySceneEventAttachPropToSlot::Clone() const
{
	return new CStorySceneEventAttachPropToSlot( *this );
}

void CStorySceneEventAttachPropToSlot::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const 
{
	if ( m_propId && m_propId != m_actorName )
	{
		Bool skipped = scenePlayer->DidJustSkipElement() ;

		StorySceneEventsCollector::AttachPropToBone evt( this, m_propId );
		evt.m_isAttached = m_activate;
		evt.m_snapAtStart = skipped ? true : m_snapAtStart;
		evt.m_targetSlot = m_slotName;
		evt.m_targetEntity = m_actorName;
		evt.m_offset = m_offset;		
		collector.AddEvent( evt );

		if ( scenePlayer->GetStoryScene()->GetPropDefinition( m_actorName ) || scenePlayer->GetCurrentStoryScene()->GetPropDefinition( m_actorName ) )
		{
			StorySceneEventsCollector::PropVisibility visEvt( this, m_actorName );		
			if( skipped && !m_activate )
			{
				visEvt.m_showHide = false;
			}
			else
			{
				visEvt.m_showHide = m_showHide;
			}		
			collector.AddEvent( visEvt );
		}		

		StorySceneEventsCollector::PropVisibility visEvt( this, m_propId );		
		if( skipped && !m_activate )
		{
			visEvt.m_showHide = false;
		}
		else
		{
			visEvt.m_showHide = m_showHide;
		}		
		collector.AddEvent( visEvt );
	}
}
