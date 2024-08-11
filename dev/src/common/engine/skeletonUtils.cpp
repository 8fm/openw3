#include "build.h"
#include "skeletonUtils.h"
#include "../../common/engine/behaviorGraphOutput.h"
#include "animMath.h"
#include "behaviorGraphUtils.inl"
#include "renderFragment.h"
#include "viewport.h"
#include "animatedComponent.h"
#include "skeleton.h"
#include "entity.h"
#include "skeletalAnimation.h"
#include "wetnessComponent.h"

namespace SkeletonBonesUtils
{
	void CalcBoundingBoxModelSpace( const ISkeletonDataProvider::SBonesData& bonesData, const Int32* boneMapping,  Uint32 numBoneMapping, const TDynArray< Matrix >& skeletonModelSpace )
	{
		RedAABB &aabbMS = reinterpret_cast< RedAABB& >( bonesData.m_outBoxMS );

		const Int32 numSkeletonBones = skeletonModelSpace.SizeInt();
		for ( Uint32 i=0; i<bonesData.m_numBones; i++ )
		{
			Int32 boneIndex = bonesData.m_boneIndices[i];
			if ( boneMapping )
			{
				//ASSERT( boneIndex < numBoneMapping );
				boneIndex = boneMapping[ boneIndex ];
			}

			if ( boneIndex < numSkeletonBones && boneIndex != -1 )
			{
				if ( bonesData.m_vertexEpsilons == nullptr )
				{
					aabbMS.AddPoint( reinterpret_cast< const RedVector4& >( skeletonModelSpace[ boneIndex ].GetTranslationRef() ) );
				}
				else
				{
					const RedMatrix4x4* mBone = ( const RedMatrix4x4* ) &skeletonModelSpace[ boneIndex ];
					RedMatrix4x4 result;

					// Update bounding box
					const Float vertexEpsilon = bonesData.m_vertexEpsilons[i];
					RedVector4 boneVertexEpsilonBox( vertexEpsilon, vertexEpsilon, vertexEpsilon, 0.0f ); // TODO: optimize !
					const RedVector4& bonePosition = reinterpret_cast< const RedVector4& >( skeletonModelSpace[ boneIndex ].GetTranslationRef() );
					aabbMS.AddPoint( Sub( bonePosition, boneVertexEpsilonBox ) );
					aabbMS.AddPoint( Add( bonePosition, boneVertexEpsilonBox ) );
				}
			}
		}
	}

