/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphInstance.h"
#include "animatedComponent.h"
#include "skeleton.h"

class IMimicLookAtSolver
{
protected:
	const static Float					HOR_RANGE;
	const static Float					VER_RANGE;

	struct SolverData
	{
#ifdef USE_HAVOK_ANIMATION
		hkVector4						m_targetMS;
		hkQsTransform					m_placerMS;
#else
		RedVector4						m_targetMS;
		RedQsTransform					m_placerMS;
#endif
		Float							m_verMin;
		Float							m_verMax;
		Float							m_verOffset;
		Float							m_horMax;
		Float							m_weight;
		Bool							m_mirrored;

		Float							m_horTrackValue;
		Float							m_verTrackValue;

		SolverData() : m_horTrackValue( 0.f ), m_verTrackValue( 0.f ), m_verOffset( 0.f ), m_mirrored( false ) {}
	};

	//! Solver returns false if the target is over limits 
	Bool Solve( SolverData& data ) const;

	static Matrix FindChildBoneMatrixWS( CBehaviorGraphInstance& instance, Int32 bone )
	{
		const CAnimatedComponent* ac = instance.GetAnimatedComponent();
		const CSkeleton* skeleton = ac->GetSkeleton();

		if ( skeleton )
		{
			Uint32 boneNum = skeleton->GetBonesNum();

			for ( Uint32 i=0; i<boneNum; ++i )
			{
				Int32 boneParent = skeleton->GetParentBoneIndex( i );
				if ( boneParent == bone )
				{
					return instance.GetAnimatedComponent()->GetBoneMatrixWorldSpace( i );
				}
			}
		}

		return Matrix::IDENTITY;
	}
};
