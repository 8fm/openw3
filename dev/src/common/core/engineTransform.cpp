/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "engineTransform.h"

IMPLEMENT_SIMPLE_RTTI_TYPE( EngineTransform );

const EngineTransform EngineTransform::ZEROS;

EngineTransform::EngineTransform()
	: m_data( NULL )
{
}

EngineTransform::EngineTransform( const EngineTransform& otherTransform )
	: m_data( NULL )
{
	Init( otherTransform );
}

EngineTransform::EngineTransform( const Vector& position )
	: m_data( NULL )
{
	Init( position );
}

EngineTransform::EngineTransform( const Vector& position, const EulerAngles& rotation )
	: m_data( NULL )
{
	Init( position, rotation );
}

EngineTransform::EngineTransform( const Vector& position, const EulerAngles& rotation, const Vector& scale )
	: m_data( NULL )
{
	Init( position, rotation, scale );
}

EngineTransform::EngineTransform( const Matrix& localToWorld )
	: m_data( NULL )
{
	Init( localToWorld );
}

EngineTransform::~EngineTransform()
{
	Identity();
}

EngineTransform& EngineTransform::Identity()
{
	if ( m_data )
	{
		RED_MEMORY_FREE( MemoryPool_SmallObjects, MC_Transform, m_data );
		m_data = NULL;
	}

	return *this;
}

EngineTransform& EngineTransform::Init( const EngineTransform& other )
{
	if ( &other != this )
	{
		// Reset to identity
		if ( other.IsIdentity() )
		{
			return Identity();
		}

		// Copy data
		SInternalData& data = GetIternal();
		data.m_position = other.GetPosition();
		data.m_rotation = other.GetRotation();
		data.m_scale = other.GetScale();
	}

	return *this;
}

#define POS_MAX			(1000000.0f)
#define SCALE_MAX		(1000.0f)
#define SCALE_MIN		(0.001f)

