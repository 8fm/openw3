
#pragma once

#include "animDangleConstraint.h"
#include "showFlags.h"
#include "../core/allocArray.h"

class CAnimSkeletalDangleConstraint : public IAnimDangleConstraint
{
	DECLARE_ENGINE_CLASS( CAnimSkeletalDangleConstraint, IAnimDangleConstraint, 0 );

private:
	THandle< CSkeleton >	m_skeleton;
	Bool					m_dispSkeleton;
	Bool					m_dispBoneNames;
	Bool					m_dispBoneAxis;
	CWetnessSupplier*		m_wetnessSupplier;

protected:
	Bool					m_firstUpdate;	// This is a hack for attachments system. We don't have proper callbacks from appearance system.

	Box						m_bbox;
	TDynArray< Int32 >		m_dirtyBones;
	TDynArray< Matrix >		m_skeletonModelSpace;
	TDynArray< Matrix >		m_skeletonWorldSpace;


//////////////////////////////////////////////////////////////////////////
//++ FOR USERS
// You need to select one of the callback function from OnParentUpdatedAttachedAnimatedObjects_ function set. If you don't know what is the best for you please talk with Tomsin.
// If you want to modify bone please use SetBoneModelSpace function.
// In your update function always use StartUpdate/StartUpdateWithBoneAlignment and EndUpdate functions.
// You have to override HasCachedBones and CacheBones. Please cache all bones only there.

protected: // core
	RED_INLINE void SetBoneModelSpace( Int32 boneIdx, const Matrix& matrix ) { ASSERT( matrix.IsOk() ); m_skeletonModelSpace[ boneIdx ] = matrix; SetBoneDirty( boneIdx ); }

	void StartUpdate();
	void StartUpdateWithBoneAlignment( const TDynArray< Matrix >* poseMS, const TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones );
	void StartUpdateWithBoneAlignmentFull( const TDynArray< Matrix >* poseMS, const TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones );
	void EndUpdate( TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones );


protected: // helpers
	void OnDrawSkeletonWS( CRenderFrame* frame ) const;

	const CComponent* GetParentComponent() const;

	RED_INLINE const Matrix& GetBoneModelSpace( Int32 index ) const { return m_skeletonModelSpace[ index ]; }
	RED_INLINE const Matrix& GetBoneWorldSpace( Int32 index ) const { return m_skeletonWorldSpace[ index ]; }

	RED_INLINE Uint32 GetNumBonesModelSpace() const { return m_skeletonModelSpace.Size(); }
	RED_INLINE Uint32 GetNumBonesWorldSpace() const { return m_skeletonWorldSpace.Size(); }

	RED_INLINE Bool HasSkeletalMatrices() const { return GetNumBonesModelSpace() > 0 && GetNumBonesWorldSpace() > 0; }

	void ForceTPose();

protected: // bone caching
	// Returns true if you have cached bones already
	virtual Bool HasCachedBones() const;

	// Cache your bone index inside. Parent and/or skeleton can be null
	virtual void CacheBones( const CComponent* parent, const CSkeleton* skeleton );

	// Get skeleton function. Returns m_skeleton by default but you can return your skeleton.
	virtual const CSkeleton* GetSkeleton() const { return m_skeleton.Get(); }


//-- FOR USERS
//////////////////////////////////////////////////////////////////////////


public:
	CAnimSkeletalDangleConstraint();

#ifndef NO_EDITOR
	virtual void OnPropertyPostChange( IProperty* property );
#endif

	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );

	virtual void OnItemEntityAttached( const CEntity* par ) override;
	virtual void OnAttachmentAdded( const IAttachment* attachment );
	virtual void OnAttachmentBroken( const IAttachment* attachment );

	virtual void OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;

	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );

	virtual Int32 FindBoneByName( const Char* name ) const;
	virtual Uint32 GetBones( TDynArray< ISkeletonDataProvider::BoneInfo >& bones ) const;
	virtual Uint32 GetBones( TAllocArray< ISkeletonDataProvider::BoneInfo >& bones ) const;
	virtual Matrix CalcBoneMatrixModelSpace( Uint32 boneIndex ) const;
	virtual Matrix GetBoneMatrixModelSpace( Uint32 boneIndex ) const;
	virtual Matrix GetBoneMatrixWorldSpace( Uint32 boneIndex ) const;
	virtual void GetBoneMatricesAndBoundingBoxModelSpace( const ISkeletonDataProvider::SBonesData& bonesData ) const override;

	virtual Uint32 GetRuntimeCacheIndex() const override;
	virtual const struct SSkeletonSkeletonCacheEntryData* GetSkeletonMaping( const ISkeletonDataProvider* parentSkeleton ) const override;

	virtual void OnParentUpdatedAttachedAnimatedObjectsLS( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSAsync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSSync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSConstrainted( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );
	virtual void OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones );

private:
	void AlignBones( const TDynArray< Matrix >* poseMS, const TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones );

	RED_INLINE void ClearDirtyBones()				{ m_dirtyBones.ClearFast(); }			// ClearFast has to be here
	RED_INLINE void SetBoneDirty( Int32 boneIdx ) { m_dirtyBones.PushBack( boneIdx ); }

#ifndef NO_EDITOR
public:
	virtual Bool PrintDebugComment( String& str ) const override;
#endif
};

BEGIN_CLASS_RTTI( CAnimSkeletalDangleConstraint );
	PARENT_CLASS( IAnimDangleConstraint );
	PROPERTY_EDIT( m_skeleton, TXT("Skeleton") );
	PROPERTY_EDIT( m_dispSkeleton, String::EMPTY );
	PROPERTY_EDIT( m_dispBoneNames, String::EMPTY );
	PROPERTY_EDIT( m_dispBoneAxis, String::EMPTY );
END_CLASS_RTTI();
