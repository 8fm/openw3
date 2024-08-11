#include "build.h"
#include "dressMeshComponent.h"
#include "mesh.h"
#include "renderer.h"
#include "skeletonUtils.h"
#include "animatedIterators.h"
#include "world.h"
#include "entity.h"

IMPLEMENT_ENGINE_CLASS( CDressMeshComponent );

CDressMeshComponent::CDressMeshComponent()
	: m_ofweight( 0.8f )
	, m_p1( 0.1f, -0.11f, 0.f )
	, m_p2( 0.06f, -0.02f, 0.0f )
	, m_p3( 0.f, 0.0f, 0.0f )
	, m_r1( 0.f, 0.0f, 2.7f )
	, m_r2( 0.f, 0.0f, 12.8f )
	, m_r3( 0.f, 0.0f, -9.4f )
	, m_thighBoneWeight( 0.2f )
	, m_shinBoneWeight( 0.4f )
	, m_kneeRollBoneWeight( 0.8f )
{

}

CDressMeshComponent::~CDressMeshComponent()
{
	SAFE_RELEASE( m_skinningData );
}

void CDressMeshComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CDressMeshComponent_OnAttached );

	if ( m_skeleton )
	{
		m_skeletonModelSpace.Resize( m_skeleton->GetBonesNum() );
		m_skeletonWorldSpace.Resize( m_skeleton->GetBonesNum() );

		MakeSkinningMapping();
		MakeDressMapping();

#ifndef NO_EDITOR
		ForceTPose();
#endif
	}

	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Behavior );
}

void CDressMeshComponent::OnDetached( CWorld* world )
{
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Behavior );

	SAFE_RELEASE( m_skinningData );

	m_pose.Deinit();

	m_skinningMapping.Clear();
	m_dressBoneIndices.Clear();

	TBaseClass::OnDetached( world );
}

Bool CDressMeshComponent::IsValid() const
{
	return m_skeleton.IsValid() && GetMeshNow() != nullptr && m_skeleton->IsValid() && !m_dressBoneIndices.Empty() && !m_skinningMapping.Empty();
}

void CDressMeshComponent::OnParentUpdatedAttachedAnimatedObjectsLS( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	ProcessParentPose( poseLS, bones );
}

void CDressMeshComponent::OnParentUpdatedAttachedAnimatedObjectsLSAsync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	ProcessParentPose( poseLS, bones );
}

