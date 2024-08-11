/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/


namespace RedMath
{
	namespace Checks
	{
#ifdef _DEBUG
		RED_INLINE void ValidateQuaternion( const SIMD::RedQuaternion& quat )
		{
			RED_MATH_MEM_ALIGNMENT_CHECK(&quat, RED_MATH_SSE_MEMORY_ALIGNMENT);
			RED_ASSERT( Red::Math::NumericalUtils::IsFinite(quat.Quat.X), TXT("X component of quaternion is invalid") );
			RED_ASSERT( Red::Math::NumericalUtils::IsFinite(quat.Quat.Y), TXT("Y component of quaternion is invalid") );
			RED_ASSERT( Red::Math::NumericalUtils::IsFinite(quat.Quat.Z), TXT("Z component of quaternion is invalid") );
			RED_ASSERT( Red::Math::NumericalUtils::IsFinite(quat.Quat.W), TXT("W component of quaternion is invalid") );
			RED_UNUSED( quat );
		}
#else
		RED_INLINE void ValidateQuaternion( const SIMD::RedQuaternion& ) {};
#endif
	}

	namespace SIMD
	{
		RedQuaternion::RedQuaternion()
		{
		}

		RedQuaternion::RedQuaternion( float _x, float _y, float _z, float _w )
		{
			Quat.Set( _x, _y, _z, _w );
			Checks::ValidateQuaternion(*this);
		}

		RedQuaternion::RedQuaternion( const RedVector4& _axis, float _angle )
		{
			SetAxisAngle( _axis, _angle );
		}

		RedQuaternion::RedQuaternion( const RedQuaternion& _q )
		{
			Quat = _q.Quat;
			Checks::ValidateQuaternion(*this);
		}

		RedQuaternion::~RedQuaternion()
		{
		}

		void RedQuaternion::operator=( const RedQuaternion& _q )
		{
			Quat = _q.Quat;
			Checks::ValidateQuaternion(*this);
		}

		void RedQuaternion::operator=( const RedVector4& _v )
		{
			Quat = _v;
			Checks::ValidateQuaternion(*this);
		}

		void RedQuaternion::ConstructFromMatrix( const RedMatrix3x3& _rotMat )
		{
			float trace = _rotMat.Row0.X + _rotMat.Row1.Y + _rotMat.Row2.Z;
			RedVector4 vec;
			if( trace > 0.0f )
			{
				float root = Red::Math::MSqrt( trace + 1.0f );
				float t = 0.5f / root;
				vec.X = ( _rotMat.Row2.Y - _rotMat.Row1.Z ) * t;
				vec.Y = ( _rotMat.Row0.Z - _rotMat.Row2.X ) * t;
				vec.Z = ( _rotMat.Row1.X - _rotMat.Row0.Y ) * t;
				vec.W = 0.5f * root;
			}
			else
			{
				const int next[] = { 1, 2, 0 };
				int i = 0;
				if( _rotMat.Row1.Y > _rotMat.Row0.X )
				{
					i = 1;
				}
				if( _rotMat.Row2.Z > _rotMat.Rows[i].f[i] )
				{
					i = 2;
				}

				int j = next[i];
				int k = next[j];

				float root = Red::Math::MSqrt( _rotMat.Rows[i].f[i] - ( _rotMat.Rows[j].f[j] + _rotMat.Rows[k].f[k] ) + 1.0f );
				float t = 0.5f / root;

				vec.f[i] = 0.5f * root;
				vec.f[3] = ( _rotMat.Rows[k].f[j] - _rotMat.Rows[j].f[k] ) * t;
				vec.f[j] = ( _rotMat.Rows[j].f[i] + _rotMat.Rows[i].f[j] ) * t;
				vec.f[k] = ( _rotMat.Rows[k].f[i] + _rotMat.Rows[i].f[k] ) * t;
			}
			Quat = vec;
			Checks::ValidateQuaternion(*this);
		}

		void RedQuaternion::Set( float _x, float _y, float _z, float _w )
		{
			Quat.Set( _x, _y, _z, _w );
			Checks::ValidateQuaternion(*this);
		}

		void RedQuaternion::Set( const RedMatrix3x3& _rotMat )
		{
			ConstructFromMatrix( _rotMat );
			Checks::ValidateQuaternion(*this);
		}

		void RedQuaternion::SetIdentity()
		{
			Quat.Set( 0.0f, 0.0f, 0.0f, 1.0f );
		}

		void RedQuaternion::SetInverse( const RedQuaternion& _q )
		{
			Quat = RedMath::SIMD::Mul( _q.Quat, RedScalar(-1.0f) );
  			Quat.W =  _q.Quat.W;

			Checks::ValidateQuaternion(*this);
		}

