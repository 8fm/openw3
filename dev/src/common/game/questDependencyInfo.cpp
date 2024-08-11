/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#ifndef NO_EDITOR

#include "QuestDependencyInfo.h"
#include "questGraph.h"
#include "..\core\garbageCollector.h"
#include "..\core\feedback.h"
#include "questPauseConditionBlock.h"
#include "questCondition.h"
#include "questTriggerCondition.h"
#include "questConditionBlock.h"

using namespace QuestDependencyInfo;

void QuestDependencyInfo::Start( CQuestGraph* questGraph, TDynArray< CQuestGraphBlock* > blocks, String folderToDepFiles )
{
	String questName = questGraph->GetFriendlyName();
	blocks = blocks;
	TDynArray<StringAnsi> myText;

	myText.PushBack("CREATE TABLE IF NOT EXISTS QuestGraphBlocks (id INTEGER PRIMARY KEY, name);\n");
	myText.PushBack("CREATE TABLE IF NOT EXISTS PauseConditionBlocks (id INTEGER PRIMARY KEY, name, conditions, triggerTag, actorTag, isInside);\n");

	// File to save the data
	String pathQuest = folderToDepFiles;
	if ( !GFileManager->FileExist( pathQuest.AsChar() ) )
	{
		if( !GFileManager->CreatePath( pathQuest.AsChar() ) )
		{
			RED_LOG( Session, TXT("ERROR: Unable to create output folder '%ls'."), pathQuest.AsChar() );
		}
	}
	// Write the file here 
	FILE* nFile; 
	String depFilePath = pathQuest + questName.AsChar() + TXT(".dep");
	if ( GFileManager->FileExist( depFilePath.AsChar() ) )
	{
		GFileManager->DeleteFileW( depFilePath.AsChar() );
	}

	Uint32 garbageCounter = 0;
	Uint32 blockSize = blocks.Size();
	// Get all the blocks from here

	// Progressbar for the generation of dep files
	GFeedback->BeginTask( TXT("Creating Dependency file"), false );
	GFeedback->UpdateTaskProgress( 0, blockSize );
	
	// Write the data here into the file
	nFile = _wfopen( depFilePath.AsChar(), TXT("a") );

	for ( Uint32 i = 0; i < blockSize; ++i )
	{
		myText.PushBack( "insert or ignore into QuestGraphBlocks (name) values ('" );

		TDynArray< String > dbStrings;
		dbStrings.PushBack( blocks[i]->GetBlockName() );
		// Making a proper db string out of all the data that we gathered

		myText.PushBack( MakeDBString( dbStrings ) );

		// If this is a pause condition block we add it to its own table
		CQuestPauseConditionBlock* pauseBlock = Cast< CQuestPauseConditionBlock >(blocks[i]);
		
		if ( pauseBlock )
		{
			TDynArray< IQuestCondition* > conditions = pauseBlock->GetConditions();
			TDynArray< String > conditionNames;
			TDynArray< String > descriptions;
			String triggerTag = TXT("");
			String actorTag = TXT("");
			String isInside = TXT("");
			for ( Uint32 j = 0; j < conditions.Size(); ++j )
			{
				if ( conditions[j] )
				{
					if ( CQuestEnterTriggerCondition* enterTrigger = Cast<CQuestEnterTriggerCondition>(conditions[j]) )
					{
						conditionNames.PushBack( enterTrigger->GetName().AsString() );
						triggerTag = enterTrigger->GetTriggerTag().AsString();
						actorTag = enterTrigger->GetActorTag().AsString();
					}
					else if ( CQuestInsideTriggerCondition* insideTrigger = Cast<CQuestInsideTriggerCondition>(conditions[j]) )
					{
						conditionNames.PushBack( insideTrigger->GetName().AsString() );
						triggerTag = insideTrigger->GetTriggerTag().AsString();
						actorTag = insideTrigger->GetActorTag().AsString();
						isInside = ToString( insideTrigger->IsInside() );
					}
				}
			}
			String conditionsString = String::Join( conditionNames, TXT(", ") );

			myText.PushBack( "insert or ignore into PauseConditionBlocks (name, conditions, triggerTag, actorTag, isInside) values ('" );

			TDynArray< String > dbqStrings;
			pauseBlock->GetBlockName();

			dbqStrings.PushBack( blocks[i]->GetBlockName() );
			dbqStrings.PushBack( conditionsString );
			dbqStrings.PushBack( triggerTag );
			dbqStrings.PushBack( actorTag );
			dbqStrings.PushBack( isInside );
			// Making a proper db string out of all the data that we gathered
			myText.PushBack( MakeDBString( dbqStrings ) );
		}

		CQuestConditionBlock* conditionBlock = Cast< CQuestConditionBlock >(blocks[i]);
		if ( conditionBlock )
		{
			IQuestCondition* condition = conditionBlock->GetCondition();
			TDynArray< String > conditionNames;
			TDynArray< String > descriptions;
			String triggerTag = TXT("");
			String actorTag = TXT("");
			String isInside = TXT("");

			if ( condition )
			{
				if ( CQuestEnterTriggerCondition* enterTrigger = Cast<CQuestEnterTriggerCondition>(condition) )
				{
					conditionNames.PushBack( enterTrigger->GetName().AsString() );
					triggerTag = enterTrigger->GetTriggerTag().AsString();
					actorTag = enterTrigger->GetActorTag().AsString();
				}
				else if ( CQuestInsideTriggerCondition* insideTrigger = Cast<CQuestInsideTriggerCondition>(condition) )
				{
					conditionNames.PushBack( insideTrigger->GetName().AsString() );
					triggerTag = insideTrigger->GetTriggerTag().AsString();
					actorTag = insideTrigger->GetActorTag().AsString();
					isInside = ToString( insideTrigger->IsInside() );
				}
			}

			String conditionsString = String::Join( conditionNames, TXT(", ") );

			myText.PushBack( "insert or ignore into PauseConditionBlocks (name, conditions, triggerTag, actorTag, isInside) values ('" );

			TDynArray< String > dbqStrings;
			conditionBlock->GetBlockName();

			dbqStrings.PushBack( blocks[i]->GetBlockName() );
			dbqStrings.PushBack( conditionsString );
			dbqStrings.PushBack( triggerTag );
			dbqStrings.PushBack( actorTag );
			dbqStrings.PushBack( isInside );
			//dbqStrings.PushBack( descriptionsString );
			// Making a proper db string out of all the data that we gathered
			myText.PushBack( MakeDBString( dbqStrings ) );
		}

		garbageCounter++;

		if ( garbageCounter>500 )
		{
			SGarbageCollector::GetInstance().CollectNow();
			garbageCounter = 0;
		}

		
		if ( nFile != NULL )
		{
			for( Uint32 l=0; l < myText.Size(); ++l )
			{
				fputs( myText[l].AsChar(), nFile );
			}
		}

		myText.Clear();
		GFeedback->UpdateTaskProgress( i, blockSize );
	
	}
	fclose(nFile);
	GFeedback->EndTask();
}

void QuestDependencyInfo::MakeNiceString( String& myString )
{
	myString.ReplaceAll( TXT("'"), TXT("@") );
	myString.ReplaceAll( TXT("\n"), TXT("#") );
	myString.Trim();
}

TDynArray< StringAnsi > QuestDependencyInfo::MakeDBString( TDynArray< String > dbStrings )
{
	TDynArray<StringAnsi> myText;
	for( Uint32 i=0; i<dbStrings.Size(); ++i )
	{
		MakeNiceString(dbStrings[i]);
		myText.PushBack( UNICODE_TO_ANSI( dbStrings[i].AsChar() ) );
		if( i == dbStrings.Size()-1 )
		{
			myText.PushBack( "');\n" );
		}
		else
		{
			myText.PushBack( "','" );
		}
	}
	return myText;
}

#endif