void CDressMeshComponent::ProcessParentPose( SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	// THIS CODE IS CRAP. WE SHOULD REWRITE IT!!!

	// Check needed objects
	// No parent animated component is used
	if ( poseLS == NULL || !IsValid() )
	{
		return;
	}

	// Check Bones
	if ( bones.SizeInt() != m_skeleton->GetBonesNum() )
	{
		ERR_ENGINE( TXT( "Not all dress bones are mapped" ) );
		ASSERT( bones.SizeInt() == m_skeleton->GetBonesNum() );		// Don't use getMesh - we don't want to load the mesh for just an assert
		return;
	}

	for ( Uint32 i=0; i<bones.Size(); ++i )
	{
#ifdef USE_HAVOK_ANIMATION
		const hkQsTransform& parentTransform = poseLS->m_outputPose[ bones[ i ].m_boneB ];
		hkQsTransform& dressTransform = m_pose.m_outputPose[ bones[ i ].m_boneA ];
		dressTransform = parentTransform;
#else
		const RedQsTransform& parentTransform = poseLS->m_outputPose[ bones[ i ].m_boneB ];
		RedQsTransform& dressTransform = m_pose.m_outputPose[ bones[ i ].m_boneA ];
		dressTransform = parentTransform;
#endif
	}
#ifdef USE_HAVOK_ANIMATION
	// Calc roll - get bone directions
	hkQsTransform& boneLeft = m_pose.m_outputPose[ m_dressBoneIndices[ BI_ThighLeft ] ];
	hkQsTransform& boneRight = m_pose.m_outputPose[ m_dressBoneIndices[ BI_ThighRight ] ];

	Matrix boneLeftMatrix;
	Matrix boneRightMatrix;

	HavokTransformToMatrix_Renormalize( boneLeft, &boneLeftMatrix );
	HavokTransformToMatrix_Renormalize( boneRight, &boneRightMatrix );

	Vector dirXBoneLeft = boneLeftMatrix.GetAxisX();
	Vector dirXBoneRight = boneRightMatrix.GetAxisX();

	hkQsTransform& boneLeft2 = m_pose.m_outputPose[ m_dressBoneIndices[ BI_ShinLeft ] ];
	hkQsTransform& boneRight2 = m_pose.m_outputPose[ m_dressBoneIndices[ BI_ShinRight ] ];

	Matrix boneLeftMatrix2;
	Matrix boneRightMatrix2;

	HavokTransformToMatrix_Renormalize( boneLeft2, &boneLeftMatrix2 );
	HavokTransformToMatrix_Renormalize( boneRight2, &boneRightMatrix2 );

	Vector dirXBoneLeft2 = boneLeftMatrix2.GetAxisX();
	Vector dirXBoneRight2 = boneRightMatrix2.GetAxisX();
#else
	// Calc roll - get bone directions
	RedQsTransform& boneLeft = m_pose.m_outputPose[ m_dressBoneIndices[ BI_ThighLeft ] ];
	RedQsTransform& boneRight = m_pose.m_outputPose[ m_dressBoneIndices[ BI_ThighRight ] ];

	Matrix boneLeftMatrix;
	Matrix boneRightMatrix;
	RedMatrix4x4 conversionMatrix;
	
	conversionMatrix = boneLeft.ConvertToMatrixNormalized();
	boneLeftMatrix = reinterpret_cast<const Matrix&>( conversionMatrix );
	conversionMatrix = boneRight.ConvertToMatrixNormalized();
	boneRightMatrix = reinterpret_cast<const Matrix&>( conversionMatrix );

	Vector dirXBoneLeft = boneLeftMatrix.GetAxisX();
	Vector dirXBoneRight = boneRightMatrix.GetAxisX();

	RedQsTransform& boneLeft2 = m_pose.m_outputPose[ m_dressBoneIndices[ BI_ShinLeft ] ];
	RedQsTransform& boneRight2 = m_pose.m_outputPose[ m_dressBoneIndices[ BI_ShinRight ] ];

	Matrix boneLeftMatrix2;
	Matrix boneRightMatrix2;
		
	conversionMatrix = boneLeft2.ConvertToMatrixNormalized();
	boneLeftMatrix2 = reinterpret_cast<const Matrix&>( conversionMatrix );
	conversionMatrix = boneRight2.ConvertToMatrixNormalized();
	boneRightMatrix2 = reinterpret_cast<const Matrix&>( conversionMatrix );

	Vector dirXBoneLeft2 = boneLeftMatrix2.GetAxisX();
	Vector dirXBoneRight2 = boneRightMatrix2.GetAxisX();
#endif
	// Calc diff
	Float diff = -(dirXBoneLeft.X - dirXBoneRight.X);

		Vector Angles;
		Angles.X=dirXBoneLeft.X;
		Angles.Y=dirXBoneRight.X;
		Angles.Z=dirXBoneLeft2.Y;
		Angles.W=dirXBoneRight2.Y;
		//Angles.Normalize4();

		Vector AnglesPerfect;
		AnglesPerfect.X=-1;
		AnglesPerfect.Y=-1;
		AnglesPerfect.Z=-1;
		AnglesPerfect.W=-1;
		//AnglesPerfect.Normalize4();

		Float bsk = pow((AnglesPerfect.X-Angles.X),2) + pow((AnglesPerfect.Y-Angles.Y),2) + pow((AnglesPerfect.Z-Angles.Z),2) + pow((AnglesPerfect.W-Angles.W),2);
		bsk/=16.0f;
		Float w = 1-bsk;

		//Float w = Angles.Dot4(Angles,AnglesPerfect);

		Float bias=m_ofweight;
		w=(w-bias)/(1.0f-bias);

		if(w>1){w=1;}
		if(w<0){w=0;}

		//BEH_LOG( TXT("wek: [%f,%f,%f,%f] waga: %f"), Angles.X, Angles.Y, Angles.Z, Angles.W, w);

#ifdef USE_HAVOK_ANIMATION
	EulerAngles ea1 = EulerAngles(m_r1.X,m_r1.Y,m_r1.Z);
	EulerAngles ea2 = EulerAngles(m_r2.X,m_r2.Y,m_r2.Z);
	EulerAngles ea3 = EulerAngles(m_r3.X,m_r3.Y,m_r3.Z);
	hkQuaternion q1,q2,q3;
	EulerAnglesToHavokQuaternion( ea1, q1 );
	EulerAnglesToHavokQuaternion( ea2, q2 );
	EulerAnglesToHavokQuaternion( ea3, q3 );

	//w = 0.f;

	hkQsTransform ident( hkVector4(0.0f, 0.0f, 0.0f, 0.0f ), hkQuaternion(0,0,0,1), hkVector4(1.0f, 1.0f, 1.0f, 1.0f ) );
	hkQsTransform matshin( hkVector4(m_p1.X, m_p1.Y, m_p1.Z, 0.0f ), q1, hkVector4(1.0f, 1.0f, 1.0f, 1.0f ) );
	hkQsTransform matroll( hkVector4(m_p2.X, m_p2.Y, m_p2.Z, 0.0f ), q2, hkVector4(1.0f, 1.0f, 1.0f, 1.0f ) );
	hkQsTransform matth( hkVector4(m_p3.X, m_p3.Y, m_p3.Z, 0.0f ), q3, hkVector4(1.0f, 1.0f, 1.0f, 1.0f ) );

	matshin.setInterpolate4(ident, matshin, w);
	matroll.setInterpolate4(ident, matroll, w);
	matth.setInterpolate4(ident, matth, w);
#else
	RedEulerAngles ea1 = RedEulerAngles(m_r1.X,m_r1.Y,m_r1.Z);
	RedEulerAngles ea2 = RedEulerAngles(m_r2.X,m_r2.Y,m_r2.Z);
	RedEulerAngles ea3 = RedEulerAngles(m_r3.X,m_r3.Y,m_r3.Z);
	RedQuaternion q1( ea1.ToQuaternion() );
	RedQuaternion q2( ea2.ToQuaternion() );
	RedQuaternion q3( ea3.ToQuaternion() );


	RedQsTransform ident( RedVector4(0.0f, 0.0f, 0.0f, 0.0f ), RedQuaternion(0,0,0,1), RedVector4(1.0f, 1.0f, 1.0f, 1.0f ) );
	RedQsTransform matshin( RedVector4(m_p1.X, m_p1.Y, m_p1.Z, 0.0f ), q1, RedVector4(1.0f, 1.0f, 1.0f, 1.0f ) );
	RedQsTransform matroll( RedVector4(m_p2.X, m_p2.Y, m_p2.Z, 0.0f ), q2, RedVector4(1.0f, 1.0f, 1.0f, 1.0f ) );
	RedQsTransform matth( RedVector4(m_p3.X, m_p3.Y, m_p3.Z, 0.0f ), q3, RedVector4(1.0f, 1.0f, 1.0f, 1.0f ) );

	matshin.Lerp(ident, matshin, w);
	matroll.Lerp(ident, matroll, w);
	matth.Lerp(ident, matth, w);
#endif

	// Roll dress bones
	RollBone( m_pose, m_dressBoneIndices[ BI_ThighLeft ],	m_dressBoneIndices[ BI_ShinLeft ], diff * m_thighBoneWeight, matth, NULL );
	RollBone( m_pose, m_dressBoneIndices[ BI_ThighRight ], m_dressBoneIndices[ BI_ShinRight ], diff * m_thighBoneWeight, matth, NULL );

	RollBone( m_pose, m_dressBoneIndices[ BI_ShinLeft ], m_dressBoneIndices[ BI_KneeRollLeft ], diff * m_shinBoneWeight, matshin, NULL );
	RollBone( m_pose, m_dressBoneIndices[ BI_ShinRight ], m_dressBoneIndices[ BI_KneeRollRight ], diff * m_shinBoneWeight, matshin, NULL );

	RollBone( m_pose, m_dressBoneIndices[ BI_KneeRollLeft ], -1, diff * m_kneeRollBoneWeight, matroll, NULL );
	RollBone( m_pose, m_dressBoneIndices[ BI_KneeRollRight ], -1, diff * m_kneeRollBoneWeight, matroll, NULL );
}

