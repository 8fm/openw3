#include "build.h"
#include "questSpawnNotStreamedBoatBlock.h"
#include "r4CreateEntityManager.h"
#include "../../common/engine/game.h"
#include "../../common/engine/tagManager.h"
#include "../../common/engine/idTagManager.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/layerInfo.h"
#include "../../common/game/boatComponent.h"

#ifndef NO_EDITOR_GRAPH_SUPPORT
    #include "../../common/engine/graphConnectionRebuilder.h"
    #include "../../common/game/questGraphSocket.h"
#endif
#include "../../common/physics/physicsWrapper.h"
#include "../../common/engine/world.h"
#include "../../common/physics/physicsWorld.h"

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CQuestSpawnNotStreamedBoatBlock );

//////////////////////////////////////////////////////////////////////////

void CNotStreamedBoatSpawnEventHandler::OnPostAttach( CEntity* entity )
{
	// Set this boat as spawned from quest block
	CBoatComponent* boatComponent = entity->FindComponent<CBoatComponent>();
	if( boatComponent != nullptr )
	{
		boatComponent->m_isSpawnedFromQuestBlock = true;
	}
	
	// Change layer
    CLayer* attachLayer = nullptr;

    // Find target layer by tag
    if( m_spawnLayerTag != CName::NONE )
        attachLayer = GGame->GetActiveWorld()->FindLayerByTag( m_spawnLayerTag );
    
    // If not found or no tag get dynamic layer
    if( attachLayer == nullptr )
    {
        ASSERT( false, TXT("Layer not found, attaching to CDynamicLayer") );
        attachLayer = GGame->GetActiveWorld()->GetDynamicLayer();
    }
    // Check it found layer is dynamic quest layer
    else
    {
        if( attachLayer->GetLayerInfo()->GetLayerType() != LT_NonStatic )
        {
            ASSERT( false, TXT("Found layer is a static layer, attaching to CDynamicLayer") );
            attachLayer = GGame->GetActiveWorld()->GetDynamicLayer();
        }
        else if( attachLayer->GetLayerInfo()->GetLayerBuildTag() != LBT_Quest )
        {
            ASSERT( false, TXT("Found layer is not a quest layer, attaching to CDynamicLayer") );
            attachLayer = GGame->GetActiveWorld()->GetDynamicLayer();
        }
    }


    CLayer* removeLayer = entity->GetLayer();

    if( removeLayer == nullptr || attachLayer == nullptr )
        return;

    if( attachLayer == removeLayer )
        return;

    // Swap layers
    removeLayer->RemoveEntity( entity );
    attachLayer->AddEntity( entity );
}

//////////////////////////////////////////////////////////////////////////

CQuestSpawnNotStreamedBoatBlock::CQuestSpawnNotStreamedBoatBlock(void)
    : m_forceNonStreamed( false )
    , m_spawnLayerTag()
    , m_spawnPointTag()
{
    m_name = TXT( "Spawn Not Streamed Boat" );
}

//////////////////////////////////////////////////////////////////////////

CQuestSpawnNotStreamedBoatBlock::~CQuestSpawnNotStreamedBoatBlock(void)
{
}

//////////////////////////////////////////////////////////////////////////

#ifndef NO_EDITOR_GRAPH_SUPPORT
String CQuestSpawnNotStreamedBoatBlock::GetCaption() const
{
    return String::Printf( TXT("%s (Boat)"), m_name.AsChar() );
}

//////////////////////////////////////////////////////////////////////////

void CQuestSpawnNotStreamedBoatBlock::OnRebuildSockets()
{
    GraphConnectionRebuilder rebuilder( this );
    TBaseClass::OnRebuildSockets();

    CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
    CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}
#endif


//////////////////////////////////////////////////////////////////////////

void CQuestSpawnNotStreamedBoatBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
    TBaseClass::OnActivate( data, inputName, parentThread );

    CTagManager* tagMgr = GGame->GetActiveWorld()->GetTagManager();
    TDynArray< CEntity* > entities;
    tagMgr->CollectTaggedEntities( m_spawnPointTag, entities );
    
    CR4CreateEntityManager *const createEntityManager	= GR4Game->GetR4CreateEntityManager();

    if( entities.Size() != 0 )
    {
        CEntity *const spawnEntity				= entities[ 0 ];
        const Matrix & spawnPointWorldMatrix	= spawnEntity->GetLocalToWorld();

        static IdTag idTag = GGame->GetIdTagManager()->GetReservedId( RESERVED_TAG_ID_INDEX_GERALT_BOAT );
        CR4CreateEntityHelper *const createEntityHelper = new CR4CreateEntityHelper();
        createEntityManager->SpawnAliasEntityToPosition( createEntityHelper, spawnPointWorldMatrix, TXT("boat"), idTag, m_tagsToSet, new CNotStreamedBoatSpawnEventHandler( m_spawnLayerTag ), true, m_forceNonStreamed );
    }

    ActivateOutput( data, CNAME( Out ) );	
}

//////////////////////////////////////////////////////////////////////////