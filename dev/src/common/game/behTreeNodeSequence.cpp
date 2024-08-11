#include "build.h"
#include "behTreeNodeSequence.h"

#include "../core/gameSave.h"

#include "behTreeInstance.h"

BEHTREE_STANDARD_SPAWNCOMPOSITE_FUNCTION( CBehTreeNodeSequenceDefinition )
BEHTREE_STANDARD_SPAWNCOMPOSITE_FUNCTION( CBehTreeNodePersistantSequenceDefinition )
BEHTREE_STANDARD_SPAWNCOMPOSITE_FUNCTION( CBehTreeNodeSequenceFowardAndBackDefinition )

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeSequenceInstance
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeSequenceInstance::Activate()
{
	ASSERT( m_activeChild == INVALID_CHILD );
	if ( !m_children[ 0 ]->Activate() )
	{
		DebugNotifyActivationFail();
		return false;
	}
	m_activeChild = 0;
	return Super::Activate();
}
void CBehTreeNodeSequenceInstance::OnSubgoalCompleted( eTaskOutcome outcome )
{
	if ( outcome == BTTO_SUCCESS )
	{
		if ( ++m_activeChild < m_children.Size() )
		{
			if ( !m_children[ m_activeChild ]->Activate() )
			{
				m_activeChild = INVALID_CHILD;
				Complete( BTTO_FAILED );
			}
		}
		else
		{
			m_activeChild = INVALID_CHILD;
			Complete( BTTO_SUCCESS );
		}
	}
	else
	{
		m_activeChild = INVALID_CHILD;
		Complete( BTTO_FAILED );
	}
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodePersistantSequenceInstance
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodePersistantSequenceInstance::Activate()
{
	ASSERT( m_activeChild == INVALID_CHILD );
	if ( !m_children[ m_selectedChild ]->Activate() )
	{
		return false;
	}
	m_activeChild = m_selectedChild;
	return Super::Activate();
}
void CBehTreeNodePersistantSequenceInstance::OnSubgoalCompleted( eTaskOutcome outcome )
{
	if ( outcome == BTTO_SUCCESS )
	{
		// increase iterator
		if ( ++m_selectedChild >= m_children.Size() )
		{
			m_selectedChild = 0;
			m_activeChild = INVALID_CHILD;
			Complete( BTTO_SUCCESS );
		}
		// activate next child goal
		else if ( !m_children[ m_selectedChild ]->Activate() )
		{
			m_activeChild = INVALID_CHILD;
			Complete( BTTO_FAILED );
		}
		// successfully activated next child
		else
		{
			m_activeChild = m_selectedChild;
		}
	}
	else
	{
		m_activeChild = INVALID_CHILD;
		Complete( BTTO_FAILED );
	}
}

Bool CBehTreeNodePersistantSequenceInstance::IsSavingState() const
{
	return m_selectedChild != 0;
}
void CBehTreeNodePersistantSequenceInstance::SaveState( IGameSaver* writer )
{
	writer->WriteValue< Uint16 >( CNAME( selectedChild ), Uint16( m_selectedChild ) );
}
Bool CBehTreeNodePersistantSequenceInstance::LoadState( IGameLoader* reader )
{
	Uint16 selectedChild = 0;
	reader->ReadValue< Uint16 >( CNAME( selectedChild ), selectedChild );
	m_selectedChild = selectedChild;
	return true;
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeSequenceCheckAvailabilityDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeCompositeInstance* CBehTreeNodeSequenceCheckAvailabilityDefinition::SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	if ( m_updateCheckIsAvailable == false )
	{
		return new CBehTreeNodeSequenceCheckAvailabilityInstance( *this, owner, context, parent );
	}
	else
	{
		return new CBehTreeNodeSequenceCheckAvailAndUpdateInstance( *this, owner, context, parent );
	}
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeSequenceCheckAvailabilityDefinition
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeSequenceCheckAvailabilityInstance::Activate()
{
	for ( m_activeChild = 0; m_activeChild < m_children.Size(); ++m_activeChild )
	{
		if ( m_children[ m_activeChild ]->IsAvailable() )
		{
			if ( m_children[ m_activeChild ]->Activate() )
			{
				return Super::Activate();
			}
			else if ( !m_continueSequenceOnChildFailure )
			{
				DebugNotifyActivationFail();
				return false;
			}
		}
	}

	m_activeChild = INVALID_CHILD;
	DebugNotifyActivationFail();
	return false;
}
void CBehTreeNodeSequenceCheckAvailabilityInstance::OnSubgoalCompleted( eTaskOutcome outcome )
{
	if ( outcome == BTTO_FAILED && !m_continueSequenceOnChildFailure )
	{
		m_activeChild = INVALID_CHILD;
		Complete( BTTO_FAILED );
		return;
	}
	for ( ++m_activeChild; m_activeChild < m_children.Size(); ++m_activeChild )
	{
		if ( m_children[ m_activeChild ]->IsAvailable() )
		{
			if ( m_children[ m_activeChild ]->Activate() )
			{
				return;
			}
			else if ( !m_continueSequenceOnChildFailure )
			{
				m_activeChild = INVALID_CHILD;
				Complete( BTTO_FAILED );
				return;
			}
		}
	}
	m_activeChild = INVALID_CHILD;
	Complete( BTTO_SUCCESS );
}
Bool CBehTreeNodeSequenceCheckAvailAndUpdateInstance::Activate()
{
	return Super::Activate();
	m_nextTest = 0.0f;
}
/////////////////////////////////////////////////////////////////////
// CBehTreeNodeSequenceCheckAvailAndUpdateInstance
void CBehTreeNodeSequenceCheckAvailAndUpdateInstance::Update()
{
	const Float currentTime = m_owner->GetLocalTime();
	if ( m_nextTest < currentTime )
	{
		m_nextTest = currentTime + m_updateCheckIsAvailFreq;
		// check if current child is still available // if not try to deactivate (interrupt)
		if ( m_children[ m_activeChild ]->IsAvailable() == false && m_children[ m_activeChild ]->Interrupt() )
		{
			
			if ( m_activeChild == m_children.Size() - 1 )
			{
				m_activeChild = INVALID_CHILD;
				Complete( BTTO_SUCCESS );
			}
			else
			{			
				// then get the next available child and activate it :
				for ( Uint32 currentChild = m_activeChild + 1; currentChild < m_children.Size(); ++currentChild )
				{
					if ( m_children[ currentChild ]->IsAvailable() )
					{
						if ( m_children[ currentChild ]->Activate() )
						{
							m_activeChild = currentChild;
							break;
						}
						else if ( m_continueSequenceOnChildFailure )
						{
							// setting time to 0 to retest as soon as possible
							m_nextTest = 0.f;
						}
						else 
						{
							m_activeChild = INVALID_CHILD;
							Complete( BTTO_FAILED );
							return;
						}
					}
					else
					{
						// advancing active child because it has been ruled out :
						m_activeChild = currentChild;
					}
				}
			}
		}		
	}
	Super::Update();
}

/////////////////////////////////////////////////////////////////////
// CBehTreeNodeSequenceFowardAndBackInstance
/////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeSequenceFowardAndBackInstance::Activate()
{
	Uint32 child = 0;
	Uint32 childrenCount = m_children.Size();

	do 
	{
		if( !m_children[ child ]->IsAvailable() )
		{
			break;
		}
		m_sequencePoint = child;
	}
	while ( ++child < m_children.Size() );

	if ( child == 0 )
	{
		DebugNotifyActivationFail();
		return false;
	}

	m_nextCheck = m_owner->GetLocalTime() + m_checkFrequency;

	if ( m_children[ m_sequencePoint ]->Activate() )
	{
		m_activeChild = m_sequencePoint;
	}

	return IBehTreeNodeInstance::Activate();
}
void CBehTreeNodeSequenceFowardAndBackInstance::Update()
{
	Float time = m_owner->GetLocalTime();
	if ( m_nextCheck < time )
	{
		m_nextCheck = time + m_checkFrequency;

		// first try to move forward in sequence
		if ( m_sequencePoint < m_children.Size()-1 && m_children[ m_sequencePoint+1 ]->IsAvailable() )
		{
			do 
			{
				++m_sequencePoint;
			} while ( m_sequencePoint < m_children.Size()-1 && m_children[ m_sequencePoint+1 ]->IsAvailable() );
		}
		// now try to hold on current behaviors if above failed
		else
		{
			while( !m_children[ m_sequencePoint ]->IsAvailable() )
			{
				if ( m_sequencePoint == 0 )
				{
					Complete( BTTO_FAILED );
					return;
				}
				else
				{
					--m_sequencePoint;
				}
			}
		}

		if ( m_sequencePoint != m_activeChild )
		{
			if ( m_activeChild != INVALID_CHILD )
			{
				m_children[ m_activeChild ]->Deactivate();
			}

			if ( m_children[ m_sequencePoint ]->Activate() )
			{
				m_activeChild = m_sequencePoint;
			}
			else
			{
				m_activeChild = INVALID_CHILD;
			}
		}
	}

	if ( m_activeChild != INVALID_CHILD )
	{
		m_children[ m_activeChild ]->Update();
	}
}

Bool CBehTreeNodeSequenceFowardAndBackInstance::OnEvent( CBehTreeEvent& e )
{
	if ( m_activeChild != INVALID_CHILD )
	{
		if ( m_children[ m_activeChild ]->OnEvent( e ) )
		{
			m_nextCheck = 0.f;
			return true;
		}
	}
	return false;
}
void CBehTreeNodeSequenceFowardAndBackInstance::OnSubgoalCompleted( eTaskOutcome outcome )
{
	m_activeChild = INVALID_CHILD;

	switch ( outcome )
	{
	default:
	case BTTO_FAILED:
		
		Complete( BTTO_FAILED );
		break;
	case BTTO_SUCCESS:
		if ( m_sequencePoint >= m_children.Size()-1 )
		{
			Complete( BTTO_SUCCESS );
		}
		else
		{
			m_nextCheck = 0.f;
		}
		break;
	}
}

void CBehTreeNodeSequenceFowardAndBackInstance::MarkDirty()
{
	m_nextCheck = 0.f;
}
void CBehTreeNodeSequenceFowardAndBackInstance::MarkParentSelectorDirty()
{
	m_nextCheck = 0.f;
}