/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

namespace RedMath
{
	namespace SIMD
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

		void RedQsTransform::operator=( const RedQsTransform& _other )
		{
			Translation = _other.Translation;
			Rotation = _other.Rotation;
			Scale = _other.Scale;
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

			float x2 = q.X + q.X;
			float y2 = q.Y + q.Y;
			float z2 = q.Z + q.Z;
			float xx = q.X * x2;
			float xy = q.X * y2;
			float xz = q.X * z2;
			float yy = q.Y * y2;
			float yz = q.Y * z2;
			float zz = q.Z * z2;
			float wx = q.W * x2;
			float wy = q.W * y2;
			float wz = q.W * z2;
			
			return RedMatrix4x4( RedVector4( ( 1.0f - ( yy + zz ) ) * Scale.X, ( xy + wz ), ( xz - wy ), 0.0f ),
						  RedVector4( ( xy - wz ), ( 1.0f - ( xx + zz ) ) * Scale.Y, ( yz + wx ), 0.0f ),
						  RedVector4( ( xz + wy ), ( yz - wx ), ( 1.0f - ( xx + yy ) ) * Scale.Z, 0.0f ),
						  RedVector4( Translation.X, Translation.Y, Translation.Z, 1.0f ) );
		}

		RedMatrix4x4 RedQsTransform::ConvertToMatrixNormalized() const
		{
			//TODO: Chris I know this isn't the nicest bit of code - I need to refactor this further
			RedQsTransform t = *this;
			t.Rotation.Normalize();

			const RedVector4& q = t.Rotation.Quat;

			float x2 = q.X + q.X;
			float y2 = q.Y + q.Y;
			float z2 = q.Z + q.Z;
			float xx = q.X * x2;
			float xy = q.X * y2;
			float xz = q.X * z2;
			float yy = q.Y * y2;
			float yz = q.Y * z2;
			float zz = q.Z * z2;
			float wx = q.W * x2;
			float wy = q.W * y2;
			float wz = q.W * z2;
			return RedMatrix4x4( RedVector4( ( 1.0f - ( yy + zz ) ) * t.Scale.X, xy + wz, xz - wy, 0.0f ),
								 RedVector4( xy - wz, ( 1.0f - ( xx + zz ) ) * t.Scale.Y, yz + wx, 0.0f ),
								 RedVector4( xz + wy, yz - wx, ( 1.0f - ( xx + yy ) ) * t.Scale.Z, 0.0f ),
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
			Set4x4ColumnMajor( &_t.GetRotation().Row0.f[0] );
		}

		void RedQsTransform::CopyToTransform( RedTransform& _tOut ) const
		{
			Get4x4ColumnMajor( &_tOut.GetRotation().Row0.f[0] );
		}

		void RedQsTransform::SetInverse( const RedQsTransform& _t )
		{
			Translation.InverseRotateDirection( _t.Rotation, _t.Translation);
			Translation.Negate();
			
			Rotation.SetInverse( _t.Rotation );

			Scale = Div(RedScalar(1.f), _t.Scale);
			Scale.W = 0.0f;
		}

		void RedQsTransform::SetMul( const RedQsTransform& _a, const RedQsTransform& _b )
		{
			RedVector4 extra;
			extra.RotateDirection( _a.Rotation, _b.Translation );

			Translation = Add( _a.Translation, extra );
			Rotation = RedQuaternion::Mul(_a.Rotation, _b.Rotation );

			Scale = Mul( _a.Scale, _b.Scale );
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
			SetAdd( Translation, extra );
			Rotation = RedQuaternion::Mul( Rotation, _t.Rotation );

			// Scale goes apart
			RedMath::SIMD::SetMul( Scale, _t.Scale );
		}

		bool RedQsTransform::Set4x4ColumnMajor( const float* _f )
		{
			RedVector4 trans;
			RedQuaternion rot;
			RedVector4 scale;
			RedScalar halfscalar( 0.5f );

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

					RedMatrix3x3 temp( Add( qCur.GetColumn( 0 ), qInvT.GetColumn( 0 ) ),
									   Add( qCur.GetColumn( 1 ), qInvT.GetColumn( 1 ) ), 
									   Add( qCur.GetColumn( 2 ), qInvT.GetColumn( 2 ) ) );

					qNext.Set( Mul( temp.GetColumn(0), halfscalar), Mul( temp.GetColumn(1), halfscalar), Mul( temp.GetColumn(2), halfscalar) );
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

				RedVector3 r0( Cross( c2, c3 ) );

				const float determinant = Dot( c1, r0 );

				if (determinant < 0.0f )
				{
					RedMath::SIMD::SetMul( c1,  RedScalar( -1.0f ) );

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

				scaleAndSkew = Mul( rotInvM, t.GetRotation() );
			}

			// Scale
			{

				// Diagonal
				RedVector3 tempScale( scaleAndSkew.Row0.X, scaleAndSkew.Row1.Y, scaleAndSkew.Row2.Z );
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
				scaleInv.Row0.X = 1.0f / scale.X;
				scaleInv.Row1.Y = 1.0f / scale.Y;
				scaleInv.Row2.Z = 1.0f / scale.Z;
				skew = Mul( scaleInv, scaleAndSkew );
	
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
			scaMatrix.Row0.X = Scale.X;
			scaMatrix.Row1.Y = Scale.Y;
			scaMatrix.Row2.Z = Scale.Z;

			RedMatrix3x3 rotSca = Mul( rotMatrix, scaMatrix );

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
			if( Dot( Rotation.Quat, _other.Rotation.Quat ) < 0.0f )
			{
				RedMath::SIMD::SetMul( toCompThis, -1.0f );
			}
			return Translation.IsAlmostEqual( _other.Translation, _epsilon ) && toCompThis.IsAlmostEqual( _other.Rotation.Quat ) && Scale.IsAlmostEqual( _other.Scale, _epsilon );
		}
			
		void RedQsTransform::BlendAddMul(const RedQsTransform& _other,  float _weight )
		{
			RedScalar weight( _weight );
			SetAdd( Translation, Mul( _other.Translation, weight ) );
			SetAdd( Scale, Mul( _other.Scale, weight ) );

			const float signedWeight = ( Dot( Rotation.Quat, _other.Rotation.Quat ) < 0.0f ) ? -_weight : _weight;

			SetAdd( Rotation.Quat, Mul( _other.Rotation.Quat, signedWeight) );
		}

		void RedQsTransform::BlendNormalize( float _totalWeight )
		{
			if( Red::Math::MAbs( _totalWeight ) < FLT_EPSILON )
			{
				SetIdentity();
				return;
			}

			{
				const RedScalar invWeight( 1.0f / _totalWeight );

				RedMath::SIMD::SetMul(Translation, invWeight);
				RedMath::SIMD::SetMul(Scale, invWeight);
			}

			{
				const float length = Rotation.Quat.SquareLength4();
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
			const RedScalar invWeight( 1.0f / _totalWeight );

			RedMath::SIMD::SetMul( Translation, invWeight );
			RedMath::SIMD::SetMul( Scale, invWeight );
			Rotation.Normalize();
		}

		// 
		void RedQsTransform::FastRenormalizeBatch( RedQsTransform* _poseOut, float* _weight, unsigned int _numTransforms)
		{
			for (unsigned int i = 0; i < _numTransforms; ++i)
			{
				const RedScalar invWeight( 1.0f / _weight[i] );
				RedMath::SIMD::SetMul( _poseOut[i].Translation, invWeight );
				RedMath::SIMD::SetMul( _poseOut[i].Scale, invWeight );
			}

			// now normalize 4 quaternions at once
			RedQsTransform* blockStart = _poseOut;
			unsigned int numTransformsOver4 = _numTransforms / 4;
			for ( unsigned int i = 0; i < numTransformsOver4; ++i )
			{
				RedVector4 dots;
				dots.X = Dot( blockStart[0].Rotation.Quat, blockStart[0].Rotation.Quat );
				dots.Y = Dot( blockStart[1].Rotation.Quat, blockStart[1].Rotation.Quat );
				dots.Z = Dot( blockStart[2].Rotation.Quat, blockStart[2].Rotation.Quat );
				dots.W = Dot( blockStart[3].Rotation.Quat, blockStart[3].Rotation.Quat );
	
				RedVector4 inverseSqrtDots;
				inverseSqrtDots.X = Red::Math::MRsqrt( dots.X );
				inverseSqrtDots.Y = Red::Math::MRsqrt( dots.Y );
				inverseSqrtDots.Z = Red::Math::MRsqrt( dots.Z );
				inverseSqrtDots.W = Red::Math::MRsqrt( dots.W );

				RedScalar invA( inverseSqrtDots.X );
				RedScalar invB( inverseSqrtDots.Y );
				RedScalar invC( inverseSqrtDots.Z );
				RedScalar invD( inverseSqrtDots.W );

				RedMath::SIMD::SetMul( blockStart[0].Rotation.Quat, invA );
				RedMath::SIMD::SetMul( blockStart[1].Rotation.Quat, invB );
				RedMath::SIMD::SetMul( blockStart[2].Rotation.Quat, invC );
				RedMath::SIMD::SetMul( blockStart[3].Rotation.Quat, invD );

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