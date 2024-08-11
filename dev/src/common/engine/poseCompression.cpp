/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "poseCompression.h"
#include "animMath.h"
#include "skeleton.h"

IMPLEMENT_ENGINE_CLASS( IPoseCompression );
IMPLEMENT_ENGINE_CLASS( CPoseCompressionNone );
IMPLEMENT_ENGINE_CLASS( CPoseCompressionDefault );
IMPLEMENT_ENGINE_CLASS( CPoseCompressionDefaultWithExtraBones );
IMPLEMENT_ENGINE_CLASS( CPoseCompressionCharacter );
IMPLEMENT_ENGINE_CLASS( CPoseCompressionCamera );

IMPLEMENT_ENGINE_CLASS( ICompressedPose );
IMPLEMENT_ENGINE_CLASS( CNoCompressedPose );
IMPLEMENT_ENGINE_CLASS( CDefaultCompressedPose2 );
IMPLEMENT_ENGINE_CLASS( CDefaultCompressedPoseWithExtraBones );
IMPLEMENT_ENGINE_CLASS( CCharacterCompressedPose );
IMPLEMENT_ENGINE_CLASS( CCameraCompressedPose );

//////////////////////////////////////////////////////////////////////////

namespace
{
	Bool AreQuaternionsEqual( const AnimQuaternion& q1, const AnimQuaternion& q2, Float eps )
	{
#ifdef USE_HAVOK_ANIMATION
		hkVector4 toCompare1 = q1.m_vec;
		hkVector4 toCompare2 = q2.m_vec;

		toCompare1.normalize4();
		toCompare2.normalize4();

		// Make sure they are both in same hemisphere
		if ( toCompare1.dot4( toCompare2 ) < 0.0f )
		{
			toCompare1.mul4( -1.0f );
		}

		return (0 != toCompare1.equals4( toCompare2, eps ));
#else
		RedVector4 toCompare1 = q1.Quat;
		RedVector4 toCompare2 = q2.Quat;

		toCompare1.Normalize3();
		toCompare2.Normalize3();

		// Make sure they are both in same hemisphere
		if ( Dot( toCompare1, toCompare2) < 0.0f )
		{
			SetMul( toCompare1, -1.0f );
		}

		return toCompare1.IsAlmostEqual( toCompare2, eps );
#endif
	}

	Bool AreTranslationsEqual( const AnimVector4& v1, const AnimVector4& v2, Float eps )
	{
#ifdef USE_HAVOK_ANIMATION
		return (0 != v1.equals3( v2, eps ));
#else
		return v1.AsVector3().IsAlmostEqual( v2.AsVector3(), eps );
#endif
	}

	void FindMinMaxBoneTranslation( const AnimQsTransform* bones, Int32 first, Int32 last, Float& max, Float& min )
	{
#ifdef USE_HAVOK_ANIMATION
		for ( Int32 i=first; i<last; ++i )
		{
			const hkQsTransform& boneIn = bones[ i ];
			const hkVector4& boneTrans = boneIn.m_translation;

			max = Max( max, boneTrans( 0 ) );
			max = Max( max, boneTrans( 1 ) );
			max = Max( max, boneTrans( 2 ) );

			min = Min( min, boneTrans( 0 ) );
			min = Min( min, boneTrans( 1 ) );
			min = Min( min, boneTrans( 2 ) );
		}
#else
		for ( Int32 i=first; i<last; ++i )
		{
			const RedQsTransform& boneIn = bones[ i ];
			const RedVector4& boneTrans = boneIn.Translation;

			max = Red::Math::NumericalUtils::Max( max, boneTrans.X );
			max = Red::Math::NumericalUtils::Max( max, boneTrans.Y );
			max = Red::Math::NumericalUtils::Max( max, boneTrans.Z );

			min = Red::Math::NumericalUtils::Min( min, boneTrans.X );
			min = Red::Math::NumericalUtils::Min( min, boneTrans.Y );
			min = Red::Math::NumericalUtils::Min( min, boneTrans.Z );
		}
#endif
	}
}

//////////////////////////////////////////////////////////////////////////

Bool CPoseCompressionNone::IsValid() const
{
	return true;
}

