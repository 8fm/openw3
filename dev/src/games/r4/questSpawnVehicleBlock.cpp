#include "build.h"
#include "questSpawnVehicleBlock.h"
#include "../../common/game/questGraphSocket.h"
#include "../../common/game/createEntityHelper.h"
#include "../../common/engine/idTagManager.h"
#include "r4CreateEntityManager.h"
#include "../../common/engine/tagManager.h"
#include "../../common/engine/graphConnectionRebuilder.h"

IMPLEMENT_RTTI_ENUM( EVehicleType );
IMPLEMENT_ENGINE_CLASS( CQuestSpawnVehicleBlock );

RED_DEFINE_STATIC_NAME( OnPlayerHorseSummoned )

CQuestSpawnVehicleBlock::CQuestSpawnVehicleBlock()
{
	m_name = TXT( "Spawn Player's Vehicle" );
}

CQuestSpawnVehicleBlock::~CQuestSpawnVehicleBlock()
{

}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CQuestSpawnVehicleBlock::GetCaption() const
{
	String vehicle;
	switch ( m_vehicleType )
	{
	case EVT_Horse:
		vehicle = TXT("Horse");
		break;
	case EVT_Boat:
		vehicle = TXT("Boat");
		break;
	case EVT_Undefined:
		vehicle = TXT("Undefined");
		break;
	}
	return String::Printf( TXT("%s (%s)"), m_name.AsChar(), vehicle.AsChar() );
}

void CQuestSpawnVehicleBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}
#endif

void CQuestSpawnVehicleBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	CTagManager* tagMgr = GGame->GetActiveWorld()->GetTagManager();
	TDynArray< CEntity* > entities;
	tagMgr->CollectTaggedEntities( m_spawnPointTag, entities );

	CR4CreateEntityManager *const createEntityManager	= GR4Game->GetR4CreateEntityManager();

	if ( entities.Size() != 0 )
	{
		CEntity *const spawnEntity				= entities[ 0 ];
		const Matrix & spawnPointWorldMatrix	= spawnEntity->GetLocalToWorld();
		switch( m_vehicleType )
		{
		case EVT_Horse:
		{
			CR4CreateEntityHelper *const createEntityHelper = new CR4CreateEntityHelper();
			// this callback must be set because in scripts there is an additional logic which is performed after horse is summoned
			// that additional logic must be performed after summoning horse from quest block as well
			createEntityHelper->SetPostAttachedScriptCallback( GR4Game, CNAME( OnPlayerHorseSummoned ) );

			createEntityManager->SummonPlayerHorse( createEntityHelper, true, &spawnPointWorldMatrix );
			break;
		}
		case EVT_Boat:
		{
			static IdTag idTag = GGame->GetIdTagManager()->GetReservedId( RESERVED_TAG_ID_INDEX_GERALT_BOAT );
			CR4CreateEntityHelper *const createEntityHelper = new CR4CreateEntityHelper();
			createEntityManager->SpawnUniqueAliasVehicle( createEntityHelper, true, TXT("boat"), idTag, TDynArray<CName>(), &spawnPointWorldMatrix );
			break;
		}
		}
	}
	ActivateOutput( data, CNAME( Out ) );	
}
