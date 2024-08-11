
#pragma once

#include "skeletonProvider.h"
#include "showFlags.h"
#include "component.h"

class CRenderFrame;
struct SBehaviorGraphOutput;

//#define DEBUG_ANIM_CONSTRAINTS
//#define DEBUG_ANIM_DANGLES

class IAnimDangleConstraint : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IAnimDangleConstraint, CObject );

protected:
	const CComponent* GetComponent() const { return FindParent< CComponent >(); }

public:
	static void AlignBones( const TDynArray< Matrix >* poseMS, const TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones, TDynArray< Matrix >& skeletonModelSpace, TDynArray< Matrix >& skeletonWorldSpace );
	static void AlignBonesFull( const TDynArray< Matrix >* poseMS, const TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones, TDynArray< Matrix >& skeletonModelSpace, TDynArray< Matrix >& skeletonWorldSpace, const CSkeleton* skel );

public:
	virtual void OnAttached( CWorld* world ) {}
	virtual void OnDetached( CWorld* world ) {}

	virtual void OnAttachmentAdded( const IAttachment* attachment ) {}
	virtual void OnAttachmentBroken( const IAttachment* attachment ) {}

	virtual void OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) {}
	virtual void OnItemEntityAttached( const CEntity* par ) {}

	virtual void SetBlendToAnimationWeight( Float w ) {}
	virtual void ForceReset() {}
	virtual void ForceResetWithRelaxedState() {}

	virtual void OnShowDebugRender( Bool flag ) {}
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags ) {}

	virtual void OnCollectAnimationSyncTokens( CName animationName, TDynArray< CAnimationSyncToken* >& tokens ) {}

	virtual Int32 FindBoneByName( const Char* name ) const { return -1; }
	virtual Uint32 GetBones( TDynArray< ISkeletonDataProvider::BoneInfo >& bones ) const { return 0; }
	virtual Uint32 GetBones( TAllocArray< ISkeletonDataProvider::BoneInfo >& bones ) const { return 0; }

	virtual Matrix CalcBoneMatrixModelSpace( Uint32 boneIndex ) const { return Matrix::IDENTITY; }
	virtual Matrix GetBoneMatrixModelSpace( Uint32 boneIndex ) const { return Matrix::IDENTITY; }
	virtual Matrix GetBoneMatrixWorldSpace( Uint32 boneIndex ) const { return Matrix::IDENTITY; }

	virtual void GetBoneMatricesAndBoundingBoxModelSpace( const ISkeletonDataProvider::SBonesData& bonesData ) const {}

	virtual Uint32 GetRuntimeCacheIndex() const = 0;
	virtual const struct SSkeletonSkeletonCacheEntryData* GetSkeletonMaping( const ISkeletonDataProvider* parentSkeleton ) const = 0;

	virtual void OnParentUpdatedAttachedAnimatedObjectsLS( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones ) {}
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSAsync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones ) {}
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSSync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones ) {}
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSConstrainted( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones ) {}
	virtual void OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones ) {}

public: // customs
	virtual void SetShakeFactor( Float factor ) {}

#ifndef NO_EDITOR
public:
	virtual Bool PrintDebugComment( String& str ) const;
#endif
};

BEGIN_ABSTRACT_CLASS_RTTI( IAnimDangleConstraint );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();