	void GetBoneMatricesModelSpace_opt( const ISkeletonDataProvider::SBonesData& bonesData, const TDynArray< Matrix >& skeletonModelSpace, const TDynArray< Matrix >& skeletonBindSpace )
	{
		Float* outMatrices = reinterpret_cast<Float*>( bonesData.m_outMatricesArray );
		const Int32 numSkeletonBones = skeletonModelSpace.SizeInt();
		ptrdiff_t outMatricesStep = ( bonesData.m_outMatricesType == SDMT_3x4Transposed ? 12 : 16 );
		if ( bonesData.m_rigMatrices )
		{
			for ( Uint32 i=0; i<bonesData.m_numBones; i++, outMatrices+=outMatricesStep )
			{
				const Int32 boneIndex = bonesData.m_boneIndices[i];
				if ( boneIndex < numSkeletonBones && boneIndex != -1 )
				{
#if defined( RED_PLATFORM_WINPC )
					const char* ptr = ( const char* )&bonesData.m_rigMatrices[ i + 4 ];
					_mm_prefetch( ptr, _MM_HINT_T0 );
#endif
					// Multiply using Havok
					const RedMatrix4x4* mRig = reinterpret_cast< const RedMatrix4x4* >( &bonesData.m_rigMatrices[ i ] );
					const RedMatrix4x4* mBone = reinterpret_cast< const RedMatrix4x4* >( &skeletonModelSpace[ boneIndex ] );
					RedMatrix4x4 result;
#ifdef _DEBUG
					result = Mul( *mRig, *mBone );
#else
					//result.setMulAffine( *mBone, *mRig );
					//TODO: Need MulAffine.
					result = Mul( *mRig, *mBone );

#endif
					const Matrix& resultMatrix = reinterpret_cast<const Matrix&>( result );

					const Matrix& resultMatrix2 = skeletonBindSpace[ boneIndex ];
					ASSERT( Vector::Equal4( resultMatrix.V[ 0 ], resultMatrix2.V[ 0 ] ) );
					ASSERT( Vector::Equal4( resultMatrix.V[ 1 ], resultMatrix2.V[ 1 ] ) );
					ASSERT( Vector::Equal4( resultMatrix.V[ 2 ], resultMatrix2.V[ 2 ] ) );
					ASSERT( Vector::Equal4( resultMatrix.V[ 3 ], resultMatrix2.V[ 3 ] ) );

					CopyMatrix( outMatrices, resultMatrix2, bonesData.m_outMatricesType );
				}
				else
				{
					CopyMatrix( outMatrices, Matrix::IDENTITY, bonesData.m_outMatricesType );

					RED_WARNING_ONCE( boneIndex != -1, "Invalid bone index!" );
				}
			}
		}
		else
		{
			for ( Uint32 i=0; i<bonesData.m_numBones; i++, outMatrices+= outMatricesStep )
			{
				const Int32 boneIndex = bonesData.m_boneIndices[i];
				if ( boneIndex < numSkeletonBones )
				{
					CopyMatrix( outMatrices, skeletonModelSpace[ boneIndex ], bonesData.m_outMatricesType );
				}
				else
				{
					CopyMatrix( outMatrices, Matrix::IDENTITY, bonesData.m_outMatricesType );
				}
			}
		}
	}

	void GetBoneMatricesModelSpaceWithWetnessData( const ISkeletonDataProvider::SBonesData& boneData, const TDynArray< Matrix >& skeletonModelSpace, const CWetnessSupplier* wetnessSupplier )
	{
		Float* outMatrices = reinterpret_cast<Float*>( boneData.m_outMatricesArray );
		const Int32 numSkeletonBones = skeletonModelSpace.SizeInt();
		ptrdiff_t outMatricesStep = ( boneData.m_outMatricesType == SDMT_3x4Transposed ? 12 : 16 );
		if ( boneData.m_rigMatrices )
		{
			for ( Uint32 i=0; i<boneData.m_numBones; i++, outMatrices+=outMatricesStep )
			{
				const Int32 boneIndex = boneData.m_boneIndices[i];
				if ( boneIndex < numSkeletonBones && boneIndex != -1 )
				{
#if defined( RED_PLATFORM_WINPC )
					const char* ptr = ( const char* ) &boneData.m_rigMatrices[ i + 4 ];
					_mm_prefetch( ptr, _MM_HINT_T0 );
#endif
					// Multiply using Havok
					const RedMatrix4x4* mRig = reinterpret_cast< const RedMatrix4x4* >( &boneData.m_rigMatrices[ i ] );
					const RedMatrix4x4* mBone = reinterpret_cast< const RedMatrix4x4* >( &skeletonModelSpace[ boneIndex ] );
					Float extra = wetnessSupplier->GetWetnessDataFromBone( boneIndex );
					RedMatrix4x4 result;
#ifdef _DEBUG
					result = Mul( *mRig, *mBone );
#else
					//result.setMulAffine( *mBone, *mRig );
					//TODO: Need MulAffine.
					result = Mul( *mRig, *mBone );

#endif				// inversion is to make here 1.0f normally
					result.Row3.W = 1.0f - extra;
					const Matrix& resultMatrix = reinterpret_cast<const Matrix&>( result );

					CopyMatrix( outMatrices, resultMatrix, boneData.m_outMatricesType );
				}
				else
				{
					CopyMatrix( outMatrices, Matrix::IDENTITY, boneData.m_outMatricesType );

					RED_WARNING_ONCE( boneIndex != -1, "Invalid bone index!" );
				}
			}
		}
		else
		{
			for ( Uint32 i=0; i<boneData.m_numBones; i++, outMatrices+= outMatricesStep )
			{
				const Int32 boneIndex = boneData.m_boneIndices[i];
				if ( boneIndex < numSkeletonBones )
				{
					CopyMatrix( outMatrices, skeletonModelSpace[ boneIndex ], boneData.m_outMatricesType );
				}
				else
				{
					CopyMatrix( outMatrices, Matrix::IDENTITY, boneData.m_outMatricesType );
				}
			}
		}
	}

