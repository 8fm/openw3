
#pragma once

#include "skeletonProvider.h"
#include "animMath.h"
#include "renderSkinningData.h"

class CRenderFrame;
class CWetnessSupplier;

namespace SkeletonBonesUtils
{
	RED_FORCE_INLINE void CopyMatrix( Float* dest, const Matrix& source, ESkinningDataMatrixType destType )
	{
		if ( destType == SDMT_3x4Transposed )
		{
			dest[0]  = source.V[0].A[0];
			dest[1]  = source.V[1].A[0];
			dest[2]  = source.V[2].A[0];
			dest[3]  = source.V[3].A[0];

			dest[4]  = source.V[0].A[1];
			dest[5]  = source.V[1].A[1];
			dest[6]  = source.V[2].A[1];
			dest[7]  = source.V[3].A[1];

			dest[8]  = source.V[0].A[2];
			dest[9]  = source.V[1].A[2];
			dest[10] = source.V[2].A[2];
			dest[11] = source.V[3].A[2];
		}
		else
		{
			ASSERT( SDMT_4x4 == destType );
			Red::System::MemoryCopy( dest, source.AsFloat(), sizeof( Matrix ) );
		}
	}

	RED_FORCE_INLINE void CopyMatrix( Matrix& dest, const Float* source, ESkinningDataMatrixType sourceType )
	{
		if ( sourceType == SDMT_3x4Transposed )
		{
			for ( Uint32 i=0; i<3; ++i )
			{
				dest.V[0].A[i] = source[4 * i + 0];
				dest.V[1].A[i] = source[4 * i + 1];
				dest.V[2].A[i] = source[4 * i + 2];
				dest.V[3].A[i] = source[4 * i + 3];
			}
			dest.V[0].A[3] = 0;
			dest.V[1].A[3] = 0;
			dest.V[2].A[3] = 0;
			dest.V[3].A[3] = 1;
		}
		else
		{
			ASSERT( SDMT_4x4 == sourceType );
			Red::System::MemoryCopy( dest.AsFloat(), source, sizeof( Matrix ) );
		}
	}

	RED_FORCE_INLINE Uint32 GetMatrixNumFloats( ESkinningDataMatrixType type )
	{
		if ( type == SDMT_3x4Transposed )
		{
			return 3 * 4;
		}
		else
		{
			ASSERT( SDMT_4x4 == type );
			return 4 * 4;
		}
	}
	
	void GetBoneMatricesModelSpaceWithWetnessData( const ISkeletonDataProvider::SBonesData& bonesData, const TDynArray< Matrix >& skeletonModelSpace, const CWetnessSupplier* wetnessSupplier );
	void GetBoneMatricesModelSpace( const ISkeletonDataProvider::SBonesData& bonesData, const TDynArray< Matrix >& skeletonModelSpace );
	void GetBoneMatricesModelSpace_opt( const ISkeletonDataProvider::SBonesData& bonesData, const TDynArray< Matrix >& skeletonModelSpace, const TDynArray< Matrix >& skeletonBindSpace );
	void CalcBoundingBoxModelSpace( const ISkeletonDataProvider::SBonesData& bonesData, const Int32* boneMapping,  Uint32 numBoneMapping, const TDynArray< Matrix >& skeletonModelSpace );

	void MulMatrices( const Matrix* src, Matrix* dest, Uint32 count, const Matrix* localToWorld );
	void MulMatrices( const Matrix* srcA, const Matrix* srcB, Matrix* dest, Uint32 count );

	Bool CalcBoneOffset( const CEntity* entity, const CSkeletalAnimation* anim, const CName& bone, Float time, AnimQsTransform& offset );
	Bool CalcBoneOffset( const CAnimatedComponent* ac, const CSkeletalAnimation* anim, const CName& bone, Float time, AnimQsTransform& offset );
}

namespace SkeletonUtils
{
	void CopyPoseToBuffer( const CAnimatedComponent* component, TDynArray< Matrix >& poseWS );

	Bool SamplePoseMS( const CSkeletalAnimation* animation, Float time, const CAnimatedComponent* component, TDynArray< Matrix >& pose );
	Bool SamplePoseWS( const CSkeletalAnimation* animation, Float time, const CAnimatedComponent* component, TDynArray< Matrix >& pose ); // Not tested

	Bool SamplePoseWithMotionMS( const CSkeletalAnimation* animation, Float time, const CAnimatedComponent* component, TDynArray< Matrix >& pose );
	Bool SamplePoseWithMotionWS( const CSkeletalAnimation* animation, Float time, const CAnimatedComponent* component, TDynArray< Matrix >& pose ); // Not tested
#ifdef USE_HAVOK_ANIMATION
	void CalcLastBoneInChain( const TDynArray< hkQsTransform >& bones, hkQsTransform& lastBoneMS );
#else
	void CalcLastBoneInChain( const TDynArray< RedQsTransform >& bones, RedQsTransform& lastBoneMS );
#endif
};

namespace SkeletonRenderingUtils
{
	void DrawSkeleton( const CAnimatedComponent* component, const Color& color, CRenderFrame *frame, Bool skeletonVisible = true, Bool skeletonNames = false, Bool skeletonAxis = false );

	void DrawSkeleton( const CSkeletalAnimation* animation, Float time, const CAnimatedComponent* component, const Color& color, CRenderFrame *frame );

	void DrawSkeleton( const TDynArray< Matrix >& transformsWS, TDynArray< ISkeletonDataProvider::BoneInfo >& bones, const TDynArray< Int32 >& bonesToDraw, const Color& color, CRenderFrame *frame );

	void DrawSkeleton( const TDynArray< Matrix >& transformsWS, const CAnimatedComponent* component, const Color& color, CRenderFrame *frame );

	void DrawSkeleton( const TDynArray< Matrix >& transformsWS, const CSkeleton* skeleton, const Color& color, CRenderFrame *frame, Bool skeletonVisible = true, Bool skeletonNames = false, Bool skeletonAxis = false, Bool withoutExtraBones = false );
};
