#include "build.h"

#include "journalQuestBlock.h"

#include "r4JournalManager.h"

#include "commonMapManager.h"

// Temporary
#include "../../common/game/journalBlock.h"
#include "../../common/engine/graphConnectionRebuilder.h"


IMPLEMENT_ENGINE_CLASS( CJournalQuestBlock );

CJournalQuestBlock::CJournalQuestBlock()
	: m_questEntry( NULL )
	, m_enableAutoSave( true )
{
	m_name = TXT( "Quest" );
}

CJournalQuestBlock::~CJournalQuestBlock()
{

}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CJournalQuestBlock::GetBlockAltName() const
{
	if( m_questEntry )
	{
		return m_questEntry->GetPathAsString();
	}

	return GetBlockName();
}

Color CJournalQuestBlock::GetClientColor() const
{
	if ( m_questEntry )
	{
		if ( !m_questEntry->IsValid() )
		{
			return Color( 255, 70, 70 );
		}
	}
	return Color( 243, 172, 172 );
}

Color CJournalQuestBlock::GetTitleColor() const
{
	if ( m_questEntry )
	{
		if ( !m_questEntry->IsValid() )
		{
			return Color( 255, 70, 70 );
		}
	}
	return TBaseClass::GetTitleColor();
}

String CJournalQuestBlock::GetCaption() const
{
	if ( m_questEntry )
	{
		const CJournalBase* target = m_questEntry->GetTarget();
		if( target )
		{
			return target->GetFriendlyName();
		}
	}
	return m_name;
}

void CJournalQuestBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	//TODO: Remove Hack

	// Create inputs
	GraphSocketSpawnInfo info( ClassID< CQuestGraphSocket >() );

	info.m_name = CNAME( Activate );
	info.m_direction = LSD_Input;
	info.m_placement = LSP_Left;
	info.m_isMultiLink = (info.m_direction == LSD_Input);
	CreateSocket( info );

	info.m_name = CNAME( Deactivate );
	info.m_direction = LSD_Input;
	info.m_placement = LSP_Left;
	info.m_isMultiLink = (info.m_direction == LSD_Input);
	CreateSocket( info );

	info.m_name = CNAME( Success );
	info.m_direction = LSD_Input;
	info.m_placement = LSP_Left;
	info.m_isMultiLink = (info.m_direction == LSD_Input);
	CreateSocket( info );

	info.m_name = CNAME( Failure );
	info.m_direction = LSD_Input;
	info.m_placement = LSP_Left;
	info.m_isMultiLink = (info.m_direction == LSD_Input);
	CreateSocket( info );

	// Create output

	info.m_name = CNAME( Out );
	info.m_direction = LSD_Output;
	info.m_placement = LSP_Right;
	info.m_isMultiLink = (info.m_direction == LSD_Input);
	CreateSocket( info );
}

#endif

#ifndef NO_EDITOR
String CJournalQuestBlock::GetSearchCaption() const
{
	String pathString;
	String typeString = TXT("<unknown>");

	const CJournalPath* path = m_questEntry;
	if ( path )
	{
		if ( path->IsValid() )
		{
			pathString = path->GetPathAsString();
			const CObject* target = path->GetTarget();
			if ( target )
			{
				const CClass* typeClass = target->GetClass();
				if ( typeClass )
				{
					typeString = typeClass->GetName().AsString();
				}
			}
		}
		else
		{
			pathString = TXT("<invalid>");
		}
	}
	else
	{
		pathString = TXT("<undefined>");
	}

	String socketsStr;

	const TDynArray< CGraphSocket* >& sockets = GetSockets();
	for ( Uint32 i = 0; i < sockets.Size(); ++i )
	{
		if ( sockets[ i ]->GetDirection() == LSD_Input )
		{
			if ( sockets[ i ]->HasConnections() )
			{
				if ( !socketsStr.Empty() )
				{
					socketsStr += TXT(" ");
				}
				socketsStr += sockets[ i ]->GetName().AsString();
			}
		}
	}
	return String::Printf( TXT("Quest, type: [%ls], sockets: [%ls], path: [%ls], guid: [%X-%X-%X-%X]"), typeString.AsChar(), socketsStr.AsChar(), pathString.AsChar(), m_guid.guid[ 0 ], m_guid.guid[ 1 ], m_guid.guid[ 2 ], m_guid.guid[ 3 ] );
}
#endif

void CJournalQuestBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	ASSERT( m_questEntry && m_questEntry->IsValid() );

	if( !m_questEntry )
	{
		LOG_R4( TXT( "Null Journal Quest path" ) );
		return;
	}

	if( !m_questEntry || !m_questEntry->IsValid() )
	{
		LOG_R4( TXT( "Invalid Journal Quest path: \"%s\" (\"%s\")" ), m_questEntry->GetPathAsString().AsChar(), m_questEntry->GetFilePaths().AsChar() );
		return;
	}

	/////////////////////////////////////////
	//
	// TTP 110940 & 110941
	//
	RED_LOG( RED_LOG_CHANNEL( SucceededQuest ), TXT("CJournalQuestBlock %X-%X-%X-%X"), m_guid.guid[ 0 ], m_guid.guid[ 1 ], m_guid.guid[ 2 ], m_guid.guid[ 3 ] );
	//
	//
	//
	/////////////////////////////////////////

	CWitcherJournalManager* manager = GCommonGame->GetSystem< CWitcherJournalManager >();

	if( inputName == CNAME( Activate ) )
	{
		manager->ActivateEntry( m_questEntry, JS_Active, !m_showInfoOnScreen );

		if ( m_track )
		{
			const CJournalQuest* quest = m_questEntry->GetEntryAs< CJournalQuest >();
			if ( quest )
			{
				eQuestType questType = quest->GetType();
				if ( questType == QuestType_Story || questType == QuestType_Chapter )
				{
					if ( manager->CanTrackQuest( quest ) )
					{
						manager->TrackQuest( quest->GetGUID(), false );
						
						/*	TO BE DECIDED:
							for now this feature is disabled, I'm waiting on a final decision on this (after a few reviews I guess)
						if ( m_enableAutoSave )
						{
							GCommonGame->RequestAutoSave( TXT("CJournalQuestBlock/TrackQuest"), false );
						}  */
					}
				}
			}

			if ( manager->IsTrackedQuest( quest ) )
			{
				const CJournalQuestObjective* objective = manager->GetHighlightedObjective();
				if ( objective && manager->GetEntryStatus( objective ) != JS_Active )
				{
					// if there is highlighted objective but it's not active anymore, highlight currently activated objective
					const CJournalQuestObjective* objective = m_questEntry->GetEntryAs< CJournalQuestObjective >();
					if ( objective )
					{
					    manager->HighlightObjective( objective );

						/*	TO BE DECIDED:
							for now this feature is disabled, I'm waiting on a final decision on this (after a few reviews I guess)
						if ( m_enableAutoSave )
						{
							GCommonGame->RequestAutoSave( TXT("CJournalQuestBlock/HighlightObjective"), false );
						}  */
					}
				}
			}
		}
	}
	else if ( inputName == CNAME( Deactivate ) )
	{
		manager->ActivateEntry( m_questEntry, JS_Inactive, !m_showInfoOnScreen );
	}
	else if( inputName == CNAME( Success ) )
	{
		manager->ActivateEntry( m_questEntry, JS_Success, !m_showInfoOnScreen );
        DeleteMapPinStates();
	}
	else if( inputName == CNAME( Failure ) )
	{
		manager->ActivateEntry( m_questEntry, JS_Failed, !m_showInfoOnScreen );
        DeleteMapPinStates();
	}

	CCommonMapManager* commonMapManager = GCommonGame->GetSystem< CCommonMapManager >();
	if ( commonMapManager )
	{
		commonMapManager->InvalidateQuestMapPinData();
	}

	ActivateOutput( data, CNAME( Out ) );
}

Bool CJournalQuestBlock::IsValid() const
{
	// can't be null & can't be invalid
	return m_questEntry && m_questEntry->IsValid();
}

void CJournalQuestBlock::DeleteMapPinStates() const
{
    // removes map pin states in map pin manager

   	ASSERT( m_questEntry && m_questEntry->IsValid() );

   	CWitcherJournalManager* journalManager = GCommonGame->GetSystem< CWitcherJournalManager >();
    if (journalManager)
    {
        EJournalStatus status = journalManager->GetEntryStatus(m_questEntry);
        if ( status == JS_Success || status == JS_Failed )
        {
            // only if entry status is failed or succeeded
            CJournalBase* target = m_questEntry->GetTarget();
            if ( target )
            {
                DeleteMapPinStatesFromEntry( target );
            }
        }
    }
}

void CJournalQuestBlock::DeleteMapPinStatesFromEntry( const CJournalBase* entry ) const
{
    if ( entry == NULL )
    {
        return;
    }

    if ( entry->IsA< CJournalQuest >() ||
	     entry->IsA< CJournalQuestPhase >()	||
		 entry->IsA< CJournalQuestObjective >() )
    {
        const CJournalContainer* container = Cast< CJournalContainer >( entry );
        if ( container )
        {
            for ( Uint32 i = 0; i < container->GetNumChildren(); i++ )
            {
                DeleteMapPinStatesFromEntry( container->GetChild( i ) );
            }
        }
    }
    else if ( entry->IsA< CJournalQuestMapPin >() )
    {
        const CJournalQuestMapPin* mapPin = Cast<CJournalQuestMapPin>( entry );
        if ( mapPin )
        {
			CCommonMapManager* commonMapPinManager  = GCommonGame->GetSystem< CCommonMapManager >();
			if ( commonMapPinManager )
			{
				commonMapPinManager->DeleteJournalQuestMapPin( mapPin->GetParentGUID(), mapPin->GetGUID() );
			}
        }
    }
}