	void GetBoneMatricesModelSpace( const ISkeletonDataProvider::SBonesData& boneData, const TDynArray< Matrix >& skeletonModelSpace )
	{
		Float* outMatrices = reinterpret_cast<Float*>( boneData.m_outMatricesArray );
		const Int32 numSkeletonBones = skeletonModelSpace.SizeInt();
		ptrdiff_t outMatricesStep = ( boneData.m_outMatricesType == SDMT_3x4Transposed ? 12 : 16 );
		if ( boneData.m_rigMatrices )
		{
			for ( Uint32 i=0; i<boneData.m_numBones; i++, outMatrices+=outMatricesStep )
			{
				const Int32 boneIndex = boneData.m_boneIndices[i];
				if ( boneIndex < numSkeletonBones && boneIndex != -1 )
				{
#if defined( RED_PLATFORM_WINPC )
					const char* ptr = ( const char* )&boneData.m_rigMatrices[ i + 4 ];
					_mm_prefetch( ptr, _MM_HINT_T0 );
#endif
					const RedMatrix4x4* mRig = reinterpret_cast< const RedMatrix4x4* >( &boneData.m_rigMatrices[ i ] );
					const RedMatrix4x4* mBone = reinterpret_cast< const RedMatrix4x4* >( &skeletonModelSpace[ boneIndex ] );
					RedMatrix4x4 result;
#ifdef _DEBUG
					result = Mul( *mRig, *mBone );
#else
					//result.setMulAffine( *mBone, *mRig );
					//TODO: Need MulAffine.
					result = Mul( *mRig, *mBone );
#endif
					// means no wetness shader puts 1 over there
					const Matrix& resultMatrix = reinterpret_cast<const Matrix&>( result );
					CopyMatrix( outMatrices, resultMatrix, boneData.m_outMatricesType );
				}
				else
				{
					CopyMatrix( outMatrices, Matrix::IDENTITY, boneData.m_outMatricesType );
					RED_WARNING_ONCE( boneIndex != -1, "Invalid bone index!" );
				}
			}
		}
		else
		{
			for ( Uint32 i=0; i<boneData.m_numBones; i++, outMatrices+= outMatricesStep )
			{
				const Int32 boneIndex = boneData.m_boneIndices[i];
				if ( boneIndex < numSkeletonBones )
				{
					CopyMatrix( outMatrices, skeletonModelSpace[ boneIndex ], boneData.m_outMatricesType );
				}
				else
				{
					CopyMatrix( outMatrices, Matrix::IDENTITY, boneData.m_outMatricesType );
				}
			}
		}
	}

	void MulMatrices( const Matrix* src, Matrix* dest, Uint32 count, const Matrix* localToWorld )
	{
		const AnimMatrix44* localToWorldMatrix = reinterpret_cast< const AnimMatrix44* >( localToWorld );
		const AnimMatrix44* srcMatrix = reinterpret_cast< const AnimMatrix44* >( src );
		AnimMatrix44* destMatrix = reinterpret_cast< AnimMatrix44* >( dest );

		while ( count-- )
		{
			// Prefetch
#ifdef W2_PLATFORM_WIN32
			_mm_prefetch( (const char*)( srcMatrix + 4 ), _MM_HINT_T0 );
#endif

			// Multiply
			// TODO use setMulAffine !!!
			*destMatrix = Mul( *srcMatrix, *localToWorldMatrix );

			// Move to next
			++destMatrix;
			++srcMatrix;
		}
	}

