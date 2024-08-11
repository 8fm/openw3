#include "build.h"
#include "duplicatesFinder.h"
#include "../../common/engine/meshTypeComponent.h"
#include "../../common/engine/meshTypeResource.h"
#include "errorsListDlg.h"

void CDuplicatesFinder::FindDuplicates( TDynArray< String >& duplicatesOutInfo, const TDynArray< CLayerInfo* >& layers, Bool modifiedOnly /* = true */ )
{
	GFeedback->BeginTask( TXT("Looking for duplicates..."), true );
	Int32 total = layers.Size();
	Int32 current = 0;

	THashMap< DuplicateInfo, DuplicatesContainer > duplicatesChecker;
	for ( auto layerIt = layers.Begin(); layerIt != layers.End(); ++layerIt )
	{
		CLayerInfo* li = *layerIt;

		GFeedback->UpdateTaskProgress( ++current, total );
		GFeedback->UpdateTaskInfo( String::Printf( TXT("Looking for duplicates in %ls"), li->GetDepotPath().AsChar() ).AsChar() );

		if ( li->IsLoaded() && li->GetLayer() && ( !modifiedOnly || li->GetLayer()->IsModified() ) )
		{
			FindDuplicatesInLayer( li, duplicatesChecker );
		}

		if ( GFeedback->IsTaskCanceled() )
		{
			break;
		}
	}

	PrepareDuplicatesInfo( duplicatesOutInfo, duplicatesChecker );
	GFeedback->EndTask();
}

void CDuplicatesFinder::FindDuplicatesInLayer( const CLayerInfo* li, THashMap< DuplicateInfo, DuplicatesContainer >& duplicatesChecker )
{
	TDynArray< CEntity* > entities;
	li->GetLayer()->GetEntities( entities );

	String layerPath;
	li->GetHierarchyPath( layerPath, true );

	for ( CEntity* ent : entities )
	{
		if ( ent->GetEntityTemplate() )
		{
			HandleKey( ent->GetName(), ent->GetEntityTemplate()->GetDepotPath(), layerPath, ent->GetWorldPosition(), ent->GetWorldRotation(), duplicatesChecker, ent->GetEntityTemplate() );
		}

		SEntityStreamingState worldState;
		ent->PrepareStreamingComponentsEnumeration( worldState, true, SWN_DoNotNotifyWorld );
		ent->ForceFinishAsyncResourceLoads();

		TDynArray< CResource* > usedResources;
		ent->CollectUsedResources( usedResources );

		CEntityTemplate* entityTemplate = nullptr;

		for ( CResource* resource : usedResources )
		{
			if ( resource->IsA< CEntityTemplate >() )
			{
				entityTemplate = Cast< CEntityTemplate >( resource );
				break;
			}
		}

		for ( CComponent* comp : ent->GetComponents() )
		{
			if ( const CMeshTypeComponent* meshComp = Cast< CMeshTypeComponent >( comp ) )
			{
				if ( meshComp->GetMeshTypeResource() )
				{
					HandleKey( ent->GetName(), meshComp->GetMeshTypeResource()->GetDepotPath(), layerPath, meshComp->GetWorldPosition(), meshComp->GetWorldRotation(), duplicatesChecker, entityTemplate );
				}
			}
			else
			{
				TDynArray< const CResource* > resources;
				comp->GetResource( resources );

				for ( const CResource* res : resources )
				{
					if ( res )
					{
						HandleKey( ent->GetName(), res->GetDepotPath(), layerPath, comp->GetWorldPosition(), comp->GetWorldRotation(), duplicatesChecker, entityTemplate );
					}
				}
			}
		}
		ent->FinishStreamingComponentsEnumeration( worldState );
	}
}

void CDuplicatesFinder::HandleKey( const String& name, const String& path, const String& layerPath, const Vector& pos, const EulerAngles& angles, THashMap< DuplicateInfo, DuplicatesContainer >& duplicatesChecker, CEntityTemplate* entityTemplate )
{
	DuplicateInfo key( path, pos, angles, layerPath, entityTemplate );
	if ( DuplicatesContainer* duplicatesContainer = duplicatesChecker.FindPtr( key ) )
	{
		if ( Uint32* count = duplicatesContainer->FindPtr( name ) )
		{
			(*count)++;
		}
		else
		{
			duplicatesContainer->Insert( name, 1 );
		}
	}
	else
	{
		DuplicatesContainer tmp;
		tmp.Insert( name, 1 );
		duplicatesChecker.Insert( key, tmp );
	}
}

