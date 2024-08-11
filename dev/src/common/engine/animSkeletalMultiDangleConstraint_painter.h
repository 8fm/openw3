
#pragma once

#include "animSkeletalMultiDangleConstraint.h"

class CAnimSkeletalMultiDangleConstraint_Painter : public CAnimSkeletalMultiDangleConstraint
{
	DECLARE_ENGINE_CLASS( CAnimSkeletalMultiDangleConstraint_Painter, CAnimSkeletalMultiDangleConstraint, 0 );

protected:
	TDynArray< Int32 >									m_dangleRootBones_skDangle;
	TDynArray< TPair< Int32, TDynArray< Int32 > > >		m_dangleChildrenBones_skDangle;
	TDynArray< Int32 >									m_dangleParentBones_skMain;

public:
	virtual void OnAttached( CWorld* world );

	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );

	virtual void OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones );

protected:
	virtual void CreateConstraints( const TDynArray< Int32 >& dangleParentBones_skeletonMain, const TDynArray< Int32 >& dangleRootBones_skeletonDangle, const TDynArray< TPair< Int32, TDynArray< Int32 > > >& dangleChildrenBones_skeletonDangle );
};

BEGIN_CLASS_RTTI( CAnimSkeletalMultiDangleConstraint_Painter );
	PARENT_CLASS( CAnimSkeletalMultiDangleConstraint );
END_CLASS_RTTI();
