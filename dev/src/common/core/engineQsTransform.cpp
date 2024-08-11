/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "engineQsTransform.h"
#include "mathConversions.h"

IMPLEMENT_SIMPLE_RTTI_TYPE( EngineQsTransform );

EngineQsTransform::EngineQsTransform()
{
}

EngineQsTransform::EngineQsTransform( IdentityInitializer )
{
	Identity();
}

EngineQsTransform::EngineQsTransform( const EngineQsTransform& otherTransform )
{
	Init( otherTransform );
}

EngineQsTransform::EngineQsTransform( const Vector& position )
{
	Init( position );
}

EngineQsTransform::EngineQsTransform( const Vector& position, const Vector& rotation )
{
	Init( position, rotation );
}

EngineQsTransform::EngineQsTransform( const Vector& position, const Vector& rotation, const Vector& scale )
{
	Init( position, rotation, scale );
}

EngineQsTransform::EngineQsTransform( const Matrix& localToWorld )
{
	Init( localToWorld );
}

EngineQsTransform::~EngineQsTransform()
{
	
}

EngineQsTransform& EngineQsTransform::Identity()
{
	RemovePosition();
	RemoveRotation();
	RemoveScale();

	return *this;
}

EngineQsTransform& EngineQsTransform::Init( const EngineQsTransform& other )
{
	if ( &other != this )
	{
		m_position = other.GetPosition();
		m_rotation = other.GetRotation();
		m_scale = other.GetScale();
	}

	return *this;
}

#define POS_MAX			(1000000.0f)
#define SCALE_MAX		(1000.0f)
#define SCALE_MIN		(0.001f)