ICompressedPose* CPoseCompressionNone::CreateCompressedPose( CObject* parent, const CSkeleton* skeleton, const InputPose& poseIn ) const
{
	// Check input data
	if ( !skeleton || !parent )
	{
		return NULL;
	}

	// Create pose
	CNoCompressedPose* poseOut = CreateObject< CNoCompressedPose >( parent );

	// And returns it
	return poseOut;
}

//////////////////////////////////////////////////////////////////////////

CPoseCompressionDefault::CPoseCompressionDefault()
	: m_firstRotBone( -1 )
	, m_firstTransBone( -1 )
	, m_lastRotBone( -1 )
	, m_lastTransBone( -1 )
	, m_compressTranslationType( CT_8 )
{

}

void CPoseCompressionDefault::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("firstRotBoneName") )
	{
		FindBone( m_firstRotBoneName, m_firstRotBone );
	}
	else if ( property->GetName() == TXT("lastRotBoneName") )
	{
		FindBone( m_lastRotBoneName, m_lastRotBone, 1 );
	}
	else if ( property->GetName() == TXT("firstTransBoneName") )
	{
		FindBone( m_firstTransBoneName, m_firstTransBone );
	}
	else if ( property->GetName() == TXT("lastTransBoneName") )
	{
		FindBone( m_lastTransBoneName, m_lastTransBone, 1 );
	}
}

void CPoseCompressionDefault::Recreate()
{
	TBaseClass::Recreate();

	FindBone( m_firstRotBoneName, m_firstRotBone );
	FindBone( m_lastRotBoneName, m_lastRotBone, 1 );
	FindBone( m_firstTransBoneName, m_firstTransBone );
	FindBone( m_lastTransBoneName, m_lastTransBone, 1 );
}

void CPoseCompressionDefault::FindBone( const String& name, Int32& index, Int32 offset ) const
{
	CSkeleton* skeleton = SafeCast< CSkeleton >( GetParent() );
	if ( skeleton )
	{
		index = skeleton->FindBoneByName( name.AsChar() );
		if ( index != -1 )
		{
			index += offset;
		}
	}
}

Int32 CPoseCompressionDefault::GetFirstRotBone() const
{
	return m_firstRotBone;
}

Int32 CPoseCompressionDefault::GetLastRotBone() const
{
	return m_lastRotBone;
}

Int32 CPoseCompressionDefault::GetFirstTransBone() const
{
	return m_firstTransBone;
}

Int32 CPoseCompressionDefault::GetLastTransBone() const
{
	return m_lastTransBone;
}

Bool CPoseCompressionDefault::IsValid() const
{
	return TBaseClass::IsValid() && 
		m_firstRotBone != -1 && m_firstTransBone != -1 && m_lastRotBone != -1 && m_lastTransBone != -1 && 
		m_firstRotBone <= m_lastRotBone && m_firstTransBone <= m_lastTransBone;
}

ICompressedPose* CPoseCompressionDefault::CreateCompressedPose( CObject* parent, const CSkeleton* skeleton, const InputPose& poseIn ) const
{
	// Check input data
	if ( !IsValid() || !skeleton || !parent || poseIn.m_bonesNum == 0 )
	{
		return NULL;
	}

	// Create pose
	CDefaultCompressedPose2* poseOut = CreateObject< CDefaultCompressedPose2 >( parent );

	FillPose( skeleton, poseIn, poseOut );

	return poseOut;
}

void CPoseCompressionDefault::AddBone( CDefaultCompressedPose2* poseOut, const AnimQsTransform& bone ) const
{
	AddBoneRotation( poseOut, bone );
	AddBoneTranslation( poseOut, bone );
}

void CPoseCompressionDefault::AddBoneRotation( CDefaultCompressedPose2* poseOut, const AnimQsTransform& bone ) const
{
#ifdef USE_HAVOK_ANIMATION
	poseOut->m_rotationsBuffer.AddQuaternion( bone.m_rotation );
#else
	poseOut->m_rotationsBuffer.AddQuaternion( bone.Rotation );
#endif
}

