
#include "build.h"
#include "storySceneEventScenePropPlacement.h"
#include "storyScenePlayer.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneEventScenePropPlacement );

CStorySceneEventScenePropPlacement::CStorySceneEventScenePropPlacement()
	: CStorySceneEvent()
	, m_rotationCyclesPitch( 0 )
	, m_rotationCyclesRoll( 0 )
	, m_rotationCyclesYaw( 0 )
	, m_showHide( true )
{

}

CStorySceneEventScenePropPlacement::CStorySceneEventScenePropPlacement( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const String& trackName, const CName& id, const EngineTransform& placement )
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_placement( placement )
	, m_rotationCyclesPitch( 0 )
	, m_rotationCyclesRoll( 0 )
	, m_rotationCyclesYaw( 0 )
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

CStorySceneEventScenePropPlacement* CStorySceneEventScenePropPlacement::Clone() const
{
	return new CStorySceneEventScenePropPlacement( *this );
}

void CStorySceneEventScenePropPlacement::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	if ( m_propId )
	{
		Matrix sceneL2W, propLS;
		scenePlayer->GetSceneDirector()->GetCurrentScenePlacement().CalcLocalToWorld( sceneL2W );	
		m_placement.CalcLocalToWorld( propLS );			

		StorySceneEventsCollector::PropPlacement evt( this, m_propId );
		evt.m_eventTimeAbs = timeInfo.m_timeAbs;
		evt.m_eventTimeLocal = timeInfo.m_timeLocal;
		evt.m_placementWS = EngineTransform( propLS * sceneL2W );
		collector.AddEvent( evt );

		StorySceneEventsCollector::PropVisibility vis( this, m_propId );
		vis.m_showHide = m_showHide;
		collector.AddEvent( vis );
	}
}

#ifndef NO_EDITOR

void CStorySceneEventScenePropPlacement::OnPreviewPropertyChanged( const CStoryScenePlayer* previewPlayer, const CName& propertyName )
{
	TBaseClass::OnPreviewPropertyChanged( previewPlayer, propertyName );
}

#endif

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
