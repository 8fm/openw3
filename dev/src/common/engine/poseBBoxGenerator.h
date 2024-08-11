
#pragma once

class CPoseBBoxGenerator : public CObject
{
	DECLARE_ENGINE_CLASS( CPoseBBoxGenerator, CObject, 0 );

	TDynArray< String >		m_boneNames;
	TDynArray< Int32 >		m_boneIndex;

public:
	CPoseBBoxGenerator();

	virtual void OnPropertyPostChange( IProperty* property );

public:
	// Generate bounding box for animation
	Bool GenerateBBox( const CSkeletalAnimation* animation, Box& out ) const;

	Bool IsEmpty() const;
	void Fill( const CSkeleton* skeleton );
};

BEGIN_CLASS_RTTI( CPoseBBoxGenerator );
	PARENT_CLASS( CObject );
	PROPERTY_CUSTOM_EDIT_ARRAY( m_boneNames, TXT("Bones"), TXT("SkeletonBoneSelection") );
	PROPERTY_RO( m_boneIndex, TXT("") );
END_CLASS_RTTI();
