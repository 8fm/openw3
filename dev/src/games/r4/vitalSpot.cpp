#include "build.h"
#include "vitalSpot.h"
#include "journalCreature.h"

IMPLEMENT_ENGINE_CLASS( CVitalSpot );
IMPLEMENT_ENGINE_CLASS( CVitalSpotsParam );
IMPLEMENT_ENGINE_CLASS( SVitalSpotEnableConditions );

RED_DEFINE_STATIC_NAME( journalCreaturVitalSpotsPath );
RED_DEFINE_STATIC_NAME( vitalSpotEntry );

#ifndef NO_EDITOR

CVitalSpot*  CVitalSpotsParam::FindVitalSpotWithTag( CName uniqueScriptIdentifier )
{
	for ( auto i = m_vitalSpots.Begin() ; i != m_vitalSpots.End(); ++i)
	{
		if( (*i) && (*i)->m_vitalSpotEntry )
		{
			if( (*i)->m_editorLabel == uniqueScriptIdentifier)
			{
				return *i;
			}
		}
	}
	return NULL;
}

Bool CVitalSpotsParam::OnPropModified( CName fieldName )
{
	if( fieldName == CNAME(journalCreaturVitalSpotsPath) )
	{
		if( m_journalCreaturVitalSpotsPath && m_journalCreaturVitalSpotsPath->GetTarget() )
		{
			CJournalCreatureVitalSpotGroup * group = Cast<CJournalCreatureVitalSpotGroup>( m_journalCreaturVitalSpotsPath->GetTarget() );
			if( group )
			{
				TDynArray<CVitalSpot*> newSpots;
				for( Uint32 i = 0; i < group->GetNumChildren() ; ++i )
				{
					CJournalCreatureVitalSpotEntry * entry = Cast<CJournalCreatureVitalSpotEntry>( group->GetChild(i) );
					CName spotId = entry->GetUniqueScriptIdentifier();
					CVitalSpot* spotExist = FindVitalSpotWithTag( spotId );
					if( spotExist )
					{
						newSpots.PushBack( spotExist );
					}
					else
					{
						CVitalSpot * vs = CreateObject<CVitalSpot>( this );
						vs->m_vitalSpotEntry = CJournalPath::ConstructPathFromTargetEntry(entry, NULL );
						vs->m_editorLabel = entry->GetUniqueScriptIdentifier();
						newSpots.PushBack(vs);
					}					
				}
				m_vitalSpots = newSpots;
			}			
			return true;
		}		
	}
	else if( fieldName == CNAME(vitalSpotEntry) )
	{
		for( auto i = m_vitalSpots.Begin(); i!= m_vitalSpots.End(); ++i )
		{
			if( (*i) && (*i)->m_vitalSpotEntry && (*i)->m_vitalSpotEntry->GetTarget())
			{
				(*i)->m_editorLabel = (*i)->m_vitalSpotEntry->GetTarget()->GetUniqueScriptIdentifier();
			}
		}
		return true;
	}
	return false;
}

#endif

void CVitalSpot::funcGetJournalEntry( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_OBJECT( m_vitalSpotEntry ? m_vitalSpotEntry->GetTarget() : NULL );
}
