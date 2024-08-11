/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

namespace Red
{
	namespace Math
	{
		namespace Checks
		{
#ifdef _DEBUG
			RED_INLINE void ValidateQuaternion( const Fpu::RedQuaternion& quat )
			{
				RED_ASSERT( Red::Math::NumericalUtils::IsFinite(quat.Quat.X), TXT("X component of quaternion is invalid") );
				RED_ASSERT( Red::Math::NumericalUtils::IsFinite(quat.Quat.Y), TXT("Y component of quaternion is invalid") );
				RED_ASSERT( Red::Math::NumericalUtils::IsFinite(quat.Quat.Z), TXT("Z component of quaternion is invalid") );
				RED_ASSERT( Red::Math::NumericalUtils::IsFinite(quat.Quat.W), TXT("W component of quaternion is invalid") );
				RED_UNUSED( quat );
			}
#else
			RED_INLINE void ValidateQuaternion( const Fpu::RedQuaternion& ) {};
#endif
		}

		namespace Fpu
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
				float trace = _rotMat.Matrix[0].V[0] + _rotMat.Matrix[1].V[1] + _rotMat.Matrix[2].V[2];
				RedVector4 vec;
				if( trace > 0.0f )
				{
					float root = MSqrt( trace + 1.0f );
					float t = 0.5f / root;
					vec.X = ( _rotMat.Matrix[2].V[1] - _rotMat.Matrix[1].V[2] ) * t;
					vec.Y = ( _rotMat.Matrix[0].V[2] - _rotMat.Matrix[2].V[0] ) * t;
					vec.Z = ( _rotMat.Matrix[1].V[0] - _rotMat.Matrix[0].V[1] ) * t;
					vec.W = 0.5f * root;
				}
				else
				{
					const int next[] = { 1, 2, 0 };
					int i = 0;
					if( _rotMat.Matrix[1].V[1] > _rotMat.Matrix[0].V[0] )
					{
						i = 1;
					}
					if( _rotMat.Matrix[2].V[2] > _rotMat.Matrix[i].V[i] )
					{
						i = 2;
					}

					int j = next[i];
					int k = next[j];

					float root = MSqrt( _rotMat.Matrix[i].V[i] - ( _rotMat.Matrix[j].V[j] + _rotMat.Matrix[k].V[k] ) + 1.0f );
					float t = 0.5f / root;

					vec.V[i] = 0.5f * root;
					vec.V[3] = ( _rotMat.Matrix[k].V[j] - _rotMat.Matrix[j].V[k] ) * t;
					vec.V[j] = ( _rotMat.Matrix[j].V[i] + _rotMat.Matrix[i].V[j] ) * t;
					vec.V[k] = ( _rotMat.Matrix[k].V[i] + _rotMat.Matrix[i].V[k] ) * t;
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
				Quat.X = -_q.Quat.X;
				Quat.Y = -_q.Quat.Y;
				Quat.Z = -_q.Quat.Z;
				Quat.W =  _q.Quat.W;
				Checks::ValidateQuaternion(*this);
			}

			void RedQuaternion::Normalize()
			{
				Quat.Normalize();
				Checks::ValidateQuaternion(*this);
			}

			RedQuaternion RedQuaternion::Mul( const RedQuaternion& _a, const RedQuaternion& _b )
			{
				Checks::ValidateQuaternion(_a);
				Checks::ValidateQuaternion(_b);

				RedVector4 vec;
				RedVector1 wVal;
				vec = RedVector4::Cross( _a.GetImaginary(), _b.GetImaginary(), 0.0f );
				wVal.Set( _a.Quat.W );
				vec += _b.GetImaginary() * wVal;
				wVal.Set( _b.Quat.W );
				vec += _a.GetImaginary() * wVal;
				float w = _a.GetReal() * _b.GetReal() - RedVector4::Dot3(_a.GetImaginary(), _b.GetImaginary() );

				RedQuaternion ret(vec.X, vec.Y, vec.Z, w );
				Checks::ValidateQuaternion(ret);
				return ret;
			}

			RedQuaternion RedQuaternion::Add( const RedQuaternion& _a, const RedQuaternion& _b )
			{
				Checks::ValidateQuaternion(_a);
				Checks::ValidateQuaternion(_b);
				RedVector4 result = _a.Quat + _b.Quat;
				return RedQuaternion( result.X, result.X, result.Z, result.W );
			}