void CPoseCompressionDefault::AddBoneTranslation( CDefaultCompressedPose2* poseOut, const AnimQsTransform& bone ) const
{
#ifdef USE_HAVOK_ANIMATION
	ASSERT( poseOut->m_compressTranslation );
	poseOut->m_compressTranslation->AddTranslation( bone.m_translation );
#else
	ASSERT( poseOut->m_compressTranslation );
	poseOut->m_compressTranslation->AddTranslation( bone.Translation );
#endif
}

CompressedTranslation* CPoseCompressionDefault::CreateCompressTranslation( Float transMin, Float transMax, Uint32 size ) const
{
	CompressedTranslation* comprTrans = CompressedTranslation::CreateCompressTranslation( m_compressTranslationType );
	comprTrans->Init( transMin, transMax, size );
	return comprTrans;
}

Bool CPoseCompressionDefault::FillPose( const CSkeleton* skeleton, const InputPose& poseIn, CDefaultCompressedPose2* poseOut ) const
{
	// Hack
	if ( poseIn.m_doubleRotationQuality )
	{
		poseOut->m_rotationsBuffer.SetType( CRT_64 );
	}

	// Bone number
	const Uint32 numRef = skeleton->GetBonesNum();
	const Uint32 numPoseIn = poseIn.m_bonesNum;

	// Check
	if ( numRef != numPoseIn )
	{
		ASSERT( numRef == numPoseIn );
		return false;
	}

	const Int32 numBoneRot = m_lastRotBone - m_firstRotBone;
	const Int32 numBoneTrans = m_lastTransBone - m_firstTransBone;

	// Rotations
	if ( numBoneRot > 0 )
	{
		poseOut->m_rotationsBuffer.Reserve( numBoneRot );

		for ( Int32 i=m_firstRotBone; i<m_lastRotBone; ++i )
		{
			AddBoneRotation( poseOut, poseIn.m_bones[ i ] );
		}
	}

	// Translation
	if ( numBoneTrans > 0 )
	{
		Float transMax = NumericLimits< Float >::Min();
		Float transMin = NumericLimits< Float >::Max();

		// Find max/min
		FindMinMaxBoneTranslation( poseIn.m_bones, m_firstTransBone, m_lastTransBone, transMax, transMin );

		// Ok I know know...
		CheckOtherExtraBonesForMaxMin( poseIn.m_bones, poseIn.m_bonesNum, transMax, transMin );

		// Create compress translation
		poseOut->m_compressTranslation.Reset( CreateCompressTranslation( transMin, transMax, 3 * numBoneTrans ) );

		// Add bones translations
		for ( Int32 i=m_firstTransBone; i<m_lastTransBone; ++i )
		{
			AddBoneTranslation( poseOut, poseIn.m_bones[ i ] );
		}
	}

	return true;
}

void CPoseCompressionDefault::CheckOtherExtraBonesForMaxMin( const AnimQsTransform* bones, Uint32 boneNum, Float& max, Float &min ) const
{
}

//////////////////////////////////////////////////////////////////////////

ICompressedPose* CPoseCompressionDefaultWithExtraBones::CreateCompressedPose( CObject* parent, const CSkeleton* skeleton, const InputPose& poseIn ) const
{
	return NULL;
}

//////////////////////////////////////////////////////////////////////////

CPoseCompressionCharacter::CPoseCompressionCharacter()
	: m_leftWeapon( -1 )
	, m_rightWeapon( -1 )
{

}

void CPoseCompressionCharacter::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("leftWeaponName") )
	{
		FindBone( m_leftWeaponName, m_leftWeapon );
	}
	else if ( property->GetName() == TXT("rightWeaponName") )
	{
		FindBone( m_rightWeaponName, m_rightWeapon );
	}
}

void CPoseCompressionCharacter::Recreate()
{
	TBaseClass::Recreate();

	FindBone( m_leftWeaponName, m_leftWeapon );
	FindBone( m_rightWeaponName, m_rightWeapon );
}

Int32 CPoseCompressionCharacter::GetLeftWeaponBone() const
{
	return m_leftWeapon;
}

Int32 CPoseCompressionCharacter::GetRightWeaponBone() const
{
	return m_rightWeapon;
}

