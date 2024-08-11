/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

EulerAngles::EulerAngles( Float roll, Float pitch, Float yaw )
	: Roll( roll )
	, Pitch( pitch )
	, Yaw( yaw )
{
}

EulerAngles::EulerAngles( const EulerAngles &ea )
: Roll( ea.Roll )
, Pitch( ea.Pitch )
, Yaw( ea.Yaw )
{
}

EulerAngles EulerAngles::operator-() const
{
	return EulerAngles( -Roll, -Pitch, -Yaw );
}

EulerAngles EulerAngles::operator+( const EulerAngles& a ) const
{
	return EulerAngles( Roll + a.Roll, Pitch + a.Pitch, Yaw + a.Yaw );
}

EulerAngles EulerAngles::operator-( const EulerAngles& a ) const
{
	return EulerAngles( Roll - a.Roll, Pitch - a.Pitch, Yaw - a.Yaw );
}

EulerAngles EulerAngles::operator*( const EulerAngles& a ) const
{
	return EulerAngles( Roll * a.Roll, Pitch * a.Pitch, Yaw * a.Yaw );
}

EulerAngles EulerAngles::operator/( const EulerAngles& a ) const
{
	return EulerAngles( Roll / a.Roll, Pitch / a.Pitch, Yaw / a.Yaw );
}

EulerAngles EulerAngles::operator+( Float a ) const
{
	return EulerAngles( Roll + a, Pitch + a, Yaw + a );
}

EulerAngles EulerAngles::operator-( Float a ) const
{
	return EulerAngles( Roll - a, Pitch - a, Yaw - a );
}

EulerAngles EulerAngles::operator*( Float a ) const
{
	return EulerAngles( Roll * a, Pitch * a, Yaw * a );
}

EulerAngles EulerAngles::operator/( Float a ) const
{
	return EulerAngles( Roll / a, Pitch / a, Yaw / a );
}

EulerAngles& EulerAngles::operator+=( const EulerAngles& a )
{
	Roll += a.Roll;
	Pitch += a.Pitch;
	Yaw += a.Yaw;
	return *this;
}

EulerAngles& EulerAngles::operator-=( const EulerAngles& a )
{
	Roll -= a.Roll;
	Pitch -= a.Pitch;
	Yaw -= a.Yaw;
	return *this;
}

EulerAngles& EulerAngles::operator*=( const EulerAngles& a )
{
	Roll *= a.Roll;
	Pitch *= a.Pitch;
	Yaw *= a.Yaw;
	return *this;
}

EulerAngles& EulerAngles::operator/=( const EulerAngles& a )
{
	Roll /= a.Roll;
	Pitch /= a.Pitch;
	Yaw /= a.Yaw;
	return *this;
}

EulerAngles& EulerAngles::operator+=( Float a )
{
	Roll += a;
	Pitch += a;
	Yaw += a;
	return *this;
}

EulerAngles& EulerAngles::operator-=( Float a )
{
	Roll -= a;
	Pitch -= a;
	Yaw -= a;
	return *this;
}

EulerAngles& EulerAngles::operator*=( Float a )
{
	Roll *= a;
	Pitch *= a;
	Yaw *= a;
	return *this;
}

EulerAngles& EulerAngles::operator/=( Float a )
{
	Roll /= a;
	Pitch /= a;
	Yaw /= a;
	return *this;
}

Bool EulerAngles::operator==( const EulerAngles& a ) const
{
	return ( Roll == a.Roll ) && ( Pitch == a.Pitch ) && ( Yaw == a.Yaw );
}

Bool EulerAngles::AlmostEquals( const EulerAngles& a, Float epsilon /* = 0.01f */ ) const
{
	return	Abs< Float >( Roll - a.Roll ) < epsilon &&
			Abs< Float >( Pitch - a.Pitch ) < epsilon &&
			Abs< Float >( Yaw - a.Yaw ) < epsilon;
}

Bool EulerAngles::operator!=( const EulerAngles& a ) const
{
	return ( Roll != a.Roll ) || ( Pitch != a.Pitch ) || ( Yaw != a.Yaw );
}

