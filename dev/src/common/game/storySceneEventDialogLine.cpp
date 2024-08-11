
#include "build.h"
#include "storySceneEventDialogLine.h"
#include "storySceneLine.h"
#include "storyScenePlayer.h"
#include "storySceneEventsCollector.h"
#include "storySceneEventsCollector_events.h"
#include "storySceneLine.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneEventDialogLine );

CStorySceneEventDialogLine::CStorySceneEventDialogLine(void) 
	: CStorySceneEvent()
{

}

CStorySceneEventDialogLine::CStorySceneEventDialogLine( const String & eventName, CStorySceneElement* sceneElement, Float startTime, const String& trackName, CStorySceneLine* l )
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_line( l )
{

}

CStorySceneEventDialogLine* CStorySceneEventDialogLine::Clone() const
{
	return new CStorySceneEventDialogLine( *this );
}

void CStorySceneEventDialogLine::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	if ( m_line && m_line->GetVoiceTag() )
	{
		StorySceneEventsCollector::PlayDialogLine evt( this, m_line->GetVoiceTag() );
		evt.m_eventTimeLocal = timeInfo.m_timeLocal;
		evt.m_eventTimeAbs = timeInfo.m_timeAbs;

		evt.m_line = m_line;
		evt.m_display = scenePlayer->GetSceneDisplay();

		collector.AddEvent( evt );
	}
}

String CStorySceneEventDialogLine::GetLineText() const 
{ 
	return m_line ? m_line->GetContent() : String::EMPTY; 
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
