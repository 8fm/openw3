#include "build.h"
#include "journalQuestMappinStateBlock.h"
#include "../../common/game/questGraphSocket.h"
#include "../../common/game/journalPath.h"
#include "commonMapManager.h"
#include "journalQuest.h"
#include "../../common/engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CJournalQuestMappinStateBlock );

CJournalQuestMappinStateBlock::CJournalQuestMappinStateBlock()
    : m_mappinEntry( NULL )
	, m_enableOnlyIfLatest( false )
	, m_disableAllOtherMapPins( false )
{
	m_name = TXT("Map Pin State");
}

CJournalQuestMappinStateBlock::~CJournalQuestMappinStateBlock()
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

Color CJournalQuestMappinStateBlock::GetTitleColor() const
{
	if ( m_mappinEntry )
	{
		if ( !m_mappinEntry->IsValid() )
		{
			return Color( 255, 70, 70 );
		}
	}
	return TBaseClass::GetTitleColor();
}

Color CJournalQuestMappinStateBlock::GetClientColor() const
{
	if ( m_mappinEntry )
	{
		if ( !m_mappinEntry->IsValid() )
		{
			return Color( 255, 70, 70 );
		}
	}
	return Color( 243, 172, 172 );
}

String CJournalQuestMappinStateBlock::GetCaption() const
{
	if ( m_mappinEntry )
	{
		const CJournalBase* target = m_mappinEntry->GetTarget();
		if( target )
		{
			return target->GetFriendlyName();
		}
	}
	return m_name;
}

void CJournalQuestMappinStateBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Enable ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Disable ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

String CJournalQuestMappinStateBlock::GetBlockAltName() const
{
	if( m_mappinEntry )
	{
		return m_mappinEntry->GetPathAsString();
	}

	return GetBlockName();
}

#endif


void CJournalQuestMappinStateBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
    CJournalBase* entry = m_mappinEntry->GetTarget();

    if ( entry->IsA< CJournalQuestMapPin >() )
    {
        const CJournalQuestMapPin* questMapPin = Cast< CJournalQuestMapPin >( entry );
        if ( questMapPin )
        {
			CCommonMapManager* commonMapManager = GCommonGame->GetSystem< CCommonMapManager >();
			if ( commonMapManager )
			{
				if( inputName == CNAME( Enable ) )
				{
					Bool doEnable = true;

					if ( m_enableOnlyIfLatest )
					{
						CJournalQuestObjective* objective = Cast< CJournalQuestObjective >( questMapPin->GetParent() );
						if ( objective )
						{
							Bool checkIfEnabled = false;
							for ( Uint32 i = 0; i < objective->GetNumChildren(); ++i )
							{
								const CJournalQuestMapPin* otherMapPin = Cast< CJournalQuestMapPin >( objective->GetChild( i ) );
								if ( otherMapPin )
								{
									if ( checkIfEnabled )
									{
										if ( commonMapManager->IsJournalQuestMapPinEnabled( otherMapPin->GetParentGUID(), otherMapPin->GetGUID() ) )
										{
											doEnable = false;
											break;
										}
									}
									else if ( otherMapPin == questMapPin )
									{
										checkIfEnabled = true;
									}
								}
							}
						}
					}

					if ( doEnable )
					{
						commonMapManager->EnableJournalQuestMapPin( questMapPin->GetParentGUID(), questMapPin->GetGUID(), true);

						if ( m_disableAllOtherMapPins )
						{
							CJournalQuestObjective* objective = Cast< CJournalQuestObjective >( questMapPin->GetParent() );
							if ( objective )
							{
								for ( Uint32 i = 0; i < objective->GetNumChildren(); ++i )
								{
									const CJournalQuestMapPin* otherMapPin = Cast< CJournalQuestMapPin >( objective->GetChild( i ) );
									if ( otherMapPin )
									{
										if ( otherMapPin != questMapPin )
										{
											commonMapManager->EnableJournalQuestMapPin( otherMapPin->GetParentGUID(), otherMapPin->GetGUID(), false );
										}
									}
								}
							}
						}
					}
				}
				else if( inputName == CNAME( Disable ) )
				{
					commonMapManager->EnableJournalQuestMapPin( questMapPin->GetParentGUID(), questMapPin->GetGUID(), false);
				}
				commonMapManager->InvalidateQuestMapPinData();
			}
        }
    }

    ActivateOutput( data, CNAME( Out ) );
}

Bool CJournalQuestMappinStateBlock::IsValid() const
{
	// can't be null & can't be invalid
	return m_mappinEntry && m_mappinEntry->IsValid();
}