static void _ValidateQsPosition( const Vector& position )
{
	ASSERT( !Red::Math::NumericalUtils::IsNan( position.X ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( position.Y ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( position.Z ) );
	RED_UNUSED( position );
}

static void _ValidateQsRotation( const Vector& rotation )
{
	ASSERT( !Red::Math::NumericalUtils::IsNan( rotation.X ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( rotation.Y ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( rotation.Z ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( rotation.W ) );
	RED_UNUSED( rotation );
}

static void _ValidateQsScale( const Vector& scale )
{
	ASSERT( !Red::Math::NumericalUtils::IsNan( scale.X ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( scale.Y ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( scale.Z ) );
	RED_UNUSED( scale );
}

static Vector _AdjustQsPosition( const Vector& srcPosition )
{
	_ValidateQsPosition( srcPosition );
	return Vector::Clamp4( srcPosition, -POS_MAX, POS_MAX );
}

static Vector _AdjustQsRotation( const Vector& srcRotation )
{
	_ValidateQsRotation( srcRotation );
	Vector newRotation = srcRotation;
	newRotation.Normalize4();
	return newRotation;
}

static Float _ClampQsScale( Float scale )
{
	if ( scale < 0.0f )
	{
		return Clamp< Float >( scale, -SCALE_MAX, -SCALE_MIN );
	}
	else
	{
		return Clamp< Float >( scale, SCALE_MIN, SCALE_MAX );
	}
}

static Vector _AdjustQsScale( const Vector& srcScale )
{
	_ValidateQsScale( srcScale );
	Vector newScale;
	newScale.X = _ClampQsScale( srcScale.X );
	newScale.Y = _ClampQsScale( srcScale.Y );
	newScale.Z = _ClampQsScale( srcScale.Z );
	newScale.W = 1.0f;
	return newScale;
}

Bool EngineQsTransform::IsIdentity() const
{
	if ( !Vector::Equal3( m_position, Vector::ZERO_3D_POINT ) ) return false;
	if ( !Vector::Equal3( m_rotation, Vector::ZERO_3D_POINT ) ) return false;
	if ( !Vector::Equal3( m_scale, Vector::ONES ) ) return false;
	return true;
}

EngineQsTransform& EngineQsTransform::Init( const Vector& position )
{
	m_position = _AdjustQsPosition( position );
	m_rotation = Vector::ZERO_3D_POINT;
	m_scale = Vector::ONES;

	return *this;
}

EngineQsTransform& EngineQsTransform::Init( const Vector& position, const Vector& rotation )
{
	m_position = _AdjustQsPosition( position );
	m_rotation = _AdjustQsRotation( rotation );
	m_scale = Vector::ONES;

	return *this;
}

EngineQsTransform& EngineQsTransform::Init( const Vector& position, const Vector& rotation, const Vector& scale )
{
	m_position = _AdjustQsPosition( position );
	m_rotation = _AdjustQsRotation( rotation );
	m_scale = _AdjustQsScale( scale );

	return *this;
}

EngineQsTransform& EngineQsTransform::Init( const Matrix& localToWorld )
{
	const Vector pos = localToWorld.GetTranslation();
	
	Vector rot;
	MatrixToQuaternion( localToWorld, rot );

	const Vector scale = localToWorld.GetScale33();
	return Init( pos, rot, scale );
}

EngineQsTransform& EngineQsTransform::SetPosition( const Vector& position )
{
	m_position = _AdjustQsPosition( position );
	return *this;
}

EngineQsTransform& EngineQsTransform::SetRotation( const Vector& rotation )
{
	m_rotation = _AdjustQsRotation( rotation );
	return *this;
}

EngineQsTransform& EngineQsTransform::SetScale( const Vector& scale )
{
	m_scale = _AdjustQsScale( scale );
	return *this;
}

EngineQsTransform& EngineQsTransform::RemovePosition()
{
	m_position = Vector::ZERO_3D_POINT;
	return *this;
}

EngineQsTransform& EngineQsTransform::RemoveRotation()
{
	m_rotation = Vector::ZERO_3D_POINT;
	return *this;
}

EngineQsTransform& EngineQsTransform::RemoveScale()
{
	m_scale = Vector::ONES;
	return *this;
}

const Vector& EngineQsTransform::GetPosition() const
{
	return m_position;
}

const Vector& EngineQsTransform::GetRotation() const
{
	return m_rotation;
}

const Vector& EngineQsTransform::GetScale() const
{
	return m_scale;
}

void EngineQsTransform::CalcLocalToWorld( Matrix& outLocalToWorld ) const
{
	QuaternionToMatrix( m_rotation, outLocalToWorld );
	outLocalToWorld.SetTranslation( m_position );
	outLocalToWorld.SetScale33( m_scale );
}

void EngineQsTransform::CalcWorldToLocal( Matrix& outWorldToLocal ) const
{
	// TEMPSHIT, can be optimized !
	Matrix localToWorld;
	CalcLocalToWorld( localToWorld );
	outWorldToLocal = localToWorld.FullInverted();
}

enum EEngineQsTransformBitFlag
{
	EQT_Position	= FLAG( 0 ),
	EQT_Rotation	= FLAG( 1 ),
	EQT_Scale		= FLAG( 2 ),
};

void EngineQsTransform::Serialize( IFile& file )
{
	if ( file.IsReader() )
	{
		// Transform flag
		Uint8 transformFields = 0;
		file << transformFields;

		// Load data
		if ( transformFields != 0 )
		{
			// Load position, raw
			Vector position = Vector::ZERO_3D_POINT;
			if ( transformFields & EQT_Position )
			{
				file << position.X;
				file << position.Y;
				file << position.Z;
			}

			// Load rotation, raw
			Vector rotation = Vector::ZERO_3D_POINT;
			if ( transformFields & EQT_Rotation )
			{
				file << rotation.X;
				file << rotation.Y;
				file << rotation.Z;
				file << rotation.W;
			}

			// Load scale
			Vector scale = Vector::ONES;
			if ( transformFields & EQT_Scale )
			{
				file << scale.X;
				file << scale.Y;
				file << scale.Z;
			}

			// Initialize
			Init( position, rotation, scale );
		}
		else
		{
			// Set to identity
			Identity();
		}
	}
	else if ( file.IsWriter() )
	{
		// Determine transform fields to write
		Uint8 transformFields = 0;
		if ( !Vector::Near3( GetPosition(), Vector::ZERO_3D_POINT ) ) transformFields |= EQT_Position;
		if ( !Vector::Near3( GetRotation(), Vector::ZERO_3D_POINT ) ) transformFields |= EQT_Rotation;
		if ( !Vector::Near3( GetScale(), Vector::ONES ) ) transformFields |= EQT_Scale;

		// Transform flag
		file << transformFields;

		// Position, raw
		if ( transformFields & EQT_Position )
		{
			file << m_position.X;
			file << m_position.Y;
			file << m_position.Z;
		}

		// Rotation, raw
		if ( transformFields & EQT_Rotation )
		{
			file << m_rotation.X;
			file << m_rotation.Y;
			file << m_rotation.Z;
			file << m_rotation.W;
		}

		// Scale, raw
		if ( transformFields & EQT_Scale )
		{
			file << m_scale.X;
			file << m_scale.Y;
			file << m_scale.Z;
		}
	}
}

Bool EngineQsTransform::operator==( const EngineQsTransform& other ) const
{
	if ( GetPosition() != other.GetPosition() )
	{
		return false;
	}

	if ( GetRotation() != other.GetRotation() )
	{
		return false;
	}

	if ( GetScale() != other.GetScale() )
	{
		return false;
	}

	return true;
}

String EngineQsTransform::ToString() const
{
	// Return string
	String txt;

	// Get transform
	if ( !IsIdentity() )
	{
		// Position
		const Vector& pos = GetPosition();
		if ( !Vector::Equal3( pos, Vector::ZERO_3D_POINT ) )
		{
			Char format[ 256 ];
			Red::System::SNPrintF( format, ARRAY_COUNT( format ), TXT("Pos [%1.2f,%1.2f,%1.2f]  "), pos.X, pos.Y, pos.Z );
			txt += format;
		}

		// Rotation
		const Vector& rot = GetRotation();
		if ( !Vector::Equal3( rot, Vector::ZERO_3D_POINT ) )
		{
			Char format[ 256 ];
			Red::System::SNPrintF( format, ARRAY_COUNT( format ), TXT("Rot [%1.2f,%1.2f,%1.2f,%1.2f]  "), rot.X, rot.Y, rot.Z, rot.W );
			txt += format;
		}

		// Scale
		const Vector& scale = GetScale();
		if ( !Vector::Equal3( scale, Vector::ONES ) )
		{
			Char format[ 256 ];
			Red::System::SNPrintF( format, ARRAY_COUNT( format ), TXT("Scale [%1.2f,%1.2f,%1.2f]"), scale.X, scale.Y, scale.Z );
			txt += format;
		}
	}

	// No shit
	if ( !txt.GetLength() )
	{
		return TXT("Identity");
	}

	// Return text
	return txt;
}

EngineQsTransform& EngineQsTransform::operator=( const EngineQsTransform& other )
{
	Init( other );
	return *this;
}
