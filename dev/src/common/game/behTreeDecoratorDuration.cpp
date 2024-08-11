#include "build.h"
#include "behTreeDecoratorDuration.h"

#include "behTreeInstance.h"


BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeDurationDecoratorDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeDurationRangeDecoratorDefinition )


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDurationInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeDurationDecoratorInstance::CBehTreeNodeDurationDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeDecoratorInstance( def, owner, context, parent )
	, m_duration( def.m_duration.GetVal( context ) )
	, m_chance( def.m_chance.GetVal( context ) )
	, m_endWithFailure( def.m_endWithFailure )
{}
Bool CBehTreeNodeDurationDecoratorInstance::Activate()
{
	m_delay = m_owner->GetLocalTime() + m_duration;

	return Super::Activate();
}
void CBehTreeNodeDurationDecoratorInstance::Update()
{
	if ( m_delay < m_owner->GetLocalTime() )
	{
		if ( GEngine->GetRandomNumberGenerator().Get< Float >() < m_chance )
		{
			Complete( m_endWithFailure ? BTTO_FAILED : BTTO_SUCCESS );
			return;
		}
		m_delay += m_duration;
	}
	Super::Update();
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDurationRangeDecoratorInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeDurationRangeDecoratorInstance::CBehTreeNodeDurationRangeDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_durationMin( def.m_durationMin.GetVal( context ) )
	, m_endWithFailure( def.m_endWithFailure )			
{
	Float durationMax = def.m_durationMax.GetVal( context );
	m_durationDiff = durationMax - m_durationMin;
}


Bool CBehTreeNodeDurationRangeDecoratorInstance::Activate()
{
	m_delay = m_owner->GetLocalTime() + m_durationMin + m_durationDiff * GEngine->GetRandomNumberGenerator().Get< Float >();

	return Super::Activate();
}
void CBehTreeNodeDurationRangeDecoratorInstance::Update()
{
	if ( m_delay < m_owner->GetLocalTime() )
	{
		Complete( m_endWithFailure ? BTTO_FAILED : BTTO_SUCCESS );
		return;
	}
	Super::Update();
}
