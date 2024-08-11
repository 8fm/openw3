#include "build.h"

#include "../../common/game/questGraphSocket.h"

#include "commonMapManager.h"
#include "questManageFastTravelBlock.h"
#include "../../common/engine/graphConnectionRebuilder.h"


//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CQuestManageFastTravelBlock )
IMPLEMENT_RTTI_ENUM( EQuestManageFastTravelOperation )
//////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( OnManageFastTravelAreas )
RED_DEFINE_STATIC_NAME( OnManageFastTravelPoints )

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestManageFastTravelBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif

void CQuestManageFastTravelBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	CCommonMapManager* manager = GCommonGame ? GCommonGame->GetSystem< CCommonMapManager >(): 0;
	if ( manager )
	{
		if ( m_affectedAreas.Size() )
		{
			manager->CallEvent( CNAME( OnManageFastTravelAreas ), m_operation, m_enable, m_show, m_affectedAreas );
		}
		if ( m_affectedFastTravelPoints.Size() )
		{
			manager->CallEvent( CNAME( OnManageFastTravelPoints ), m_operation, m_enable, m_show, m_affectedFastTravelPoints );
		}
	}

	ActivateOutput( data, CNAME( Out ) );
}
