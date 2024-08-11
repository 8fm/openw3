/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "redqstransform_simd.h"

namespace RedMath
{
	namespace SIMD
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
			return Rotation.Rows[_row].f[_column];
		}

		RED_INLINE const float& RedTransform::operator() ( int _row, int _column ) const
		{
			return Rotation.Rows[_row].f[_column];
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
			RedVector4 rotVec( TransformVector( Rotation, Translation.AsVector3() ), 0.0f );
			Translation = rotVec;
		}

		void RedTransform::SetMul( const RedTransform& _a, const RedTransform& _b )
		{
			RedMatrix3x3 unaliased = _a.GetRotation();
			int i = 3;
			do
			{
				RedVector3 bCol = _b.GetRotation().GetColumn(i);				
				RedVector3 result = TransformVector( unaliased, bCol );
				Rotation.SetColumn( i, result );
				
			} while ( --i >= 0 );
			RedMath::SIMD::SetAdd(Translation, _a.GetTranslation());
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
				result.X = unaliased.Row0.X * rotCol.X +
					          unaliased.Row1.X * rotCol.Y +
							  unaliased.Row2.X * rotCol.Z;

				result.Y = unaliased.Row0.Y * rotCol.X +
					          unaliased.Row1.Y * rotCol.Y +
							  unaliased.Row2.Y * rotCol.Z;

				result.Z = unaliased.Row0.Z * rotCol.X +
					          unaliased.Row1.Z * rotCol.Y +
							  unaliased.Row2.Z * rotCol.Z;

				Rotation.SetColumn( i, result );
			} while( --i >= 0 );

			RedVector3 h( ( Sub( _b.Translation, _a.Translation ) ).AsVector3() );
			
			Translation.X = _a.Rotation.Row0.X * h.X + _a.Rotation.Row1.X * h.Y + _a.Rotation.Row2.X * h.Z;
			Translation.Y = _a.Rotation.Row0.Y * h.X + _a.Rotation.Row1.Y * h.Y + _a.Rotation.Row2.Y * h.Z;
			Translation.Z = _a.Rotation.Row0.Z * h.X + _a.Rotation.Row1.Z * h.Y + _a.Rotation.Row2.Z * h.Z;
			Translation.W = 0.0f;
		}

		void RedTransform::SetMulEq( const RedTransform& _b )
		{
			RedTransform trans = *this;
			SetMul( trans, _b );
		}


		void RedTransform::Get4x4ColumnMajor( float* _f ) const
		{
			const float* p = &Rotation.Row0.f[0];
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
			float* p = &Rotation.Row0.f[0];
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
			return RedVector4( Rotation.Row0.f[_i], Rotation.Row1.f[_i], Rotation.Row2.f[_i], Translation.f[_i] );
		}
	};
};
