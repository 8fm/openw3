/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

namespace RedMath
{
	namespace SIMD
	{
		RedEulerAngles::RedEulerAngles()
		{
		}

		RedEulerAngles::RedEulerAngles( const RedEulerAngles& _e ) 
			: angles( _e.angles )
		{
		}

		RedEulerAngles::RedEulerAngles( float _roll, float _pitch, float _yaw )
			: Roll( _roll ),
			  Pitch( _pitch ),
			  Yaw( _yaw )
		{
		}

		RedEulerAngles::RedEulerAngles( const RedVector3& _v )
		{
			Roll = 0.f;
			
			if ( !_v.X && !_v.Y )
			{
				Yaw = 0.0f; 
				Pitch = ( _v.Z > 0.0f ) ? 90.0f : -90.0f;
			}
			else
			{
				Yaw = RAD2DEG( -Red::Math::MATan2( _v.X, _v.Y ) );
				Pitch = RAD2DEG( Red::Math::MATan2( -_v.Z, sqrtf(  _v.X * _v.X + _v.Y * _v.Y )) );
			}
		}

		RedEulerAngles::RedEulerAngles( const RedVector4& _v )
		{
			Roll = 0.f;
			
			if ( !_v.X && !_v.Y )
			{
				Yaw = 0.0f; 
				Pitch = ( _v.Z > 0.0f ) ? 90.0f : -90.0f;
			}
			else
			{
				Yaw = RAD2DEG( -Red::Math::MATan2( _v.X, _v.Y ) );
				Pitch = RAD2DEG( Red::Math::MATan2( -_v.Z, sqrtf(  _v.X * _v.X + _v.Y * _v.Y )) );
			}
		}

		RedEulerAngles::~RedEulerAngles()
		{
		}


		bool RedEulerAngles::IsAlmostEqual( const RedEulerAngles& _e, float _epsilon ) const
		{

			return 	Red::Math::NumericalUtils::Abs< float >( Roll - _e.Roll ) < _epsilon && 
				    Red::Math::NumericalUtils::Abs< float >( Pitch - _e.Pitch ) < _epsilon && 
				    Red::Math::NumericalUtils::Abs< float >( Yaw - _e.Yaw ) < _epsilon;
		}

		RedMatrix4x4 RedEulerAngles::ToMatrix4() const
		{
			float cosRoll = Red::Math::MCos( DEG2RAD( Roll ) );
			float sinRoll = Red::Math::MSin( DEG2RAD( Roll ) );
			float cosPitch = Red::Math::MCos( DEG2RAD( Pitch ) );
			float sinPitch = Red::Math::MSin( DEG2RAD( Pitch ) );
			float cosYaw = Red::Math::MCos( DEG2RAD( Yaw ) );
			float sinYaw = Red::Math::MSin( DEG2RAD( Yaw ) );

			RedMatrix4x4 ret;
			ret.SetIdentity();
			ret.Row0.X = cosRoll * cosYaw - sinPitch * sinRoll * sinYaw;
			ret.Row0.Y = sinPitch * sinRoll * cosYaw + cosRoll * sinYaw;
			ret.Row0.Z = -cosPitch * sinRoll;
			ret.Row1.X = cosPitch * -sinYaw;
			ret.Row1.Y = cosPitch * cosYaw;
			ret.Row1.Z = sinPitch;
			ret.Row2.X = sinPitch * cosRoll * sinYaw + sinRoll * cosYaw;
			ret.Row2.Y = sinRoll * sinYaw - sinPitch * cosRoll * cosYaw;
			ret.Row2.Z = cosPitch * cosRoll;
			return ret;
		}

		void RedEulerAngles::ToMatrix4( RedMatrix4x4& _out ) const
		{
			float cosRoll = Red::Math::MCos( DEG2RAD( Roll ) );
			float sinRoll = Red::Math::MSin( DEG2RAD( Roll ) );
			float cosPitch = Red::Math::MCos( DEG2RAD( Pitch ) );
			float sinPitch = Red::Math::MSin( DEG2RAD( Pitch ) );
			float cosYaw = Red::Math::MCos( DEG2RAD( Yaw ) );
			float sinYaw = Red::Math::MSin( DEG2RAD( Yaw ) );

			_out.Row0.X = cosRoll * cosYaw - sinPitch * sinRoll * sinYaw;
			_out.Row0.Y = sinPitch * sinRoll * cosYaw + cosRoll * sinYaw;
			_out.Row0.Z = -cosPitch * sinRoll;
			_out.Row0.W = 0.0f;

			_out.Row1.X = cosPitch * -sinYaw;
			_out.Row1.Y = cosPitch * cosYaw;
			_out.Row1.Z = sinPitch;
			_out.Row1.W = 0.0f;

			_out.Row2.X = sinPitch * cosRoll * sinYaw + sinRoll * cosYaw;
			_out.Row2.Y = sinRoll * sinYaw - sinPitch * cosRoll * cosYaw;
			_out.Row2.Z = cosPitch * cosRoll;
			_out.Row2.W = 0.0f;

			_out.Row3.X = 0.0f;
			_out.Row3.Y = 0.0f;
			_out.Row3.Z = 0.0f;
			_out.Row3.W = 1.0f;
		}

