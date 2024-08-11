/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "skeleton.h"
#include "controlRigDefinition.h"
#include "controlRigPropertySet.h"
#include "animationManager.h"
#include "poseProvider.h"
#include "poseBBoxGenerator.h"
#include "behaviorGraphOutput.h"


IMPLEMENT_RTTI_BITFIELD( ESkeletonBoneFlags );
IMPLEMENT_ENGINE_CLASS( SSkeletonBone );
IMPLEMENT_ENGINE_CLASS( SSkeletonTrack );
IMPLEMENT_ENGINE_CLASS( CSkeleton );

//////////////////////////////////////////////////////////////////////////

SSkeletonBone::SSkeletonBone()
	: m_flags( 0 )
{
}

SSkeletonTrack::SSkeletonTrack()
{
}

//////////////////////////////////////////////////////////////////////////

CSkeleton::CSkeleton()
	: m_walkSpeed( 2.0f )
	, m_slowRunSpeed( 2.5f )
	, m_fastRunSpeed( 5.38f )
	, m_sprintSpeed( 6.66f )

	, m_walkSpeedRel( 1.0f )
	, m_slowRunSpeedRel( 2.0f )
	, m_fastRunSpeedRel( 3.0f )
	, m_sprintSpeedRel( 4.0f )
	, m_lastNonStreamableBoneName( CNAME( l_weapon ) )
	, m_poseCompression( nullptr )
	, m_bboxGenerator( nullptr )
	, m_controlRigDefinition( nullptr )
	, m_controlRigDefaultPropertySet( nullptr )
	, m_controlRigSettings( nullptr )
	, m_lodBoneNum_1( -1 )
	, m_referencePoseLS( nullptr )
	, m_referencePoseMS( nullptr )
	, m_referencePoseInvMS( nullptr )

	, m_runtimeIndex( CResource::AllocateRuntimeIndex() )
	, m_mappingCache( this )
{
}

void CSkeleton::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("bboxGenerator") )
	{
		if ( m_bboxGenerator && m_bboxGenerator->IsEmpty() )
		{
			m_bboxGenerator->Fill( this );
		}
	}
	else if ( property->GetName() == TXT("lodBoneNum_1") )
	{
		m_lodBoneNum_1 = Min( m_lodBoneNum_1, m_bones.SizeInt() );
	}
}

void CSkeleton::OnSerialize( IFile &file )
{
	TBaseClass::OnSerialize( file );
	{
		// We always have valid bone count here
		const Uint32 numBones = m_bones.Size();

		// Since RedMath does not support serialization and we don't want to convert the data
		// between the disk and memory format, therefore the reference pose is saved directly.
		if ( file.IsWriter() && !file.IsGarbageCollector() )
		{

			// NOTE: RedMath does not support easy implementation of any endianess aware byte swapping, switching to manual mode ;]
			if ( file.IsCooker() )
			{
				// Since all manually serialized objects do not have their endianess swapped
				// save each of the elements one-by one to allow for endiness swapping if needed.
				// NOTE: this heavily depends on the memory order of the structure, can break if RedQsTransform is changed
				for ( Uint32 i=0; i<numBones; ++i )
				{
					RedQsTransform& transform = m_referencePoseLS[i];

					file << reinterpret_cast<Float&>( transform.Translation.X );
					file << reinterpret_cast<Float&>( transform.Translation.Y );
					file << reinterpret_cast<Float&>( transform.Translation.Z );
					file << reinterpret_cast<Float&>( transform.Translation.W );
					file << reinterpret_cast<Float&>( transform.Rotation.Quat.X );
					file << reinterpret_cast<Float&>( transform.Rotation.Quat.Y );
					file << reinterpret_cast<Float&>( transform.Rotation.Quat.Z );
					file << reinterpret_cast<Float&>( transform.Rotation.Quat.W );
					file << reinterpret_cast<Float&>( transform.Scale.X );
					file << reinterpret_cast<Float&>( transform.Scale.Y );
					file << reinterpret_cast<Float&>( transform.Scale.Z );
					file << reinterpret_cast<Float&>( transform.Scale.W );
				}
			}
			else
			{
				// Save the whole buffer in one data blob
				//NOTE: This will break if format of the RedQsTransform is changed!
				const Uint32 dataSize = sizeof(RedQsTransform) * numBones;
				file.Serialize( m_referencePoseLS, dataSize );
			}
		}
		else if ( file.IsReader() )
		{
			// Always load the pose directly (in place)
			m_referencePoseLS = AllocReferencePose< RedQsTransform >( numBones );
			const Uint32 dataSize = sizeof(RedQsTransform) * numBones;
			file.Serialize( m_referencePoseLS, dataSize );
		}

#ifdef USE_DEBUG_ANIM_POSES
		for ( Uint32 i=0; i<numBones; ++i )
		{
			const RedQsTransform& transform = m_referencePoseLS[i];
			DEBUG_ANIM_TRANSFORM( transform );
		}
#endif
	}
}

