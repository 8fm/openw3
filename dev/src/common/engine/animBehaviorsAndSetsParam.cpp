
#include "build.h"
#include "animBehaviorsAndSetsParam.h"

IMPLEMENT_ENGINE_CLASS( CAnimBehaviorsParam );

const TDynArray< SBehaviorGraphInstanceSlot >& CAnimBehaviorsParam::GetSlots() const
{
	return m_slots;
}

void CAnimBehaviorsParam::GetSlots( TDynArray< SBehaviorGraphInstanceSlot* >& slots )
{
	const Uint32 size = m_slots.Size();

	slots.Reserve( slots.Size() + size );
	
	for ( Uint32 i=0; i<size; ++i )
	{
		slots.PushBack( &(m_slots[ i ] ) );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CAnimAnimsetsParam );

const TDynArray< THandle< CSkeletalAnimationSet > >& CAnimAnimsetsParam::GetAnimationSets() const
{
	return m_animationSets;
}
