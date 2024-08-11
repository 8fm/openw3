/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "math.h"

// Align transform data to 16 bytes for SIMD
#define DEFAULT_ENGINE_TRANSFORM_ALIGNMENT 16

/// TRS transform
class EngineTransform
{
private:
	struct SInternalData
	{
		Vector			m_position;
		Vector			m_scale;
		EulerAngles		m_rotation;
	};

private:
	SInternalData*		m_data;			//!< Transform data, NULL if identity

public:
	//! Is the transform identity ?
	RED_INLINE Bool IsIdentity() const { return m_data == NULL; }

	Bool HasScale() const;

public:
	//! Initialize as identity
	EngineTransform();

	//! Initialize from other transform
	EngineTransform( const EngineTransform& otherTransform );

	//! Initialize translation only
	EngineTransform( const Vector& position );

	//! Initialize from translation and rotation
	EngineTransform( const Vector& position, const EulerAngles& rotation );

	//! Full set - initialize from translation, rotation and scale
	EngineTransform( const Vector& position, const EulerAngles& rotation, const Vector& scale );

	//! Initialize from transform matrix - decompose
	EngineTransform( const Matrix& localToWorld );

	//! Destructor
	~EngineTransform();

	//! Reset transform to identity
	EngineTransform& Identity();

	//! Initialize from other transform
	EngineTransform& Init( const EngineTransform& other );

	//! Initialize from translation only transform
	EngineTransform& Init( const Vector& position );

	//! Initialize from translation and rotation
	EngineTransform& Init( const Vector& position, const EulerAngles& rotation );

	//! Initialize to full transform - translation, rotation and scale
	EngineTransform& Init( const Vector& position, const EulerAngles& rotation, const Vector& scale );

	//! Initialize from local to world matrix
	EngineTransform& Init( const Matrix& localToWorld );

public:
	//! Change translation value
	EngineTransform& SetPosition( const Vector& position );

	//! Change rotation
	EngineTransform& SetRotation( const EulerAngles& rotation );

	//! Change scale
	EngineTransform& SetScale( const Vector& scale );

	//! Remove translation
	EngineTransform& RemovePosition();

	//! Remove rotation
	EngineTransform& RemoveRotation();

	//! Remove scale
	EngineTransform& RemoveScale();

	//! Concatenates two transforms and stores the result in itself
	void Mul( const EngineTransform& a, const EngineTransform& b );

public:
	//! Get translation vector ( position )
	const Vector& GetPosition() const;

	//! Get rotation
	const EulerAngles& GetRotation() const;

	//! Get scale
	const Vector& GetScale() const;

public:
	// Calculate local to world matrix for this translation
	void CalcLocalToWorld( Matrix& outLocalToWorld ) const;

	// Calculate world to local matrix for this translation
	void CalcWorldToLocal( Matrix& outWorldToLocal ) const;

	// Transforms given point
	Vector TransformPoint( const Vector& point ) const;

	// Serialize to binary file
	void Serialize( IFile& file );

	// Get human readable string
	String ToString() const;

	static const EngineTransform ZEROS;

public:
	// Serialization operator
	friend IFile& operator<<( IFile& file, EngineTransform& et )
	{
		et.Serialize( file );
		return file;
	}

	// Compare
	Bool operator==( const EngineTransform& other ) const;

	// Copy
	EngineTransform& operator=( const EngineTransform& other );

private:
	SInternalData& GetIternal();
};

//////////////////////////////////////////////////////////////////////////
// RTTI type

RED_DECLARE_RTTI_NAME( EngineTransform );

class CSimpleRTTITypeEngineTransform : public TSimpleRTTIType< EngineTransform >
{
public:
	virtual void Destruct( void *object ) const
	{
		static_cast< EngineTransform* >(object)->~EngineTransform();
	}

	virtual const CName& GetName() const
	{
		return CNAME( EngineTransform );
	}

	virtual ERTTITypeType GetType() const
	{
		return RT_Simple;
	}

	virtual Bool ToString( const void* data, String& valueString ) const
	{
		valueString = static_cast< const EngineTransform* >(data)->ToString();
		return true;
	}

	virtual Bool FromString( void*, const String& ) const
	{
		return false;
	}
};

template<>
struct TTypeName< EngineTransform >
{													
	static const CName& GetTypeName() { return CNAME( EngineTransform ); }	
};
