#include "build.h"

#include "behTreeNodeRandom.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeProbabilitySelectorDefinition
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeProbabilitySelectorDefinition::CanAddChild() const
{
	return m_children.Size() < MAX_CHILDS;
}

void CBehTreeNodeProbabilitySelectorDefinition::AddChild( IBehTreeNodeDefinition* node )
{
	ASSERT( m_children.Size() < MAX_CHILDS );
	TBaseClass::AddChild( node );
}


IBehTreeNodeCompositeInstance* CBehTreeNodeProbabilitySelectorDefinition::SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}
void CBehTreeNodeProbabilitySelectorDefinition::CustomPostInstanceSpawn( IBehTreeNodeCompositeInstance* instance ) const
{
	static_cast< Instance* >( instance )->CalculateOverallProbabilityInternal();
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeProbabilitySelectorInstance
////////////////////////////////////////////////////////////////////////

CBehTreeNodeProbabilitySelectorInstance::CBehTreeNodeProbabilitySelectorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeCompositeInstance( def, owner, context, parent )
	, m_testAvailability( def.m_testAvailability )
	, m_probability0( def.m_probability0 )
	, m_probability1( def.m_probability1 )
	, m_probability2( def.m_probability2 )
	, m_probability3( def.m_probability3 )
	, m_probability4( def.m_probability4 )
	, m_probability5( def.m_probability5 )
	, m_overallProbability( 0xffff )
{
}

//! Internal function run by definition after probabilities are all set
void CBehTreeNodeProbabilitySelectorInstance::CalculateOverallProbabilityInternal()
{
	// TODO: change spawning mechanism
	Uint32 n = m_children.Size();
	m_overallProbability = m_probability0;
	if ( n >= 2 )
	{
		m_overallProbability += m_probability1;
		if ( n >= 3 )
		{
			m_overallProbability += m_probability2;
			if ( n >= 4 )
			{
				m_overallProbability += m_probability3;
				if ( n >= 5 )
				{
					m_overallProbability += m_probability4;
					if ( n >= 6 )
					{
						m_overallProbability += m_probability5;
					}
				}
			}
		}
	}

	if ( m_overallProbability == 0 )
		m_overallProbability = 1;
}

void CBehTreeNodeProbabilitySelectorInstance::SelectChild( Int16 random, Bool* choices )
{
	if( m_testAvailability )
	{
		Uint32 n = m_children.Size();

		if ( ( n == 1 || ( choices[0] && (random -= m_probability0) < 0 ) ) )
		{
			m_activeChild = 0;
		}
		else
		{
			if ( ( n == 2 || ( choices[1] && (random -= m_probability1) < 0 ) ) )
			{
				m_activeChild = 1;
			}
			else
			{
				if ( ( n == 3 || ( choices[2] && (random -= m_probability2) < 0 ) ) )
				{
					m_activeChild = 2;
				}
				else
				{
					if ( ( n == 4 || ( choices[3] && (random -= m_probability3) < 0 ) ) )
					{
						m_activeChild = 3;
					}
					else
					{
						if ( ( n == 5 || ( choices[4] && (random -= m_probability4) < 0 ) ) )
						{
							m_activeChild = 4;
						}
						else
						{
							if( choices[5] )
							{
								m_activeChild = 5;
							}
							else
							{
								m_activeChild = INVALID_CHILD;
							}
						}
					}
				}
			}
		}
	}
	else
	{
		m_activeChild = INVALID_CHILD;
	}
}

void CBehTreeNodeProbabilitySelectorInstance::SelectChild( Int16 random )
{
	Uint32 n = m_children.Size();

	if ( n == 1 || (random -= m_probability0) < 0 )
	{
		m_activeChild = 0;
	}
	else
	{
		if ( n == 2 || (random -= m_probability1) < 0 )
		{
			m_activeChild = 1;
		}
		else
		{
			if ( n == 3 || (random -= m_probability2) < 0 )
			{
				m_activeChild = 2;
			}
			else
			{
				if ( n == 4 || (random -= m_probability3) < 0 )
				{
					m_activeChild = 3;
				}
				else
				{
					if ( n == 5 || (random -= m_probability4) < 0 )
					{
						m_activeChild = 4;
					}
					else
					{
						m_activeChild = 5;
					}
				}
			}
		}
	}
}

Bool CBehTreeNodeProbabilitySelectorInstance::Activate()
{
	if( m_testAvailability )
	{
		Uint16 probability = m_overallProbability;
		Bool choices[ Definition::MAX_CHILDS ];	
		for( Uint32 i = 0; i < m_children.Size(); ++i )
		{
			choices[ i ] = true;
		}

		Int16 random = 0;
		while( probability > 0 )
		{
			// Selection
			random = GEngine->GetRandomNumberGenerator().Get< Uint16 >( probability );
			SelectChild( random, choices );

			// Test		
			if( m_children[m_activeChild]->IsAvailable() )
			{
				// Found the right one
				break;
			}
			
			// Remove invalid child
			switch( m_activeChild )
			{
			case 0:
				probability -= m_probability0;
				break;
			case 1:
				probability -= m_probability1;
				break;
			case 2:
				probability -= m_probability2;
				break;
			case 3:
				probability -= m_probability3;
				break;
			case 4:
				probability -= m_probability4;
				break;
			case 5:
				probability -= m_probability5;
				break;
			}
			choices[ m_activeChild ] = false;
			m_activeChild = INVALID_CHILD;
		}
	}
	else
	{
		SelectChild( GEngine->GetRandomNumberGenerator().Get< Int16 >( m_overallProbability ) );
	}

	if ( m_activeChild != INVALID_CHILD && m_children[ m_activeChild ]->Activate() )
	{
		return Super::Activate();
	}

	DebugNotifyActivationFail();
	return false;
}