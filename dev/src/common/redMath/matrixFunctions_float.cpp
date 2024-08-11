#include "matrixFunctions_float.h"

namespace RedMath
{
	namespace FLOAT
	{
		//////////////////////////////////////////////////////////////////////////
		RedMatrix4x4 BuildPerspectiveLH( Red::System::Float _fovy, Red::System::Float _aspect, Red::System::Float _zn, Red::System::Float _zf )
		{	
			const Red::System::Float ys = 1.0f / Red::Math::MTan( _fovy * 0.5f );
			const Red::System::Float xs = ys / _aspect;
			const Red::System::Float zs = _zf/(_zf-_zn);
			return RedMatrix4x4( RedVector4( xs, 0.0f, 0.0f, 0.0f ),
				RedVector4( 0.0f, ys, 0.0f, 0.0f ),
				RedVector4( 0.0f, 0.0f, zs, 1.0f ),
				RedVector4( 0.0f, 0.0f, ( ( -_zn ) * zs ), 0.0f )
				);;
		}

		//////////////////////////////////////////////////////////////////////////
		RedMatrix4x4 BuildOrthoLH( Red::System::Float _w, Red::System::Float _h, Red::System::Float _zn, Red::System::Float _zf )
		{
			return RedMatrix4x4( RedVector4( ( 2.0f / _w ), 0.0f, 0.0f, 0.0f ),
				RedVector4( 0.0f, ( 2.0f / _h ), 0.0f, 0.0f ),
				RedVector4( 0.0f, 0.0f, ( 1.0f / ( _zf - _zn ) ), 0.0f ),
				RedVector4( 0.0f, 0.0f, -_zn/(_zf-_zn), 1.0f ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void ToAngleVectors( const RedMatrix4x4& _mat, RedVector4& _forward, RedVector4& _right, RedVector4& _up )
		{
			_forward.Set( _mat.m10, _mat.m11, _mat.m12, 0.0f );
			_right.Set( _mat.m00, _mat.m01, _mat.m02, 0.0f );
			_up.Set( _mat.m20, _mat.m21, _mat.m22, 0.0f );
		}

		//////////////////////////////////////////////////////////////////////////
		RedMatrix4x4 BuildFromDirectionVector( const RedVector4& _dirVec )
		{
			RedVector4 vB( _dirVec.Normalized3() );
			RedVector4 vC( 0.0f, 0.0f, 1.0f, 0.0f );
			
			RedVector4 vA( ( vB.Y * vC.Z ) + ( vB.Z * vC.Y ), ( vB.Z * vC.X ) + ( vB.X * vC.Z ), ( vB.X * vC.Y ) + ( vB.Y * vC.X ), 0.0f ); 
			vC = RedVector4( ( vA.Y * vB.Z ) + ( vA.Z * vB.Y ), ( vA.Z * vB.X ) + ( vA.X * vB.Z ), ( vA.X * vB.Y ) + ( vA.Y * vB.X ), 0.0f );
			vA.Normalize3();
			vC.Normalize3();
			return RedMatrix4x4( vC, vB, vC, RedVector4::EW );
		}

		//////////////////////////////////////////////////////////////////////////
		RedMatrix4x4 BuildFromQuaternion( const RedVector4& _quaternion )
		{

			float x2 = _quaternion.X + _quaternion.X;
			float xx = _quaternion.X * x2;
			float wx = _quaternion.W * x2;

			float y2 = _quaternion.Y + _quaternion.Y;
			float xy = _quaternion.X * y2;
			float yy = _quaternion.Y * y2;
			float wy = _quaternion.W * y2;
			
			float z2 = _quaternion.Z + _quaternion.Z;
			float xz = _quaternion.X * z2;
			float yz = _quaternion.Y * z2;
			float zz = _quaternion.Z * z2;
			float wz = _quaternion.W * z2;

			RedMatrix4x4 mat(
				RedVector4( 1.0f - ( yy + zz ), ( xy + wz ), ( xz - wy ), 0.0f ),
				RedVector4( ( xy - wz ), 1.0f - ( xx + zz ), ( yz + wx ), 0.0f ),
				RedVector4( ( xz + wy ), ( yz - wx ), 1.0f - ( xx + yy ), 0.0f ),
				RedVector4( 0.0f, 0.0f, 0.0f, 1.0f )
				);

			return mat;
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 TransformVector( const RedMatrix4x4& _mat, const RedVector4& _v )
		{
			return RedVector4( 
				( _mat.m00 * _v.X ) + ( _mat.m10 * _v.Y ) + ( _mat.m20 * _v.Z ), 
				( _mat.m01 * _v.X ) + ( _mat.m11 * _v.Y ) + ( _mat.m21 * _v.Z ), 
				( _mat.m02 * _v.X ) + ( _mat.m12 * _v.Y ) + ( _mat.m22 * _v.Z ),
				0.0f );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 TransformVectorWithW( const RedMatrix4x4& _mat, const RedVector4& _v )
		{
			return RedVector4( 
				( _mat.m00 * _v.X ) + ( _mat.m10 * _v.Y ) + ( _mat.m20 * _v.Z ) + ( _mat.m30 * _v.W ),
				( _mat.m01 * _v.X ) + ( _mat.m11 * _v.Y ) + ( _mat.m21 * _v.Z ) + ( _mat.m31 * _v.W ),
				( _mat.m02 * _v.X ) + ( _mat.m12 * _v.Y ) + ( _mat.m22 * _v.Z ) + ( _mat.m32 * _v.W ),
				( _mat.m03 * _v.X ) + ( _mat.m13 * _v.Y ) + ( _mat.m23 * _v.Z ) + ( _mat.m33 * _v.W ) 
				);
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 TransformPoint( const RedMatrix4x4& _mat, const RedVector4& _v )
		{
			return RedVector4( 
				( _mat.m00 * _v.X ) + ( _mat.m10 * _v.Y ) + ( _mat.m20 * _v.Z ) + _mat.m30, 
				( _mat.m01 * _v.X ) + ( _mat.m11 * _v.Y ) + ( _mat.m21 * _v.Z ) + _mat.m31, 
				( _mat.m02 * _v.X ) + ( _mat.m12 * _v.Y ) + ( _mat.m22 * _v.Z ) + _mat.m32,
				1.0f );
		}

		//////////////////////////////////////////////////////////////////////////
		void ExtractScale( const RedMatrix4x4& _m, RedMatrix4x4& _trMatrix, RedVector4& _scale )
		{
			RedVector4 xAxis = _m.GetAxisX();
			RedVector4 yAxis = _m.GetAxisY();
			RedVector4 zAxis = _m.GetAxisZ();

			// Extract scale and normalize axes
			_scale.V[0] = xAxis.Length4().X;
			_scale.V[1] = yAxis.Length4().X;
			_scale.V[2] = zAxis.Length4().X;

			_trMatrix.SetRows( xAxis.Normalize4(), yAxis.Normalize4(), zAxis.Normalize4(),  RedVector4::EW );
		}
	}
}