void CSkeleton::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	if ( ShouldRegisterToAnimationManager() )
	{
		m_poseProvider = GAnimationManager->AcquirePoseProvider( this );
	}

#ifdef DEBUG_CORRUPT_TRANSFORMS
	{
		const Int32 numBones = (Int32)GetBonesNum();
		for ( Int32 i=0; i<numBones; ++i )
		{
			RED_ASSERT( m_referencePoseLS[i].Rotation.Quat.IsOk(), TXT("Animation rotation data appears to be courrpt!") );
			RED_ASSERT( m_referencePoseLS[i].Translation.IsOk(), TXT("Animation translation data appears to be courrpt!") );
			RED_ASSERT( m_referencePoseLS[i].Scale.IsOk(), TXT("Animation scale data appears to be courrpt!") );

			if ( !m_referencePoseLS[i].Rotation.Quat.IsOk() )
			{
				m_referencePoseLS[i].Rotation.SetIdentity();
			}

			if ( !m_referencePoseLS[i].Translation.IsOk() )
			{
				m_referencePoseLS[i].Translation.SetZeros();
			}

			if ( !m_referencePoseLS[i].Scale.IsOk() )
			{
				m_referencePoseLS[i].Scale.SetOnes();
			}
		}
	}
#endif

	CalcReferencePosesMS();

	ConvertBoneNames();
}

void CSkeleton::CalcReferencePosesMS()
{
	if ( m_referencePoseLS == nullptr )
	{
		return;
	}

	const Int32 numBones = (Int32)GetBonesNum();
	
	m_referencePoseMS = AllocReferencePose< RedQsTransform >( numBones );
	m_referencePoseInvMS = AllocReferencePose< Matrix >( numBones );

	for ( Int32 boneIdx=0; boneIdx<numBones; boneIdx++)
	{
		const AnimQsTransform& boneLS = m_referencePoseLS[ boneIdx ];

		AnimQsTransform& boneMS = m_referencePoseMS[ boneIdx ];
		Matrix& boneInvMSMat = m_referencePoseInvMS[ boneIdx ];

		const Int32 parentIdx = GetParentBoneIndex( boneIdx );
		if ( parentIdx != -1 )
		{
			ASSERT( parentIdx < boneIdx );

			const AnimQsTransform& parentMS = m_referencePoseMS[ parentIdx ];

			boneMS.SetMul( parentMS, boneLS );
			boneMS.Rotation.Normalize();
		}
		else
		{
			boneMS = boneLS;
		}

		AnimQsTransform boneInvMS;
		boneInvMS.SetInverse( boneMS );

		RedMatrix4x4 convertionMatrix = boneInvMS.ConvertToMatrixNormalized();
		boneInvMSMat = reinterpret_cast< const Matrix& >( convertionMatrix );
	}
}

