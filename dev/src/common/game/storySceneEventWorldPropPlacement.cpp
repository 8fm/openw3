
#include "build.h"
#include "storySceneEventWorldPropPlacement.h"
#include "storyScenePlayer.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneEventWorldPropPlacement );

CStorySceneEventWorldPropPlacement::CStorySceneEventWorldPropPlacement()
	: CStorySceneEvent()
	, m_showHide( true )
{

}

CStorySceneEventWorldPropPlacement::CStorySceneEventWorldPropPlacement( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName id, const String& trackName )
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_showHide( true )
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


CStorySceneEventWorldPropPlacement* CStorySceneEventWorldPropPlacement::Clone() const 
{
	return new CStorySceneEventWorldPropPlacement( *this );
}

void CStorySceneEventWorldPropPlacement::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	if ( m_propId )
	{
		StorySceneEventsCollector::PropPlacement evt( this, m_propId );
		evt.m_eventTimeAbs = timeInfo.m_timeAbs;
		evt.m_eventTimeLocal = timeInfo.m_timeLocal;
		evt.m_placementWS = m_placement;
		collector.AddEvent( evt );

		StorySceneEventsCollector::PropVisibility vis( this, m_propId );
		vis.m_showHide = m_showHide;
		collector.AddEvent( vis );
	}
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
