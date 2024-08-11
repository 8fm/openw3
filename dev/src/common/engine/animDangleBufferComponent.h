
#pragma once
#include "component.h"
#include "animatedObjectInterface.h"
#include "skeletonProvider.h"
#include "wetnessComponent.h"

class CAnimDangleBufferComponent : public CComponent, public IAnimatedObjectInterface, public ISkeletonDataProvider
{
	DECLARE_ENGINE_CLASS( CAnimDangleBufferComponent, CComponent, 0 );

	struct AnimAttSorter
	{
		static RED_INLINE Bool Less( CAnimatedAttachment* const & a, CAnimatedAttachment* const & b )
		{
			CComponent* chA = a->GetChild() ? a->GetChild()->AsComponent() : nullptr;
			CComponent* chB = b->GetChild() ? b->GetChild()->AsComponent() : nullptr;

			IAnimatedObjectInterface* aA = chA ? chA->QueryAnimatedObjectInterface() : nullptr;
			IAnimatedObjectInterface* aB = chB ? chB->QueryAnimatedObjectInterface() : nullptr;

			if ( aA && aB )
			{
				return aA->GetAttPrio() < aB->GetAttPrio();
			}
			else
			{
				return a < b;
			}
		}
	};

private:
	THandle< CSkeleton >				m_skeleton;
	Bool								m_debugRender;
	CWetnessSupplier*					m_wetnessSupplier;

private:
	TSortedArray< CAnimatedAttachment*, AnimAttSorter >	m_cachedAnimatedAttachments;
	CAnimatedComponent*					m_cachedParentAnimatedComponent;

	Box									m_bbox;
	TDynArray< Matrix >					m_skeletonModelSpace;
	TDynArray< Matrix >					m_skeletonWorldSpace;

public:
	CAnimDangleBufferComponent();

	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;

	virtual void OnParentAttachmentBroken( IAttachment* attachment ) override;
	virtual void OnChildAttachmentAdded( IAttachment* attachment ) override;
	virtual void OnChildAttachmentBroken( IAttachment* attachment ) override;

	virtual void OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;

#ifndef NO_EDITOR_FRAGMENTS
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags ) override;
#endif

	virtual IAnimatedObjectInterface* QueryAnimatedObjectInterface() override;

	virtual const ISkeletonDataProvider* QuerySkeletonDataProvider() const;

private:
	void ForceTPose();

	const CSkeleton* GetSkeleton() const;

	void CacheAttachedToAnimParent();
	void CacheAnimatedAttachments();
	void CheckWetnessSupport();

public: // ISkeletonDataProvider
	virtual Int32 FindBoneByName( const Char* name ) const;
	virtual Uint32 GetBones( TDynArray< BoneInfo >& bones ) const;
	virtual Uint32 GetBones( TAllocArray< BoneInfo >& bones ) const;

	virtual Matrix CalcBoneMatrixModelSpace( Uint32 boneIndex ) const;
	virtual Matrix GetBoneMatrixModelSpace( Uint32 boneIndex ) const;
	virtual Matrix GetBoneMatrixWorldSpace( Uint32 boneIndex ) const;

	virtual void GetBoneMatricesAndBoundingBoxModelSpace( const ISkeletonDataProvider::SBonesData& bonesData ) const override;

public: // IAnimatedObjectInterface
	virtual CEntity* GetAnimatedObjectParent() const override { return GetEntity(); }
	virtual Bool HasSkeleton() const override;
	virtual Bool HasTrajectoryBone() const override { return false; }
	virtual Int32 GetTrajectoryBone() const override { return -1; }
	virtual Bool UseExtractedMotion() const override { return false; }
	virtual Bool UseExtractedTrajectory() const override { return false; }
	virtual Int32 GetBonesNum() const override;
	virtual Int32 GetTracksNum() const override;
	virtual Int32 GetParentBoneIndex( Int32 bone ) const override;
	virtual const Int16* GetParentIndices() const override;
	virtual CEventNotifier< CAnimationEventFired >*	GetAnimationEventNotifier( const CName &eventName ) override { return NULL; }
	virtual void PlayEffectForAnimation( const CName& animation, Float time ) override {}
	virtual Uint32 GetRuntimeCacheIndex() const override;
	virtual const struct SSkeletonSkeletonCacheEntryData* GetSkeletonMaping( const ISkeletonDataProvider* parentSkeleton ) const override;

	virtual void OnParentUpdatedAttachedAnimatedObjectsLS( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSAsync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSSync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSConstrainted( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );
	virtual void OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones );
};

BEGIN_CLASS_RTTI( CAnimDangleBufferComponent )
	PARENT_CLASS( CComponent );
	PROPERTY_EDIT( m_skeleton, TXT("") )
	PROPERTY_EDIT( m_debugRender, TXT("") )
END_CLASS_RTTI();