ICompressedPose* CPoseCompressionCharacter::CreateCompressedPose( CObject* parent, const CSkeleton* skeleton, const InputPose& poseIn ) const
{
	// Check input data
	if ( !IsValid() || !skeleton || !parent || poseIn.m_bonesNum == 0 )
	{
		return NULL;
	}

	// Create pose
	CCharacterCompressedPose* poseOut = CreateObject< CCharacterCompressedPose >( parent );

	// Fill base buffer
	if ( !FillPose( skeleton, poseIn, poseOut ) )
	{
		return NULL;
	}

	// Add extra bones for character
	if ( AreExtraBonesValid( (Int32)poseIn.m_bonesNum ) )
	{
		AddBone( poseOut, poseIn.m_bones[ m_leftWeapon ] );
		AddBone( poseOut, poseIn.m_bones[ m_rightWeapon ] );
	}

	return poseOut;
}

Bool CPoseCompressionCharacter::AreExtraBonesValid( Int32 max ) const
{
	return m_leftWeapon != -1 && m_leftWeapon < max && m_rightWeapon != -1 && m_rightWeapon < max;
}

void CPoseCompressionCharacter::CheckOtherExtraBonesForMaxMin( const AnimQsTransform* bones, Uint32 boneNum, Float& max, Float &min ) const
{
	if ( AreExtraBonesValid( (Int32)boneNum ) )
	{
		FindMinMaxBoneTranslation( bones, m_leftWeapon, m_leftWeapon + 1, max, min );
		FindMinMaxBoneTranslation( bones, m_rightWeapon, m_rightWeapon + 1, max, min );
	}
}

//////////////////////////////////////////////////////////////////////////

ICompressedPose* CPoseCompressionCamera::CreateCompressedPose( CObject* parent, const CSkeleton* skeleton, const InputPose& poseIn ) const
{
	// Check input data
	if ( !IsValid() || !skeleton || !parent || poseIn.m_bonesNum == 0 )
	{
		return NULL;
	}

	// Create pose
	CCameraCompressedPose* poseOut = CreateObject< CCameraCompressedPose >( parent );

	// Fill base buffer
	FillPose( skeleton, poseIn, poseOut );

	// Create and fill tracks buffer
	if ( poseIn.m_tracksNum > 0 )
	{
		poseOut->m_tracks.Resize( poseIn.m_tracksNum );
		
		for ( Uint32 i=0; i<poseIn.m_tracksNum; ++i )
		{
			poseOut->m_tracks[ i ] = poseIn.m_tracks[ i ];
		}
	}

	return poseOut;
}

//////////////////////////////////////////////////////////////////////////

Uint32 CNoCompressedPose::GetSize() const
{
	return 0;
}

Uint32 CNoCompressedPose::GetBufferSize() const 
{ 
	return 0; 
}

Bool CNoCompressedPose::DecompressAndSample( Uint32 boneNumIn, AnimQsTransform* bonesOut, Uint32 tracksNumIn, AnimFloat* tracksOut, const CSkeleton* skeleton ) const
{
	if ( skeleton->GetBonesNum() != (Int32)boneNumIn || !skeleton->GetPoseCompression() )
	{
		return false;
	}

	// Pose compression
	const IPoseCompression* compression = skeleton->GetPoseCompression();

	// Check if compression can work with this pose
	if ( compression->GetClass() != CPoseCompressionNone::GetStaticClass() )
	{
		return false;
	}

	// Decompressed pose
	const RedQsTransform* bonesRef = skeleton->GetReferencePoseLS();
	Red::System::MemoryCopy( bonesOut, bonesRef, boneNumIn * sizeof( RedQsTransform ) );

	return true;
}

//////////////////////////////////////////////////////////////////////////

CDefaultCompressedPose2::CDefaultCompressedPose2()
	: m_compressTranslation( NULL )
{
}

void CDefaultCompressedPose2::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( file.IsReader() )
	{
		Uint32 type = 0;
		file << type;

		m_compressTranslation.Reset( CompressedTranslation::CreateCompressTranslation( (ECompressTranslationType)type ) );
		if ( m_compressTranslation )
		{
			m_compressTranslation->Serialize( file );
		}
	}
	else
	{
		if ( m_compressTranslation )
		{
			Uint32 type = m_compressTranslation->GetType();
			file << type;
			m_compressTranslation->Serialize( file );
		}
		else
		{
			Uint32 type = CT_None;
			file << type;
		}
	}

	m_rotationsBuffer.Serialize( file );
}

