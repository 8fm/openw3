/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

namespace Red
{
	namespace Math
	{
		namespace Fpu
		{
			RedQsTransform::RedQsTransform()
			{
			}

			RedQsTransform::RedQsTransform( const RedQsTransform& _other )
			{
				Translation = _other.Translation;
				Rotation = _other.Rotation;
				Scale = _other.Scale;
			}

			RedQsTransform::RedQsTransform( const RedVector4& _translation, const RedQuaternion& _rotation, const RedVector4& _scale ) 
				: Translation( _translation ),
				  Rotation( _rotation ),
				  Scale( _scale )
			{

			}

			RedQsTransform::RedQsTransform( const RedVector4& _translation, const RedQuaternion& _rotation )
				: Translation( _translation ),
				  Rotation( _rotation )
			{
				Scale.SetOnes();
			}

			RedQsTransform::~RedQsTransform()
			{

			}

			void RedQsTransform::Set( const RedVector4& _translation, const RedQuaternion& _rotation, const RedVector4& _scale )
			{
				Translation = _translation;
				Rotation = _rotation;
				Scale = _scale;
			}

			void RedQsTransform::Set( const RedMatrix4x4& _m )
			{
				Translation = _m.GetTranslation();
				Scale = _m.GetScale();
				Rotation = _m.ToQuaternion(); 
			}

			void RedQsTransform::SetIdentity()
			{
				Translation.SetZeros();
				Rotation.SetIdentity();
				Scale.SetOnes();
			}

			void RedQsTransform::SetZero()
			{
				Translation.SetZeros();
				Rotation.Quat.SetZeros();
				Scale.SetZeros();
			}

			void RedQsTransform::SetTranslation( const RedVector4& _v )
			{
				Translation = _v;
			}

			void RedQsTransform::SetRotation( const RedQuaternion& _r )
			{
				Rotation = _r;
			}

			void RedQsTransform::SetScale ( const RedVector4& _s )
			{
				Scale = _s;
			}

			const RedVector4& RedQsTransform::GetTranslation() const
			{
				return Translation;
			}

			const RedQuaternion& RedQsTransform::GetRotation() const
			{
				return Rotation;
			}

			const RedVector4& RedQsTransform::GetScale() const
			{
				return Scale;
			}

			void RedQsTransform::Lerp( const RedQsTransform& _a, const RedQsTransform& _b, float _t)
			{
				Scale = RedVector4::Lerp( _a.Scale, _b.Scale, _t );
				Translation = RedVector4::Lerp( _a.Translation, _b.Translation, _t );
				Rotation.SetLerp( _a.Rotation, _b.Rotation , _t );
			}

			void RedQsTransform::Slerp( const RedQsTransform& _a, const RedQsTransform& _b, float _t)
			{
				Scale = RedVector4::Lerp( _a.Scale, _b.Scale, _t );
				Translation = RedVector4::Lerp( _a.Translation, _b.Translation, _t );
				Rotation.SetSlerp( _a.Rotation, _b.Rotation, _t );
			}

			RedMatrix4x4 RedQsTransform::ConvertToMatrix() const
			{
				//TODO: Chris I know this isn't the nicest bit of code - I need to refactor this further
				const RedVector4& q = Rotation.Quat;

				float x2 = q.V[0] + q.V[0];
				float y2 = q.V[1] + q.V[1];
				float z2 = q.V[2] + q.V[2];
				float xx = q.V[0] * x2;
				float xy = q.V[0] * y2;
				float xz = q.V[0] * z2;
				float yy = q.V[1] * y2;
				float yz = q.V[1] * z2;
				float zz = q.V[2] * z2;
				float wx = q.V[3] * x2;
				float wy = q.V[3] * y2;
				float wz = q.V[3] * z2;
				
				return RedMatrix4x4( RedVector4( ( 1.0f - ( yy + zz ) ) * Scale.V[0], ( xy + wz ), ( xz - wy ), 0.0f ),
							  RedVector4( ( xy - wz ), ( 1.0f - ( xx + zz ) ) * Scale.V[1], ( yz + wx ), 0.0f ),
							  RedVector4( ( xz + wy ), ( yz - wx ), ( 1.0f - ( xx + yy ) ) * Scale.V[2], 0.0f ),
							  RedVector4( Translation.V[0], Translation.V[1], Translation.V[2], 1.0f ) );
			}

