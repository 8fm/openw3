#include "build.h"
#include "TemplateInsertion.h"
#include "../../common/game/swarmLairEntity.h"
#include "../../common/game/boidActivationTriggerComponent.h"
#include "../../common/game/boidAreaComponent.h"
#include "../../games/r4/flyingCrittersLairEntity.h"
#include "../../games/r4/humbleCrittersLairEntity.h"

#include "../../common/core/depot.h"

////////////////////////////////////////////////////
// CTemplateInsertion
IMPLEMENT_ENGINE_CLASS( CTemplateInsertion );

// Create entity in world
CEntity* CTemplateInsertion::CreateEntity( const String& templateName, CLayer* layer, const Vector& spawnPosition, CResource* baseResource, Bool detachFromTemplate )const
{
	ASSERT( layer );

	// Spawn entity
	EntitySpawnInfo entitySpawnInfo;
	entitySpawnInfo.m_spawnPosition		= spawnPosition;
	entitySpawnInfo.m_resource			= baseResource;
	entitySpawnInfo.m_template			= LoadResource< CEntityTemplate >( templateName.AsChar() );
	entitySpawnInfo.m_detachTemplate	= detachFromTemplate;
	
	return layer->CreateEntitySync( entitySpawnInfo );
}

////////////////////////////////////////////////////
// CSwarmTemplateInsertion
IMPLEMENT_ENGINE_CLASS( CSwarmTemplateInsertion );
#define ON_TEMPLATE_INSERTED_ERROR() \
	lairEntity->Destroy();\
	areaEntity->Destroy();\
	triggerEntity->Destroy();\
	poiEntity1->Destroy();\
	poiEntity2->Destroy();\
	poiEntity3->Destroy();\
	poiEntity4->Destroy();\

