
#include "build.h"
#include "animDangleConstraint.h"
#include "animatedAttachment.h"
#include "skeleton.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( IAnimDangleConstraint );

void IAnimDangleConstraint::AlignBones( const TDynArray< Matrix >* poseMS, const TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones, TDynArray< Matrix >& skeletonModelSpace, TDynArray< Matrix >& skeletonWorldSpace )
{
	const Int32 numPoseMS = poseMS->SizeInt();
	const Int32 numPoseWS = poseWS->SizeInt();

	const Int32 numMS = skeletonModelSpace.SizeInt();
	const Int32 numWS = skeletonWorldSpace.SizeInt();

	ASSERT( numMS == numWS );

	if ( numPoseMS == numPoseWS && numMS == numWS )
	{
		const Uint32 size = bones.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const SBoneMapping& m = bones[ i ];

			ASSERT( m.m_boneA < numMS );
			ASSERT( m.m_boneA < numWS );

			ASSERT( m.m_boneB < numPoseMS );
			ASSERT( m.m_boneB < numPoseWS );

			if ( m.m_boneA < numMS && m.m_boneA < numWS ) // TODO - do it smart!
			{
				skeletonModelSpace[ m.m_boneA ] = (*poseMS)[ m.m_boneB ];
				skeletonWorldSpace[ m.m_boneA ] = (*poseWS)[ m.m_boneB ];
			}
		}
	}
}

void IAnimDangleConstraint::AlignBonesFull( const TDynArray< Matrix >* poseMS, const TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones, TDynArray< Matrix >& skeletonModelSpace, TDynArray< Matrix >& skeletonWorldSpace, const CSkeleton* skel )
{
	const Int32 numBones = skel->GetBonesNum();

	Int32 mindex = 0;
	for( Int32 i=0;i<numBones;++i )
	{
		if( mindex<Int32(bones.Size()) && i==bones[mindex].m_boneA )
		{
			skeletonModelSpace[ i ] = (*poseMS)[ bones[mindex].m_boneB ];
			skeletonWorldSpace[ i ] = (*poseWS)[ bones[mindex].m_boneB ];
			mindex++;
		}
		else
		{
			Matrix loc = skel->GetBoneMatrixLS(i);
			Int32 par = skel->GetParentBoneIndex(i);
			if( par>=0 )
			{
				skeletonModelSpace[ i ] = Matrix::Mul( skeletonModelSpace[par], loc );
				skeletonWorldSpace[ i ] = Matrix::Mul( skeletonWorldSpace[par], loc );
			}
			else
			{
				skeletonModelSpace[ i ] = loc;
				skeletonWorldSpace[ i ] = loc;
			}
		}
	}
}

#ifndef NO_EDITOR
Bool IAnimDangleConstraint::PrintDebugComment( String& str ) const
{
	Bool ret = false;

	const CHardAttachment* parentAtt = GetComponent()->GetTransformParent();
	if ( parentAtt )
	{
		const CAnimatedAttachment* animAtt = Cast< const CAnimatedAttachment >( parentAtt );
		if ( animAtt )
		{
			Int32 prevParentIdx = -1;

			const auto mappingData = animAtt->GetSkeletonMaping();
			if ( mappingData )
			{
				const auto& mapping = mappingData->m_boneMapping;
				const Uint32 numMapping = mapping.Size();
				for ( Uint32 i=0; i<numMapping; ++i )
				{
					const SBoneMapping& bm = mapping[ i ];
					if ( bm.m_boneA == -1 || bm.m_boneB == -1 )
					{
						str += TXT("Parent attachment is not 'Animated Attachment'; ");
						break;
					}

					if ( bm.m_boneA <= prevParentIdx )
					{
						str += TXT("Animated attachment mapping is invalid; ");
					}

					prevParentIdx = bm.m_boneA;
				}
			}
		}
		else
		{
			str += TXT("Parent attachment is not 'Animated Attachment'; ");
		}

		// check children mesh att mapping
		//...
	}

	return ret;
}
#endif

#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",on)
#endif

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
