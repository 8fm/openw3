
#pragma once

#include "behaviorGraphOutput.h"
#include "meshComponent.h"
#include "skeletonProvider.h"
#include "animatedObjectInterface.h"

class CBehaviorGraphConstraintNodeDress;
class CAnimatedComponent;
class IRenderSkinningData;

class CDressMeshComponent : public CMeshComponent, public ISkeletonDataProvider, public IAnimatedObjectInterface
{
	typedef TDynArray< Int16 > TBoneMapping;

	DECLARE_ENGINE_CLASS( CDressMeshComponent, CMeshComponent, 0 );

protected:
	THandle< CSkeleton >	m_skeleton;

	Float				m_thighBoneWeight;
	Float				m_shinBoneWeight;
	Float				m_kneeRollBoneWeight;

	Float				m_ofweight;
	Vector				m_p1;
	Vector				m_p2;
	Vector				m_p3;

	Vector				m_r1;
	Vector				m_r2;
	Vector				m_r3;

	TDynArray< Matrix >		m_skeletonModelSpace;
	TDynArray< Matrix >		m_skeletonWorldSpace; //<-- To REMOVED!!!

	IRenderSkinningData*	m_skinningData;

protected:
	TBoneMapping			m_skinningMapping;
	TBoneMapping			m_dressBoneIndices;

	SBehaviorGraphOutput	m_pose;
	
	enum EBoneIndex		{ BI_ThighLeft, BI_ShinLeft, BI_KneeRollLeft, BI_ThighRight, BI_ShinRight, BI_KneeRollRight, BI_Root, BI_Pelvis };

public:
	RED_INLINE void SetSkeleton( CSkeleton* skeleton ) { m_skeleton = skeleton; }

public:
	// Find bone by name
	virtual Int32 FindBoneByName( const Char* name ) const;

	// Get bones in the skeleton, returns number of bones in skeleton
	virtual Uint32 GetBones( TDynArray< BoneInfo >& bones ) const;

	// Get bones in the skeleton, returns number of bones in skeleton
	virtual Uint32 GetBones( TAllocArray< BoneInfo >& bones ) const;

	// Calc bone matrix in model space
	virtual Matrix CalcBoneMatrixModelSpace( Uint32 boneIndex ) const;

	// Get bone matrix in model space
	virtual Matrix GetBoneMatrixModelSpace( Uint32 boneIndex ) const;

	// Get bone matrix in world space
	virtual Matrix GetBoneMatrixWorldSpace( Uint32 boneIndex ) const;

	// Get bone matrices in the world space
	virtual void GetBoneMatricesAndBoundingBoxModelSpace( const ISkeletonDataProvider::SBonesData& bonesData ) const override;

	// Get the runtime cache index
	virtual Uint32 GetRuntimeCacheIndex() const override;

	// Create skeleton mapping for another skeleton
	virtual const struct SSkeletonSkeletonCacheEntryData* GetSkeletonMaping( const ISkeletonDataProvider* parentSkeleton ) const override;

public:
	CDressMeshComponent();
	virtual ~CDressMeshComponent();

	// Attach
	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );

	// Get animated object interface
	virtual IAnimatedObjectInterface* QueryAnimatedObjectInterface();

	//! Get skeleton data provider interface
	virtual const ISkeletonDataProvider* QuerySkeletonDataProvider() const;

	// Update world space bounding box
	virtual void OnUpdateBounds();

	// Update transform
	virtual void OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;

	// Generate editor fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

public:
	virtual void OnParentUpdatedAttachedAnimatedObjectsLS( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSAsync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );

public:
	virtual CEntity* GetAnimatedObjectParent() const;

	virtual Bool HasSkeleton() const;

	virtual Bool HasTrajectoryBone() const;
	virtual Int32 GetTrajectoryBone() const;

	virtual Bool UseExtractedMotion() const;
	virtual Bool UseExtractedTrajectory() const;

	virtual Int32 GetBonesNum() const;
	virtual Int32 GetTracksNum() const;

	virtual Int32 GetParentBoneIndex( Int32 bone ) const;
	virtual const Int16* GetParentIndices() const;

	virtual CEventNotifier< CAnimationEventFired >*	GetAnimationEventNotifier( const CName &eventName );

	virtual void PlayEffectForAnimation( const CName& animation, Float time );

protected:
	Bool IsValid() const;

	void MakeSkinningMapping();

	void MakeDressMapping();
#ifdef USE_HAVOK_ANIMATION
	void RollBone( SBehaviorGraphOutput& pose, Int32 boneIndex, Int32 childBoneIndex, Float angle, hkQsTransform & offset, const CAnimatedComponent* animatedComponent ) const;
#else
	void RollBone( SBehaviorGraphOutput& pose, Int32 boneIndex, Int32 childBoneIndex, Float angle, RedQsTransform & offset, const CAnimatedComponent* animatedComponent ) const;
#endif

	void ProcessParentPose( SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );

	void ForceTPose();
};

BEGIN_CLASS_RTTI( CDressMeshComponent )
	PARENT_CLASS( CMeshComponent )
	PROPERTY_EDIT( m_skeleton, TXT( "Dress skeleton" ) )
	PROPERTY_EDIT( m_thighBoneWeight, TXT("") )
	PROPERTY_EDIT( m_shinBoneWeight, TXT("") )
	PROPERTY_EDIT( m_kneeRollBoneWeight, TXT("") )
	PROPERTY_EDIT( m_ofweight, TXT("") )
	PROPERTY_EDIT( m_p1, TXT("") )
	PROPERTY_EDIT( m_p2, TXT("") )
	PROPERTY_EDIT( m_p3, TXT("") )
	PROPERTY_EDIT( m_r1, TXT("") )
	PROPERTY_EDIT( m_r2, TXT("") )
	PROPERTY_EDIT( m_r3, TXT("") )
END_CLASS_RTTI()