void CDressMeshComponent::ForceTPose()
{
	if ( m_skeleton && m_pose.IsValid() )
	{
		m_pose.SetPose( m_skeleton.Get() );

#ifdef USE_HAVOK_ANIMATION
		m_pose.m_deltaReferenceFrameLocal.setIdentity();
#else	
		m_pose.m_deltaReferenceFrameLocal.SetIdentity();
#endif
		m_pose.ClearEventsAndUsedAnims();

		ScheduleUpdateTransformNode();
	}
}

void CDressMeshComponent::OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	PC_SCOPE( CDressMeshComponent );

	TBaseClass::OnUpdateTransformComponent( context, prevLocalToWorld );
	
	if ( m_pose.m_numBones == 0 || m_pose.m_outputPose == NULL )
	{
		return;
	}

	if ( !m_skinningData || m_skeletonModelSpace.SizeInt() != m_skeleton->GetBonesNum() )
	{
		SAFE_RELEASE( m_skinningData );
		m_skinningData = GRender->CreateSkinningBuffer( m_skinningMapping.Size(), true );
	}
	
	if ( m_skeletonModelSpace.SizeInt() != m_skeleton->GetBonesNum() )
	{
		m_skeletonModelSpace.Resize( m_skeleton->GetBonesNum() );
		m_skeletonWorldSpace.Resize( m_skeleton->GetBonesNum() );
	}

	m_pose.GetBonesModelSpace( m_skeleton.Get(), m_skeletonModelSpace );

	ASSERT( m_skeletonModelSpace.Size() == m_skeletonWorldSpace.Size() );
	SkeletonBonesUtils::MulMatrices( m_skeletonModelSpace.TypedData(), m_skeletonWorldSpace.TypedData(), m_skeletonModelSpace.Size(), &m_localToWorld );

	CMesh* theMeshResource = GetMeshNow();
	if( theMeshResource )
	{
		Box boxMS;
		MeshUtilities::UpdateTransformAndSkinningDataMS( this, theMeshResource, m_skinningMapping, GetLocalToWorld(), GetRenderProxy(), m_skinningData, boxMS );
		const Box boxWS = GetLocalToWorld().TransformBox( boxMS );
		SetBoundingBox( boxWS );
	}
}
#ifdef USE_HAVOK_ANIMATION
void CDressMeshComponent::RollBone( SBehaviorGraphOutput& pose, Int32 boneIndex, Int32 childBoneIndex, Float angle, hkQsTransform & offset, const CAnimatedComponent* animatedComponent ) const
{
	// Roll only bone without bone's child

	hkQuaternion rollQuat( hkVector4( 1, 0, 0, 1 ), angle );
	hkQsTransform roll( hkVector4(0.0f, 0.0f, 0.0f, 0.0f ), rollQuat, hkVector4(1.0f, 1.0f, 1.0f, 1.0f ) );

	ASSERT( boneIndex != -1 );

	if ( childBoneIndex != -1 ) 
	{
		hkQsTransform preChildBoneMS = pose.GetBoneModelTransform( childBoneIndex, m_skeleton->GetHavokSkeleton()->m_parentIndices );

		hkQsTransform& bone = pose.m_outputPose[ boneIndex ];
		//hkQsTransform off = bone;
		//off.setMul( bone, offset );
		bone.setMul( bone, offset );
		bone.setMul( bone, roll );


		hkQsTransform postChildBoneMS = pose.GetBoneModelTransform( childBoneIndex, m_skeleton->GetHavokSkeleton()->m_parentIndices );

		hkQsTransform childBoneDiff;
		childBoneDiff.setMulInverseMul( postChildBoneMS, preChildBoneMS );

		hkQsTransform& childBone = pose.m_outputPose[ childBoneIndex ];
		childBone.setMul( childBone, childBoneDiff );
	}
	else
	{
		hkQsTransform& bone = pose.m_outputPose[ boneIndex ];
		bone.setMul( bone, roll );
		bone.setMul( bone, offset );
	}
}
#else
void CDressMeshComponent::RollBone( SBehaviorGraphOutput& pose, Int32 boneIndex, Int32 childBoneIndex, Float angle, RedQsTransform & offset, const CAnimatedComponent* animatedComponent ) const
{
	// Roll only bone without bone's child

	RedQuaternion rollQuat( RedVector4( 1, 0, 0, 1 ), angle );
	RedQsTransform roll( RedVector4(0.0f, 0.0f, 0.0f, 0.0f ), rollQuat, RedVector4(1.0f, 1.0f, 1.0f, 1.0f ) );

	ASSERT( boneIndex != -1 );

	if ( childBoneIndex != -1 ) 
	{
		//TODO: No havok skeleton SO...
		//RedQsTransform preChildBoneMS = pose.GetBoneModelTransform( childBoneIndex, m_skeleton->GetHavokSkeleton()->m_parentIndices );

		RedQsTransform& bone = pose.m_outputPose[ boneIndex ];
		
		bone.SetMul( bone, offset );
		bone.SetMul( bone, roll );


		//RedQsTransform postChildBoneMS = pose.GetBoneModelTransform( childBoneIndex, m_skeleton->GetHavokSkeleton()->m_parentIndices );

		RedQsTransform childBoneDiff;
		//childBoneDiff.SetMulInverseMul( postChildBoneMS, preChildBoneMS );

		RedQsTransform& childBone = pose.m_outputPose[ childBoneIndex ];
		childBone.SetMul( childBone, childBoneDiff );
	}
	else
	{
		RedQsTransform& bone = pose.m_outputPose[ boneIndex ];
		bone.SetMul( bone, roll );
		bone.SetMul( bone, offset );
	}
}
#endif
const ISkeletonDataProvider* CDressMeshComponent::QuerySkeletonDataProvider() const
{
	return static_cast< const ISkeletonDataProvider* >( this );
}

