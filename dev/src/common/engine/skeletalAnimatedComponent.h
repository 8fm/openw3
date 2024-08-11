/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "animationController.h"
#include "skeletonProvider.h"
#include "slotProvider.h"
#include "skeletalAnimationSet.h"
#include "component.h"

class CSkeletalAnimationSet;

class CSkeletalAnimatedComponent	: public CComponent
									, public ISkeletonDataProvider
									, protected ISlotProvider
{
	DECLARE_ENGINE_CLASS( CSkeletalAnimatedComponent, CComponent, 0 );

protected:
	TDynArray< Matrix >					m_skeletonWorldSpace;
	TDynArray< Matrix >					m_skeletonModelSpace;

protected:
	THandle< CSkeleton >				m_skeleton;
	THandle< CSkeletalAnimationSet >	m_animset;
	IAnimationController*				m_controller;
	Bool								m_processEvents;

protected:
	Bool								m_isOk;

public:
	//! Constructor
	CSkeletalAnimatedComponent();

	//! Component was attached to world
	virtual void OnAttached( CWorld* world ) override;

	//! Component was detached from world
	virtual void OnDetached( CWorld *world ) override;

public:
#ifndef NO_EDITOR
	//! You need only this
	virtual void UpdateAsync( Float timeDelta );

	//! Calc box
	virtual void CalcBox( Box& box ) const;
#else
	//! You need only this
	void UpdateAsync( Float timeDelta );

	//! Calc box
	void CalcBox( Box& box ) const;
#endif

	//! Is valid
	Bool IsValid() const;

	//! Initialize
	void Initialize();

	//! Deinitialize
	void Deinitialize();

public:
	// Sync to
	void SyncTo( const CSyncInfo& info );

	// Get sync info
	Bool GetSyncInfo( CSyncInfo& info );

	//! Sync to rand pose
	void RandSync();

public: // ISkeletonDataProvider
	virtual const ISkeletonDataProvider* QuerySkeletonDataProvider() const;

	virtual Int32 FindBoneByName( const Char* name ) const;
	virtual Int32 FindBoneByName( const AnsiChar* name ) const;
	using ISkeletonDataProvider::FindBoneByName;

	virtual Int32 GetBonesNum() const;

	virtual Uint32 GetBones( TDynArray< BoneInfo >& bones ) const;
	virtual Uint32 GetBones( TAllocArray< BoneInfo >& bones ) const;

	virtual Matrix CalcBoneMatrixModelSpace( Uint32 boneIndex ) const;
	virtual Matrix GetBoneMatrixModelSpace( Uint32 boneIndex ) const;
	virtual Matrix GetBoneMatrixWorldSpace( Uint32 boneIndex ) const;

	virtual Uint32 GetRuntimeCacheIndex() const  override;
	virtual const struct SSkeletonSkeletonCacheEntryData* GetSkeletonMaping( const ISkeletonDataProvider* parentSkeleton ) const override;

	virtual void GetBoneMatricesAndBoundingBoxModelSpace( const ISkeletonDataProvider::SBonesData& bonesData ) const override;

public:
	virtual ISlotProvider* QuerySlotProvider();

	virtual ISlot* CreateSlot( const String& slotName );
	virtual void EnumSlotNames( TDynArray< String >& slotNames ) const;

protected:
	void CreateMatrix();
	void InitController();
	void DestroyController();

	void SampleAnimations();

	void UpdateAttachedSkinningComponentsTransforms() const;

	void ProcessAnimEvents( const SBehaviorGraphOutput* pose );

#ifndef NO_EDITOR
public:
	void SetController( IAnimationController* controller )	{ m_controller = controller; }
	IAnimationController* GetController()					{ return m_controller; }

	void SetAnimset( CSkeletalAnimationSet* set )			{ m_animset = set; }

	void SetSkeleton( CSkeleton* skeleton )					{ m_skeleton = skeleton; }
#endif
};

BEGIN_CLASS_RTTI( CSkeletalAnimatedComponent )
	PARENT_CLASS( CComponent );
	PROPERTY_EDIT( m_skeleton, TXT("Skeleton") );
	PROPERTY_EDIT( m_animset, TXT("Animset") );
	PROPERTY_INLINED( m_controller, TXT("Controller") );
	PROPERTY_EDIT( m_processEvents, TXT("") );
END_CLASS_RTTI();