	void MulMatrices( const Matrix* srcA, const Matrix* srcB, Matrix* dest, Uint32 count )
	{
		const AnimMatrix44* srcAMatrix = reinterpret_cast< const AnimMatrix44* >( srcA );
		const AnimMatrix44* srcBMatrix = reinterpret_cast< const AnimMatrix44* >( srcB );
		AnimMatrix44* destMatrix = reinterpret_cast< AnimMatrix44* >( dest );

		while ( count-- )
		{
			// Prefetch
#ifdef W2_PLATFORM_WIN32
			_mm_prefetch( (const char*)( srcAMatrix + 4 ), _MM_HINT_T0 );
#endif

			// Multiply
			// TODO use setMulAffine !!!
			*destMatrix = Mul( *srcBMatrix, *srcAMatrix );

			// Move to next
			++destMatrix;
			++srcAMatrix;
			++srcBMatrix;
		}
	}

	Bool CalcBoneOffset( const CEntity* entity, const CSkeletalAnimation* anim, const CName& bone, Float time, AnimQsTransform& offset )
	{
		const CAnimatedComponent* comp = entity->GetRootAnimatedComponent();

		return CalcBoneOffset( comp, anim, bone, time, offset );
	}

	Bool CalcBoneOffset( const CAnimatedComponent* comp, const CSkeletalAnimation* anim, const CName& bone, Float time, AnimQsTransform& offset )
	{
		offset = AnimQsTransform::IDENTITY;

		if ( !comp->GetSkeleton() )
		{
			return false;
		}

		const Int32 boneIdx = comp->FindBoneByName( bone );
		if ( boneIdx == -1 )
		{
			return false;
		}

		const CSkeleton* skeleton = comp->GetSkeleton();

		const Uint32 boneNum = (Uint32)skeleton->GetBonesNum();
		const Uint32 tracksNum = (Uint32)skeleton->GetTracksNum();

		SBehaviorGraphOutput pose;
		pose.Init( boneNum, tracksNum, false );

		if ( anim->Sample( time, pose.m_numBones, pose.m_numFloatTracks, pose.m_outputPose, pose.m_floatTracks ) )
		{
			if ( comp->UseExtractedTrajectory() )
			{
				pose.ExtractTrajectory( comp );
			}

			offset = pose.GetBoneModelTransform( comp, boneIdx );
			return true;
		}

		return false;
	}
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

namespace SkeletonUtils
{
	void CopyPoseToBuffer( const CAnimatedComponent* component, TDynArray< Matrix >& poseWS )
	{
		const CSkeleton* skeleton = component->GetSkeleton();
		if ( skeleton )
		{
			const Uint32 boneNum = skeleton->GetBonesNum();

			poseWS.Resize( boneNum );

			for ( Uint32 i=0; i<boneNum; ++i )
			{
				poseWS[ i ] = component->GetBoneMatrixWorldSpace( i );
			}
		}
		else
		{
			poseWS.Clear();
		}
	}

	Bool SamplePoseMS( const CSkeletalAnimation* animation, Float time, const CAnimatedComponent* component, TDynArray< Matrix >& transformsMS )
	{
		if ( !animation || !component )
		{
			return false;
		}

		TDynArray< ISkeletonDataProvider::BoneInfo > bones;
		const Uint32 bonesNum = component->GetBones( bones );

		transformsMS.Resize( bonesNum );

		SBehaviorGraphOutput pose;
		pose.Init( animation->GetBonesNum(), animation->GetTracksNum() );

		if ( animation->Sample( time, pose.m_numBones, pose.m_numFloatTracks, pose.m_outputPose, pose.m_floatTracks ) )
		{
			if ( component->HasTrajectoryBone() && component->UseExtractedTrajectory() )
			{
				pose.ExtractTrajectory( component );
			}

			pose.GetBonesModelSpace( component, transformsMS );

			return true;
		}

		return false;
	}

