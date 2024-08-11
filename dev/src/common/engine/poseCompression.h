/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "compressedBuffers.h"
#include "compressedTranslation.h"
#include "animMath.h"

class ICompressedPose;
class CDefaultCompressedPose2;

class IPoseCompression : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IPoseCompression, CObject );

public:
	struct InputPose
	{
		AnimQsTransform* m_bones;
		Uint32 m_bonesNum;

		AnimFloat* m_tracks;
		Uint32 m_tracksNum;

		// Hack
		Bool m_doubleRotationQuality;

		RED_INLINE InputPose()
			: m_bones( NULL )
			, m_bonesNum( 0 )
			, m_tracks( NULL )
			, m_tracksNum( 0 )
			, m_doubleRotationQuality( false )
		{}
	};

public:
	virtual Bool IsValid() const { return true; }

	virtual ICompressedPose* CreateCompressedPose( CObject* /*parent*/, const CSkeleton* /*skeleton*/, const InputPose& /*poseIn*/ ) const { return NULL; }

	virtual void Recreate() {}
};

BEGIN_ABSTRACT_CLASS_RTTI( IPoseCompression );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CPoseCompressionNone : public IPoseCompression
{
	DECLARE_ENGINE_CLASS( CPoseCompressionNone, IPoseCompression, 0 );

public:
	virtual Bool IsValid() const;

	virtual ICompressedPose* CreateCompressedPose( CObject* parent, const CSkeleton* skeleton, const InputPose& poseIn ) const;
};

BEGIN_CLASS_RTTI( CPoseCompressionNone );
	PARENT_CLASS( IPoseCompression );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CPoseCompressionDefault : public IPoseCompression
{
	DECLARE_ENGINE_CLASS( CPoseCompressionDefault, IPoseCompression, 0 );

protected:
	String					m_firstRotBoneName;
	String					m_lastRotBoneName;
	String					m_firstTransBoneName;
	String					m_lastTransBoneName;

	Int32						m_firstRotBone;
	Int32						m_lastRotBone;
	Int32						m_firstTransBone;
	Int32						m_lastTransBone;

	ECompressTranslationType m_compressTranslationType;

public:
	CPoseCompressionDefault();

	Int32 GetFirstRotBone() const;
	Int32 GetLastRotBone() const;
	Int32 GetFirstTransBone() const;
	Int32 GetLastTransBone() const;

public:
	virtual void OnPropertyPostChange( IProperty* property );

	virtual Bool IsValid() const;
	
	virtual ICompressedPose* CreateCompressedPose( CObject* parent, const CSkeleton* skeleton, const InputPose& poseIn ) const;

	virtual void Recreate();

protected:
	Bool FillPose( const CSkeleton* skeleton, const InputPose& poseIn, CDefaultCompressedPose2* poseOut ) const;
	CompressedTranslation* CreateCompressTranslation( Float transMin, Float transMax, Uint32 size ) const;
	void AddBoneRotation( CDefaultCompressedPose2* poseOut, const AnimQsTransform& bone ) const;
	void AddBoneTranslation( CDefaultCompressedPose2* poseOut, const AnimQsTransform& bone ) const;
	void AddBone( CDefaultCompressedPose2* poseOut, const AnimQsTransform& bone ) const;
	virtual void CheckOtherExtraBonesForMaxMin( const AnimQsTransform* bones, Uint32 boneNum, Float& max, Float &min ) const;

	void FindBone( const String& name, Int32& index, Int32 offset = 0 ) const;
};

