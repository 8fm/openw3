#include "build.h"
#include "behaviorAnimationMultiplyEntity.h"
#include "animatedComponent.h"
#include "componentIterator.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorAnimationMultiplyEntity );

CBehaviorAnimationMultiplyEntity::CBehaviorAnimationMultiplyEntity()
	: m_multiplier ( 1.0f )
{
}

void CBehaviorAnimationMultiplyEntity::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	for ( ComponentIterator< CAnimatedComponent > it( this ); it; ++it )
	{
		(*it)->SetAnimMultiplier( m_multiplier );
	}
}