		void RedQuaternion::Normalize()
		{
			Quat.Normalize4();
			Checks::ValidateQuaternion(*this);
		}

		RedQuaternion RedQuaternion::Mul( const RedQuaternion& _a, const RedQuaternion& _b )
		{
			Checks::ValidateQuaternion(_a);
			Checks::ValidateQuaternion(_b);

			RedVector4 vec;
			RedScalar wVal;
			vec = Cross( _a.GetImaginary(), _b.GetImaginary() );
			wVal.Set( _a.Quat.W );
			SetAdd(vec, RedMath::SIMD::Mul(_b.GetImaginary(), wVal));
			wVal.Set( _b.Quat.W );
			SetAdd(vec, RedMath::SIMD::Mul(_a.GetImaginary(), wVal));
			float w = _a.GetReal() * _b.GetReal() - Dot3(_a.GetImaginary(), _b.GetImaginary() );

			RedQuaternion ret(vec.X, vec.Y, vec.Z, w );
			Checks::ValidateQuaternion(ret);
			return ret;
		}

		RedQuaternion RedQuaternion::Add( const RedQuaternion& _a, const RedQuaternion& _b )
		{
			Checks::ValidateQuaternion(_a);
			Checks::ValidateQuaternion(_b);
			RedVector4 result = RedMath::SIMD::Add(_a.Quat, _b.Quat);
			return RedQuaternion( result.X, result.X, result.Z, result.W );
		}

		void RedQuaternion::SetMul( const RedQuaternion& _a, const RedQuaternion& _b )
		{
			Checks::ValidateQuaternion(_a);
			Checks::ValidateQuaternion(_b);

			RedVector4 vec;
			RedScalar wVal;
			vec = Cross(_a.GetImaginary(), _b.GetImaginary());
			wVal.Set( _a.Quat.W );
			SetAdd(vec, RedMath::SIMD::Mul(_b.GetImaginary(), wVal));
			wVal.Set( _b.Quat.W );
			SetAdd(vec, RedMath::SIMD::Mul(_a.GetImaginary(), wVal));
			float w = _a.GetReal() * _b.GetReal() - Dot3(_a.GetImaginary(), _b.GetImaginary() );
			Quat = vec;
			Quat.W = w;

			Checks::ValidateQuaternion(*this);
		}

		void RedQuaternion::SetMulInverse( const RedQuaternion& _a, const RedQuaternion& _b )
		{
			RedVector4 vec;
			RedVector4 wVal( _a.Quat.W );
			vec = Cross( _a.GetImaginary(), _b.GetImaginary() );
			SetSub(vec, RedMath::SIMD::Mul(_b.GetImaginary(), wVal));
			wVal.Set( _b.Quat.W );
			SetAdd(vec, RedMath::SIMD::Mul(_a.GetImaginary(), wVal));
			Quat.Set( vec.X, vec.Y, vec.Z, Dot( _a.Quat, _b.Quat ) );
			Checks::ValidateQuaternion(*this);
		}

		void RedQuaternion::SetInverseMul( const RedQuaternion& _a, const RedQuaternion& _b )
		{
			RedVector4 vec;
			RedVector4 wVal( _a.Quat.W );
			vec = Cross( _b.GetImaginary(), _a.GetImaginary() );
			SetAdd(vec, RedMath::SIMD::Mul(_b.GetImaginary(), wVal));
			wVal.Set( _b.Quat.W );
			SetSub(vec, RedMath::SIMD::Mul(_a.GetImaginary(), wVal));
			Quat.Set( vec.X, vec.Y, vec.Z, Dot( _a.Quat, _b.Quat ) );
			Checks::ValidateQuaternion(*this);
		}

		RedVector4 RedQuaternion::EstimateAngleTo( const RedQuaternion& _to )
		{
			RedVector4 angleOut;
			RedVector4 wVal;
			const RedQuaternion& from = *this;
			angleOut = Cross( from.GetImaginary(), _to.GetImaginary() );
			wVal.Set( _to.Quat.W );
			SetSub(angleOut, RedMath::SIMD::Mul(from.GetImaginary(), wVal));
			wVal.Set( from.Quat.W );
			SetAdd(angleOut, RedMath::SIMD::Mul(_to.GetImaginary(), wVal));
			if( Dot( _to.GetImaginary(), from.GetImaginary() ) < 0.0f )
			{
				angleOut.Set( -angleOut.X, -angleOut.Y, -angleOut.Z, -angleOut.W );
			}
			return RedVector4( angleOut );
		}

		void RedQuaternion::SetFlippedRotation( const RedVector4& _from )
		{
			RedVector4 vec;
			CalculatePerpendicularVector( _from, vec );
			vec.Normalize3();
			Quat = vec;
			Checks::ValidateQuaternion(*this);
		}

