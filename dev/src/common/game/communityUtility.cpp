/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "communityUtility.h"
#include "../engine/layerInfo.h"
#include "../engine/layerGroup.h"
#include "../core/tokenizer.h"
#include "../engine/worldIterators.h"

void CCommunityUtility::FindLayersByNames( const TDynArray< CName > &layersNames, TDynArray< CLayer* > &foundLayers /* out */ )
{
	CWorld* world = GGame->GetActiveWorld();
	for ( WorldAttachedLayerIterator it( world ); it; ++it )
	{
		CLayer* layer = *it;
		if ( layer->GetLayerInfo() )
		{
			String depotPath = layer->GetLayerInfo()->GetDepotPath();
			// remove ".w2l" extension. This is not very nice, but is faster than looking for the last dot.
			// we take the whole depot path, so the spawnset is able to distinct two layers with same file names in different folders.
			depotPath = depotPath.LeftString( depotPath.Size() - 5 );
			for ( Uint32 j = 0; j < layersNames.Size(); ++j )
			{
				String layerToCheck = layersNames[ j ].AsString();
				layerToCheck.ToLower();
				if ( depotPath.EndsWith( layerToCheck ) )
				{
					foundLayers.PushBack( layer );
					break;
				}
			}
		}
	}
}

CLayerInfo* CCommunityUtility::GetCachedLayerInfo( const CName &layerName )
{
	PC_SCOPE( GetLayerInfo );

	CStaticTokenizer<256> tokenizerLW( layerName.AsString().AsChar(), TXT(";") );

	Char* layerChars = tokenizerLW.GetNextToken();
	Char* worldChars = tokenizerLW.GetNextToken();

	if( layerChars && worldChars )
	{
		//GGame->GetActiveWorld()->GetDepotPath

		String path;		                

		path = GGame->GetActiveWorld()->GetDepotPath( );
		String leftCurrentWorldPart;
		path.Split( TXT(".w2w"), &leftCurrentWorldPart, NULL );	

		String expectedWorldPath( worldChars );
		if( leftCurrentWorldPart != expectedWorldPath )
		{
			return nullptr;
		}

		CStaticTokenizer<256> tokenizer( layerChars, TXT("\\") );

		CLayerGroup *worldLayerGroup = GGame->GetActiveWorld()->GetWorldLayers();

		Char* lastGroupName = tokenizer.GetNextToken();
		while( true )
		{
			Char* currentGroupName = tokenizer.GetNextToken();
			if( currentGroupName == NULL )
				break;
			worldLayerGroup = worldLayerGroup->FindGroupCaseless( lastGroupName );
			if ( !worldLayerGroup ) return NULL; // subgroup not found
			lastGroupName = currentGroupName;
		}

		return worldLayerGroup->FindLayerCaseless( lastGroupName );

	}
	else
	{
		CStaticTokenizer<256> tokenizer( layerName.AsString().AsChar(), TXT("\\") );

		CLayerGroup *worldLayerGroup = GGame->GetActiveWorld()->GetWorldLayers();

		Char* lastGroupName = tokenizer.GetNextToken();
		while( true )
		{
			Char* currentGroupName = tokenizer.GetNextToken();
			if( currentGroupName == NULL )
				break;
			worldLayerGroup = worldLayerGroup->FindGroupCaseless( lastGroupName );
			if ( !worldLayerGroup ) return NULL; // subgroup not found
			lastGroupName = currentGroupName;
		}

		return worldLayerGroup->FindLayerCaseless( lastGroupName );
	}
}

String CCommunityUtility::GetFriendlyAgentStateName( ECommunityAgentState agentState )
{
	switch ( agentState )
	{
	case CAS_Unknown:
		return TXT("Unknown");
	case CAS_MovingToActionPoint:
		return TXT("Moving to AP");
	case CAS_WorkInProgress:
		return TXT("Work in progress");
	case CAS_Despawning:
		return TXT("Despawning");
	case CAS_NoAPFound:
		return TXT("No AP Found");
	case CAS_ReadyToWork:
		return TXT("Ready to work");
	case CAS_ToDespawn:
		return TXT("To despawn");
	case CAS_InitAfterCreated:
		return TXT("Init after created");
	case CAS_AcquireNextAP:
		return TXT("Acquire next AP for work");
	case CAS_Spawned:
		return TXT("Agent is spawned");
	default:
		return TXT("Error: Unknown spawnset actor state.");
	}
}

String CCommunityUtility::GetFriendlyFindAPStateDescription( EFindAPResult apResult )
{
	switch( apResult )
	{
	case FAPR_Success:
		return TXT("Action point has been found.");
	case FAPR_NoFreeAP:
		return TXT("There is at least one good AP, but none is free.");
	case FAPR_NoCategoryAP:
		return TXT("There is no AP with specified category.");
	case FAPR_LayerNotLoaded:
		return TXT("Layer exists, but it's not loaded.");
	case FAPR_LayerNotFound:
		return TXT("Cannot find layer with specified path (maybe casing or typographic error).");
	case FAPR_TimetabEmpty:
		return TXT("Cannot find AP because timetable is empty.");
	case FAPR_TimetabEmptyCategory:
		return TXT("Timetable exists, but it has empty action category field.");
	case FAPR_NoCandidates:
		return TXT("No AP candidates found.");
	case FAPR_Default:
		return TXT("No find AP call has been made.");
	case FAPR_UnknownError:
		return TXT("Unknown error.");
	default:
		return TXT("Enum not recognized. Update code dude!");
	}
}
