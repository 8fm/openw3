/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

namespace Red
{
	namespace Math
	{
		namespace Fpu
		{
			RedEulerAngles::RedEulerAngles()
			{
			}

			RedEulerAngles::RedEulerAngles( const RedEulerAngles& _e ) 
				: Roll( _e.Roll ),
				  Pitch( _e.Pitch ),
				  Yaw( _e.Yaw )
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
					Yaw = RAD2DEG( -MATan2( _v.X, _v.Y ) );
					Pitch = RAD2DEG( MATan2( -_v.Z, sqrtf(  _v.X * _v.X + _v.Y * _v.Y )) );
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
					Yaw = RAD2DEG( -MATan2( _v.X, _v.Y ) );
					Pitch = RAD2DEG( MATan2( -_v.Z, sqrtf(  _v.X * _v.X + _v.Y * _v.Y )) );
				}
			}

			RedEulerAngles::~RedEulerAngles()
			{
			}

			RedEulerAngles RedEulerAngles::operator-() const
			{
				return RedEulerAngles( -Roll, -Pitch, -Yaw );
			}

			RedEulerAngles RedEulerAngles::operator+( const RedEulerAngles& _e ) const
			{
				return RedEulerAngles( Roll + _e.Roll, Pitch + _e.Pitch, Yaw + _e.Yaw );
			}

			RedEulerAngles RedEulerAngles::operator-( const RedEulerAngles& _e ) const
			{
				return RedEulerAngles( Roll - _e.Roll, Pitch - _e.Pitch, Yaw - _e.Yaw );
			}

			RedEulerAngles RedEulerAngles::operator*( const RedEulerAngles& _e ) const
			{
				return RedEulerAngles( Roll * _e.Roll, Pitch * _e.Pitch, Yaw * _e.Yaw );
			}

			RedEulerAngles RedEulerAngles::operator/( const RedEulerAngles& _e ) const
			{
				return RedEulerAngles( Roll / _e.Roll, Pitch / _e.Pitch, Yaw / _e.Yaw );
			}

			RedEulerAngles RedEulerAngles::operator+( float _f ) const
			{
				return RedEulerAngles( Roll + _f, Pitch + _f, Yaw + _f );
			}

			RedEulerAngles RedEulerAngles::operator-( float _f ) const
			{
				return RedEulerAngles( Roll - _f, Pitch - _f, Yaw - _f );
			}

			RedEulerAngles RedEulerAngles::operator*( float _f ) const
			{
				return RedEulerAngles( Roll * _f, Pitch * _f, Yaw * _f );
			}

			RedEulerAngles RedEulerAngles::operator/( float _f ) const
			{
				return RedEulerAngles( Roll / _f, Pitch / _f, Yaw / _f );
			}

			RedEulerAngles& RedEulerAngles::operator+=( const RedEulerAngles& _e )
			{
				Roll += _e.Roll;
				Pitch += _e.Pitch;
				Yaw += _e.Yaw;
				return *this;
			}

			RedEulerAngles& RedEulerAngles::operator-=( const RedEulerAngles& _e )
			{
				Roll -= _e.Roll;
				Pitch -= _e.Pitch;
				Yaw -= _e.Yaw;
				return *this;
			}

			RedEulerAngles& RedEulerAngles::operator*=( const RedEulerAngles& _e )
			{
				Roll *= _e.Roll;
				Pitch *= _e.Pitch;
				Yaw *= _e.Yaw;
				return *this;
			}

			RedEulerAngles& RedEulerAngles::operator/=( const RedEulerAngles& _e )
			{
				Roll /= _e.Roll;
				Pitch /= _e.Pitch;
				Yaw /= _e.Yaw;
				return *this;
			}

			RedEulerAngles& RedEulerAngles::operator+=( float _f )
			{
				Roll += _f;
				Pitch += _f;
				Yaw += _f;
				return *this;
			}

			RedEulerAngles& RedEulerAngles::operator-=( float _f )
			{
				Roll -= _f;
				Pitch -= _f;
				Yaw -= _f;
				return *this;
			}

