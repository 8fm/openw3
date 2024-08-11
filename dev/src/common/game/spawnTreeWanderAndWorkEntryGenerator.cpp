#include "build.h"

#include "spawnTreeWanderAndWorkEntryGenerator.h"

IMPLEMENT_ENGINE_CLASS( CWanderAndWorkEntryGenerator )
IMPLEMENT_ENGINE_CLASS( CWorkEntryGenerator )
IMPLEMENT_ENGINE_CLASS( SWanderAndWorkEntryGeneratorParams )
IMPLEMENT_ENGINE_CLASS( SWanderHistoryEntryGeneratorParams )
IMPLEMENT_ENGINE_CLASS( SWorkWanderSmartAIEntryGeneratorParam )
IMPLEMENT_ENGINE_CLASS( SWorkEntryGeneratorParam )
IMPLEMENT_ENGINE_CLASS( SWorkSmartAIEntryGeneratorNodeParam )

void CWanderAndWorkEntryGenerator::OnPropertyPostChange( IProperty* property )
{
	if ( property->GetName() == TXT("entries") )
	{
		for( Int32 i=0; i<m_entries.SizeInt(); ++i )
		{
			m_entries[ i ].m_creatureEntry.m_parent = this;
		}
	}
}

void CWorkEntryGenerator::OnPropertyPostChange( IProperty* property )
{
	if ( property->GetName() == TXT("entries") )
	{
		for( Int32 i=0; i<m_entries.SizeInt(); ++i )
		{
			m_entries[ i ].m_creatureEntry.m_parent = this;
		}
	}
}