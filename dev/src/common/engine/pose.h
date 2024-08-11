/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "animMath.h"
#include "poseHandle.h"

class CPose
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Animation );

public:
	CPoseHandle				m_poseLS;
	CPoseHandle				m_poseMS;

private:
	enum EPoseBoneFlag : Uint8
	{
		PBF_LocalDirty =			FLAG( 0 ),
		PBF_ModelDirty =			FLAG( 1 ),
		PBF_InternalCalcMS =		FLAG( 3 ),
		PBF_InternalLS =			FLAG( 4 ),
	};

	mutable TDynArray< Uint8, MC_Animation > m_boneFlags;

	const CSkeleton*		m_skeleton;
	Bool					m_modelSync;
	Bool					m_localSync;

public:
	static Uint32 GetMemSize( const Uint32 numBones ); // For alloca type of allocation

public:
	CPose( const CSkeleton* skeleton );

	Bool IsOk() const;

	Bool IsSyncedLS() const;
	Bool IsSyncedMS() const;

	void ResetPoseLS( Bool forceTPose );
	void SetTPoseLS();

	void SyncLSFromMS();
	void SyncMSFromLS();
	void SyncAll();

	const AnimQsTransform& GetBoneLS( const Int32 boneIdx ) const;
	const AnimQsTransform& GetBoneMS( const Int32 boneIdx ) const;

	AnimQsTransform& AccessBoneLS( const Int32 boneIdx );
	AnimQsTransform& AccessBoneMS( const Int32 boneIdx, const Bool propagate );

	void SetBoneLS( const Int32 boneIdx, const AnimQsTransform& boneLS, Bool propagate );
	void SetBoneMS( const Int32 boneIdx, const AnimQsTransform& boneMS, Bool propagate );

	Uint32 GetBonesNum() const;
	Uint32 GetAnimEventsNum() const;

	Uint32 GetFloatTrackNum() const;
	Float GetFloatTrack( Uint32 track ) const;

	Bool HasAnimationEventOccurred( const CName& animEventName ) const;

public: // use with care
	SBehaviorGraphOutput* AccessSyncedPoseLS();
	SBehaviorGraphOutput* AccessSyncedPoseMS();

	SBehaviorGraphOutput* AccessUnsyncedPoseLS();
	SBehaviorGraphOutput* AccessUnsyncedPoseMS();

public:
	void SetPoseLS( const AnimQsTransform* poseLS, Uint32 numBones );
	void SetPoseMS( const AnimQsTransform* poseMS, Uint32 numBones );

private:
	CPose& operator=( const CPose& rhs ); // no copy
	CPose( const CPose& ); // no copy

	const AnimQsTransform& CalcBoneMSFromLS( const Int32 boneIdx ) const;
	const AnimQsTransform& CalcBoneLSFromMS( const Int32 boneIdx ) const;

	void ResetTracksEventsAndAnims();
	Bool ValidatePoseData() const;

	void MakeAllChildrenLocalSpace( const Int32 boneIdx );
	void MakeFirstChildrenModelSpace( const Int32 boneIdx );

	Bool HasFlag( Int32 boneIdx, EPoseBoneFlag flag ) const;
	void SetFlag( Int32 boneIdx, EPoseBoneFlag flag ) const;
	void ClearFlag( Int32 boneIdx, EPoseBoneFlag flag ) const;
	Bool CheckFlagIsClear( EPoseBoneFlag flag ) const;

	Bool HasFlagExplicit( Uint8 f, EPoseBoneFlag flag ) const;
	void SetFlagExplicit( Uint8& f, EPoseBoneFlag flag ) const;
	void ClearFlagExplicit( Uint8& f, EPoseBoneFlag flag ) const;
};