		void RedQuaternion::SetShortestRotation( const RedVector4& _from, const RedVector4& _to )
		{
			const float dotProduct = Dot3( _from,  _to ); // cos(theta)
			const float nearlyOne = 1.0f - 1e-5f;
			const float somewhatNearlyOne = 1.0f - 1e-3f;

			if( dotProduct > nearlyOne )
			{
				SetIdentity();
			}
			else if( dotProduct < -nearlyOne )
			{
				SetFlippedRotation( _from );
			}
			else
			{
				const float cos = (dotProduct + 1.0f ) * 0.5f;
				const float cosT2 = Red::Math::MSqrt( cos );

				RedVector4 cross;
				cross = Cross( _from, _to );

				float rescaleSine = 0.5f / cosT2;

				if( dotProduct < -somewhatNearlyOne )
				{
					const float sinT2 = Red::Math::MSqrt( cosT2 * cosT2 - dotProduct );
					const float approxSinT = cross.Length3().X;
					const float sinT = 2.0f * sinT2 * cosT2;
					rescaleSine *= ( sinT / approxSinT );
				}

				RedMath::SIMD::SetMul(cross, rescaleSine);
				cross.W = cosT2;
				Quat = cross;
			}

			Checks::ValidateQuaternion(*this);
		}

		void RedQuaternion::SetShortestRotationDamped( float _gain, const RedVector4& _from, const RedVector4& _to )
		{
			const float dotProduct = Dot( _from, _to );//from.dot3( to ); // cos(theta)
			const float dampedDot = 1.0f - ( _gain + _gain ) * dotProduct;
			const float nearlyOne = 1.0f - 1e-6f;

			const float c = (dampedDot + 1.0f) * 0.5f;
			if( (c <= 0.0f) || (dotProduct < -nearlyOne) )
			{
				SetFlippedRotation( _from );
			}
			else
			{
				if( dotProduct > nearlyOne )
				{
					SetIdentity();
				}
				else
				{
					const float cosT2 = Red::Math::MSqrt( c );

					RedVector4 cross;
					cross = Cross( _from, _to ); // sin(theta)* axis

					// Using sin(theta) = 2*cos(theta/2)*sin(theta/2)
					const float rescaleSin  = _gain * 0.5f / cosT2;
					RedMath::SIMD::SetMul(cross, rescaleSin);
					cross.W = cosT2;

					// renormalize for gain.
					cross.Normalize4();
					Quat = cross;
				}
			}

			Checks::ValidateQuaternion(*this);
		}

		void RedQuaternion::SetAxisAngle( const RedVector4& _axis, float _angle )
		{
			float halvedAngle = 0.5f * _angle;
			float sinHalf = Red::Math::MSin( halvedAngle );
			Quat = RedMath::SIMD::Mul(_axis, sinHalf);
			Quat.W = Red::Math::MCos( halvedAngle );
			Checks::ValidateQuaternion(*this);
		}

		void RedQuaternion::SetAndNormalize( const RedMatrix3x3& _rotMat )
		{
			ConstructFromMatrix( _rotMat );
			Normalize();
		}

		void RedQuaternion::RemoveAxisComponent( const RedVector4& _axis )
		{
			// Rotate the desired axis 
			RedVector4 rotatedAxis;
			rotatedAxis.RotateDirection(*this, _axis);

			const float dotProd = Dot3( _axis, rotatedAxis );

			// Check to see if they're parallel.
			if ( ( dotProd - 1.0f ) > -1e-3f )
			{
				SetIdentity();
				return;
			}

			// Check to see if they're opposite
			if ( ( dotProd + 1.0f ) < 1e-3f )
			{
				RedVector4 perpVector;
				CalculatePerpendicularVector( _axis, perpVector );
				this->SetAxisAngle(perpVector, M_PI);
				return;
			}

			{
				const float rotationAngle = Red::Math::MAcos_safe( dotProd );

				RedVector4 rotationAxis = Cross( _axis, rotatedAxis );
				rotationAxis.Normalize3();

				SetAxisAngle(rotationAxis, rotationAngle);
			}

			Checks::ValidateQuaternion(*this);
		}

		void RedQuaternion::DecomposeRestAxis( const RedVector4& _axis, RedQuaternion& _restOut, float& _angleOut ) const
		{
			RedQuaternion axisRot;
			{
				_restOut = *this;
				_restOut.RemoveAxisComponent( _axis );

				RedQuaternion invRest;
				invRest.SetInverse(_restOut);
				axisRot.SetMul( invRest, *this );
			}

			_angleOut = axisRot.GetAngle();
			const bool rev = ( ( axisRot.GetReal() * Dot3(axisRot.GetImaginary(), _axis ) ) < 0.0f );

			if(rev)
			{
				_angleOut = -_angleOut;
			}
		}