void CSkeleton::ConvertBoneNames()
{
	const Uint32 numBones = m_bones.Size();
	for ( Uint32 i=0; i<numBones; ++i )
	{
		SSkeletonBone& bone = m_bones[ i ];
		bone.m_nameAsCName = CName( String( ANSI_TO_UNICODE( bone.m_name.AsChar() ) ) );
	}

	const Uint32 numTracks = m_tracks.Size();
	for ( Uint32 i=0; i<numTracks; ++i )
	{
		SSkeletonTrack& track = m_tracks[ i ];
		track.m_nameAsCName = CName( String( ANSI_TO_UNICODE( track.m_name.AsChar() ) ) );
	}
}

CSkeleton::~CSkeleton()
{
	// destroy pose allocator in destructor, not in OnFinalize, as things referring to skeleton may still need access to pose allocator
	// skeleton should be destroyed afterwards by garbage collector
	FreeReferencePose( m_referencePoseLS );
	FreeReferencePose( m_referencePoseMS );
	FreeReferencePose( m_referencePoseInvMS );
}

#ifndef NO_RESOURCE_IMPORT

CSkeleton* CSkeleton::Create( const FactoryInfo& data )
{
	// Not enough bones
	if (data.m_bones.Empty())
	{
		WARN_ENGINE( TXT("Empty source skeleton, failed to build CSkeleton resource from provided data.") );
		return NULL;
	}

	// Validate bones
	const Uint32 numBones = data.m_bones.Size();
	for ( Uint32 i=0; i<numBones; ++i )
	{
		const FactoryInfo::BoneImportInfo& bone = data.m_bones[i];

		// Validate bone names
		if ( bone.m_name.Empty() )
		{
			WARN_ENGINE( TXT("Bone %d has no name, failed to build CSkeleton resource from provided data."), i );
			return NULL;
		}

		// Validate parent indices
		if (bone.m_parentIndex != -1 && (Uint32)bone.m_parentIndex >= i)
		{
			WARN_ENGINE
			(
				TXT( "Invalid parent bone index (%d) in bone '%" ) RED_PRIWas TXT( "', failed to build CSkeleton resource from provided data." ), 
				bone.m_parentIndex,
				bone.m_name.AsChar()
			);

			return NULL;
		}

		// Make sure there are no duplicated names
		for ( Uint32 j=0; j<i; ++j )
		{
			const FactoryInfo::BoneImportInfo& otherBone = data.m_bones[j];
			if ( otherBone.m_name.EqualsNC( bone.m_name ) )
			{
				WARN_ENGINE
				(
					TXT("Bone %d and %d have duplicated name '%" ) RED_PRIWas TXT( "', failed to build CSkeleton resource from provided data."), 
					i,
					j,
					bone.m_name.AsChar()
				);

				return NULL;
			}
		}
	}


	// Validate tracks
	const Uint32 numTracks = data.m_tracks.Size();
	for ( Uint32 i=0; i<numTracks; ++i )
	{
		const FactoryInfo::TrackImportInfo& track = data.m_tracks[i];

		// Validate track names
		if ( track.m_name.Empty() )
		{
			WARN_ENGINE( TXT("Track %d has no name, failed to build CSkeleton resource from provided data."), i );
			return NULL;
		}

		// Make sure there are no duplicated names
		for ( Uint32 j=0; j<i; ++j )
		{
			const FactoryInfo::TrackImportInfo& otherTrack = data.m_tracks[j];
			if ( otherTrack.m_name.EqualsNC( track.m_name ) )
			{
				WARN_ENGINE( TXT("Track %d and %d have duplicated name '%ls', failed to build CSkeleton resource from provided data."), 
					i, j, ANSI_TO_UNICODE(track.m_name.AsChar()) );
				return NULL;
			}
		}
	}

	// Create and initialize skeleton data
	CSkeleton* obj = data.CreateResource();
	obj->m_bones.Resize( numBones );
	obj->m_tracks.Resize( numTracks );
	obj->m_parentIndices.Resize( numBones );
	obj->m_referencePoseLS = AllocReferencePose< RedQsTransform >( numBones );

	for ( Uint32 i=0; i<numBones; ++i )
	{
		const FactoryInfo::BoneImportInfo& importBone = data.m_bones[i];

		obj->m_bones[i].m_name = importBone.m_name;
		obj->m_bones[i].m_flags = 0;

		if ( importBone.m_lockTranslation )
		{
			obj->m_bones[i].m_flags |= SBF_LockTranslation;
		}

		obj->m_parentIndices[i] = importBone.m_parentIndex;
		obj->m_referencePoseLS[i] = importBone.m_referencePose;

		if ( !obj->m_referencePoseLS[i].Rotation.Quat.IsOk() )
		{
			return nullptr;
		}

		if ( !obj->m_referencePoseLS[i].Translation.IsOk() )
		{
			return nullptr;
		}

		if ( !obj->m_referencePoseLS[i].Scale.IsOk() )
		{
			return nullptr;
		}
	}

	for ( Uint32 i=0; i<numTracks; ++i )
	{
		const FactoryInfo::TrackImportInfo& importTrack = data.m_tracks[i];

		obj->m_tracks[i].m_name = importTrack.m_name;
	}

	obj->CalcReferencePosesMS();
	obj->ConvertBoneNames();

	obj->m_mappingCache.Reset();

	return obj;
}