Uint32 CDefaultCompressedPose2::GetSize() const
{
	Uint32 size = m_rotationsBuffer.DataSize() + sizeof ( this );
	if ( m_compressTranslation )
	{
		size += m_compressTranslation->DataSize();
	}
	return size;
}

Uint32 CDefaultCompressedPose2::GetBufferSize() const 
{ 
	Uint32 size = m_rotationsBuffer.DataSize() + 2 * sizeof ( Float );
	if ( m_compressTranslation )
	{
		size += m_compressTranslation->DataSize();
	}
	return size;
}

Bool CDefaultCompressedPose2::DecompressAndSample( Uint32 boneNumIn, AnimQsTransform* bonesOut, Uint32 tracksNumIn, AnimFloat* tracksOut, const CSkeleton* skeleton ) const
{
	if ( skeleton->GetBonesNum() != (Int32)boneNumIn || !skeleton->GetPoseCompression() )
	{
		return false;
	}

	// Pose compression
	const IPoseCompression* compression = skeleton->GetPoseCompression();
	 
	// Check if compression can work with this pose
	if ( compression->GetClass() != CPoseCompressionDefault::GetStaticClass() )
	{
		return false;
	}

	const CPoseCompressionDefault* poseCompression = static_cast< const CPoseCompressionDefault* >( compression );

	InternalDecompressAndSample( poseCompression, boneNumIn, bonesOut, skeleton );

	return true;
}

void CDefaultCompressedPose2::InternalDecompressAndSample( const CPoseCompressionDefault* poseCompression, Uint32 boneNumIn, AnimQsTransform* bonesOut, const CSkeleton* skeleton ) const
{
	// Sizes
	const Int32 numRot = m_rotationsBuffer.Size();
	const Int32 numTrans = m_compressTranslation ? m_compressTranslation->Size() : 0;

	// Reference pose
	const AnimQsTransform* bonesRef = skeleton->GetReferencePoseLS();

	// Index
	const Int32 firstRotBone = poseCompression->GetFirstRotBone();
	const Int32 lastRotBone = poseCompression->GetLastRotBone();
	const Int32 firstTransBone = poseCompression->GetFirstTransBone();
	const Int32 lastTransBone = poseCompression->GetLastTransBone();

	// Decompressed pose
	const Int32 boneNum = (Int32)boneNumIn;
	for ( Int32 i=0; i<boneNum; ++i )
	{
		AnimQsTransform& bone = bonesOut[ i ];
		const AnimQsTransform& boneRef = bonesRef[ i ];

		Bool decompressRot = firstRotBone != -1 && lastRotBone != -1 && firstRotBone <= i && i < lastRotBone && i < numRot;
		Bool decompressTrans = firstTransBone != -1 && lastTransBone != -1 && firstTransBone <= i && i < lastTransBone && 3 * i <= numTrans;

		if ( decompressRot )
		{
			DecompressBoneRotation( bone, i - firstRotBone );
		}
		else
		{
#ifdef USE_HAVOK_ANIMATION
			bone.m_rotation = boneRef.m_rotation;
#else
			bone.Rotation = boneRef.Rotation;
#endif
		}

		if ( decompressTrans )
		{
			DecompressBoneTranslation( bone, i - firstTransBone );
		}
		else
		{
#ifdef USE_HAVOK_ANIMATION
			bone.m_translation = boneRef.m_translation;
#else
			bone.Translation = boneRef.Translation;
#endif
		}

#ifdef USE_HAVOK_ANIMATION
		bone.m_scale.setAll( 1.0f );
#else
		bone.Scale.SetOnes();
#endif
	}

	if ( numRot > 0 && firstRotBone >= 0 )
	{
		// Normalize - have to be after decompression
#ifdef USE_HAVOK_ANIMATION
		hkaSkeletonUtils::normalizeRotations( &(bonesOut[ firstRotBone ]), numRot );
#endif
	}
}