			void RedQuaternion::SetMul( const RedQuaternion& _a, const RedQuaternion& _b )
			{
				Checks::ValidateQuaternion(_a);
				Checks::ValidateQuaternion(_b);

				RedVector4 vec;
				RedVector1 wVal;
				vec = RedVector4::Cross(_a.GetImaginary(), _b.GetImaginary());
				wVal.Set( _a.Quat.W );
				vec += _b.GetImaginary() * wVal;
				wVal.Set( _b.Quat.W );
				vec += _a.GetImaginary() * wVal;
				float w = _a.GetReal() * _b.GetReal() - RedVector4::Dot3(_a.GetImaginary(), _b.GetImaginary() );
				Quat = vec;
				Quat.W = w;

				Checks::ValidateQuaternion(*this);
			}

			void RedQuaternion::AddMul( const RedVector1& _a, const RedQuaternion& _b )
			{
				Quat += _b.Quat * _a;
				Checks::ValidateQuaternion(*this);
			}

			void RedQuaternion::SetMulInverse( const RedQuaternion& _a, const RedQuaternion& _b )
			{
				RedVector4 vec;
				RedVector1 wVal( _a.Quat.W );
				vec = RedVector4::Cross( _a.GetImaginary(), _b.GetImaginary() );
				vec -= _b.GetImaginary() * wVal;
				wVal.Set( _b.Quat.W );
				vec += _a.GetImaginary() * wVal;
				Quat.Set( vec.X, vec.Y, vec.Z, RedVector4::Dot( _a.Quat, _b.Quat ) );
				Checks::ValidateQuaternion(*this);
			}

			void RedQuaternion::SetInverseMul( const RedQuaternion& _a, const RedQuaternion& _b )
			{
				RedVector4 vec;
				RedVector1 wVal( _a.Quat.W );
				vec = RedVector4::Cross( _b.GetImaginary(), _a.GetImaginary() );
				vec += _b.GetImaginary() * wVal;
				wVal.Set( _b.Quat.W );
				vec -= _a.GetImaginary() * wVal;
				Quat.Set( vec.X, vec.Y, vec.Z, RedVector4::Dot( _a.Quat, _b.Quat ) );
				Checks::ValidateQuaternion(*this);
			}

			RedVector4 RedQuaternion::EstimateAngleTo( const RedQuaternion& _to )
			{
				RedVector4 angleOut;
				RedVector1 wVal;
				const RedQuaternion& from = *this;
				angleOut = RedVector4::Cross( from.GetImaginary(), _to.GetImaginary() );
				wVal.Set( _to.Quat.W );
				angleOut -= from.GetImaginary() * wVal;
				wVal.Set( from.Quat.W );
				angleOut += _to.GetImaginary() * wVal;
				if( RedVector4::Dot( _to.GetImaginary(), from.GetImaginary() ) < 0.0f )
				{
					angleOut.Set( -angleOut.X, -angleOut.Y, -angleOut.Z, -angleOut.W );
				}
				return RedVector4( angleOut );
			}

			void RedQuaternion::SetFlippedRotation( const RedVector4& _from )
			{
				RedVector4 vec;
				RedVector4::CalculatePerpendicularVector( _from, vec );
				vec.Normalize3();
				Quat = vec;
				Checks::ValidateQuaternion(*this);
			}

			void RedQuaternion::SetShortestRotation( const RedVector4& _from, const RedVector4& _to )
			{
				const float dotProduct = RedVector4::Dot3( _from,  _to ); // cos(theta)
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
					const float cosT2 = MSqrt( cos );

					RedVector4 cross;
					cross = RedVector4::Cross( _from, _to );

					float rescaleSine = 0.5f / cosT2;

					if( dotProduct < -somewhatNearlyOne )
					{
						const float sinT2 = MSqrt( cosT2 * cosT2 - dotProduct );
						const float approxSinT = cross.Length3();
						const float sinT = 2.0f * sinT2 * cosT2;
						rescaleSine *= ( sinT / approxSinT );
					}

					cross *= rescaleSine;
					cross.V[3] = cosT2;
					Quat = cross;
				}

				Checks::ValidateQuaternion(*this);
			}