		void RedQuaternion::SetLerp( const RedQuaternion& _a, const RedQuaternion& _b, float _t )
		{
			const RedScalar oneMinusT( 1.0f - _t );

			const RedScalar signedT( Dot( _a.Quat, _b.Quat ) < 0.0f ? -_t : _t );

			Quat = RedMath::SIMD::Add(RedMath::SIMD::Mul(_b.Quat, signedT), RedMath::SIMD::Mul( _a.Quat, oneMinusT));
			Normalize();

			Checks::ValidateQuaternion(*this);
		}

		void RedQuaternion::SetSlerp( const RedQuaternion& _a, const RedQuaternion& _b, float _t )
		{
			float cosTheta = Dot( _a.Quat, _b.Quat );

			float signOfT1 = 1.0f;
			if( cosTheta < 0.0f )
			{
				cosTheta = -cosTheta;
				signOfT1 = -1.0f;
			}

			RedVector4 slerp;

			if( cosTheta < 1.0f - 1e-3f )
			{
				float theta = Red::Math::MAcos( cosTheta );
				float iSinTheta = Red::Math::MRsqrt( 1.0f - cosTheta * cosTheta );
				float tTheta = _t * theta;

				float t0 = Red::Math::MSin( theta - tTheta ) * iSinTheta;
				float t1 = Red::Math::MSin( tTheta ) * iSinTheta;

				slerp = RedMath::SIMD::Mul(_a.Quat, t0);
				RedMath::SIMD::SetAdd(slerp, RedMath::SIMD::Mul(_b.Quat, t1 * signOfT1));
			}
			else
			{
				float t0 = 1.0f - _t;
				float t1 = _t;

				slerp = RedMath::SIMD::Mul(_a.Quat, t0);
				RedMath::SIMD::SetAdd(slerp, RedMath::SIMD::Mul(_b.Quat, RedMath::SIMD::Mul(t1, signOfT1)));
			}
			slerp.Normalize4();
			Quat = slerp;

			Checks::ValidateQuaternion(*this);
		}

		void RedQuaternion::SetReal( float _f )
		{
			Quat.W = _f;
			Checks::ValidateQuaternion(*this);
		}

		float RedQuaternion::GetReal() const
		{
			return Quat.W;
		}

		void RedQuaternion::SetImaginary( const RedVector4& _v )
		{
			Quat.X = _v.X;
			Quat.Y = _v.Y;
			Quat.Z = _v.Z;
			Checks::ValidateQuaternion(*this);
		}

		const RedVector4& RedQuaternion::GetImaginary() const
		{
			return Quat;
		}

		float RedQuaternion::GetAngle() const
		{
			float angle = Red::Math::MAbs( Quat.W );
			angle = Red::Math::MAcos_safe( angle );
			angle *= 2.0f;
			return angle;
		}

		RedVector4 RedQuaternion::GetAxis() const
		{
			RedVector4 axisTmp;

			axisTmp = Quat;
			axisTmp.Normalize3();

			if ( Quat.W < 0.0f )
			{
				RedMath::SIMD::SetMul( axisTmp, RedScalar(-1.f) );
			}
			return axisTmp;
		}

		float RedQuaternion::GetYaw() const
		{
			RedMatrix3x3 rotMatrix;
			rotMatrix.BuildFromQuaternion( Quat );
			if ( Red::Math::MAbs( rotMatrix.GetColumn( 1 ).Z ) < 0.995f )
			{
				return -RAD2DEG( Red::Math::MATan2( rotMatrix.GetColumn( 1 ).X, rotMatrix.GetColumn( 1 ).Y ) );
			}

			return 0.0f;
			/*if ( havokRotationMatrix.getColumn(1)(2) > 0.995f || havokRotationMatrix.getColumn(1)(2) < -0.995f )
			{
			return 0.0f;
			}
			else
			{
			return -RAD2DEG( atan2f( havokRotationMatrix.getColumn(1)(0), havokRotationMatrix.getColumn(1)(1) ) );
			} */
		}

		bool RedQuaternion::HasValidAxis() const
		{
			return ( Quat.Length3() > FLT_EPSILON );
		}

		bool RedQuaternion::IsOk() const
		{
			bool valid = Quat.IsOk();
			float length = Quat.SquareLength4();
			float err = length - 1.0f;
			bool validLength = ( Red::Math::MAbs( err ) < 0.0015f ); // changed because the vectorized math has a little bit less precision.
			valid = valid && validLength;
			return valid;
		}
	};
};
