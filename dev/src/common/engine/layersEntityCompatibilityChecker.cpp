#include "build.h"
#include "layersEntityCompatibilityChecker.h"
#include "layerInfo.h"
#include "../core/dataError.h"
#include "layer.h"
#include "../core/feedback.h"



Bool CLayersEntityCompatibilityChecker::IsEntityCompatible( const CEntity* entity, const CLayerInfo* layerInfo, String& outInfo )
{
	if ( entity->IsA< CPeristentEntity >() && Cast< CPeristentEntity >( entity )->IsSavable() && layerInfo->IsEnvironment() )
	{
		String layerPath;
		layerInfo->GetHierarchyPath( layerPath, true );
		outInfo = String::Printf( TXT( "Persistent entity %ls shouldn't be placed on environment layer %ls" ), entity->GetName().AsChar(), layerPath.AsChar() );

		DATA_HALT( DES_Major, layerInfo->GetLayer(), TXT("Layer"), outInfo.AsChar() );
		
		return false;
	}

	return true;
}

Bool CLayersEntityCompatibilityChecker::CheckLayersEntitiesCompatibility( const CLayerInfo* layerInfo, TDynArray< String >& compatibilityErrors )
{
	compatibilityErrors.Clear();

	if ( layerInfo->IsLoaded() )
	{
		TDynArray< CEntity* > ents;
		layerInfo->GetLayer()->GetEntities( ents );

		for ( Uint32 entInd = 0; entInd < ents.Size(); ++entInd )
		{
			String error;
			if ( !IsEntityCompatible( ents[ entInd ], layerInfo, error ) )
			{
				compatibilityErrors.PushBack( error );
			}
		}
	}

	return compatibilityErrors.Empty();
}

Bool CLayersEntityCompatibilityChecker::CheckLayersEntitiesCompatibility( const TDynArray< CLayerInfo* >& layerInfos, TDynArray< String >& compatibilityErrors, Bool onlyModified )
{
	GFeedback->BeginTask( TXT( "Checking entities compatibility..." ), false );
	compatibilityErrors.Clear();

	Uint32 num = 0;
	for ( const CLayerInfo* li : layerInfos )
	{
		GFeedback->UpdateTaskProgress( ++num, layerInfos.Size() );
		if ( !onlyModified || ( li->GetLayer() && li->GetLayer()->IsModified() ) )
		{
			TDynArray< String > errors;
			if ( !CheckLayersEntitiesCompatibility( li, errors ) )
			{
				compatibilityErrors.PushBackUnique( errors );
			}
		}
	}

	GFeedback->EndTask();
	return compatibilityErrors.Empty();
}
