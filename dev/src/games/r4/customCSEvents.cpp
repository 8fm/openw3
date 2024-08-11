#include "build.h"
#include "customCSEvents.h"
#include "gameplayFXSurfacePost.h"
#include "..\..\common\engine\cutsceneInstance.h"

IMPLEMENT_ENGINE_CLASS( CExtAnimCutsceneSurfaceEffect );

void CExtAnimCutsceneSurfaceEffect::Process( const CAnimationEventFired& info, CAnimatedComponent* component, CCutsceneInstance* cs ) const
{
	CGameplayFXSurfacePost* surfPPSys = GCommonGame->GetSystem< CGameplayFXSurfacePost >();
	surfPPSys->Init( Vector3( 0.5f,0.5f,1.0f) );

	surfPPSys->AddSurfacePostGroup( m_worldPos ? m_position : cs->GetCsPosition().TransformPoint( m_position ), m_fadeInTime, m_fadeOutTime, m_durationTime, m_radius, Uint32( m_type ) );		
}

IMPLEMENT_ENGINE_CLASS( CExtAnimCutsceneHideEntityEvent );

void CExtAnimCutsceneHideEntityEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component, CCutsceneInstance* cs ) const
{
	TDynArray< CEntity* > ents;

	if( CWorld* world = cs->GetLayer()->GetWorld() )
	{
		world->GetTagManager()->CollectTaggedEntities( m_entTohideTag, ents );
	}

	for ( CEntity* ent : ents )
	{
		cs->HideEntityForCsDuration( ent );
	}
}