void CDuplicatesFinder::PrepareDuplicatesInfo( TDynArray< String >& duplicatesOutInfo, const THashMap< DuplicateInfo, DuplicatesContainer >& duplicatesChecker )
{
	CResource::FactoryInfo< C2dArray > info;
	C2dArray* outCSV = info.CreateResource();
	outCSV->AddColumn( TXT("Layer/template_path"), TXT("") );
	outCSV->AddColumn( TXT("World_position"), TXT("") );
	outCSV->AddColumn( TXT("Duplicated_resource's_depot_path"), TXT("") );
	outCSV->AddColumn( TXT("Node_name(s)"), TXT("") );

	Uint32 allCount = 0, duplicatedCount = 0, templateDuplicates = 0, layerDuplicates = 0;
	for ( THashMap< DuplicateInfo, DuplicatesContainer >::const_iterator it = duplicatesChecker.Begin(); it != duplicatesChecker.End(); ++it )
	{
		++allCount;
		const DuplicatesContainer& namesAndCounts = it->m_second;
		if ( namesAndCounts.Size() > 1 || ( namesAndCounts.Size() == 1 && namesAndCounts.Begin().Value() > 1 ) ) // more than 1 instance on the same spot (on a layer or in the same instance)
		{
			++duplicatedCount;
			if ( namesAndCounts.Size() > 1 )
			{
				for ( DuplicatesContainer::const_iterator namesAndCountsIt = namesAndCounts.Begin(); namesAndCountsIt != namesAndCounts.End(); ++namesAndCountsIt )
				{
					// duplication occurs inside of an entity template
					if ( namesAndCountsIt->m_second > 1 )
					{
						HandleTemplateInfo( duplicatesOutInfo, outCSV, it->m_first, namesAndCounts );
						++templateDuplicates;
					}
				}

				// duplication occurs on a layer
				HandleLayerInfo( duplicatesOutInfo, outCSV, it->m_first, namesAndCounts );
				++layerDuplicates;
			}
			else // duplication occurs inside of an entity template
			{
				HandleTemplateInfo( duplicatesOutInfo, outCSV, it->m_first, namesAndCounts );
				++templateDuplicates;
			}
		}
	}

	outCSV->SaveAs( GDepot, TXT("search_duplicates_results.csv"), true );
	duplicatesOutInfo.Insert( 0, String::Printf( TXT("From %d objects checked there are %d duplicates (%d inside entity templates, %d on layers)"), allCount, duplicatedCount, templateDuplicates, layerDuplicates ) );
}

void CDuplicatesFinder::HandleTemplateInfo( TDynArray< String >& duplicatesOutInfo, C2dArray* outCSV, const DuplicateInfo& info, const DuplicatesContainer& container )
{
	if ( info.m_entityTemplate && info.m_entityTemplate->GetFile() )
	{
		String pos = ToString( info.m_worldPos );
		String cameraString = String::Printf( TXT("[[ %s|0 0 0]]"), pos.AsChar() );
		String templatePath = info.m_entityTemplate->GetFile()->GetDepotPath();
		String msg = String::Printf( TXT( "Duplicate component %ls found in an entity template %ls in position [%ls]" ), info.m_depotPath.AsChar(), templatePath.AsChar(), pos.AsChar() );
		duplicatesOutInfo.PushBack( msg );

		TDynArray< String > rowData;
		rowData.PushBack( templatePath );
		rowData.PushBack( cameraString );
		rowData.PushBack( info.m_depotPath );
		outCSV->AddRow( rowData );
	}
	else
	{
		HandleLayerInfo( duplicatesOutInfo, outCSV, info, container );
	}
}

void CDuplicatesFinder::HandleLayerInfo( TDynArray< String >& duplicatesOutInfo, C2dArray* outCSV, const DuplicateInfo& info, const DuplicatesContainer& container )
{
	String pos = ToString( info.m_worldPos );
	String cameraString = String::Printf( TXT("[[ %s|0 0 0]]"), pos.AsChar() );
	String msg = String::Printf( TXT( "Duplicates found on layer %ls in position [%ls]: " ), info.m_layer.AsChar(), pos.AsChar() );
	
	TDynArray< String > rowData;
	rowData.PushBack( info.m_layer );
	rowData.PushBack( cameraString );
	rowData.PushBack( info.m_depotPath );

	TDynArray< String > names;
	container.GetKeys( names );
	String nodesString;
	for ( String name : names )
	{
		nodesString += name;
		nodesString += TXT(",");
	}

	rowData.PushBack( nodesString );
	outCSV->AddRow( rowData );

	msg += nodesString;
	msg += String::Printf( TXT(" (%ls)" ), info.m_depotPath.AsChar() );
	duplicatesOutInfo.PushBack( msg );
}