class CSpeciesData
{
	String	m_species;
	String	m_template;
};
void CSwarmTemplateInsertion::OnTemplateInserted( const TemplateInfo *const templateInfo, const Vector& clickPosition, CLayer* layer, CResource* baseResource )
{
	const String separator			= TXT(".");	
	TDynArray< String > splitArray	= templateInfo->m_group.Split( separator );
	if ( splitArray.Size() == 0 )
	{
		return;
	}
	const String & speciesName = splitArray[ splitArray.Size() - 1 ];

	CName poiName1, poiName2, poiName3, poiName4;
	Float lairScale				= 50.0f;
	Float lairHeight			= 5.0f;
	Float triggerEntityZ		= -0.5f;
	Float poiEntityZ			= 0.5f;

	CEntity* lairEntity			= CreateEntity( templateInfo->m_templateName, layer, Vector( 0.0f, 0.0f, 0.0f ), baseResource, templateInfo->m_detachFromTemplate );
	if ( lairEntity->IsA<CFlyingCrittersLairEntity>( ) )
	{
		lairScale				= 100.0f;
		lairHeight				= 10.0f;
		triggerEntityZ			= -0.5f;
		poiEntityZ				= 0.5f;
	}
	else if ( lairEntity->IsA< CHumbleCrittersLairEntity >() )
	{
		lairScale				= 50.0f;
		lairHeight				= 5.0f;
		triggerEntityZ			= -0.5f;
		poiEntityZ				= 0.5f;
	}
	else
	{
		// Entity template is wrong !
		lairEntity->Destroy();
		return;
	}
	AddSwarmToLayer( speciesName, lairEntity, lairScale, lairHeight, triggerEntityZ, poiEntityZ , templateInfo, clickPosition, layer, baseResource );
}
void CSwarmTemplateInsertion::AddSwarmToLayer(	const String & speciesName, CEntity *const lairEntity,
												Float lairScale, Float lairHeight, Float triggerEntityZ, Float poiEntityZ,
												const TemplateInfo *const templateInfo, const Vector& clickPosition, CLayer* layer, CResource* baseResource )
{
	if ( templateInfo->m_templateName.Empty() )
	{
		return;
	}
	const String areaTemplate(TXT("engine\\templates\\editor\\boid_area.w2ent"));
	const String triggerTemplate(TXT("engine\\templates\\editor\\boid_activation_trigger.w2ent"));
	const String poiTemplate(TXT("engine\\templates\\editor\\boid_poi.w2ent"));

	const Vector lairEntityPosition		= clickPosition + Vector( 0.0f, 0.0f, 0.01f );
	const Vector areaEntityPosition		= clickPosition + Vector( 0.0f, 0.0, triggerEntityZ );
	const Vector triggerEntityPosition	= clickPosition + Vector( 0.0f, 0.0, triggerEntityZ );
	const Vector poiEntity1Position		= clickPosition + Vector( 0.1f, 0.1f, 0.01f ) * lairScale + Vector( 0.0f, 0.0f, poiEntityZ );
	const Vector poiEntity2Position		= clickPosition + Vector( -0.1f, 0.1f, 0.0f ) * lairScale + Vector( 0.0f, 0.0f, poiEntityZ );
	const Vector poiEntity3Position		= clickPosition + Vector( -0.1f, -0.1f, 0.0f ) * lairScale + Vector( 0.0f, 0.0f, poiEntityZ );
	const Vector poiEntity4Position		= clickPosition + Vector( 0.1f, -0.1f, 0.0f ) * lairScale + Vector( 0.0f, 0.0f, poiEntityZ );

	CEntity* areaEntity		= CreateEntity( areaTemplate, layer, areaEntityPosition, baseResource, templateInfo->m_detachFromTemplate );
	CEntity* triggerEntity	= CreateEntity( triggerTemplate, layer, triggerEntityPosition, baseResource, templateInfo->m_detachFromTemplate );
	CEntity* poiEntity1		= CreateEntity( poiTemplate, layer, poiEntity1Position, baseResource, templateInfo->m_detachFromTemplate );
	CEntity* poiEntity2		= CreateEntity( poiTemplate, layer, poiEntity2Position, baseResource, templateInfo->m_detachFromTemplate );
	CEntity* poiEntity3		= CreateEntity( poiTemplate, layer, poiEntity3Position, baseResource, templateInfo->m_detachFromTemplate );
	CEntity* poiEntity4		= CreateEntity( poiTemplate, layer, poiEntity4Position, baseResource, templateInfo->m_detachFromTemplate );
	lairEntity->SetPosition( lairEntityPosition );
	ComponentIterator< CBoidAreaComponent > areaIt( areaEntity );	
	if ( !areaIt )
	{
		RED_LOG_ERROR( RED_LOG_CHANNEL( Editor ), TXT("Swarm insertion error: Missing CBoidAreaComponent"));
		ON_TEMPLATE_INSERTED_ERROR();
		return;
	}
	CBoidAreaComponent* boidAreaComponent	= *areaIt;

	ComponentIterator< CBoidActivationTriggerComponent > triggerIt( triggerEntity );	
	if ( !triggerIt )
	{
		RED_LOG_ERROR( RED_LOG_CHANNEL( Editor ), TXT("Swarm insertion error: Missing CBoidActivationTriggerComponent"));
		ON_TEMPLATE_INSERTED_ERROR();
		return;
	}
	CBoidActivationTriggerComponent* boidActivationTriggerComponent	= *triggerIt;

	ComponentIterator< CBoidPointOfInterestComponent > poi1It( poiEntity1 );	
	if ( !poi1It )
	{
		RED_LOG_ERROR( RED_LOG_CHANNEL( Editor ), TXT("Swarm insertion error: Missing CBoidPointOfInterestComponent"));
		ON_TEMPLATE_INSERTED_ERROR();
		return;
	}
	CBoidPointOfInterestComponent* boidPointOfInterestComponent1	= *poi1It;

	ComponentIterator< CBoidPointOfInterestComponent > poi2It( poiEntity2 );	
	if ( !poi2It )
	{
		RED_LOG_ERROR( RED_LOG_CHANNEL( Editor ), TXT("Swarm insertion error: Missing CBoidPointOfInterestComponent"));
		ON_TEMPLATE_INSERTED_ERROR();
		return;
	}
	CBoidPointOfInterestComponent* boidPointOfInterestComponent2	= *poi2It;

	ComponentIterator< CBoidPointOfInterestComponent > poi3It( poiEntity3 );	
	if ( !poi3It )
	{
		RED_LOG_ERROR( RED_LOG_CHANNEL( Editor ), TXT("Swarm insertion error: Missing CBoidPointOfInterestComponent"));
		ON_TEMPLATE_INSERTED_ERROR();
		return;
	}
	CBoidPointOfInterestComponent* boidPointOfInterestComponent3	= *poi3It;

	ComponentIterator< CBoidPointOfInterestComponent > poi4It( poiEntity4 );	
	if ( !poi4It )
	{
		RED_LOG_ERROR( RED_LOG_CHANNEL( Editor ), TXT("Swarm insertion error: Missing CBoidPointOfInterestComponent"));
		ON_TEMPLATE_INSERTED_ERROR();
		return;
	}
	CBoidPointOfInterestComponent* boidPointOfInterestComponent4	= *poi4It;

	if ( lairEntity->IsA< CSwarmLairEntity >() == false || boidAreaComponent == nullptr || boidActivationTriggerComponent == nullptr 
		|| boidPointOfInterestComponent1 == nullptr || boidPointOfInterestComponent2 == nullptr || boidPointOfInterestComponent3 == nullptr || boidPointOfInterestComponent4 == nullptr)
	{
		RED_LOG_ERROR( RED_LOG_CHANNEL( Editor ), TXT("Unknown swarm insertion error: %s") );
		ON_TEMPLATE_INSERTED_ERROR();
		return;
	}

	CSwarmLairEntity *const swarmLairEntity = static_cast< CSwarmLairEntity* >( lairEntity );

	areaEntity->SetScale( Vector( lairScale, lairScale, lairHeight ) );
	triggerEntity->SetScale( Vector( lairScale * 1.5f, lairScale * 1.5f, lairHeight ) );
	boidActivationTriggerComponent->SetupFromTool( swarmLairEntity );	
	swarmLairEntity->SetupFromTool( areaEntity, CName( speciesName ) );

	const CBoidLairParams *const boidLairParams = GCommonGame->GetBoidSpecies()->GetParamsByName( CName( speciesName ) );
	Int32 i = 0;
	boidPointOfInterestComponent1->ChangePoiTypeFromTool( boidLairParams->m_spawnPointArray[  i ] );
	i = boidLairParams->m_spawnPointArray.Size() > 1 ? ++i : i;
	boidPointOfInterestComponent2->ChangePoiTypeFromTool( boidLairParams->m_spawnPointArray[ i ] );
	i = boidLairParams->m_spawnPointArray.Size() > 2 ? ++i : i;
	boidPointOfInterestComponent3->ChangePoiTypeFromTool( boidLairParams->m_spawnPointArray[ i ] );
	i = boidLairParams->m_spawnPointArray.Size() > 3 ? ++i : i;
	boidPointOfInterestComponent4->ChangePoiTypeFromTool( boidLairParams->m_spawnPointArray[ i ] );
}