			RedMatrix4x4 RedQsTransform::ConvertToMatrixNormalized() const
			{
				//TODO: Chris I know this isn't the nicest bit of code - I need to refactor this further
				RedQsTransform t = *this;
				t.Rotation.Normalize();

				const RedVector4& q = t.Rotation.Quat;

				float x2 = q.V[0] + q.V[0];
				float y2 = q.V[1] + q.V[1];
				float z2 = q.V[2] + q.V[2];
				float xx = q.V[0] * x2;
				float xy = q.V[0] * y2;
				float xz = q.V[0] * z2;
				float yy = q.V[1] * y2;
				float yz = q.V[1] * z2;
				float zz = q.V[2] * z2;
				float wx = q.V[3] * x2;
				float wy = q.V[3] * y2;
				float wz = q.V[3] * z2;
				return RedMatrix4x4( RedVector4( ( 1.0f - ( yy + zz ) ) * t.Scale.V[0], xy + wz, xz - wy, 0.0f ),
									 RedVector4( xy - wz, ( 1.0f - ( xx + zz ) ) * t.Scale.V[1], yz + wx, 0.0f ),
									 RedVector4( xz + wy, yz - wx, ( 1.0f - ( xx + yy ) ) * t.Scale.V[2], 0.0f ),
									 RedVector4( t.Translation.X, t.Translation.Y, t.Translation.Z, 1.0f  ) );
			}

			void RedQsTransform::SetFromTransformNoScale( const RedTransform& _t )
			{
				Rotation.ConstructFromMatrix( _t.GetRotation() );
				Translation.Set( _t.GetTranslation() );
				Scale.SetOnes();
			}

			void RedQsTransform::CopyToTransformNoScale( RedTransform& _tOut ) const
			{
				_tOut.Set( Rotation, Translation );
			}

			void RedQsTransform::SetFromTransform( const RedTransform& _t )
			{
				Set4x4ColumnMajor( &_t.GetRotation().Matrix[0].V[0] );
			}

			void RedQsTransform::CopyToTransform( RedTransform& _tOut ) const
			{
				Get4x4ColumnMajor( &_tOut.GetRotation().Matrix[0].V[0] );
			}

			void RedQsTransform::SetInverse( const RedQsTransform& _t )
			{
				Translation.InverseRotateDirection( _t.Rotation, _t.Translation);
				Translation.Negate();
				
				Rotation.SetInverse( _t.Rotation );

				Scale.X = 1.0f / _t.Scale.X;
				Scale.Y = 1.0f / _t.Scale.Y;
				Scale.Z = 1.0f / _t.Scale.Z;
				Scale.W = 0.0f;
			}

			void RedQsTransform::SetMul( const RedQsTransform& _a, const RedQsTransform& _b )
			{
				RedVector4 extra;
				extra.RotateDirection( _a.Rotation, _b.Translation );
				Translation = _a.Translation + extra;
				Rotation = RedQuaternion::Mul(_a.Rotation, _b.Rotation );

				Scale = _a.Scale * _b.Scale;
			}

			void RedQsTransform::SetMulInverseMul( const RedQsTransform& _a, const RedQsTransform& _b )
			{
				RedQsTransform h;
				h.SetInverse( _a );
				SetMul(h, _b);
			}

			void RedQsTransform::SetMulMulInverse( const RedQsTransform& _a, const RedQsTransform& _b )
			{
				RedQsTransform h;
				h.SetInverse( _b );
				SetMul( _a, h);
			}

			void RedQsTransform::SetMulEq( const RedQsTransform& _t )
			{
				// Rotation and position go together
				RedVector4 extra;
				extra.RotateDirection( Rotation, _t.Translation );
				Translation += extra;
				Rotation = RedQuaternion::Mul( Rotation, _t.Rotation );

				// Scale goes apart
				Scale *= _t.Scale;
			}