BEGIN_CLASS_RTTI( CPoseCompressionDefault );
	PARENT_CLASS( IPoseCompression );
	PROPERTY_CUSTOM_EDIT( m_firstRotBoneName, TXT(""), TXT("SkeletonBoneSelection") );
	PROPERTY_CUSTOM_EDIT( m_lastRotBoneName, TXT(""), TXT("SkeletonBoneSelection") );
	PROPERTY_CUSTOM_EDIT( m_firstTransBoneName, TXT(""), TXT("SkeletonBoneSelection") );
	PROPERTY_CUSTOM_EDIT( m_lastTransBoneName, TXT(""), TXT("SkeletonBoneSelection") );
	PROPERTY_RO( m_firstRotBone, TXT("") );
	PROPERTY_RO( m_lastRotBone, TXT("") );
	PROPERTY_RO( m_firstTransBone, TXT("") );
	PROPERTY_RO( m_lastTransBone, TXT("") );
	PROPERTY_EDIT( m_compressTranslationType, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CPoseCompressionDefaultWithExtraBones : public CPoseCompressionDefault
{
	DECLARE_ENGINE_CLASS( CPoseCompressionDefaultWithExtraBones, CPoseCompressionDefault, 0 );

protected:
	TDynArray< Int32 > m_extraRotBones;
	TDynArray< Int32 > m_extraTransBones;

public:
	virtual ICompressedPose* CreateCompressedPose( CObject* parent, const CSkeleton* skeleton, const InputPose& poseIn ) const;
};

BEGIN_CLASS_RTTI( CPoseCompressionDefaultWithExtraBones );
	PARENT_CLASS( CPoseCompressionDefault );
	PROPERTY_EDIT( m_extraRotBones, TXT("") );
	PROPERTY_EDIT( m_extraTransBones, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CPoseCompressionCharacter : public CPoseCompressionDefault
{
	DECLARE_ENGINE_CLASS( CPoseCompressionCharacter, CPoseCompressionDefault, 0 );

protected:
	String		m_leftWeaponName;
	String		m_rightWeaponName;

	Int32			m_leftWeapon;
	Int32			m_rightWeapon;

public:
	CPoseCompressionCharacter();

	virtual void OnPropertyPostChange( IProperty* property );

	Int32 GetLeftWeaponBone() const;
	Int32 GetRightWeaponBone() const;

	virtual ICompressedPose* CreateCompressedPose( CObject* parent, const CSkeleton* skeleton, const InputPose& poseIn ) const;

	virtual void Recreate();

private:
	Bool AreExtraBonesValid( Int32 max ) const;
	virtual void CheckOtherExtraBonesForMaxMin( const AnimQsTransform* bones, Uint32 boneNum, Float& max, Float &min ) const;
};

BEGIN_CLASS_RTTI( CPoseCompressionCharacter );
	PARENT_CLASS( CPoseCompressionDefault );
	PROPERTY_CUSTOM_EDIT( m_leftWeaponName, TXT(""), TXT("SkeletonBoneSelection") );
	PROPERTY_CUSTOM_EDIT( m_rightWeaponName, TXT(""), TXT("SkeletonBoneSelection") );
	PROPERTY_RO( m_leftWeapon, TXT("") );
	PROPERTY_RO( m_rightWeapon, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CPoseCompressionCamera : public CPoseCompressionDefault
{
	DECLARE_ENGINE_CLASS( CPoseCompressionCamera, CPoseCompressionDefault, 0 );

public:
	virtual ICompressedPose* CreateCompressedPose( CObject* parent, const CSkeleton* skeleton, const InputPose& poseIn ) const;
};

BEGIN_CLASS_RTTI( CPoseCompressionCamera );
	PARENT_CLASS( CPoseCompressionDefault );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

typedef Int32 CompressedPoseHandle;

class ICompressedPose : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ICompressedPose, CObject );

public:
	virtual Uint32 GetSize() const { return 0; }
	virtual Uint32 GetBufferSize() const { return 0; }
	virtual Bool DecompressAndSample( Uint32 , AnimQsTransform* , Uint32 , AnimFloat* , const CSkeleton*  ) const { return false; }
};

BEGIN_ABSTRACT_CLASS_RTTI( ICompressedPose );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CNoCompressedPose : public ICompressedPose
{
	DECLARE_ENGINE_CLASS( CNoCompressedPose, ICompressedPose, 0 );

public:
	virtual Uint32 GetSize() const;
	virtual Uint32 GetBufferSize() const;
	virtual Bool DecompressAndSample( Uint32 boneNumIn, AnimQsTransform* bonesOut, Uint32 tracksNumIn, AnimFloat* tracksOut, const CSkeleton* skeleton ) const;
};

BEGIN_CLASS_RTTI( CNoCompressedPose );
	PARENT_CLASS( ICompressedPose );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CDefaultCompressedPose2 : public ICompressedPose
{
	friend class CPoseCompressionDefault;

	DECLARE_ENGINE_CLASS( CDefaultCompressedPose2, ICompressedPose, 0 );

protected:
	Red::TScopedPtr< CompressedTranslation > m_compressTranslation;
	CompressedQuaternionBuffer	m_rotationsBuffer;

public:
	CDefaultCompressedPose2();

	virtual void OnSerialize( IFile& file );

public:
	Int32 GetFirstRotBone() const;
	Int32 GetLastRotBone() const;
	Int32 GetFirstTransBone() const;
	Int32 GetLastTransBone() const;

	virtual Uint32 GetSize() const;
	virtual Uint32 GetBufferSize() const;

	virtual Bool DecompressAndSample( Uint32 boneNumIn, AnimQsTransform* bonesOut, Uint32 tracksNumIn, AnimFloat* tracksOut, const CSkeleton* skeleton ) const;

protected:
	void InternalDecompressAndSample( const CPoseCompressionDefault* poseCompression, Uint32 boneNumIn, AnimQsTransform* bonesOut, const CSkeleton* skeleton ) const;
	void DecompressBoneRotation( AnimQsTransform& bone, Int32 rotIndex ) const;
	void DecompressBoneTranslation( AnimQsTransform& bone, Int32 transIndex ) const;
};

BEGIN_CLASS_RTTI( CDefaultCompressedPose2 );
	PARENT_CLASS( ICompressedPose );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CDefaultCompressedPoseWithExtraBones : public CDefaultCompressedPose2
{
	friend class CPoseCompressionDefaultWithExtraBones;

	DECLARE_ENGINE_CLASS( CDefaultCompressedPoseWithExtraBones, CDefaultCompressedPose2, 0 );

public:
	virtual Bool DecompressAndSample( Uint32 boneNumIn, AnimQsTransform* bonesOut, Uint32 tracksNumIn, AnimFloat* tracksOut, const CSkeleton* skeleton ) const;
};

BEGIN_CLASS_RTTI( CDefaultCompressedPoseWithExtraBones );
	PARENT_CLASS( CDefaultCompressedPose2 );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CCharacterCompressedPose : public CDefaultCompressedPose2
{
	friend class CPoseCompressionCharacter;

	DECLARE_ENGINE_CLASS( CCharacterCompressedPose, CDefaultCompressedPose2, 0 );

public:
	virtual Bool DecompressAndSample( Uint32 boneNumIn, AnimQsTransform* bonesOut, Uint32 tracksNumIn, AnimFloat* tracksOut, const CSkeleton* skeleton ) const;

protected:
	void DecompressBone( AnimQsTransform& bone, Int32 index, Int32 offset ) const;
};

BEGIN_CLASS_RTTI( CCharacterCompressedPose );
	PARENT_CLASS( CDefaultCompressedPose2 );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CCameraCompressedPose : public CDefaultCompressedPose2
{
	friend class CPoseCompressionCamera;

	DECLARE_ENGINE_CLASS( CCameraCompressedPose, CDefaultCompressedPose2, 0 );

protected:
	TDynArray< Float >	m_tracks;

public:
	virtual Bool DecompressAndSample( Uint32 boneNumIn, AnimQsTransform* bonesOut, Uint32 tracksNumIn, AnimFloat* tracksOut, const CSkeleton* skeleton ) const;
};

BEGIN_CLASS_RTTI( CCameraCompressedPose );
	PARENT_CLASS( CDefaultCompressedPose2 );
	PROPERTY( m_tracks );
END_CLASS_RTTI();