			RedEulerAngles& RedEulerAngles::operator*=( float _f )
			{
				Roll *= _f;
				Pitch *= _f;
				Yaw *= _f;
				return *this;
			}

			RedEulerAngles& RedEulerAngles::operator/=( float _f )
			{
				Roll /= _f;
				Pitch /= _f;
				Yaw /= _f;
				return *this;
			}

			bool RedEulerAngles::operator==( const RedEulerAngles& _e ) const
			{
				return ( Roll == _e.Roll ) && ( Pitch == _e.Pitch ) && ( Yaw == _e.Yaw );
			}

			bool RedEulerAngles::operator!=( const RedEulerAngles& _e ) const
			{ 
				return ( Roll != _e.Roll ) && ( Pitch != _e.Pitch ) && ( Yaw != _e.Yaw );
			}

			bool RedEulerAngles::IsAlmostEqual( const RedEulerAngles& _e, float _epsilon ) const
			{

				return 	Red::Math::NumericalUtils::Abs< float >( Roll - _e.Roll ) < _epsilon && 
					    Red::Math::NumericalUtils::Abs< float >( Pitch - _e.Pitch ) < _epsilon && 
					    Red::Math::NumericalUtils::Abs< float >( Yaw - _e.Yaw ) < _epsilon;
			}

			RedMatrix4x4 RedEulerAngles::ToMatrix4() const
			{
				float cosRoll = MCos( DEG2RAD( Roll ) );
				float sinRoll = MSin( DEG2RAD( Roll ) );
				float cosPitch = MCos( DEG2RAD( Pitch ) );
				float sinPitch = MSin( DEG2RAD( Pitch ) );
				float cosYaw = MCos( DEG2RAD( Yaw ) );
				float sinYaw = MSin( DEG2RAD( Yaw ) );

				RedMatrix4x4 ret;
				ret.SetIdentity();
				ret.Matrix[0].V[0] = cosRoll * cosYaw - sinPitch * sinRoll * sinYaw;
				ret.Matrix[0].V[1] = sinPitch * sinRoll * cosYaw + cosRoll * sinYaw;
				ret.Matrix[0].V[2] = -cosPitch * sinRoll;
				ret.Matrix[1].V[0] = cosPitch * -sinYaw;
				ret.Matrix[1].V[1] = cosPitch * cosYaw;
				ret.Matrix[1].V[2] = sinPitch;
				ret.Matrix[2].V[0] = sinPitch * cosRoll * sinYaw + sinRoll * cosYaw;
				ret.Matrix[2].V[1] = sinRoll * sinYaw - sinPitch * cosRoll * cosYaw;
				ret.Matrix[2].V[2] = cosPitch * cosRoll;
				return ret;
			}

			void RedEulerAngles::ToMatrix4( RedMatrix4x4& _out ) const
			{
				float cosRoll = MCos( DEG2RAD( Roll ) );
				float sinRoll = MSin( DEG2RAD( Roll ) );
				float cosPitch = MCos( DEG2RAD( Pitch ) );
				float sinPitch = MSin( DEG2RAD( Pitch ) );
				float cosYaw = MCos( DEG2RAD( Yaw ) );
				float sinYaw = MSin( DEG2RAD( Yaw ) );

				_out.Matrix[0].V[0] = cosRoll * cosYaw - sinPitch * sinRoll * sinYaw;
				_out.Matrix[0].V[1] = sinPitch * sinRoll * cosYaw + cosRoll * sinYaw;
				_out.Matrix[0].V[2] = -cosPitch * sinRoll;
				_out.Matrix[0].V[3] = 0.0f;

				_out.Matrix[1].V[0] = cosPitch * -sinYaw;
				_out.Matrix[1].V[1] = cosPitch * cosYaw;
				_out.Matrix[1].V[2] = sinPitch;
				_out.Matrix[1].V[3] = 0.0f;

				_out.Matrix[2].V[0] = sinPitch * cosRoll * sinYaw + sinRoll * cosYaw;
				_out.Matrix[2].V[1] = sinRoll * sinYaw - sinPitch * cosRoll * cosYaw;
				_out.Matrix[2].V[2] = cosPitch * cosRoll;
				_out.Matrix[2].V[3] = 0.0f;

				_out.Matrix[3].V[0] = 0.0f;
				_out.Matrix[3].V[1] = 0.0f;
				_out.Matrix[3].V[2] = 0.0f;
				_out.Matrix[3].V[3] = 1.0f;
			}