	Bool SamplePoseWithMotionMS( const CSkeletalAnimation* animation, Float time, const CAnimatedComponent* component, TDynArray< Matrix >& pose )
	{
		if ( !SamplePoseMS( animation, time, component, pose ) )
		{
			return false;
		}

		Matrix motion( Matrix::IDENTITY );

		animation->GetMovementAtTime( time, motion );

		SkeletonBonesUtils::MulMatrices( pose.TypedData(), pose.TypedData(), pose.Size(), &motion );

		return true;
	}

	Bool SamplePoseWS( const CSkeletalAnimation* animation, Float time, const CAnimatedComponent* component, TDynArray< Matrix >& transformsWS )
	{
		if ( !SamplePoseMS( animation, time, component, transformsWS ) )
		{
			return false;
		}

		const Matrix& localToWorld = component->GetLocalToWorld();

		SkeletonBonesUtils::MulMatrices( transformsWS.TypedData(), transformsWS.TypedData(), transformsWS.Size(), &localToWorld );

		return true;
	}

	Bool SamplePoseWithMotionWS( const CSkeletalAnimation* animation, Float time, const CAnimatedComponent* component, TDynArray< Matrix >& pose )
	{
		if ( !SamplePoseMS( animation, time, component, pose ) )
		{
			return false;
		}

		Matrix motion( Matrix::IDENTITY );

		animation->GetMovementAtTime( time, motion );

		SkeletonBonesUtils::MulMatrices( pose.TypedData(), pose.TypedData(), pose.Size(), &motion );


		const Matrix& localToWorld = component->GetLocalToWorld();

		SkeletonBonesUtils::MulMatrices( pose.TypedData(), pose.TypedData(), pose.Size(), &localToWorld );

		return true;
	}
#ifdef USE_HAVOK_ANIMATION
	void CalcLastBoneInChain( const TDynArray< hkQsTransform >& bones, hkQsTransform& lastBoneMS )
	{
		const Int32 size = bones.SizeInt();

		hkQsTransform bone = size > 0 ? bones[ size - 1 ] : hkQsTransform::getIdentity();

		Int32 currBone = size - 2;
		while( currBone >= 0 )
		{			
			bone.setMul( bones[ currBone ], bone );
			--currBone;
		}

		lastBoneMS = bone;
	}
#else
	void CalcLastBoneInChain( const TDynArray< RedQsTransform >& bones, RedQsTransform& lastBoneMS )
	{
		const Int32 size = bones.SizeInt();

		RedQsTransform bone = size > 0 ? bones[ size - 1 ] : RedQsTransform::IDENTITY;

		Int32 currBone = size - 2;
		while( currBone >= 0 )
		{			
			bone.SetMul( bones[ currBone ], bone );
			--currBone;
		}

		lastBoneMS = bone;
	}
#endif
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

namespace SkeletonRenderingUtils
{
	void DrawSkeleton( const CAnimatedComponent* component, const Color& color, CRenderFrame *frame, Bool skeletonVisible, Bool skeletonNames, Bool skeletonAxis )
	{
		if ( component && ( skeletonVisible || skeletonNames || skeletonAxis ) )
		{
			TDynArray< ISkeletonDataProvider::BoneInfo > bones;
			Uint32 bonesNum = component->GetBones( bones );

			if ( skeletonVisible )
			{
				TDynArray< DebugVertex > skeletonPoints;

				// Draw bones
				for( Uint32 i=0; i<bonesNum; i++ )
				{
					Int32 parentIndex = bones[i].m_parent;
					if ( parentIndex != -1 )
					{
						const Matrix start = component->GetBoneMatrixWorldSpace( parentIndex );
						const Matrix end = component->GetBoneMatrixWorldSpace( i );

						skeletonPoints.PushBack( DebugVertex( start.GetTranslation(), color ) );
						skeletonPoints.PushBack( DebugVertex( end.GetTranslation(), color ) );
					}
				}

				if (skeletonPoints.Size() > 0)
				{
					new (frame) CRenderFragmentDebugLineList( frame, 
						Matrix::IDENTITY,
						&skeletonPoints[0],
						skeletonPoints.Size(),
						RSG_DebugOverlay );
				}

				// Draw float tracks
				Uint32 tracksNum = component->GetFloatTrackNum();				
				Uint32 offsetX = frame->GetFrameOverlayInfo().m_width - 100;
				Uint32 offsetY = 20;

				for ( Uint32 i=0; i<tracksNum; ++i )
				{
					Float track = component->GetFloatTrack( i );

					// Disp i:track
					String text = String::Printf( TXT("Track %d: %.2f"), i, track );
					frame->AddDebugScreenText( offsetX, offsetY, text, Color::RED );
					offsetY += 15;
				}

				// Draw motion extraction
				{
					Matrix motionDelta = component->GetAnimationMotionDelta();
					Vector motionTrans = motionDelta.GetTranslation();
					EulerAngles motionRot = motionDelta.ToEulerAngles();

					String text = String::Printf( TXT("dX %f"), motionTrans.X );
					frame->AddDebugScreenText( offsetX, offsetY, text, Color::RED );
					offsetY += 15;

					text = String::Printf( TXT("dY %f"), motionTrans.Y );
					frame->AddDebugScreenText( offsetX, offsetY, text, Color::RED );
					offsetY += 15;

					text = String::Printf( TXT("dZ %f"), motionTrans.Z );
					frame->AddDebugScreenText( offsetX, offsetY, text, Color::RED );
					offsetY += 15;

					text = String::Printf( TXT("rX %f"), motionRot.Pitch );
					frame->AddDebugScreenText( offsetX, offsetY, text, Color::RED );
					offsetY += 15;

					text = String::Printf( TXT("rY %f"), motionRot.Roll );
					frame->AddDebugScreenText( offsetX, offsetY, text, Color::RED );
					offsetY += 15;

					text = String::Printf( TXT("rZ %f"), motionRot.Yaw );
					frame->AddDebugScreenText( offsetX, offsetY, text, Color::RED );
					offsetY += 15;
				}

			}

			if ( skeletonNames )
			{
				for( Uint32 i=0; i<bonesNum; i++ )
				{
					Matrix boneMatrix = component->GetBoneMatrixWorldSpace( i );
					frame->AddDebugText( boneMatrix.GetTranslation(), bones[i].m_name.AsString(), false, color );
				}
			}

			if ( skeletonAxis )
			{
				for( Uint32 i=0; i<bonesNum; i++ )
				{
					Matrix boneMatrix = component->GetBoneMatrixWorldSpace( i );
					frame->AddDebugAxis( boneMatrix.GetTranslation(), boneMatrix, 0.1f, true );
				}
			}
		}
	}