IAnimatedObjectInterface* CDressMeshComponent::QueryAnimatedObjectInterface()
{
	return static_cast< IAnimatedObjectInterface* >( this );
}

Int32 CDressMeshComponent::FindBoneByName( const Char* name ) const
{
	return m_skeleton ? m_skeleton->FindBoneByName( name ) : -1;
}

Uint32 CDressMeshComponent::GetRuntimeCacheIndex() const
{
	return m_skeleton ? m_skeleton->GetRuntimeIndex() : 0;
}

const struct SSkeletonSkeletonCacheEntryData* CDressMeshComponent::GetSkeletonMaping( const ISkeletonDataProvider* parentSkeleton ) const
{
	return m_skeleton ? m_skeleton->GetMappingCache().GetMappingEntry( parentSkeleton ) : &SSkeletonSkeletonCacheEntryData::FAKE_DATA;
}

Uint32 CDressMeshComponent::GetBones( TDynArray< BoneInfo >& bones ) const
{
	if ( m_skeleton.IsValid() )
	{
		return m_skeleton->GetBones( bones );
	}
	return 0;
}

Uint32 CDressMeshComponent::GetBones( TAllocArray< BoneInfo >& bones ) const
{
	if ( m_skeleton.IsValid() )
	{
		return m_skeleton->GetBones( bones );
	}
	return 0;
}