#endif

Bool CSkeleton::IsValid() const
{
	return !m_bones.Empty() || !m_tracks.Empty();
}

Int32 CSkeleton::GetBonesNum() const
{
	return m_bones.Size();
}

Int32 CSkeleton::GetTracksNum() const
{
	return m_tracks.Size();
}

const Int16* const CSkeleton::GetParentIndices() const
{
	return m_parentIndices.TypedData();
}

AnimQsTransform CSkeleton::GetBoneLS( Int32 boneIndex ) const
{
	const Int32 numBones = (Int32) m_bones.Size();
	if ( boneIndex >= 0 && boneIndex < numBones )
	{
		return m_referencePoseLS[ boneIndex ];
	}

	return AnimQsTransform( AnimQsTransform::IDENTITY );
}

AnimQsTransform CSkeleton::GetBoneMS( Int32 boneIndex, AnimQsTransform const * usingBones, Int32 usingBonesNum ) const
{
	if ( boneIndex >= 0 && boneIndex < usingBonesNum )
	{
		RedQsTransform bone = usingBones[ boneIndex ];

		Int32 currBone = m_parentIndices[ boneIndex ];
		while ( currBone != -1 )
		{			
			bone.SetMul( usingBones[ currBone ], bone );
			currBone = m_parentIndices[ currBone ];
		}

		return bone;
	}

	return AnimQsTransform( AnimQsTransform::IDENTITY );
}

AnimQsTransform CSkeleton::GetBoneMS( Int32 boneIndex ) const
{
	const Int32 numBones = (Int32) m_bones.Size();
	if ( boneIndex >= 0 && boneIndex < numBones )
	{
		return m_referencePoseMS[ boneIndex ];
	}

	return AnimQsTransform( AnimQsTransform::IDENTITY );
}

const AnimQsTransform* CSkeleton::GetReferencePoseLS() const
{
	ASSERT( nullptr != m_referencePoseLS );
	return m_referencePoseLS;
}

const AnimQsTransform* CSkeleton::GetReferencePoseMS() const
{
	ASSERT( nullptr != m_referencePoseMS );
	return m_referencePoseLS;
}

const Matrix* CSkeleton::GetReferencePoseInvMS() const
{
	ASSERT( nullptr != m_referencePoseInvMS );
	return m_referencePoseInvMS;
}

