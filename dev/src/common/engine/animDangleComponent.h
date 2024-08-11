
#pragma once

#include "animatedObjectInterface.h"
#include "skeletonProvider.h"
#include "component.h"
#include "wetnessComponent.h"

class IAnimDangleConstraint;

class CAnimDangleComponent : public CComponent, public IAnimatedObjectInterface, public ISkeletonDataProvider
{
	DECLARE_ENGINE_CLASS( CAnimDangleComponent, CComponent, 0 );

	IAnimDangleConstraint*	m_constraint;
	Int32					m_attPrio;
	Bool					m_debugRender;

public:
	CAnimDangleComponent();

	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;

	virtual void OnChildAttachmentAdded( IAttachment* attachment ) override;
	virtual void OnChildAttachmentBroken( IAttachment* attachment ) override;

	virtual void OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;
	virtual void OnItemEntityAttached( const CEntity* par ) override;

#ifndef NO_EDITOR_FRAGMENTS
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags ) override;
#endif

	virtual IAnimatedObjectInterface* QueryAnimatedObjectInterface() override;

	virtual const ISkeletonDataProvider* QuerySkeletonDataProvider() const;

public: // customs
	virtual void SetShakeFactor( Float factor );

public:
	void SetBlendToAnimationWeight( Float w );
	
	void ForceReset();
	void ForceResetWithRelaxedState();

	void ShowDebugRender( Bool flag );

	void OnCollectAnimationSyncTokens( CName animationName, TDynArray< CAnimationSyncToken* >& tokens );

	virtual void SetResource( CResource* res ) override;
	virtual void GetResource( TDynArray< const CResource* >& resources ) const override;

public: // ISkeletonDataProvider
	virtual Int32 FindBoneByName( const Char* name ) const;
	virtual Uint32 GetBones( TDynArray< BoneInfo >& bones ) const;
	virtual Uint32 GetBones( TAllocArray< BoneInfo >& bones ) const;

	virtual Matrix CalcBoneMatrixModelSpace( Uint32 boneIndex ) const;
	virtual Matrix GetBoneMatrixModelSpace( Uint32 boneIndex ) const;
	virtual Matrix GetBoneMatrixWorldSpace( Uint32 boneIndex ) const;

	virtual void GetBoneMatricesAndBoundingBoxModelSpace( const ISkeletonDataProvider::SBonesData& bonesData ) const override;

public: // IAnimatedObjectInterface

	virtual CEntity* GetAnimatedObjectParent() const { return GetEntity(); }
	virtual Bool HasSkeleton() const { return false; }
	virtual Bool HasTrajectoryBone() const { return false; }
	virtual Int32 GetTrajectoryBone() const { return -1; }
	virtual Bool UseExtractedMotion() const { return false; }
	virtual Bool UseExtractedTrajectory() const { return false; }
	virtual Int32 GetBonesNum() const { return 0; }
	virtual Int32 GetTracksNum() const { return 0; }
	virtual Int32 GetParentBoneIndex( Int32 bone ) const { return 0; }
	virtual const Int16* GetParentIndices() const { return NULL; }
	virtual CEventNotifier< CAnimationEventFired >*	GetAnimationEventNotifier( const CName &eventName ) { return NULL; }
	virtual void PlayEffectForAnimation( const CName& animation, Float time ) {}
	virtual Uint32 GetRuntimeCacheIndex() const override;
	virtual const struct SSkeletonSkeletonCacheEntryData* GetSkeletonMaping( const ISkeletonDataProvider* parentSkeleton ) const override;

	virtual Int32 GetAttPrio() const;
	virtual void OnParentUpdatedAttachedAnimatedObjectsLS( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSAsync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSSync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSConstrainted( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );
	virtual void OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones );

	// reset physical simulation on component
	virtual void OnResetClothAndDangleSimulation() override;

#ifndef NO_EDITOR
public:
	Bool PrintDebugComment( String& str ) const;
#endif
};

BEGIN_CLASS_RTTI( CAnimDangleComponent )
	PARENT_CLASS( CComponent );
	PROPERTY_INLINED( m_constraint, String::EMPTY );
	PROPERTY_EDIT( m_attPrio, TXT("") )
	PROPERTY_EDIT( m_debugRender, TXT("") )
END_CLASS_RTTI();
