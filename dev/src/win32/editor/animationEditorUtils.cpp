
#include "build.h"
#include "animationEditorUtils.h"
#include "../../common/core/dependencySaver.h"
#include "../../common/engine/behaviorGraphContext.h"
#include "../../common/engine/dynamicLayer.h"

namespace AnimationEditorUtils
{
	void SetActorsItemsVisible( CEntity* entity )
	{
		CActor* actor = Cast< CActor >( entity ); 
		if ( actor && actor->GetInventoryComponent() )
		{
			actor->InitInventory();
			actor->GetInventoryComponent()->SpawnMountedItems();
		}
	}

	void SetActorsMeshRepresentation( CEntity* entity )
	{
		CActor* actor = Cast< CActor >( entity ); 
		if ( actor && actor->GetMovingAgentComponent() )
		{
			actor->GetMovingAgentComponent()->ForceEntityRepresentation( true );
		}
	}

	CAnimatedComponent* CloneEntity( const CAnimatedComponent* component )
	{
		TDynArray< Uint8 > buffer;
		CMemoryFileWriter writer( buffer );
		CDependencySaver saver( writer, NULL );

		CEntity* prototype = component->GetEntity();

		DependencySavingContext context( prototype );
		if ( !saver.SaveObjects( context ) )
		{
			return NULL;
		}

		LayerEntitiesArray pastedEntities;

		CLayer* layer = prototype->GetLayer()->GetWorld()->GetDynamicLayer();
		layer->PasteSerializedEntities( buffer, pastedEntities, true, Vector( 0.0f, 0.0f, 0.0f ), EulerAngles( 0.0f, 0.0f, 0.0f ) );

		ASSERT( pastedEntities.Size() == 1 );
		if ( pastedEntities.Size() == 1 )
		{
			CEntity* entity = pastedEntities[ 0 ];

			CAnimatedComponent* newComp = entity->FindComponent< CAnimatedComponent >( component->GetName() );
			ASSERT( newComp );

			return newComp;
		}

		return NULL;
	}

	Bool SyncComponentsPoses( const CAnimatedComponent* componentSrc, CAnimatedComponent* componentDest )
	{
		if ( !componentSrc || !componentDest || !componentSrc->GetBehaviorGraphSampleContext() || !componentDest->GetBehaviorGraphSampleContext() )
		{
			return false;
		}

		componentDest->ForceBehaviorPose( componentSrc->GetBehaviorGraphSampleContext()->GetSampledPose() );

		return true;
	}
};
