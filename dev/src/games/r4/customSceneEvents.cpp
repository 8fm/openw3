#include "build.h"
#include "customSceneEvents.h"
#include "..\..\common\game\storyScenePlayer.h"
#include "..\..\common\game\commonGame.h"
#include "gameplayFXSurfacePost.h"


IMPLEMENT_ENGINE_CLASS( CStorySceneEventSurfaceEffect );
IMPLEMENT_RTTI_ENUM( ESceneEventSurfacePostFXType );

void CStorySceneEventSurfaceEffect::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const 
{
	Matrix placement;
	scenePlayer->GetSceneDirector()->GetCurrentScenePlacement().CalcLocalToWorld( placement );

	CGameplayFXSurfacePost* surfPPSys = GCommonGame->GetSystem< CGameplayFXSurfacePost >();
	surfPPSys->Init( Vector3( 0.5f,0.5f,1.0f) );
	surfPPSys->AddSurfacePostGroup( placement.TransformPoint( m_position ), m_fadeInTime, m_fadeOutTime, m_durationTime, m_radius, Uint32( m_type ) );	
}

CStorySceneEventSurfaceEffect* CStorySceneEventSurfaceEffect::Clone() const
{
	return new CStorySceneEventSurfaceEffect( *this );
}
