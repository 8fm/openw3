/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "poseCompression.h"
#include "skeletonMapper.h"
#include "skeletonSkeletonMappingCache.h"
#include "skeletonProvider.h" // We need to use some structues here (BoneInfo)
#include "poseTypes.h"

#include "../core/resource.h"
#include "../core/allocArray.h"
#include "../core/bitFieldBuilder.h"


class TCrDefinition;
class TCrPropertySet;
class CControlRigMapper;
class CControlRigSettings;
class CPoseBBoxGenerator;

//////////////////////////////////////////////////////////////////////////
/// Special bone flags for skeleton bones
enum ESkeletonBoneFlags
{
	// Whether or not this bone can move relative to its parent
	SBF_LockTranslation = FLAG(0),
};

BEGIN_BITFIELD_RTTI(ESkeletonBoneFlags,sizeof(Uint16));
	BITFIELD_OPTION(SBF_LockTranslation);
END_BITFIELD_RTTI();

//////////////////////////////////////////////////////////////////////////
/// Bone info structure for new skeleton implementation
struct SSkeletonBone
{
	DECLARE_RTTI_STRUCT(SSkeletonBone);

public:
	//! Name of the bone
	StringAnsi	m_name;
	CName		m_nameAsCName;

	//! General bone flags (bit field, don't change the value type without redeclaring it in the bitfield)
	Uint16 m_flags;

public:
	SSkeletonBone();
};

