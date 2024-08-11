#include "build.h"

#include "../../common/game/questGraphSocket.h"

#include "manageSwitchBlock.h"
#include "../../common/engine/tagManager.h"
#include "../../common/engine/graphConnectionRebuilder.h"


//////////////////////////////////////////////////////////////////////////

IMPLEMENT_RTTI_ENUM( ESwitchOperation );
IMPLEMENT_ENGINE_CLASS( CManageSwitchBlock )

//////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( W3Switch )
RED_DEFINE_STATIC_NAME( OnManageSwitch )

CManageSwitchBlock::CManageSwitchBlock()
{
	m_name = TXT("Manage switch");
	m_force = true;
	m_skipEvents = true;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CManageSwitchBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif

void CManageSwitchBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	CEntity* entity = GGame->GetActiveWorld()->GetTagManager()->GetTaggedEntity( m_switchTag );
	if ( entity )
	{
		CClass* w3SwitchClass  = SRTTI::GetInstance().FindClass( CNAME( W3Switch ) );
		ASSERT( w3SwitchClass );
		if ( w3SwitchClass )
		{
			if ( entity->IsA( w3SwitchClass ) )
			{
				entity->CallEvent( CNAME( OnManageSwitch ), m_operations, m_force, m_skipEvents );
			}
		}
	}

	ActivateOutput( data, CNAME( Out ) );
}