static void _ValidatePosition( const Vector& position )
{
	ASSERT( !Red::Math::NumericalUtils::IsNan( position.X ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( position.Y ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( position.Z ) );
	RED_UNUSED( position );
}

static void _ValidateRotation( const EulerAngles& rotation )
{
	ASSERT( !Red::Math::NumericalUtils::IsNan( rotation.Roll ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( rotation.Pitch ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( rotation.Yaw ) );
	RED_UNUSED( rotation );
}

static void _ValidateScale( const Vector& scale )
{
	ASSERT( !Red::Math::NumericalUtils::IsNan( scale.X ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( scale.Y ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( scale.Z ) );
	RED_UNUSED( scale );
}

static Vector _AdjustPosition( const Vector& srcPosition )
{
	_ValidatePosition( srcPosition );
	return Vector::Clamp4( srcPosition, -POS_MAX, POS_MAX );
}

static EulerAngles _AdjustRotation( const EulerAngles& srcRotation )
{
	_ValidateRotation( srcRotation );
	EulerAngles newRotation = srcRotation;
	newRotation.Normalize();
	return newRotation;
}

static Float _ClampScale( Float scale )
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

static Vector _AdjustScale( const Vector& srcScale )
{
	_ValidateScale( srcScale );
	Vector newScale;
	newScale.X = _ClampScale( srcScale.X );
	newScale.Y = _ClampScale( srcScale.Y );
	newScale.Z = _ClampScale( srcScale.Z );
	newScale.W = 1.0f;
	return newScale;
}

static Bool _IsIdentity( const Vector* pos, const EulerAngles* rot, const Vector* scale )
{
	if ( pos && !Vector::Equal3( *pos, Vector::ZERO_3D_POINT ) ) return false;
	if ( rot && ( *rot != EulerAngles::ZEROS ) ) return false;
	if ( scale && !Vector::Equal3( *scale, Vector::ONES ) ) return false;
	return true;
}

Bool EngineTransform::HasScale() const
{
	return !Vector::Equal3( GetScale(), Vector::ONES );
}

EngineTransform& EngineTransform::Init( const Vector& position )
{
	if ( ::_IsIdentity( &position, NULL, NULL ) )
	{		
		Identity();
	}
	else
	{	
		SInternalData& data = GetIternal();
		data.m_position = _AdjustPosition( position );
		data.m_rotation = EulerAngles::ZEROS;
		data.m_scale = Vector::ONES;
	}

	return *this;
}

EngineTransform& EngineTransform::Init( const Vector& position, const EulerAngles& rotation )
{
	if ( ::_IsIdentity( &position, &rotation, NULL ) )
	{		
		Identity();
	}
	else
	{	
		SInternalData& data = GetIternal();
		data.m_position = _AdjustPosition( position );
		data.m_rotation = _AdjustRotation( rotation );
		data.m_scale = Vector::ONES;
	}

	return *this;
}

EngineTransform& EngineTransform::Init( const Vector& position, const EulerAngles& rotation, const Vector& scale )
{
	if ( ::_IsIdentity( &position, &rotation, &scale ) )
	{		
		Identity();
	}
	else
	{	
		SInternalData& data = GetIternal();
		data.m_position = _AdjustPosition( position );
		data.m_rotation = _AdjustRotation( rotation );
		data.m_scale = _AdjustScale( scale );
	}

	return *this;
}

EngineTransform& EngineTransform::Init( const Matrix& localToWorld )
{
	const Vector pos = localToWorld.GetTranslation();
	const EulerAngles rot = localToWorld.ToEulerAnglesFull();
	const Vector scale = localToWorld.GetScale33();
	return Init( pos, rot, scale );
}

EngineTransform& EngineTransform::SetPosition( const Vector& position )
{
	SInternalData& data = GetIternal();
	data.m_position = _AdjustPosition( position );
	return *this;
}

EngineTransform& EngineTransform::SetRotation( const EulerAngles& rotation )
{
	SInternalData& data = GetIternal();
	data.m_rotation = _AdjustRotation( rotation );
	return *this;
}

EngineTransform& EngineTransform::SetScale( const Vector& scale )
{
	SInternalData& data = GetIternal();
	data.m_scale = _AdjustScale( scale );
	return *this;
}

EngineTransform& EngineTransform::RemovePosition()
{
	if ( m_data )
	{
		m_data->m_position = Vector::ZERO_3D_POINT;
	}

	return *this;
}

EngineTransform& EngineTransform::RemoveRotation()
{
	if ( m_data )
	{
		m_data->m_rotation = EulerAngles::ZEROS;
	}

	return *this;
}

EngineTransform& EngineTransform::RemoveScale()
{
	if ( m_data )
	{
		m_data->m_scale = Vector::ONES;
	}

	return *this;
}

const Vector& EngineTransform::GetPosition() const
{
	if ( m_data )
	{
		return m_data->m_position;
	}
	else
	{
		static Vector nullPosition = Vector::ZERO_3D_POINT;
		return nullPosition;
	}
}

const EulerAngles& EngineTransform::GetRotation() const
{
	if ( m_data )
	{
		return m_data->m_rotation;
	}
	else
	{
		static EulerAngles nullRotation = EulerAngles::ZEROS;
		return nullRotation;
	}
}

const Vector& EngineTransform::GetScale() const
{
	if ( m_data )
	{
		return m_data->m_scale;
	}
	else
	{
		static Vector nullScale = Vector::ONES;
		return nullScale;
	}
}

void EngineTransform::Mul( const EngineTransform& a, const EngineTransform& b )
{
	// Final position = a.pos + a.rot.Transform( b.pos )

	SetPosition( a.GetPosition() + a.GetRotation().TransformPoint( b.GetPosition() ) );

	// Simply add rotations

	SetRotation( a.GetRotation() + b.GetRotation() );

	// Simply multiply scales

	SetScale( a.GetScale() * b.GetScale() );
}

Vector EngineTransform::TransformPoint( const Vector& point ) const
{
	return GetPosition() + GetRotation().TransformPoint( point * GetScale() );
}

void EngineTransform::CalcLocalToWorld( Matrix& outLocalToWorld ) const
{
	if ( m_data )
	{
		m_data->m_rotation.ToMatrix( outLocalToWorld );
		outLocalToWorld.SetTranslation( m_data->m_position );
		outLocalToWorld.SetScale33( m_data->m_scale );
	}
	else
	{
		outLocalToWorld = Matrix::IDENTITY;
	}
}

void EngineTransform::CalcWorldToLocal( Matrix& outWorldToLocal ) const
{
	if ( m_data )
	{
		// TEMPSHIT, can be optimized !
		Matrix localToWorld;
		CalcLocalToWorld( localToWorld );
		outWorldToLocal = localToWorld.FullInverted();
	}
	else
	{
		outWorldToLocal = Matrix::IDENTITY;
	}
}

EngineTransform::SInternalData& EngineTransform::GetIternal()
{
	if ( !m_data )
	{
		m_data = ( SInternalData *) RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_SmallObjects, MC_Transform , sizeof( SInternalData ), DEFAULT_ENGINE_TRANSFORM_ALIGNMENT );
		m_data->m_position = Vector::ZERO_3D_POINT;
		m_data->m_rotation = EulerAngles::ZEROS;
		m_data->m_scale = Vector::ONES;
	}

	return *m_data;
}

enum EEngineTransformBitFlag
{
	ET_Position		= FLAG( 0 ),
	ET_Rotation		= FLAG( 1 ),
	ET_Scale		= FLAG( 2 ),
};

void EngineTransform::Serialize( IFile& file )
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
			if ( transformFields & ET_Position )
			{
				file << position.X;
				file << position.Y;
				file << position.Z;
			}

			// Load rotation, raw
			EulerAngles rotation = EulerAngles::ZEROS;
			if ( transformFields & ET_Rotation )
			{
				file << rotation.Roll;
				file << rotation.Pitch;
				file << rotation.Yaw;
			}

			// Load scale
			Vector scale = Vector::ONES;
			if ( transformFields & ET_Scale )
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
		if ( !Vector::Near3( GetPosition(), Vector::ZERO_3D_POINT ) ) transformFields |= ET_Position;
		if ( GetRotation() != EulerAngles::ZEROS ) transformFields |= ET_Rotation;
		if ( !Vector::Near3( GetScale(), Vector::ONES ) ) transformFields |= ET_Scale;

		// Transform flag
		file << transformFields;

		// Position, raw
		if ( transformFields & ET_Position )
		{
			file << m_data->m_position.X;
			file << m_data->m_position.Y;
			file << m_data->m_position.Z;
		}

		// Rotation, raw
		if ( transformFields & ET_Rotation )
		{
			file << m_data->m_rotation.Roll;
			file << m_data->m_rotation.Pitch;
			file << m_data->m_rotation.Yaw;
		}

		// Scale, raw
		if ( transformFields & ET_Scale )
		{
			file << m_data->m_scale.X;
			file << m_data->m_scale.Y;
			file << m_data->m_scale.Z;
		}
	}
}

Bool EngineTransform::operator==( const EngineTransform& other ) const
{
	if ( IsIdentity() && other.IsIdentity() )
	{
		return true;
	}

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

String EngineTransform::ToString() const
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
		const EulerAngles& rot = GetRotation();
		if ( rot != EulerAngles::ZEROS )
		{
			Char format[ 256 ];
			Red::System::SNPrintF( format, ARRAY_COUNT( format ), TXT("Rot [%1.1f,%1.1f,%1.1f]  "), rot.Pitch, rot.Roll, rot.Yaw );
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

EngineTransform& EngineTransform::operator=( const EngineTransform& other )
{
	Init( other );
	return *this;
}