Bool CSkeleton::CopyReferencePoseLSTo( Uint32 bonesNum, AnimQsTransform* bones ) const
{
	ASSERT( m_bones.Size() == bonesNum, TXT("Mismatch in size of skeleton arrays") );
	if ( m_bones.Size() == bonesNum )
	{
		const Uint32 dataSize = sizeof(RedQsTransform) * bonesNum;
		Red::System::MemoryCopy( bones, m_referencePoseLS, dataSize );
		return true;
	}

	return false;
}

Matrix CSkeleton::GetBoneMatrixLS( Int32 boneIndex ) const
{
	Matrix ret;
	RedMatrix4x4 convertionMatrix;
	convertionMatrix = GetBoneLS( boneIndex ).ConvertToMatrixNormalized();
	ret = reinterpret_cast< const Matrix& >( convertionMatrix );
	return ret;
}

Matrix CSkeleton::GetBoneMatrixMS( Int32 boneIndex ) const
{
	Matrix ret;
	RedMatrix4x4 convertionMatrix;
	convertionMatrix = GetBoneMS( boneIndex ).ConvertToMatrixNormalized();
	ret = reinterpret_cast< const Matrix& >( convertionMatrix );
	return ret;
}

String CSkeleton::GetBoneName( Uint32 num ) const
{
	if ( num < m_bones.Size() )
	{
		return String( ANSI_TO_UNICODE( m_bones[num].m_name.AsChar() ) );
	}

	return String::EMPTY;
}

CName CSkeleton::GetBoneNameAsCName( Int32 num ) const
{
	if ( num != -1 && num < m_bones.SizeInt() )
	{
		return m_bones[num].m_nameAsCName;
	}

	return CName::NONE;
}

String CSkeleton::GetTrackName( Uint32 num ) const
{
	if ( num < m_tracks.Size() )
	{
		return String( ANSI_TO_UNICODE( m_tracks[num].m_name.AsChar() ) );
	}

	return String::EMPTY;
}

CName CSkeleton::GetTrackNameAsCName( Int32 num ) const
{
	if ( num != -1 && num < m_tracks.SizeInt() )
	{
		return m_tracks[num].m_nameAsCName;
	}

	return CName::NONE;
}

const AnsiChar* CSkeleton::GetTrackNameAnsi( Uint32 num ) const
{
	if ( num < m_tracks.Size() )
	{
		return m_tracks[num].m_name.AsChar();
	}

	return "";
}

const AnsiChar* CSkeleton::GetBoneNameAnsi( Uint32 num ) const
{
	if ( num < m_bones.Size() )
	{
		return m_bones[num].m_name.AsChar();
	}

	return "";
}

namespace
{
	Float CalcProp( Float arg, Float x1, Float x2, Float y1, Float y2 )
	{
		Bool sign = arg >= 0 ? true : false; 
		Float x = MAbs( arg );

		Float p = ( x - x1 ) / ( x2 - x1 );
		Float res = ( y2 - y1 ) * p + y1;

		return sign ? res : -res;
	}
}

Float CSkeleton::deprec_ConvertSpeedRelToAbs( Float speedRel ) const
{
	Float aSpeedRel = MAbs( speedRel );
	// Need to be <= because if m_walkSpeedRel == m_slowRunSpeedRel then we will end up in m_slowRunSpeed ( if speedRel == m_walkSpeedRel )
	if ( aSpeedRel <= m_walkSpeedRel )
	{
		return CalcProp( speedRel, 0.0f, m_walkSpeedRel, 0.0f, m_walkSpeed );
	}
	else if ( aSpeedRel <= m_slowRunSpeedRel  )
	{
		return CalcProp( speedRel, m_walkSpeedRel, m_slowRunSpeedRel, m_walkSpeed, m_slowRunSpeed );
	}
	else if ( aSpeedRel <= m_fastRunSpeedRel  )
	{
		return CalcProp( speedRel, m_slowRunSpeedRel, m_fastRunSpeedRel, m_slowRunSpeed, m_fastRunSpeed );
	}
	else if ( aSpeedRel <= m_sprintSpeedRel  )
	{
		return CalcProp( speedRel, m_fastRunSpeedRel, m_sprintSpeedRel, m_fastRunSpeed, m_sprintSpeed );
	}
	else
	{
		return m_sprintSpeed;
	}
}