	void DrawSkeleton( const CSkeletalAnimation* animation, Float time, const CAnimatedComponent* component, const Color& color, CRenderFrame *frame )
	{
		if ( !animation || !component )
		{
			return;
		}

		TDynArray< ISkeletonDataProvider::BoneInfo > bones;
		const Uint32 bonesNum = component->GetBones( bones );

		TDynArray< Matrix > transformsWS;
		transformsWS.Resize( bonesNum );

		SBehaviorGraphOutput pose;
		pose.Init( animation->GetBonesNum(), animation->GetTracksNum() );

		const Matrix& localToWorld = component->GetLocalToWorld();

		TDynArray< Int32 > bonesToDraw;

		if ( animation->Sample( time, pose.m_numBones, pose.m_numFloatTracks, pose.m_outputPose, pose.m_floatTracks ) )
		{
			pose.GetBonesModelSpace( component, transformsWS );

			SkeletonBonesUtils::MulMatrices( transformsWS.TypedData(), transformsWS.TypedData(), transformsWS.Size(), &localToWorld );

			DrawSkeleton( transformsWS, bones, bonesToDraw, color, frame );
		}
	}

	void DrawSkeleton( const TDynArray< Matrix >& transformsWS, TDynArray< ISkeletonDataProvider::BoneInfo >& bones, const TDynArray< Int32 >& bonesToDraw, const Color& color, CRenderFrame *frame )
	{
		TDynArray< DebugVertex > skeletonPoints;
		skeletonPoints.Reserve( 128 );

		Bool allbones = bonesToDraw.Size() == 0;

		const Uint32 bonesNum = Min( transformsWS.Size(), bones.Size() );
		for( Uint32 i=0; i<bonesNum; i++ )
		{
			const Int32 parentIndex = bones[i].m_parent;
			if ( parentIndex != -1 )
			{
				const Matrix& start = transformsWS[ parentIndex ];
				const Matrix& end = transformsWS[ i ];

				if ( !allbones && !bonesToDraw.Exist( i ) )
				{
					continue;
				}

				skeletonPoints.PushBack( DebugVertex( start.GetTranslation(), color ) );
				skeletonPoints.PushBack( DebugVertex( end.GetTranslation(), color ) );
			}
		}

		if ( skeletonPoints.Size() > 0 )
		{
			new (frame) CRenderFragmentDebugLineList( frame, 
				Matrix::IDENTITY,
				&skeletonPoints[0],
				skeletonPoints.Size(),
				RSG_DebugOverlay );
		}
	}

