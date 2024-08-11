#pragma once
#include "..\..\..\engine\behaviorGraphContext.h"

//////////////////////////////////////////////////////////////////////////
class CFootStepAction : public IAnimationPostAction
{
public:
	CFootStepAction() {}
	virtual void Process( CAnimatedComponent* ac, const SBehaviorGraphOutput& pose );

	Int32 m_boneIndex;
};