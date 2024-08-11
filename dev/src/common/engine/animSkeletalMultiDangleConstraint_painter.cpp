
#include "build.h"
#include "animSkeletalMultiDangleConstraint_painter.h"
#include "showFlags.h"

#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CAnimSkeletalMultiDangleConstraint_Painter );

void CAnimSkeletalMultiDangleConstraint_Painter::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	ForceTPose();
}

void CAnimSkeletalMultiDangleConstraint_Painter::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	TBaseClass::OnGenerateEditorFragments( frame, flags );	

	OnDrawSkeletonWS( frame );
}

void CAnimSkeletalMultiDangleConstraint_Painter::CreateConstraints( const TDynArray< Int32 >& dangleParentBones_skeletonMain, const TDynArray< Int32 >& dangleRootBones_skeletonDangle, const TDynArray< TPair< Int32, TDynArray< Int32 > > >& dangleChildrenBones_skeletonDangle )
{
	m_dangleParentBones_skMain = dangleParentBones_skeletonMain;
	m_dangleRootBones_skDangle = dangleRootBones_skeletonDangle;
	m_dangleChildrenBones_skDangle = dangleChildrenBones_skeletonDangle;
}

void CAnimSkeletalMultiDangleConstraint_Painter::OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones )
{
	StartUpdateWithBoneAlignment( poseMS, poseWS, bones );

	//...

	EndUpdate( poseMS, poseWS, bones );
}

#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",on)
#endif