Matrix CDressMeshComponent::CalcBoneMatrixModelSpace( Uint32 boneIndex ) const
{
	ASSERT( 0 );
	return Matrix::IDENTITY;
}

Matrix CDressMeshComponent::GetBoneMatrixModelSpace( Uint32 boneIndex ) const
{
	ASSERT( 0 );
	return Matrix::IDENTITY;
}

Matrix CDressMeshComponent::GetBoneMatrixWorldSpace( Uint32 boneIndex ) const
{
	ASSERT( 0 );
	return Matrix::IDENTITY;
}

void CDressMeshComponent::GetBoneMatricesAndBoundingBoxModelSpace( const ISkeletonDataProvider::SBonesData& bonesData ) const
{
	SkeletonBonesUtils::GetBoneMatricesModelSpace( bonesData, m_skeletonModelSpace );
	SkeletonBonesUtils::CalcBoundingBoxModelSpace( bonesData, nullptr, 0, m_skeletonModelSpace );
}

CEntity* CDressMeshComponent::GetAnimatedObjectParent() const
{
	return GetEntity();
}

Bool CDressMeshComponent::HasSkeleton() const
{
	return m_skeleton.IsValid();
}

Bool CDressMeshComponent::HasTrajectoryBone() const
{
	return false;
}

Int32 CDressMeshComponent::GetTrajectoryBone() const
{
	return -1;
}

Bool CDressMeshComponent::UseExtractedMotion() const
{
	return false;
}

Bool CDressMeshComponent::UseExtractedTrajectory() const
{
	return false;
}

Int32 CDressMeshComponent::GetBonesNum() const
{
	return m_skeleton ? m_skeleton->GetBonesNum() : 0;
}

Int32 CDressMeshComponent::GetTracksNum() const
{
	return m_skeleton ? m_skeleton->GetTracksNum() : 0;
}

Int32 CDressMeshComponent::GetParentBoneIndex( Int32 bone ) const
{
	return m_skeleton ? m_skeleton->GetParentBoneIndex( bone ) : -1;
}

const Int16* CDressMeshComponent::GetParentIndices() const
{
	return m_skeleton ? m_skeleton->GetParentIndices() : NULL;
}

CEventNotifier< CAnimationEventFired >* CDressMeshComponent::GetAnimationEventNotifier( const CName &eventName )
{
	return NULL;
}

void CDressMeshComponent::PlayEffectForAnimation( const CName& animation, Float time )
{
	GetEntity()->PlayEffectForAnimation( animation, time );
}

