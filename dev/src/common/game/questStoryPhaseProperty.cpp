#include "build.h"
#include "questStoryPhaseProperty.h"


IMPLEMENT_ENGINE_CLASS( IQuestSpawnsetAction )
IMPLEMENT_ENGINE_CLASS( CActivateStoryPhase )
IMPLEMENT_ENGINE_CLASS( CDeactivateSpawnset )

///////////////////////////////////////////////////////////////////////////////

CActivateStoryPhase::CActivateStoryPhase()
	: m_spawnset( NULL )
	, m_streamingPartition( String::EMPTY )
{}

void CActivateStoryPhase::CollectContent( class IQuestContentCollector& collector ) const
{
	if ( !m_spawnset.GetPath().Empty() )
	{
		collector.CollectResource( m_spawnset.GetPath() );
	}
}


void CActivateStoryPhase::ResetCachedData() const
{
	CCommunity * spawnset = m_spawnset.Get();
	if(spawnset)
	{
		spawnset->ResetInternalData();
	}
}

void CActivateStoryPhase::Perform() const
{
	// Activate story phase
	CCommunity * spawnset = m_spawnset.Get();
	if(spawnset)
	{
		spawnset->ActivatePhase( m_phase );
	}
	else
	{
		ASSERT("Error retrieving community file when activating story phase (is file path correct ?)");
	}
}

void CActivateStoryPhase::GetSpawnsetPhaseNames( IProperty *property, THashSet<CName> &outNames )
{
	if ( property->GetName() == TXT( "phase" ))
	{
		CCommunity * spawnset = m_spawnset.Get();
		if(spawnset)
		{
			spawnset->GetPhaseNames( outNames );
		}
		else
		{
			ASSERT("Error retrieving community file when activating story phase (is file path correct ?)");
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

CDeactivateSpawnset::CDeactivateSpawnset()
	: m_spawnset( NULL )
{}

void CDeactivateSpawnset::Perform() const
{
	if ( m_spawnset )
	{
		// Deactivate whole community
		m_spawnset->Deactivate( );
	}
}

void CDeactivateSpawnset::CollectContent( class IQuestContentCollector& collector ) const
{
	/*if ( !m_spawnset.GetPath().Empty() )
	{
		collector.CollectResource( m_spawnset.GetPath() );
	}*/
}

///////////////////////////////////////////////////////////////////////////////