	void DrawSkeleton( const TDynArray< Matrix >& transformsWS, const CAnimatedComponent* component, const Color& color, CRenderFrame *frame )
	{
		TDynArray< ISkeletonDataProvider::BoneInfo > bones;
		component->GetBones( bones );

		TDynArray< Int32 > allBones;

		DrawSkeleton( transformsWS, bones, allBones, color, frame );
	}

	void DrawSkeleton( const TDynArray< Matrix >& transformsWS, const CSkeleton* skeleton, const Color& color, CRenderFrame *frame, Bool skeletonVisible, Bool skeletonNames, Bool skeletonAxis, Bool withoutExtraBones )
	{
		TDynArray< DebugVertex > skeletonPoints;
		skeletonPoints.Reserve( 128 );

		const Int32 bonesNum = Min< Int32 >( transformsWS.SizeInt(), skeleton->GetBonesNum() );
		for( Int32 i=0; i<bonesNum; i++ )
		{
			const Matrix& boneMatrix = transformsWS[ i ];

			if ( skeletonVisible )
			{
				const Int32 parentIndex = skeleton->GetParentBoneIndex( i );
				if ( parentIndex != -1 )
				{
					if ( withoutExtraBones && parentIndex == 0 )
					{
						continue;
					}

					const Matrix& start = transformsWS[ parentIndex ];
					const Matrix& end = boneMatrix;

					skeletonPoints.PushBack( DebugVertex( start.GetTranslation(), color ) );
					skeletonPoints.PushBack( DebugVertex( end.GetTranslation(), color ) );
				}
			}

			if ( skeletonNames )
			{
				const CName boneName = skeleton->GetBoneNameAsCName( i );
				frame->AddDebugText( boneMatrix.GetTranslation(), boneName.AsString() );
			}

			if ( skeletonAxis )
			{
				frame->AddDebugAxis( boneMatrix.GetTranslation(), boneMatrix, 0.1f, true );
			}
		}

		if ( skeletonPoints.Size() > 0 )
		{
			new (frame) CRenderFragmentDebugLineList( frame, 
				Matrix::IDENTITY,
				&skeletonPoints[0],
				skeletonPoints.Size(),
				RSG_DebugOverlay );
		}
	}
};
