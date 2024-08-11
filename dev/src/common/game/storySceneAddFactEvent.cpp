#include "build.h"
#include "storySceneAddFactEvent.h"
#include "factsDB.h"
#include "storyScenePlayer.h"
#include "../engine/gameTimeManager.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneAddFactEvent );

CStorySceneAddFactEvent::CStorySceneAddFactEvent() 
	: m_expireTime( 2 )
	, m_factValue( 1 )
{
}

CStorySceneAddFactEvent::CStorySceneAddFactEvent( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName ) : CStorySceneEvent( eventName, sceneElement, startTime, trackName )
{
}


CStorySceneAddFactEvent* CStorySceneAddFactEvent::Clone() const
{
	return new CStorySceneAddFactEvent( *this );
}

void CStorySceneAddFactEvent::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const 
{
	CFactsDB* factsDB = GCommonGame ? GCommonGame->GetSystem< CFactsDB >() : NULL;
	if ( scenePlayer->IsSceneInGame() && factsDB )
	{
		factsDB->AddFact( m_factId, m_factValue, GGame->GetEngineTime(), m_expireTime );

		scenePlayer->DbFactAdded( m_factId );
	}
}
