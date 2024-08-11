#include "build.h"

#include "../../common/game/questGraphSocket.h"

#include "questPlayAnimationBlock.h"
#include "../../common/engine/tagManager.h"
#include "../../common/engine/graphConnectionRebuilder.h"


IMPLEMENT_RTTI_ENUM( EPropertyAnimationOperation );

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CQuestPlayAnimationBlock )

//////////////////////////////////////////////////////////////////////////

CQuestPlayAnimationBlock::CQuestPlayAnimationBlock()
	: m_operation( PAO_Play )
	, m_playCount( 0 )
	, m_playLengthScale( 1.0f )
	, m_playPropertyCurveMode( PCM_Forward )
	, m_rewindTime( 0.0f )
{
	m_name = TXT("Play animation");
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestPlayAnimationBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif

void CQuestPlayAnimationBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	CEntity* entity = GGame->GetActiveWorld()->GetTagManager()->GetTaggedEntity( m_entityTag );
	if ( entity && entity->IsA< CGameplayEntity >() )
	{
		CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( entity );
		if ( gameplayEntity )
		{
			switch ( m_operation )
			{
			case PAO_Play:
				gameplayEntity->PlayPropertyAnimation( m_animationName, m_playCount, m_playLengthScale, m_playPropertyCurveMode );
				break;
			case PAO_Stop:
				gameplayEntity->StopPropertyAnimation( m_animationName );
				break;
			case PAO_Rewind:
				gameplayEntity->RewindPropertyAnimation( m_animationName, m_rewindTime );
				break;
			case PAO_Pause:
				gameplayEntity->PausePropertyAnimation( m_animationName );
				break;
			case PAO_Unpause:
				gameplayEntity->UnpausePropertyAnimation( m_animationName );
				break;
			}
		}
	}

	ActivateOutput( data, CNAME( Out ) );
}