BEGIN_CLASS_RTTI( SSkeletonBone );
	PROPERTY_RO( m_name, TXT("Name of the bone") );
	PROPERTY_RO( m_nameAsCName, TXT("") );
	PROPERTY_BITFIELD_RO( m_flags, ESkeletonBoneFlags, TXT("Extra flags") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
/// Track (float track usually) info structure for new skeleton implementation
struct SSkeletonTrack
{
	DECLARE_RTTI_STRUCT(SSkeletonTrack);

public:
	//! Name of the additional track
	StringAnsi	m_name;
	CName		m_nameAsCName;

public:
	SSkeletonTrack();
};

BEGIN_CLASS_RTTI( SSkeletonTrack );
	PROPERTY_RO( m_name, TXT("Name of the float track") );
	PROPERTY_RO( m_nameAsCName, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
/// Animation skeleton resource class
class CSkeleton : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CSkeleton, CResource, "w2rig", "Skeleton" );	

protected:

	// Bone information
	TDynArray< SSkeletonBone >	m_bones;

	// Float track information
	TDynArray< SSkeletonTrack >	m_tracks;

	// Indices of parent bones
	TDynArray< Int16 >			m_parentIndices;

	// Skeleton reference pose in local space (separated for easier access)
	RedQsTransform*				m_referencePoseLS;

	// Skeleton reference pose in model space (separated for easier access)
	RedQsTransform*				m_referencePoseMS;

	// Skeleton reference pose in inverted model space (separated for easier access)
	Matrix*						m_referencePoseInvMS;


	//Int32						m_lodBoneNum_0; // Zero lod - all bones
	Int32						m_lodBoneNum_1; // First lod - don't use extra bones ( fingers, rolls etc. ), more or less 25 bones
	//Int32						m_lodBoneNum_2; // Second lod - only base bones ( pelvis, arms, legs, head without hands, foot etc. ), more or less 7 bones
	//Int32						m_lodBoneNum_3; // Last lod - root bone and trajectory

	CName						m_lastNonStreamableBoneName;

	Uint32						m_runtimeIndex; // Runtime index of a loaded resource - incremented every time resource is loaded

	mutable CSkeletonSkeletonMappingCache	m_mappingCache; // Skeleton <-> Skeleton mapping helper

protected:
	IPoseCompression*			m_poseCompression;
	CPoseBBoxGenerator*			m_bboxGenerator;
	TCrDefinition*				m_controlRigDefinition;
	TCrPropertySet*				m_controlRigDefaultPropertySet;
	CControlRigSettings*		m_controlRigSettings;

	TDynArray< CSkeleton2SkeletonMapper* > m_skeletonMappers;
	CTeleportDetectorData*		m_teleportDetectorData;

protected:
	Float						m_walkSpeed;			// Walk speed absolute [m/s]
	Float						m_slowRunSpeed;			// Run speed absolute [m/s]
	Float						m_fastRunSpeed;
	Float						m_sprintSpeed;

	Float						m_walkSpeedRel;			// Walk speed relative
	Float						m_slowRunSpeedRel;			// Run speed relative
	Float						m_fastRunSpeedRel;
	Float						m_sprintSpeedRel;
private:
	PoseProviderHandle			m_poseProvider;

protected:
	CSkeleton();
	virtual ~CSkeleton();

public:
	virtual void OnSerialize( IFile &file );

	virtual void OnPostLoad();

	virtual void OnPropertyPostChange( IProperty* property );

#ifndef NO_RESOURCE_IMPORT
public:
	struct FactoryInfo : public CResource::FactoryInfo< CSkeleton >
	{
		struct BoneImportInfo
		{
			StringAnsi			m_name;
			Int16				m_parentIndex;
			Bool				m_lockTranslation;
			RedQsTransform		m_referencePose;
		};
		
		struct TrackImportInfo
		{
			StringAnsi			m_name;
		};

		TDynArray<BoneImportInfo>	m_bones;
		TDynArray<TrackImportInfo>	m_tracks;
	};

	static CSkeleton* Create( const FactoryInfo& data );
#endif

#ifndef NO_EDITOR
	void RecreatePoseCompression();
#endif

public:
	RED_MOCKABLE Bool IsValid() const;

	Int32 GetBonesNum() const;
	Int32 GetTracksNum() const;

	Int32 GetParentBoneIndex( Int32 bone ) const;
	const Int16* const GetParentIndices() const;

	Matrix GetBoneMatrixLS( Int32 boneIndex ) const;
	Matrix GetBoneMatrixMS( Int32 boneIndex ) const;

	AnimQsTransform GetBoneLS( Int32 boneIndex ) const;
	AnimQsTransform GetBoneMS( Int32 boneIndex ) const;

	const AnimQsTransform*	GetReferencePoseLS() const;
	const AnimQsTransform*	GetReferencePoseMS() const;
	const Matrix*			GetReferencePoseInvMS() const;

	Bool CopyReferencePoseLSTo( Uint32 bonesNum, AnimQsTransform* bones ) const;

	AnimQsTransform GetBoneMS( Int32 boneIndex, AnimQsTransform const * usingBones, Int32 usingBonesNum ) const;

	void GetBonesMS( TDynArray< Matrix >& bonesMS ) const;

public:
	//! Get name of given bone
	String GetBoneName( Uint32 num ) const;
	CName GetBoneNameAsCName( Int32 num ) const;

	//! Get name of given track
	String GetTrackName( Uint32 num ) const;
	CName GetTrackNameAsCName( Int32 num ) const;

	//! Get name of given bone as C string
	const AnsiChar* GetTrackNameAnsi( Uint32 num ) const;

	//! Get name of given bone as C string
	const AnsiChar* GetBoneNameAnsi( Uint32 num ) const;

	//! Get teleport detector data
	const CTeleportDetectorData* GetTeleportDetectorData() const { return m_teleportDetectorData; };

public:
	// Please use speedConfig.xml
	RED_INLINE const Float & deprec_GetWalkSpeedAbs()const		{ return m_walkSpeed; }
	RED_INLINE const Float & deprec_GetSlowRunSpeedAbs()const		{ return m_slowRunSpeed; }
	RED_INLINE const Float & deprec_GetFastRunSpeedAbs()const		{ return m_fastRunSpeed; }
	RED_INLINE const Float & deprec_GetSprintSpeedAbs()const		{ return m_sprintSpeed; }

	RED_INLINE const Float & deprec_GetSpeedRelSpan()const		{ return m_sprintSpeedRel; }

	RED_INLINE const Uint32 GetRuntimeIndex() const { return m_runtimeIndex; }

	RED_INLINE CSkeletonSkeletonMappingCache& GetMappingCache() const { return m_mappingCache; }

	Float deprec_ConvertSpeedRelToAbs( Float speedRel ) const;
	Float deprec_ConvertSpeedAbsToRel( Float speedAbs ) const;
	Float deprec_GetMoveTypeRelativeMoveSpeed( EMoveType moveType )const;

	Bool HasLod_1() const;

	Int32 GetLodBoneNum_0() const;
	Int32 GetLodBoneNum_1() const;

public:
	// Find bone index matching given bone name, returns -1 if matching bone not found
	Int32 FindBoneByName( const Char* name ) const;

	// Find bone index matching given bone name, returns -1 if matching bone not found
	Int32 FindBoneByName( const AnsiChar* name ) const;

	// Find bone index matching given bone name, returns -1 if matching bone not found
	Int32 FindBoneByName( const CName& name ) const;

	// Find track (float track) index matching given name, returns -1 if matching track was not found
	Int32 FindTrackByName( const Char* name ) const;

	// Find track (float track) index matching given name, returns -1 if matching track was not found
	Int32 FindTrackByName( const AnsiChar* name ) const;

	// Find track (float track) index matching given name, returns -1 if matching track was not found
	Int32 FindTrackByName( const CName& name ) const;

	// Get bones in the skeleton, returns number of bones in skeleton
	Uint32 GetBones( TDynArray< ISkeletonDataProvider::BoneInfo >& outBones ) const;
	Uint32 GetBones( TAllocArray< ISkeletonDataProvider::BoneInfo >& outBones ) const;

public:
	Int32 GetFirstStreamableBone() const { Int32 lastNSBIndex = FindBoneByName( m_lastNonStreamableBoneName ); return lastNSBIndex == -1? lastNSBIndex : lastNSBIndex + 1; }

	const IPoseCompression* GetPoseCompression() const;
	const CPoseBBoxGenerator* GetPoseBBoxGen() const;
	const TCrDefinition* GetControlRigDefinition() const;
	const TCrPropertySet* GetDefaultControlRigPropertySet() const;
	const CControlRigSettings* GetControlRigSettings() const;

	const CSkeleton2SkeletonMapper* FindSkeletonMapper( const CSkeleton* sk ) const;

	CPoseProvider* GetPoseProvider() const;

private:
	Bool ShouldRegisterToAnimationManager() const;

	void CalcReferencePosesMS();
	void ConvertBoneNames();

	template< class T >
	static T* AllocReferencePose( Uint32 numBones )
	{
		const Uint32 dataSize = sizeof( T ) * numBones;
		T* pose = reinterpret_cast< T* >( RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Animation, MC_Animation, dataSize, 16 ) );
		return pose;
	}

	template< class T >
	static void FreeReferencePose( T*& pose )
	{
		if ( nullptr != pose )
		{
			RED_MEMORY_FREE( MemoryPool_Animation, MC_Animation, pose );
			pose = nullptr;
		}
	}
};