			void RedEulerAngles::ToAngleVectors( RedVector4* _forward, RedVector4* _right, RedVector4* _up ) const
			{
				RedVector4 f( 0, 1, 0 );
				RedVector4 r( 1, 0, 0 );
				RedVector4 u( 0, 0, 1 );
	
				RedMatrix4x4 mat = ToMatrix4();

				if ( _forward )
				{
					_forward->Set( mat.TransformVector( f ) );
				}

				if ( _right )
				{
					_right->Set( mat.TransformVector( r ) );
				}

				if ( _up )
				{
					_up->Set( mat.TransformVector( u ) );
				}
			}

			RedQuaternion RedEulerAngles::ToQuaternion() const
			{
				RedQuaternion tmp3( 0.0f, MSin( DEG2RAD( Roll ) / 2.0f), 0.0f, MCos( DEG2RAD(Roll) / 2.0f ) );
				RedQuaternion tmp2( MSin( DEG2RAD( Pitch ) / 2.0f ), 0.0f, 0.0f, MCos( DEG2RAD( Pitch ) / 2.0f ) );
				RedQuaternion tmp1( 0.0f, 0.0f, MSin( DEG2RAD( Yaw ) / 2.0f ), MCos( DEG2RAD( Yaw ) / 2.0f )  );
				
				tmp1 = RedQuaternion::Mul( tmp1, tmp2 );
				tmp1 = RedQuaternion::Mul( tmp1, tmp3 );
				tmp1.Normalize();
				tmp1.Quat.V[0] = ( tmp1.Quat.V[0] != tmp1.Quat.V[0] ) ? 0.f : tmp1.Quat.V[0];
				tmp1.Quat.V[1] = ( tmp1.Quat.V[1] != tmp1.Quat.V[1] ) ? 0.f : tmp1.Quat.V[1];
				tmp1.Quat.V[2] = ( tmp1.Quat.V[2] != tmp1.Quat.V[2] ) ? 0.f : tmp1.Quat.V[2];
				tmp1.Quat.V[3] = ( tmp1.Quat.V[3] != tmp1.Quat.V[3] ) ? 0.f : tmp1.Quat.V[3];

				return RedQuaternion( tmp1 );
			}	

			RedVector4 RedEulerAngles::TransformPoint( const RedVector4& _v ) const
			{
				RedMatrix4x4 mat = ToMatrix4();
				return mat.TransformPoint( _v );
			}

			RedVector4 RedEulerAngles::TransformVector( const RedVector4& _v ) const
			{
				RedMatrix4x4 mat = ToMatrix4();
				return mat.TransformVector( _v );
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
				if( MAbs( _x ) < 0.0001f && MAbs( _y ) < 0.0001f )
				{
					return 0.0f;
				}

				return RAD2DEG( - MATan2( _x, _y ) );
			}

			RedVector4 RedEulerAngles::YawToVector4( float _yaw )
			{
				float angle = DEG2RAD( _yaw );
				return RedVector4( -MSin( angle ), MCos( angle ), 0.0f, 0.0f );
			}

			RedVector2 RedEulerAngles::YawToVector2( float _yaw )
			{
				float angle = DEG2RAD( _yaw );
				return RedVector2( -MSin( angle ), cos( angle ) );
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

				RedVector1 oneMinusWeight = 1.0f - weight;

				// Check quaternion polarity
				const RedVector1 signedWeight = ( RedVector4::Dot(qa.Quat, qb.Quat) < 0.0f ) ? -weight : weight;

				RedQuaternion out;
				
				out.Quat = qb.Quat * signedWeight;
				out.Quat += qa.Quat * oneMinusWeight;
				out.Normalize();

				RedEulerAngles outAngle;
				RedMatrix4x4 convMat;
				convMat.BuildFromQuaternion( out.Quat );
				outAngle = convMat.ToEulerAngles();

				return outAngle;
			}
		};
	};
};