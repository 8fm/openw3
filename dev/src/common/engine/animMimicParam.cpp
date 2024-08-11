
#include "build.h"
#include "animMimicParam.h"
#include "behaviorGraphInstance.h"
IMPLEMENT_ENGINE_CLASS( CAnimMimicParam );

CAnimMimicParam::CAnimMimicParam()
{

}

CAnimMimicParam::CAnimMimicParam( const TDynArray< THandle< CSkeletalAnimationSet > >& sets, const TDynArray< SBehaviorGraphInstanceSlot >& instances )
	: m_animationSets( sets )
	, m_behaviorInstanceSlots( instances )
{

}

const TDynArray< THandle< CSkeletalAnimationSet > >& CAnimMimicParam::GetAnimsets() const
{
	return m_animationSets;
}

const TDynArray< SBehaviorGraphInstanceSlot >& CAnimMimicParam::GetInstanceSlots() const
{
	return m_behaviorInstanceSlots;
}
