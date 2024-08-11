/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "math.h"

// Tomsin TODO
// Optymalizaca & inliny

/// TRS transform - rotation is represented with a quaternion
class EngineQsTransform
{
private:
	Vector	m_position;
	Vector	m_rotation;
	Vector	m_scale;

public:
	enum IdentityInitializer
	{
		IDENTITY,
	};

public:
	//! Default
	EngineQsTransform();

	//! Initialize as identity
	EngineQsTransform( IdentityInitializer init );

	//! Initialize from other transform
	EngineQsTransform( const EngineQsTransform& otherTransform );

	//! Initialize translation only
	EngineQsTransform( const Vector& position );

	//! Initialize from translation and rotation ( quaternion )
	EngineQsTransform( const Vector& position, const Vector& rotation );

	//! Full set - initialize from translation, rotation ( quaternion ) and scale
	EngineQsTransform( const Vector& position, const Vector& rotation, const Vector& scale );

	//! Initialize from transform matrix - decompose
	EngineQsTransform( const Matrix& localToWorld );

	//! Destructor
	~EngineQsTransform();

	//! Initialize from other transform
	EngineQsTransform& Init( const EngineQsTransform& other );

	//! Initialize from translation only transform
	EngineQsTransform& Init( const Vector& position );

	//! Initialize from translation and rotation ( quaternion )
	EngineQsTransform& Init( const Vector& position, const Vector& rotation );

	//! Initialize to full transform - translation, rotation ( quaternion ) and scale
	EngineQsTransform& Init( const Vector& position, const Vector& rotation, const Vector& scale );

	//! Initialize from local to world matrix
	EngineQsTransform& Init( const Matrix& localToWorld );

public:
	//! Change translation value
	EngineQsTransform& SetPosition( const Vector& position );

	//! Change rotation ( quaternion )
	EngineQsTransform& SetRotation( const Vector& rotation );

	//! Change scale
	EngineQsTransform& SetScale( const Vector& scale );

	//! Remove translation
	EngineQsTransform& RemovePosition();

	//! Remove rotation
	EngineQsTransform& RemoveRotation();

	//! Remove scale
	EngineQsTransform& RemoveScale();

public:
	//! Reset transform to identity
	EngineQsTransform& Identity();

	//! Is the transform identity ?
	Bool IsIdentity() const;

public:
	//! Get translation vector ( position )
	const Vector& GetPosition() const;

	//! Get rotation ( quaternion )
	const Vector& GetRotation() const;

	//! Get scale
	const Vector& GetScale() const;

public:
	// Calculate local to world matrix for this translation
	void CalcLocalToWorld( Matrix& outLocalToWorld ) const;

	// Calculate world to local matrix for this translation
	void CalcWorldToLocal( Matrix& outWorldToLocal ) const;

	// Serialize to binary file
	void Serialize( IFile& file );

	// Get human readable string
	String ToString() const;

public:
	// Serialization operator
	friend IFile& operator<<( IFile& file, EngineQsTransform& et )
	{
		et.Serialize( file );
		return file;
	}

	// Compare
	Bool operator==( const EngineQsTransform& other ) const;

	// Copy
	EngineQsTransform& operator=( const EngineQsTransform& other );
};

//////////////////////////////////////////////////////////////////////////
// RTTI type

RED_DECLARE_RTTI_NAME( EngineQsTransform );

class CSimpleRTTITypeEngineQsTransform : public TSimpleRTTIType< EngineQsTransform >
{
public:
	virtual void Destruct( void *object ) const
	{
		static_cast< EngineQsTransform* >(object)->~EngineQsTransform();
	}

	virtual const CName& GetName() const
	{
		return CNAME( EngineQsTransform );
	}

	virtual ERTTITypeType GetType() const
	{
		return RT_Simple;
	}

	virtual Bool ToString( const void* data, String& valueString ) const
	{
		valueString = static_cast< const EngineQsTransform* >(data)->ToString();
		return true;
	}

	virtual Bool FromString( void*, const String& ) const
	{
		return false;
	}
};

template<>
struct TTypeName< EngineQsTransform >
{													
	static const CName& GetTypeName() { return CNAME( EngineQsTransform ); }	
};

typedef TDynArray< EngineQsTransform, MC_EngineTransformArray > TEngineQsTransformArray;