void CDefaultCompressedPose2::DecompressBoneRotation( AnimQsTransform& bone, Int32 rotIndex ) const
{
#ifdef USE_HAVOK_ANIMATION
	m_rotationsBuffer.GetQuaternion( rotIndex, bone.m_rotation );
#else
	m_rotationsBuffer.GetQuaternion( rotIndex, bone.Rotation );
#endif
}

void CDefaultCompressedPose2::DecompressBoneTranslation( AnimQsTransform& bone, Int32 transIndex ) const
{
	ASSERT( m_compressTranslation );
#ifdef USE_HAVOK_ANIMATION
	m_compressTranslation->GetTranslation( transIndex, bone.m_translation );
#else
	m_compressTranslation->GetTranslation( transIndex, bone.Translation );
#endif
}

//////////////////////////////////////////////////////////////////////////

Bool CCharacterCompressedPose::DecompressAndSample( Uint32 boneNumIn, AnimQsTransform* bonesOut, Uint32 tracksNumIn, AnimFloat* tracksOut, const CSkeleton* skeleton ) const
{
	if ( skeleton->GetBonesNum() != (Int32)boneNumIn || !skeleton->GetPoseCompression() )
	{
		return false;
	}

	// Pose compression
	const IPoseCompression* compression = skeleton->GetPoseCompression();

	// Check if compression can work with this pose
	if ( compression->GetClass() != CPoseCompressionCharacter::GetStaticClass() )
	{
		return false;
	}

	const CPoseCompressionCharacter* poseCompression = static_cast< const CPoseCompressionCharacter* >( compression );

	InternalDecompressAndSample( poseCompression, boneNumIn, bonesOut, skeleton );

	const Int32 left = poseCompression->GetLeftWeaponBone();
	const Int32 right = poseCompression->GetRightWeaponBone();

	if ( left > 0 && left < (Int32)boneNumIn && right > 0 && right < (Int32)boneNumIn && m_compressTranslation )
	{
		const Int32 rotSize = m_rotationsBuffer.Size();
		const Int32 transSize = m_compressTranslation->Size();

		const Int32 transNumOne = transSize / 3 - 2;
		const Int32 transNumTwo = transSize / 3 - 1;

		DecompressBone( bonesOut[ left ], rotSize - 2, transNumOne );
		DecompressBone( bonesOut[ right ], rotSize - 1, transNumTwo );
	}

	return true;
}

void CCharacterCompressedPose::DecompressBone( AnimQsTransform& bone, Int32 rotIndex, Int32 transIndex ) const
{
	// Rotation
	DecompressBoneRotation( bone, rotIndex );

	// Translation
	DecompressBoneTranslation( bone, transIndex );

#ifdef USE_HAVOK_ANIMATION
	// Scale
	bone.m_scale.setAll( 1.0f );
#else
	bone.Scale.SetOnes();
#endif
}

//////////////////////////////////////////////////////////////////////////
Bool CCameraCompressedPose::DecompressAndSample( Uint32 boneNumIn, AnimQsTransform* bonesOut, Uint32 tracksNumIn, AnimFloat* tracksOut, const CSkeleton* skeleton ) const
{
	if ( skeleton->GetBonesNum() != (Int32)boneNumIn || !skeleton->GetPoseCompression() )
	{
		return false;
	}

	// Pose compression
	const IPoseCompression* compression = skeleton->GetPoseCompression();

	// Check if compression can work with this pose
	if ( compression->GetClass() != CPoseCompressionCamera::GetStaticClass() )
	{
		return false;
	}

	const CPoseCompressionCamera* poseCompression = static_cast< const CPoseCompressionCamera* >( compression );

	InternalDecompressAndSample( poseCompression, boneNumIn, bonesOut, skeleton );

	const Uint32 trackNum = m_tracks.Size();
	if ( tracksNumIn >= trackNum )
	{
		for ( Uint32 i=0; i<trackNum; ++i )
		{
			tracksOut[ i ] = m_tracks[ i ];
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

Bool CDefaultCompressedPoseWithExtraBones::DecompressAndSample( Uint32 boneNumIn, AnimQsTransform* bonesOut, Uint32 tracksNumIn, AnimFloat* tracksOut, const CSkeleton* skeleton ) const
{
	return false;
}