Matrix EulerAngles::ToMatrix() const
{
	// The rotation matrix for the Euler angles
	// Order of rotation: Y ( roll ), X ( pitch ), Z ( yaw )
	// All rotations are CCW

	Float cosRoll = MCos( DEG2RAD( Roll ) );
	Float sinRoll = MSin( DEG2RAD( Roll ) );
	Float cosPitch = MCos( DEG2RAD( Pitch ) );
	Float sinPitch = MSin( DEG2RAD( Pitch ) );
	Float cosYaw = MCos( DEG2RAD( Yaw ) );
	Float sinYaw = MSin( DEG2RAD( Yaw ) );

	Matrix ret;
	ret.SetIdentity();
	ret.V[0].A[0] = cosRoll * cosYaw - sinPitch * sinRoll * sinYaw;
	ret.V[0].A[1] = sinPitch * sinRoll * cosYaw + cosRoll * sinYaw;
	ret.V[0].A[2] = -cosPitch * sinRoll;
	ret.V[1].A[0] = cosPitch * -sinYaw;
	ret.V[1].A[1] = cosPitch * cosYaw;
	ret.V[1].A[2] = sinPitch;
	ret.V[2].A[0] = sinPitch * cosRoll * sinYaw + sinRoll * cosYaw;
	ret.V[2].A[1] = sinRoll * sinYaw - sinPitch * cosRoll * cosYaw;
	ret.V[2].A[2] = cosPitch * cosRoll;
	return ret;
}

void EulerAngles::ToMatrix( Matrix& out_matrix ) const
{
	// ab> same as above, except will eliminate use of the operator=

	// The rotation matrix for the Euler angles
	// Order of rotation: Y ( roll ), X ( pitch ), Z ( yaw )
	// All rotations are CCW

	Float cosRoll = MCos( DEG2RAD( Roll ) );
	Float sinRoll = MSin( DEG2RAD( Roll ) );
	Float cosPitch = MCos( DEG2RAD( Pitch ) );
	Float sinPitch = MSin( DEG2RAD( Pitch ) );
	Float cosYaw = MCos( DEG2RAD( Yaw ) );
	Float sinYaw = MSin( DEG2RAD( Yaw ) );

	out_matrix.V[0].A[0] = cosRoll * cosYaw - sinPitch * sinRoll * sinYaw;
	out_matrix.V[0].A[1] = sinPitch * sinRoll * cosYaw + cosRoll * sinYaw;
	out_matrix.V[0].A[2] = -cosPitch * sinRoll;
	out_matrix.V[0].A[3] = 0.0f;

	out_matrix.V[1].A[0] = cosPitch * -sinYaw;
	out_matrix.V[1].A[1] = cosPitch * cosYaw;
	out_matrix.V[1].A[2] = sinPitch;
	out_matrix.V[1].A[3] = 0.0f;

	out_matrix.V[2].A[0] = sinPitch * cosRoll * sinYaw + sinRoll * cosYaw;
	out_matrix.V[2].A[1] = sinRoll * sinYaw - sinPitch * cosRoll * cosYaw;
	out_matrix.V[2].A[2] = cosPitch * cosRoll;
	out_matrix.V[2].A[3] = 0.0f;

	out_matrix.V[3].A[0] = 0.0f;
	out_matrix.V[3].A[1] = 0.0f;
	out_matrix.V[3].A[2] = 0.0f;
	out_matrix.V[3].A[3] = 1.0f;
}

Vector EulerAngles::ToQuat() const
{
	Vector q;

	float c1 = MCos( 0.5f * DEG2RAD( Yaw ) );
	float s1 = MSin( 0.5f * DEG2RAD( Yaw ) );
	float c2 = MCos( 0.5f * DEG2RAD( Pitch ) );
	float s2 = MSin( 0.5f * DEG2RAD( Pitch ) );
	float c3 = MCos( 0.5f * DEG2RAD( Roll ) );
	float s3 = MSin( 0.5f * DEG2RAD( Roll ) );
	float c1c2 = c1 * c2;
	float s1s2 = s1 * s2;
	q.W = c1c2 * c3 - s1s2 * s3;
	q.X = c1c2 * s3 + s1s2 * c3;
	q.Z = s1 * c2 * c3 + c1 * s2 * s3;
	q.Y = c1 * s2 * c3 - s1 * c2 * s3;

	return q;
}

Vector EulerAngles::TransformPoint( const Vector& a ) const
{
	Matrix mat = ToMatrix();
	return mat.TransformPoint( a );
}