		void RedEulerAngles::ToAngleVectors( RedVector4* _forward, RedVector4* _right, RedVector4* _up ) const
		{
			RedVector4 f( 0, 1, 0 );
			RedVector4 r( 1, 0, 0 );
			RedVector4 u( 0, 0, 1 );

			RedMatrix4x4 mat = ToMatrix4();

			if ( _forward )
			{
				_forward->Set( RedMath::SIMD::TransformVector( mat, f ) );
			}

			if ( _right )
			{
				_right->Set( RedMath::SIMD::TransformVector( mat, r ) );
			}

			if ( _up )
			{
				_up->Set( RedMath::SIMD::TransformVector( mat, u ) );
			}
		}

		RedQuaternion RedEulerAngles::ToQuaternion() const
		{
			RedQuaternion tmp3( 0.0f, Red::Math::MSin( DEG2RAD( Roll ) / 2.0f), 0.0f, Red::Math::MCos( DEG2RAD(Roll) / 2.0f ) );
			RedQuaternion tmp2( Red::Math::MSin( DEG2RAD( Pitch ) / 2.0f ), 0.0f, 0.0f, Red::Math::MCos( DEG2RAD( Pitch ) / 2.0f ) );
			RedQuaternion tmp1( 0.0f, 0.0f, Red::Math::MSin( DEG2RAD( Yaw ) / 2.0f ), Red::Math::MCos( DEG2RAD( Yaw ) / 2.0f )  );
			
			tmp1 = RedQuaternion::Mul( tmp1, tmp2 );
			tmp1 = RedQuaternion::Mul( tmp1, tmp3 );
			tmp1.Normalize();
			tmp1.Quat.X = ( tmp1.Quat.X != tmp1.Quat.X ) ? 0.f : tmp1.Quat.X;
			tmp1.Quat.Y = ( tmp1.Quat.Y != tmp1.Quat.Y ) ? 0.f : tmp1.Quat.Y;
			tmp1.Quat.Z = ( tmp1.Quat.Z != tmp1.Quat.Z ) ? 0.f : tmp1.Quat.Z;
			tmp1.Quat.W = ( tmp1.Quat.W != tmp1.Quat.W ) ? 0.f : tmp1.Quat.W;

			return RedQuaternion( tmp1 );
		}	

		RedVector4 RedEulerAngles::TransformPoint( const RedVector4& _v ) const
		{
			RedMatrix4x4 mat = ToMatrix4();
			return RedMath::SIMD::TransformPoint( mat, _v );
		}

		RedVector4 RedEulerAngles::TransformVector( const RedVector4& _v ) const
		{
			RedMatrix4x4 mat = ToMatrix4();
			return RedMath::SIMD::TransformVector( mat, _v );
		}

		float RedEulerAngles::NormalizeAngle( float _angle )
		{
			int cycles = static_cast<int>( _angle ) / 360;
			if ( cycles != 0 ) 
			{
				_angle -= cycles * 360.0f; // angle e ( -360, 360 )
			}

			if ( _angle < 0 ) 
			{
				_angle += 360; // angle e ( 0, 360 )
			}
			return _angle;
		}

		RedEulerAngles& RedEulerAngles::Normalize()
		{
			Roll = NormalizeAngle( Roll );
			Pitch = NormalizeAngle( Pitch );
			Yaw = NormalizeAngle( Yaw );
			return *this;
		}

		float  RedEulerAngles::YawFromXY( float _x, float _y )
		{
			// Thing this may need a rethink??? Looks bad.
			if( Red::Math::MAbs( _x ) < 0.0001f && Red::Math::MAbs( _y ) < 0.0001f )
			{
				return 0.0f;
			}

			return RAD2DEG( - Red::Math::MATan2( _x, _y ) );
		}

		RedVector4 RedEulerAngles::YawToVector4( float _yaw )
		{
			float angle = DEG2RAD( _yaw );
			return RedVector4( -Red::Math::MSin( angle ), Red::Math::MCos( angle ), 0.0f, 0.0f );
		}

		RedVector2 RedEulerAngles::YawToVector2( float _yaw )
		{
			float angle = DEG2RAD( _yaw );
			return RedVector2( -Red::Math::MSin( angle ), cos( angle ) );
		}

		float RedEulerAngles::AngleDistance( float _a, float _b )
		{
			float delta = RedEulerAngles::NormalizeAngle( _b ) - RedEulerAngles::NormalizeAngle( _a );

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

		RedEulerAngles RedEulerAngles::AngleDistance( const RedEulerAngles& _a, const RedEulerAngles& _b )
		{
			RedEulerAngles ret;

			ret.Pitch = AngleDistance( _a.Pitch, _b.Pitch );
			ret.Roll = AngleDistance( _a.Roll, _b.Roll );
			ret.Yaw = AngleDistance( _a.Yaw, _b.Yaw );

			return ret;			
		}

		RedEulerAngles RedEulerAngles::InterpolateEulerAngles(const RedEulerAngles& a, const RedEulerAngles& b, float weight)
		{
			// Interpolate base on quaternion
			RedQuaternion qa;
			RedQuaternion qb;
			qa = a.ToQuaternion();
			qb = b.ToQuaternion();

			RedScalar oneMinusWeight = 1.0f - weight;

			// Check quaternion polarity
			const RedScalar signedWeight = ( Dot(qa.Quat, qb.Quat) < 0.0f ) ? -weight : weight;

			RedQuaternion out;
			
			out.Quat = Mul( qb.Quat, signedWeight );
			SetAdd( out.Quat, Mul( qa.Quat, oneMinusWeight ) );
			out.Normalize();

			RedEulerAngles outAngle;
			RedMatrix4x4 convMat = BuildFromQuaternion( out.Quat );
			outAngle = convMat.ToEulerAngles();

			return outAngle;
		}
	};
};