Float CSkeleton::deprec_GetMoveTypeRelativeMoveSpeed( EMoveType moveType )const
{
	switch( moveType )
	{
	case MT_Walk:
		return m_walkSpeedRel;
	case MT_Run:
		return m_slowRunSpeed;
	case MT_FastRun:
		return m_fastRunSpeedRel;
	case MT_Sprint:
		return m_sprintSpeedRel;
	}
	return 0.0;
}

Float CSkeleton::deprec_ConvertSpeedAbsToRel( Float speedAbs ) const
{
	Float aSpeedAbs = MAbs( speedAbs );

	// Need to be <= because if m_walkSpeed == m_slowRunSpeed then we will end up in m_slowRunSpeed ( if speedAbs == m_walkSpeed )
	if ( aSpeedAbs <= m_walkSpeed )
	{
		return CalcProp( speedAbs, 0.0f, m_walkSpeed, 0.0f, m_walkSpeedRel );
	}
	else if ( aSpeedAbs <= m_slowRunSpeed  )
	{
		return CalcProp( speedAbs, m_walkSpeed, m_slowRunSpeed, m_walkSpeedRel, m_slowRunSpeedRel );
	}
	else if ( aSpeedAbs <= m_fastRunSpeed  )
	{
		return CalcProp( speedAbs, m_slowRunSpeed, m_fastRunSpeed, m_slowRunSpeedRel, m_fastRunSpeedRel );
	}
	else if ( aSpeedAbs <= m_sprintSpeed  )
	{
		return CalcProp( speedAbs, m_fastRunSpeed, m_sprintSpeed, m_fastRunSpeedRel, m_sprintSpeedRel );
	}
	else
	{
		return m_sprintSpeedRel;
	}
}

Bool CSkeleton::HasLod_1() const
{
	return GetLodBoneNum_1() != -1;
}

Int32 CSkeleton::GetLodBoneNum_0() const
{
	return m_bones.SizeInt();
}

Int32 CSkeleton::GetLodBoneNum_1() const
{
	return m_lodBoneNum_1;
}

const IPoseCompression* CSkeleton::GetPoseCompression() const
{
	return m_poseCompression;
}

const CPoseBBoxGenerator* CSkeleton::GetPoseBBoxGen() const
{
	return m_bboxGenerator;
}

const TCrDefinition* CSkeleton::GetControlRigDefinition() const
{
	return m_controlRigDefinition;
}

const TCrPropertySet* CSkeleton::GetDefaultControlRigPropertySet() const
{
	return m_controlRigDefaultPropertySet;
}

const CControlRigSettings* CSkeleton::GetControlRigSettings() const
{
	return m_controlRigSettings;
}

const CSkeleton2SkeletonMapper* CSkeleton::FindSkeletonMapper( const CSkeleton* sk ) const
{
	const Uint32 size = m_skeletonMappers.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const CSkeleton2SkeletonMapper* m = m_skeletonMappers[ i ];
		if ( m && m->GetSkeletonB() == sk )
		{
			return m;
		}
	}
	return nullptr;
}

CPoseProvider* CSkeleton::GetPoseProvider() const
{
	return m_poseProvider.Get();
}

Bool CSkeleton::ShouldRegisterToAnimationManager() const
{
	return GetClass() == ClassID< CSkeleton >();
}

Int32 CSkeleton::FindBoneByName( const Char* name ) const
{
	const Uint32 numBones = m_bones.Size();
	for ( Uint32 i=0; i<numBones; ++i )
	{
		const SSkeletonBone& bone = m_bones[i];
		if ( AUniAnsiStrCmp( name, bone.m_name.AsChar() ) == 0 )
		{
			return i;
		}
	}

	// not found
	return -1;
}