			bool RedQsTransform::Set4x4ColumnMajor( const float* _f )
			{
				RedVector4 trans;
				RedQuaternion rot;
				RedVector4 scale;
				RedVector1 halfscalar( 0.5f );

				bool hasScale;
				bool hasSkew;
				bool flips;
				RedMatrix3x3 scaleAndSkew;
				RedMatrix3x3 skew;

				RedMatrix3x3 basis;

				const int maxNumIterations = 30;
				const float tolerance = FLT_EPSILON;

				RedTransform t;
				t.Set4x4ColumnMajor( _f );

				{
					trans = t.GetTranslation();
				}

				{
					RedMatrix3x3 qCur;
					RedMatrix3x3 qNext;
					RedMatrix3x3 qInvT;

					int numIterations = 0;
		
					qNext = t.GetRotation();
					do
					{
						qCur = qNext;
						qInvT = qCur;
						// I commented out the following 2 lines because the Matrix3x3.Invert() does a transpose instead and the Transpose() call will revert the change again
 						//qInvT.Invert();
 						//qInvT.Transpose();

						RedMatrix3x3 temp( qCur.GetColumn( 0 ) + qInvT.GetColumn( 0 ),
										   qCur.GetColumn( 1 ) + qInvT.GetColumn( 1 ), 
										   qCur.GetColumn( 2 ) + qInvT.GetColumn( 2 ) );

						qNext.Set( temp.GetColumn(0) * halfscalar, temp.GetColumn(1) * halfscalar, temp.GetColumn(2) * halfscalar );
					} while( ( ++numIterations < maxNumIterations ) && ( !qNext.IsAlmostEqual( qCur, tolerance ) ) );
					basis = qNext;
				}

				{
					RedVector3 c1 = basis.GetColumn( 0 );
					c1.Normalize();
					RedVector3 c2 = basis.GetColumn( 1 );
					c2.Normalize();
					RedVector3 c3 = basis.GetColumn( 2 );
					c3.Normalize();

					RedVector3 r0( RedVector3::Cross( c2, c3 ) );

					const float determinant = c1.Dot(r0);

					if (determinant < 0.0f )
					{
						c1 *= RedVector1( -1.0f );

					}

					flips = ( determinant < 0.0f );

					// Rotation quaternion
					{
						RedMatrix3x3 tempRot;
						tempRot.SetColumn( 0, c1 );
						tempRot.SetColumn( 1, c2 );
						tempRot.SetColumn( 2, c3 );
						rot.ConstructFromMatrix( tempRot ); 
						rot.Normalize();
					}
				}

				// Scale and Skew
				{
					RedQuaternion rotInvQ;
					rotInvQ.SetInverse( rot );

					RedMatrix3x3 rotInvM; 
					rotInvM.BuildFromQuaternion( rotInvQ.Quat );

					scaleAndSkew = rotInvM * t.GetRotation();
				}
	
				// Scale
				{

					// Diagonal
					RedVector3 tempScale( scaleAndSkew.Matrix[0].V[0], scaleAndSkew.Matrix[1].V[1], scaleAndSkew.Matrix[2].V[2] );
					scale.Set( tempScale, 1.0f );
					RedVector3 one;
					one.SetOnes();
					hasScale = !tempScale.IsAlmostEqual( one );
				}

				// Skew
				{
					// scale and skew = scale * skew
					// skew = inv(scale) * scaleAndSkew
					RedMatrix3x3 scaleInv;
					scaleInv.Matrix[0].V[0] = 1.0f / scale.V[0];
					scaleInv.Matrix[1].V[1] = 1.0f / scale.V[1];
					scaleInv.Matrix[2].V[2] = 1.0f / scale.V[2];
					skew = scaleInv * scaleAndSkew;
		
					RedTransform identity;
					identity.SetIdentity();

					hasSkew = !skew.IsAlmostEqual(identity.GetRotation());
				}

				Translation = trans;
				Rotation = rot;
				Scale = scale;

				return !hasSkew;
			}

			void RedQsTransform::Get4x4ColumnMajor( float* _f ) const
			{
				RedMatrix3x3 rotMatrix;
				RedMatrix3x3 scaMatrix;
				rotMatrix.BuildFromQuaternion( Rotation.Quat );
				scaMatrix.SetZeros();
				scaMatrix.Matrix[0].V[0] = Scale.V[0];
				scaMatrix.Matrix[1].V[1] = Scale.V[1];
				scaMatrix.Matrix[2].V[2] = Scale.V[2];

				RedMatrix3x3 rotSca = rotMatrix * scaMatrix;

				RedTransform temp(rotSca, Translation );
				temp.Get4x4ColumnMajor( _f );
			}

			bool RedQsTransform::IsOk() const
			{
				return ( Translation.IsOk() && Rotation.IsOk() && Scale.IsOk() );
			}