			void RedQuaternion::SetShortestRotationDamped( float _gain, const RedVector4& _from, const RedVector4& _to )
			{
				const float dotProduct = RedVector4::Dot( _from, _to );//from.dot3( to ); // cos(theta)
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
						const float cosT2 = MSqrt( c );

						RedVector4 cross;
						cross = RedVector4::Cross( _from, _to ); // sin(theta)* axis

						// Using sin(theta) = 2*cos(theta/2)*sin(theta/2)
						const float rescaleSin  = _gain * 0.5f / cosT2;
						cross *= rescaleSin;
						cross.V[3] = cosT2;

						// renormalize for gain.
						cross.Normalize();
						Quat = cross;
					}
				}

				Checks::ValidateQuaternion(*this);
			}

			void RedQuaternion::SetAxisAngle( const RedVector4& _axis, float _angle )
			{
				float halvedAngle = 0.5f * _angle;
				RedVector1 sinHalf = MSin( halvedAngle );
				Quat = _axis * sinHalf;
				Quat.V[3] = MCos( halvedAngle );
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

				const float dotProd = RedVector4::Dot3( _axis, rotatedAxis );

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
					RedVector4::CalculatePerpendicularVector( _axis, perpVector );
					this->SetAxisAngle(perpVector, M_PI);
					return;
				}

				{
					const float rotationAngle = MAcos_safe( dotProd );

					RedVector4 rotationAxis = RedVector4::Cross( _axis, rotatedAxis );
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
				const bool rev = ( ( axisRot.GetReal() * RedVector4::Dot3(axisRot.GetImaginary(), _axis ) ) < 0.0f );

				if(rev)
				{
					_angleOut = -_angleOut;
				}
			}

			void RedQuaternion::SetLerp( const RedQuaternion& _a, const RedQuaternion& _b, float _t )
			{
				const RedVector1 oneMinusT( 1.0f - _t );

				const RedVector1 signedT( RedVector4::Dot( _a.Quat, _b.Quat ) < 0.0f ? -_t : _t );

				Quat = _b.Quat * signedT + _a.Quat * oneMinusT;
				Normalize();

				Checks::ValidateQuaternion(*this);
			}

			void RedQuaternion::SetSlerp( const RedQuaternion& _a, const RedQuaternion& _b, float _t )
			{
				float cosTheta = RedVector4::Dot( _a.Quat, _b.Quat );

				float signOfT1 = 1.0f;
				if( cosTheta < 0.0f )
				{
					cosTheta = -cosTheta;
					signOfT1 = -1.0f;
				}

				RedVector4 slerp;

				if( cosTheta < 1.0f - 1e-3f )
				{
					float theta = MAcos( cosTheta );
					float iSinTheta = MRsqrt( 1.0f - cosTheta * cosTheta );
					float tTheta = _t * theta;

					RedVector1 t0 = MSin( theta - tTheta ) * iSinTheta;
					RedVector1 t1 = MSin( tTheta ) * iSinTheta;

					slerp = _a.Quat * t0;
					slerp += _b.Quat * (t1 * signOfT1);
				}
				else
				{
					RedVector1 t0 = 1.0f - _t;
					RedVector1 t1 = _t;

					slerp = _a.Quat * t0;
					slerp += _b.Quat * (t1 * signOfT1);
				}
				slerp.Normalize();
				Quat = slerp;

				Checks::ValidateQuaternion(*this);
			}

			void RedQuaternion::SetReal( float _f )
			{
				Quat.V[3] = _f;
				Checks::ValidateQuaternion(*this);
			}

			float RedQuaternion::GetReal() const
			{
				return Quat.V[3];
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
				float angle = MAbs( Quat.V[3] );
				angle = MAcos_safe( angle );
				angle *= 2.0f;
				return angle;
			}

			RedVector4 RedQuaternion::GetAxis() const
			{
				RedVector4 axisTmp;

				axisTmp = Quat;
				axisTmp.Normalize3();

				if(Quat.V[3] < 0.0f)
				{
					axisTmp.X = -axisTmp.X;
					axisTmp.Y = -axisTmp.Y;
					axisTmp.Z = -axisTmp.Z;
					axisTmp.W = -axisTmp.W;
				}
				return RedVector4( axisTmp );
			}

			float RedQuaternion::GetYaw() const
			{
				RedMatrix3x3 rotMatrix;
				rotMatrix.BuildFromQuaternion( Quat );
				if ( Red::Math::MAbs( rotMatrix.GetColumn( 1 ).V[2] ) < 0.995f )
				{
					return -RAD2DEG( Red::Math::MATan2( rotMatrix.GetColumn( 1 ).V[ 0 ], rotMatrix.GetColumn( 1 ).V[1] ) );
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
				float length = Quat.SquareLength();
				float err = length - 1.0f;
				bool validLength = ( MAbs( err ) < 0.0012f ); // changed because the vectorized math has a little bit less precision.
				valid = valid && validLength;
				return valid;
			}
		};
	};
};