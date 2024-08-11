/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

namespace Red
{
	namespace Math
	{
		namespace Fpu
		{
			RedTransform::RedTransform()
			{
			}

			RedTransform::RedTransform( const RedTransform& _r )
				: Rotation( _r.Rotation ),
				  Translation( _r.Translation )
			{
			}

			RedTransform::RedTransform( const RedMatrix3x3& _r, const RedVector4& _t )
				: Rotation( _r ),
				  Translation( _t )
			{

			}

			RedTransform::RedTransform( const RedQuaternion& _r, const RedVector4& _t )
				: Translation( _t )
			{
				Rotation.BuildFromQuaternion( _r.Quat );
			}

			RedTransform::~RedTransform()
			{

			}

			RED_INLINE float& RedTransform::operator() ( int _row, int _column )
			{
				return Rotation.Matrix[_row].V[_column];
			}

			RED_INLINE const float& RedTransform::operator() ( int _row, int _column ) const
			{
				return Rotation.Matrix[_row].V[_column];
			}

			void RedTransform::Set(const RedQuaternion& _q, const RedVector4& _v )
			{
				Rotation.BuildFromQuaternion( _q.Quat );
				Translation = _v;
			}

			void RedTransform::SetIdentity()
			{
				Rotation.SetIdentity();
				Translation.SetZeros();
			}

			bool RedTransform::IsAlmostEqual( const RedTransform& _t, float _epsilon )
			{
				return Rotation.IsAlmostEqual( _t.GetRotation(), _epsilon ) && Translation.IsAlmostEqual( _t.GetTranslation(), _epsilon );
			}

			void RedTransform::SetTranslation( const RedVector4& _t )
			{
				Translation = _t;
			}

			RedVector4& RedTransform::GetTranslation()
			{
				return Translation;
			}

			const RedVector4& RedTransform::GetTranslation() const
			{
				return Translation;
			}
			
			void RedTransform::SetRotation( const RedMatrix3x3& _r )
			{
				Rotation = _r;
			}

			void RedTransform::SetRotation( const RedQuaternion& _q )
			{
				Rotation.BuildFromQuaternion( _q.Quat );
			}

			RedMatrix3x3& RedTransform::GetRotation()
			{
				return Rotation;
			}
			
			const RedMatrix3x3& RedTransform::GetRotation() const
			{
				return Rotation;
			}

			void RedTransform::SetInverse( const RedMatrix3x3& _r )
			{
				Rotation = _r.Transposed();
				RedVector4 trans;
				trans.Negate();
				RedVector4 rotVec( Rotation.TransformVector( Translation.AsVector3() ), 0.0f );
				Translation = rotVec;
			}

			void RedTransform::SetMul( const RedTransform& _a, const RedTransform& _b )
			{
				RedMatrix3x3 unaliased = _a.GetRotation();
				int i = 3;
				do
				{
					RedVector3 bCol = _b.GetRotation().GetColumn(i);				
					RedVector3 result = unaliased.TransformVector( bCol );
					Rotation.SetColumn( i, result );
					
				} while ( --i >= 0 );
				Translation += _a.GetTranslation();
			}

			void RedTransform::SetMul( const RedQsTransform& _a, const RedTransform& _b )
			{
				//TODO: May need more work - hoping to depricate the function later.
				RedTransform temp;
				_a.CopyToTransform( temp );
				SetMul( temp, _b );
			}

			void RedTransform::SetMulInverseMul( const RedTransform& _a, const RedTransform& _b )
			{

				RedMatrix3x3 unaliased = _a.GetRotation();
				int i = 2;
				do
				{
					RedVector4 rotCol = _b.Rotation.GetColumn(i); 
					RedVector3 result;
					RedMatrix3x3 matrixRes;
					result.V[0] = unaliased.Matrix[0].V[0] * rotCol.V[0] +
						          unaliased.Matrix[1].V[0] * rotCol.V[1] +
								  unaliased.Matrix[2].V[0] * rotCol.V[2];

					result.V[1] = unaliased.Matrix[0].V[1] * rotCol.V[0] +
						          unaliased.Matrix[1].V[1] * rotCol.V[1] +
								  unaliased.Matrix[2].V[1] * rotCol.V[2];

					result.V[2] = unaliased.Matrix[0].V[2] * rotCol.V[0] +
						          unaliased.Matrix[1].V[2] * rotCol.V[1] +
								  unaliased.Matrix[2].V[2] * rotCol.V[2];

					Rotation.SetColumn( i, result );
				} while( --i >= 0 );
				RedVector3 h( ( _b.Translation - _a.Translation ).AsVector3() );
				
				Translation.V[0] = _a.Rotation.Matrix[0].V[0] * h.V[0] + _a.Rotation.Matrix[1].V[0] * h.V[1] + _a.Rotation.Matrix[2].V[0] * h.V[2];
				Translation.V[1] = _a.Rotation.Matrix[0].V[1] * h.V[0] + _a.Rotation.Matrix[1].V[1] * h.V[1] + _a.Rotation.Matrix[2].V[1] * h.V[2];
				Translation.V[2] = _a.Rotation.Matrix[0].V[2] * h.V[0] + _a.Rotation.Matrix[1].V[2] * h.V[1] + _a.Rotation.Matrix[2].V[2] * h.V[2];
				Translation.V[3] = 0.0f;
			}

			void RedTransform::SetMulEq( const RedTransform& _b )
			{
				RedTransform trans = *this;
				SetMul( trans, _b );
			}


			void RedTransform::Get4x4ColumnMajor( float* _f ) const
			{
				const float* p = &Rotation.Matrix[0].V[0];
				for ( int i = 0; i < 4; ++i )
				{
					_f[0] = p[0];
					_f[1] = p[1];
					_f[2] = p[2];
					_f[3] = 0.0f;
					_f += 4;
					p += 4;
				}
				_f[-1] = 1.0f;
			}

			void RedTransform::Set4x4ColumnMajor( const float* _f )
			{
				float* p = &Rotation.Matrix[0].V[0];
				for ( int i = 0; i < 4; ++i )
				{
					p[0] = _f[0];
					p[1] = _f[1];
					p[2] = _f[2];
					p[3] = 0.0f;
					p += 4;
					_f += 4;
				}
				p[-1] = 1.0f;
			}
			
			void RedTransform::SetRows( const RedVector4& _r0, const RedVector4& _r1, const RedVector4& _r2, const RedVector4& _r3 )
			{
				Rotation.Set( _r0.AsVector3(), _r1.AsVector3(), _r2.AsVector3() );
				Translation = _r3;
			}

			bool RedTransform::IsOk() const
			{
				return ( Rotation.IsOk() && Translation.IsOk() );
			}

			RedVector4 RedTransform::GetColumn( int _i ) const
			{
				return RedVector4( Rotation.Matrix[0].V[_i], Rotation.Matrix[1].V[_i], Rotation.Matrix[2].V[_i], Translation.V[_i] );
			}
		};
	};
};