void CDressMeshComponent::MakeDressMapping()
{
	// Push special bones to bone table - use EBoneIndex sequence:
	// BI_ThighLeft, BI_ShinLeft, BI_KneeRollLeft, BI_ThighRight, BI_ShinRight, BI_KneeRollRight, BI_Root, BI_Pelvis

	const AnsiChar* boneTable[] = { 
		"l_thigh", 
		"l_shin", 
		"l_kneeRoll", 
		"r_thigh", 
		"r_shin", 
		"r_kneeRoll", 
		"Root", 
		"pelvis"
	};

	// Process initial bones
	for ( Uint32 i=0; i<ARRAY_COUNT(boneTable); ++i )
	{
		// Get bone and parent bone
		Int32 dressIndex = m_skeleton->FindBoneByName( boneTable[ i ] );

		// Bone was not mapped
		if ( dressIndex == -1 )
		{
			// Clear whole mapping
			m_dressBoneIndices.Clear();

			// Done
			return;
		}

		// Add to mapping
		m_dressBoneIndices.PushBack( dressIndex );
	}

	// Process other bones from main skeleton
	for ( BoneIterator it( m_skeleton.Get() ); it; ++it )
	{
		const AnsiChar* boneName = it.GetName();

		// Skip bones that were already added
		Bool wasAlreadyAdded = false;
		for ( Uint32 j=0; j<ARRAY_COUNT(boneTable); ++j )
		{
			if ( 0 == Red::System::StringCompareNoCase( boneTable[j], boneName ) )
			{
				wasAlreadyAdded = true;
				break;
			}
		}

		// Do not add if was already added
		if ( wasAlreadyAdded )
		{
			continue;
		}

		// Get bone and parent bone
		Int32 dressIndex = it.GetIndex();

		// Bone was not mapped
		if ( dressIndex == -1 )
		{
			// Clear whole mapping
			m_dressBoneIndices.Clear();

			// Done
			return;
		}

		// Add to mapping
		m_dressBoneIndices.PushBack( dressIndex );
	}

	// Shrink to reduce memory usage
	m_dressBoneIndices.Shrink();

	m_skeletonWorldSpace.Resize( m_dressBoneIndices.Size() );
	m_skeletonWorldSpace.Shrink();
	m_pose.Init( m_dressBoneIndices.Size(), 0 );
}

void CDressMeshComponent::MakeSkinningMapping()
{
	// Get skinned mesh
	CMesh* mesh = GetMeshNow();

	// No mesh, no mapping
	if ( !mesh )
	{
		m_skinningMapping.Clear();
		return;
	}

	// Get bones from source skeleton
	TDynArray< ISkeletonDataProvider::BoneInfo > sourceBones;
	GetBones( sourceBones );

	// Map mesh bones
	Uint32 numNotMappedBones = 0;
	Uint32 numMeshBones = mesh->GetBoneCount();
	const CName* meshBoneNames = mesh->GetBoneNames();
	m_skinningMapping.Resize( numMeshBones );
	for ( Uint32 i=0; i<numMeshBones; i++ )
	{
		// Find bone in source skeleton
		Int32 mappedBoneIndex = -1;
		for ( Uint32 j=0; j<sourceBones.Size(); j++ )
		{
			const ISkeletonDataProvider::BoneInfo& sourceBone = sourceBones[j];
			if ( sourceBone.m_name == meshBoneNames[i] )
			{
				mappedBoneIndex = j;
				break;
			}
		}

		// Bone was not mapped, inform user
		if ( mappedBoneIndex == -1 )
		{
			numNotMappedBones++;
		}

		// Remember
		m_skinningMapping[ i ] = mappedBoneIndex;
	}

	// Show error
	if ( numNotMappedBones > 0 && numMeshBones != 0 )
	{
		WARN_ENGINE( TXT("%i bones were not mapped in '%ls' when attaching '%ls'"), numNotMappedBones, GetParent()->GetFriendlyName().AsChar(), mesh->GetFriendlyName().AsChar() );
	}
}

void CDressMeshComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	// Pass to base class
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	if ( m_skeleton && flag == SHOW_Behavior )
	{
		SkeletonRenderingUtils::DrawSkeleton( m_skeletonWorldSpace, m_skeleton.Get(), Color( 255, 255, 255 ), frame, true, false, true );
	}
}

void CDressMeshComponent::OnUpdateBounds()
{
	// Do nothing - bounding box is updated in OnParentPoseApplied
}