			bool RedQsTransform::IsAlmostEqual( const RedQsTransform& _other, float _epsilon ) const
			{
				RedVector4 toCompThis = Rotation.Quat;; 
				if( RedVector4::Dot( Rotation.Quat, _other.Rotation.Quat ) < 0.0f )
				{
					toCompThis *= -1.0f;
				}
				return Translation.IsAlmostEqual( _other.Translation, _epsilon ) && toCompThis.IsAlmostEqual( _other.Rotation.Quat ) && Scale.IsAlmostEqual( _other.Scale, _epsilon );
			}
				
			void RedQsTransform::BlendAddMul(const RedQsTransform& _other,  float _weight )
			{
				RedVector1 weight( _weight );
				Translation += _other.Translation * weight;
				Scale += _other.Scale * weight;

				const RedVector1 signedWeight = ( RedVector4::Dot( Rotation.Quat, _other.Rotation.Quat) < 0.0f ) ? -weight : weight;

				Rotation.Quat += _other.Rotation.Quat * signedWeight;
			}

			void RedQsTransform::BlendNormalize( float _totalWeight )
			{
				if( MAbs( _totalWeight ) < FLT_EPSILON )
				{
					SetIdentity();
					return;
				}

				{
					const RedVector1 invWeight( 1.0f / _totalWeight );

					Translation *= invWeight;
					Scale *= invWeight;
				}

				{
					const float length = Rotation.Quat.SquareLength();
					if( length < FLT_EPSILON )
					{
						Rotation.SetIdentity();
					}
					else
					{
						Rotation.Normalize();
					}
				}

				{
					const float scaleNorm = Scale.SquareLength3();
					if( scaleNorm < FLT_EPSILON )
					{
						Scale.SetOnes();
					}
				}
			}

			void RedQsTransform::FastRenormalize( float _totalWeight )
			{
				const RedVector1 invWeight( 1.0f / _totalWeight );

				Translation *= invWeight;
				Scale *= invWeight;
				Rotation.Normalize();
			}

			void RedQsTransform::FastRenormalizeBatch( RedQsTransform* _poseOut, float* _weight, unsigned int _numTransforms)
			{
				for (unsigned int i = 0; i < _numTransforms; ++i)
				{
					const RedVector1 invWeight( 1.0f / _weight[i] );
					_poseOut[i].Translation *= invWeight;
					_poseOut[i].Scale *= invWeight;
				}

				// now normalize 4 quaternions at once
				RedQsTransform* blockStart = _poseOut;
				unsigned int numTransformsOver4 = _numTransforms / 4;
				for ( unsigned int i = 0; i < numTransformsOver4; ++i )
				{
					RedVector4 dots;
					dots.V[0] = RedVector4::Dot( blockStart[0].Rotation.Quat, blockStart[0].Rotation.Quat );
					dots.V[1] = RedVector4::Dot( blockStart[1].Rotation.Quat, blockStart[1].Rotation.Quat );
					dots.V[2] = RedVector4::Dot( blockStart[2].Rotation.Quat, blockStart[2].Rotation.Quat );
					dots.V[3] = RedVector4::Dot( blockStart[3].Rotation.Quat, blockStart[3].Rotation.Quat );
		
					RedVector4 inverseSqrtDots;
					inverseSqrtDots.V[0] = MRsqrt( dots.V[0] );
					inverseSqrtDots.V[1] = MRsqrt( dots.V[1] );
					inverseSqrtDots.V[2] = MRsqrt( dots.V[2] );
					inverseSqrtDots.V[3] = MRsqrt( dots.V[3] );
					RedVector1 invA( inverseSqrtDots.V[0] );
					RedVector1 invB( inverseSqrtDots.V[1] );
					RedVector1 invC( inverseSqrtDots.V[2] );
					RedVector1 invD( inverseSqrtDots.V[3] );

					blockStart[0].Rotation.Quat *= invA;
					blockStart[1].Rotation.Quat *= invB;
					blockStart[2].Rotation.Quat *= invC;
					blockStart[3].Rotation.Quat *= invD;

					blockStart += 4;
				}

				unsigned int remainders = _numTransforms % 4;
				for ( unsigned int i = 0; i < remainders; ++i )
				{
					blockStart[i].Rotation.Normalize();
				}
			}
		}
	}
}