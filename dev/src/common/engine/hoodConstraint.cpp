
#include "build.h"
#include "hoodConstraint.h"
#include "animMath.h"
#include "skeleton.h"
#include "renderFrame.h"

#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CAnimDangleConstraint_Hood );

CAnimDangleConstraint_Hood::CAnimDangleConstraint_Hood() 
	: CAnimSkeletalDangleConstraint()
	, m_offset( 0.13f, 0.08f, 0.0f, 1.0f )
	, m_radius( 0.16f )
	, m_cachedAllBones( false )
{
}

Bool CAnimDangleConstraint_Hood::HasCachedBones() const
{
	return m_cachedAllBones;
}

void CAnimDangleConstraint_Hood::CacheBones( const CComponent* parentComp, const CSkeleton* skeleton )
{
	const ISkeletonDataProvider* parent = parentComp ? parentComp->QuerySkeletonDataProvider() : nullptr;
	if ( parent && skeleton )
	{
		m_headIndex = parent->FindBoneByName( CNAME( head ) );

		const AnimQsTransform qstm = skeleton->GetBoneLS( 1 );
		Matrix mat;
		RedMatrix4x4 convertedMatrix = qstm.ConvertToMatrixNormalized();
		mat = reinterpret_cast< const Matrix& >( convertedMatrix );
		m_bones[0].localSpace = mat;

		const AnimQsTransform qstm2 = skeleton->GetBoneLS( 2 );
		Matrix mat2;

		RedMatrix4x4 convertedMatrix2 = qstm2.ConvertToMatrixNormalized();
		mat2 = reinterpret_cast< const Matrix& >( convertedMatrix2 );
		m_bones[1].localSpace = mat2;

		m_cachedAllBones = m_headIndex != -1;
	}
	else
	{
		m_headIndex = -1;
		m_cachedAllBones = false;
	}
}

void CAnimDangleConstraint_Hood::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	if ( m_cachedAllBones )
	{
		frame->AddDebugSphere( m_spherePos, m_radius, Matrix::IDENTITY, Color(255,0,0) );
		frame->AddDebugAxis( m_bones[0].worldSpace.GetTranslation(), m_bones[0].worldSpace, 0.1f, Color(255,255,0) );
		frame->AddDebugAxis( m_bones[1].worldSpace.GetTranslation(), m_bones[1].worldSpace, 0.1f, Color(255,255,0) );
	}
}

void CAnimDangleConstraint_Hood::OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones )
{
	StartUpdateWithBoneAlignment( poseMS, poseWS, bones );
	if ( m_cachedAllBones )
	{
		ASSERT( m_headIndex != -1 );
		if ( m_headIndex < poseMS->SizeInt() )
		{
			const Matrix& headMS = (*poseMS)[ m_headIndex ];
			m_spherePos = headMS.TransformPoint( m_offset );
		}

		Matrix headms = (*poseMS)[m_headIndex];
		(*poseMS)[1] = m_bones[0].calculate( headms, m_spherePos, m_radius );
		(*poseMS)[2] = m_bones[1].calculate( headms, m_spherePos, m_radius );
	}
	EndUpdate( poseMS, poseWS, bones );
}

#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",on)
#endif
