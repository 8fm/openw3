/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "animatedAttachment.h"
#include "animationEvent.h"

//////////////////////////////////////////////////////////////////////////
// PTom TODO
// This is placeholder!!!
// Functions have to be refactored!!!
//////////////////////////////////////////////////////////////////////////

template< class _EventType >
class CEventNotifier;

class CEntity;

class IAnimatedObjectInterface
{
public:
	virtual CEntity* GetAnimatedObjectParent() const = 0;

	virtual Bool HasSkeleton() const = 0;

	virtual Bool HasTrajectoryBone() const = 0;
	virtual Int32 GetTrajectoryBone() const = 0;

	virtual Bool UseExtractedMotion() const = 0;
	virtual Bool UseExtractedTrajectory() const = 0;

	virtual Int32 GetBonesNum() const = 0;
	virtual Int32 GetTracksNum() const = 0;

	virtual Int32 GetParentBoneIndex( Int32 bone ) const = 0;
	virtual const Int16* GetParentIndices() const = 0;

	virtual CEventNotifier< CAnimationEventFired >*	GetAnimationEventNotifier( const CName &eventName ) = 0;

	virtual void PlayEffectForAnimation( const CName& animation, Float time ) = 0;

	virtual Int32 GetAttPrio() const { return 0; }

	virtual void OnParentUpdatedAttachedAnimatedObjectsLS( Float /*dt*/, SBehaviorGraphOutput* /*poseLS*/, const BoneMappingContainer& /*bones*/ ) {}
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSAsync( Float /*dt*/, SBehaviorGraphOutput* /*poseLS*/, const BoneMappingContainer& /*bones*/ ) {}
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSSync( Float /*dt*/, SBehaviorGraphOutput* /*poseLS*/, const BoneMappingContainer& /*bones*/ ) {}
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSConstrainted( Float /*dt*/, SBehaviorGraphOutput* /*poseLS*/, const BoneMappingContainer& /*bones*/ ) {}
	virtual void OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* /*parent*/, SBehaviorGraphOutput* /*poseLS*/, TDynArray< Matrix >* /*poseMS*/, TDynArray< Matrix >* /*poseWS*/, const BoneMappingContainer& /*bones*/ ) {}
};