Vector EulerAngles::TransformVector( const Vector& a ) const
{
	Matrix mat = ToMatrix();
	return mat.TransformVector( a );
}

void EulerAngles::ToAngleVectors( Vector* forward, Vector* right, Vector* up) const
{
	Vector f( 0, 1, 0 );
	Vector r( 1, 0, 0 );
	Vector u( 0, 0, 1 );
	
	Matrix mat = ToMatrix();

	if ( forward )
	{
		forward->Set3( mat.TransformVector( f ) );
	}

	if ( right )
	{
		right->Set3( mat.TransformVector( r ) );
	}

	if ( up )
	{
		up->Set3( mat.TransformVector( u ) );
	}
};  

EulerAngles Vector::ToEulerAngles() const
{
	Float yaw, pitch;

	if ( !X && !Y )
	{
		yaw = 0.0f; 
		pitch = (Z > 0) ? 90.0f : -90.0f;
	}
	else
	{
		yaw = RAD2DEG( -atan2f( X,Y ) );
		pitch = RAD2DEG( atan2f( -Z, sqrtf( X*X + Y*Y )) );
	}

	return EulerAngles( 0.0f, pitch, yaw );
}

Float EulerAngles::NormalizeAngle( Float angle )
{
	Int32 cycles = (Int32)angle / 360;
	if ( cycles != 0 ) angle -= cycles * 360.0f; // angle e ( -360, 360 )
	if ( angle < 0 ) angle += 360; // angle e ( 0, 360 )
	return angle;
}

EulerAngles& EulerAngles::Normalize()
{
	Roll = NormalizeAngle( Roll );
	Pitch = NormalizeAngle( Pitch );
	Yaw = NormalizeAngle( Yaw );
	return *this;
}

Float EulerAngles::NormalizeAngle180( Float angle )
{
	angle = NormalizeAngle( angle );
	return angle > 180.f ? angle - 360.f : angle;
}

Float EulerAngles::ToNearestAngle( Float angle, Float referenceAngle )
{
	angle = NormalizeAngle( angle );
	while ( angle + 180.f < referenceAngle )
	{
		angle += 360.f;
	}
	while ( angle - 180.f > referenceAngle )
	{
		angle -= 360.f;
	}
	return angle;
}

Float EulerAngles::YawFromXY( Float x, Float y )
{
	if ( MAbs( x ) < 0.0001f && MAbs( y ) < 0.0001f )
		return 0.0f;

	return RAD2DEG( -MATan2( x,y ) );
}

Double EulerAngles::YawFromXY( Double x, Double y )
{
	if ( fabs( x ) < 0.0001 && fabs( y ) < 0.0001 )
		return 0.0;

	return RAD2DEG( -atan2( x,y ) );
}

Vector EulerAngles::YawToVector( Float yaw )
{
	Float angle = DEG2RAD( yaw );
	return Vector( -sin( angle ), cos( angle ), 0.f, 0.f );
}

Vector2 EulerAngles::YawToVector2( Float yaw )
{
	Float angle = DEG2RAD( yaw );
	return Vector2( -sin( angle ), cos( angle ) );
}

Float EulerAngles::AngleDistance( Float a, Float b )
{
	Float delta = EulerAngles::NormalizeAngle( b ) - EulerAngles::NormalizeAngle( a );

	// Get shortest distance
	if ( delta < -180.0f )
	{
		return delta + 360.0f;
	}
	else if ( delta > 180.0f )
	{
		return delta - 360.0f;
	}
	else
	{
		return delta;
	}
}

EulerAngles EulerAngles::AngleDistance( const EulerAngles& a, const EulerAngles& b )
{
	EulerAngles ret;

	ret.Pitch = AngleDistance( a.Pitch, b.Pitch );
	ret.Roll = AngleDistance( a.Roll, b.Roll );
	ret.Yaw = AngleDistance( a.Yaw, b.Yaw );

	return ret;
}

Float EulerAngles::Interpolate( Float a, Float b, Float weight )
{
	const Float diff = AngleDistance( a, b );

	return a + diff * weight;
}

EulerAngles EulerAngles::Interpolate( const EulerAngles& a, const EulerAngles& b, Float weight )
{
	const EulerAngles diff = AngleDistance( a, b );

	return a + diff * weight;
}

void EulerAngles::Interpolate( const EulerAngles& a, Float weight )
{
	operator+=( AngleDistance( *this, a ) * weight );
}