Int32 CSkeleton::FindBoneByName( const AnsiChar* name ) const
{
	const Uint32 numBones = m_bones.Size();
	for ( Uint32 i=0; i<numBones; ++i )
	{
		const SSkeletonBone& bone = m_bones[i];
		if ( Red::System::StringCompare( name, bone.m_name.AsChar() ) == 0 )
		{
			return i;
		}
	}

	// not found
	return -1;
}

Int32 CSkeleton::FindBoneByName( const CName& name ) const
{
	const Uint32 numBones = m_bones.Size();
	for ( Uint32 i=0; i<numBones; ++i )
	{
		const SSkeletonBone& bone = m_bones[i];
		if ( name == bone.m_nameAsCName )
		{
			return i;
		}
	}

	// not found
	return -1;
}

Int32 CSkeleton::FindTrackByName( const Char* name ) const
{
	const Uint32 numTracks = m_tracks.Size();
	for ( Uint32 i=0; i<numTracks; ++i )
	{
		const SSkeletonTrack& track = m_tracks[i];
		if ( 0 == AUniAnsiStrICmp( name, track.m_name.AsChar() ) )
		{
			return i;
		}
	}

	// Not found
	return -1;
}

Int32 CSkeleton::FindTrackByName( const AnsiChar* name ) const
{
	const Uint32 numTracks = m_tracks.Size();
	for ( Uint32 i=0; i<numTracks; ++i )
	{
		const SSkeletonTrack& track = m_tracks[i];
		if ( 0 == Red::System::StringCompare( name, track.m_name.AsChar() ) )
		{
			return i;
		}
	}

	// Not found
	return -1;
}

Int32 CSkeleton::FindTrackByName( const CName& name ) const
{
	const Uint32 numTracks = m_tracks.Size();
	for ( Uint32 i=0; i<numTracks; ++i )
	{
		const SSkeletonTrack& track = m_tracks[i];
		if ( name == track.m_nameAsCName )
		{
			return i;
		}
	}

	// Not found
	return -1;
}

Uint32 CSkeleton::GetBones( TDynArray< ISkeletonDataProvider::BoneInfo >& outBones ) const
{
	PC_SCOPE( CSkeleton GetBones );

	// Extract bones
	const Uint32 numBones = GetBonesNum();
	for ( Uint32 i=0; i<numBones; ++i )
	{
		// Setup bone info
		ISkeletonDataProvider::BoneInfo info;
		info.m_name = GetBoneNameAsCName( i );
		info.m_parent = GetParentBoneIndex( i );

		outBones.PushBack( info );
	}

	// Bones extracted
	return numBones;
}

Uint32 CSkeleton::GetBones( TAllocArray< ISkeletonDataProvider::BoneInfo >& outBones ) const
{
	PC_SCOPE( CSkeleton GetBones );

	// Extract bones
	const Uint32 numBones = GetBonesNum();
	for ( Uint32 i=0; i<numBones; ++i )
	{
		// Setup bone info
		ISkeletonDataProvider::BoneInfo info;
		info.m_name = GetBoneNameAsCName( i );
		info.m_parent = GetParentBoneIndex( i );

		outBones.PushBack( info );
	}

	// Bones extracted
	return numBones;
}

#ifndef NO_EDITOR

void CSkeleton::RecreatePoseCompression()
{
	if ( m_poseCompression )
	{
		m_poseCompression->Recreate();
	}
}

#endif

void CSkeleton::GetBonesMS( TDynArray< Matrix >& bonesMS ) const
{
	const Uint32 bonesNum = m_bones.Size();
	bonesMS.Resize( bonesNum );

	// Calculate MS transforms
	for ( Uint32 boneIdx=0; boneIdx<bonesNum; boneIdx++)
	{
		bonesMS[ boneIdx ] = GetBoneMatrixMS( boneIdx );
	}
}