RED_INLINE Int32 CSkeleton::GetParentBoneIndex( Int32 bone ) const
{
	if ( bone >= 0 && bone < (Int32)m_parentIndices.Size() )
	{
		return m_parentIndices[ bone ];
	}

	return -1; 
}

BEGIN_CLASS_RTTI( CSkeleton );
	PARENT_CLASS( CResource );
	PROPERTY_EDIT( m_lodBoneNum_1, TXT("First lod - don't use extra bones ( fingers, rolls etc. )") );

	PROPERTY_EDIT( m_walkSpeed, TXT("Walk absolute speed [m/s]") );
	PROPERTY_EDIT( m_slowRunSpeed, TXT("Slow run absolute speed [m/s]") );
	PROPERTY_EDIT( m_fastRunSpeed, TXT("Fast run absolute speed [m/s]") );
	PROPERTY_EDIT( m_sprintSpeed, TXT("Sprint absolute speed [m/s]") );

	PROPERTY_EDIT( m_walkSpeedRel, TXT("Walk relative speed") );
	PROPERTY_EDIT( m_slowRunSpeedRel, TXT("Slow Run relative speed") );
	PROPERTY_EDIT( m_fastRunSpeedRel, TXT("Fast run relative speed [m/s]") );
	PROPERTY_EDIT( m_sprintSpeedRel, TXT("Sprint relative speed [m/s]") );

	PROPERTY_INLINED( m_poseCompression, TXT("Pose compression") );
	PROPERTY_INLINED( m_bboxGenerator, TXT("Pose bbox generator") );
	PROPERTY_INLINED( m_controlRigDefinition, TXT("Definition for control rig") );
	PROPERTY_INLINED( m_controlRigDefaultPropertySet, TXT("Default property set for control rig") );
	PROPERTY_INLINED( m_skeletonMappers, TXT("Mappers to diffrent skeletons") );
	PROPERTY_INLINED( m_controlRigSettings, TXT("") );
	PROPERTY_INLINED( m_teleportDetectorData, TXT("Bone which will be used to detect pose teleport") );
	PROPERTY_CUSTOM_EDIT( m_lastNonStreamableBoneName, TXT(""), TXT("SkeletonBoneSelection") );
	PROPERTY_RO( m_bones, TXT("Bones") );
	PROPERTY_RO( m_tracks, TXT("Float tracks") );
	PROPERTY_RO( m_parentIndices, TXT("Indices of parent bones") );
END_CLASS_RTTI();
