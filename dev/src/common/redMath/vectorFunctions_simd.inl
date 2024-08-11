namespace RedMath
{
	namespace SIMD
	{
		//////////////////////////////////////////////////////////////////////////
		void SqrRoot( RedScalar& _a )
		{
			_a.V = _mm_sqrt_ss( _a.V );
			_a.V = _mm_shuffle_ps( _a.V, _a.V, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar SqrRoot( const RedScalar& _a )
		{
			SIMDVector vA = _mm_sqrt_ss( _a.V );
			return _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void SqrRoot( RedVector2& _a )
		{
			_a.V = _mm_sqrt_ps( _a.V );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2 SqrRoot( const RedVector2& _a )
		{
			return RedVector2( _mm_sqrt_ps( _a.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void SqrRoot( RedVector3& _a )
		{
			_a.V = _mm_sqrt_ps( _a.V );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 SqrRoot( const RedVector3& _a )
		{
			return RedVector3( _mm_sqrt_ps( _a.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void SqrRoot( RedVector4& _a )
		{
			_a.V = _mm_sqrt_ps( _a.V );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 SqrRoot( const RedVector4& _a )
		{
			return RedVector4( _mm_sqrt_ps( _a.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void SqrRoot3( RedVector4& _a )
		{
			_a.V = _mm_add_ps( _mm_and_ps( _a.V, W_MASK ), _mm_sqrt_ps( _mm_and_ps( _a.V, XYZ_MASK ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 SqrRoot3( const RedVector4& _a )
		{
			return RedVector4( _mm_add_ps( _mm_and_ps( _a.V, W_MASK ), _mm_sqrt_ps( _mm_and_ps( _a.V, XYZ_MASK ) ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		// 'RECIPROCAL SQUARE ROOT' Functions
		//////////////////////////////////////////////////////////////////////////
		void RSqrRoot( RedScalar& _a )
		{
			_a.V = _mm_rsqrt_ss( _a.V );
			_a.V = _mm_shuffle_ps( _a.V, _a.V, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RSqrRoot( const RedScalar& _a )
		{
			SIMDVector vA = _mm_rsqrt_ss( _a.V );
			return _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		}
		
		//////////////////////////////////////////////////////////////////////////
		void RSqrRoot( RedVector2& _a )
		{
			_a.V = _mm_rsqrt_ps( _a.V );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2 RSqrRoot( const RedVector2& _a )
		{
			return RedVector2( _mm_rsqrt_ps( _a.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void RSqrRoot( RedVector3& _a )
		{
			_a.V = _mm_add_ps( _mm_and_ps( _mm_rsqrt_ps( _a.V ), XYZ_MASK ), _mm_and_ps( _a.V, W_MASK ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 RSqrRoot( const RedVector3& _a )
		{
			return RedVector3( _mm_add_ps( _mm_and_ps( _mm_rsqrt_ps( _a.V ), XYZ_MASK ), _mm_and_ps( _a.V, W_MASK ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void RSqrRoot( RedVector4& _a )
		{
			_a.V = _mm_rsqrt_ps( _a.V );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 RSqrRoot( const RedVector4& _a )
		{
			return RedVector4( _mm_rsqrt_ps( _a.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void RSqrRoot3( RedVector4& _a )
		{
			SIMDVector a = _mm_add_ps( _mm_and_ps( _a.V, XYZ_MASK ), _mm_setr_ps( 0.0f, 0.0f, 0.0f, 1.0f ) );
			SIMDVector b = _mm_rsqrt_ps( a );
			SIMDVector c = _mm_and_ps( _a.V, W_MASK );
			_a.V = _mm_add_ps( _mm_and_ps( b, XYZ_MASK ), c );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 RSqrRoot3( const RedVector4& _a )
		{
			SIMDVector a = _mm_add_ps( _mm_and_ps( _a.V, XYZ_MASK ), _mm_setr_ps( 0.0f, 0.0f, 0.0f, 1.0f ) );
			SIMDVector b = _mm_rsqrt_ps( a );
			SIMDVector c = _mm_and_ps( _a.V, W_MASK );
			return RedVector4( _mm_add_ps( _mm_and_ps( b, XYZ_MASK ), c ) );
		}

		

		//////////////////////////////////////////////////////////////////////////
		// 'NORMALIZED DOT PRODUCT' Functions
		// Ensure the input vectors are of unit length.
		//////////////////////////////////////////////////////////////////////////
		void UnitDot( RedScalar& _a, const RedVector2& _b, const RedVector2& _c )
		{
			RedScalar dotProduct = Dot( _b, _c );
			RedScalar lengthSqr( Mul( _b.SquareLength(), _c.SquareLength() ) );
			SqrRoot( lengthSqr );
			if( lengthSqr.IsZero() )
			{
				_a.SetZeros();
				return;
			}
			dotProduct = _mm_div_ss( dotProduct.V, lengthSqr.V );
			_a.V = _mm_shuffle_ps( dotProduct.V, dotProduct.V, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar UnitDot( const RedVector2& _a, const RedVector2& _b )
		{
			RedScalar dotProduct = Dot( _a, _b );
			RedScalar lengthSqr( Mul( _a.SquareLength(), _b.SquareLength() ) );
			SqrRoot( lengthSqr );
			if( lengthSqr.IsZero() )
			{
				return 0.0f;
			}
			dotProduct = _mm_div_ss( dotProduct.V, lengthSqr.V );
			return _mm_shuffle_ps( dotProduct.V, dotProduct.V, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void UnitDot( RedScalar& _a, const RedVector3& _b, const RedVector3& _c )
		{
			RedScalar dotProduct = Dot( _b, _c );
			RedScalar lengthSqr( Mul( _b.SquareLength(), _c.SquareLength() ) );
			SqrRoot( lengthSqr );
			if( lengthSqr.IsZero() )
			{
				_a.SetZeros();
				return;
			}
			dotProduct = _mm_div_ss( dotProduct.V, lengthSqr.V );
			_a.V = _mm_shuffle_ps( dotProduct.V, dotProduct.V, _MM_SHUFFLE( 0, 0, 0, 0 ) );		
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar UnitDot( const RedVector3& _a, const RedVector3& _b )
		{
			RedScalar dotProduct = Dot( _a, _b );
			RedScalar lengthSqr( Mul( _a.SquareLength(), _b.SquareLength() ) );
			SqrRoot( lengthSqr );
			if( lengthSqr.IsZero() )
			{
				return 0.0f;
			}
			dotProduct = _mm_div_ss( dotProduct.V, lengthSqr.V );
			return _mm_shuffle_ps( dotProduct.V, dotProduct.V, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void UnitDot( RedScalar& _a, const RedVector4& _b, const RedVector3& _c )
		{
			RedScalar dotProduct = Dot( _b, _c );
			RedScalar lengthSqr( Mul( _b.SquareLength3(), _c.SquareLength() ) );
			SqrRoot( lengthSqr );
			if( lengthSqr.IsZero() )
			{
				_a.SetZeros();
				return;
			}
			dotProduct = _mm_div_ss( dotProduct.V, lengthSqr.V );
			_a.V = _mm_shuffle_ps( dotProduct.V, dotProduct.V, _MM_SHUFFLE( 0, 0, 0, 0 ) );		
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar UnitDot( const RedVector4& _a, const RedVector3& _b )
		{
			RedScalar dotProduct = Dot( _a, _b );
			RedScalar lengthSqr( Mul( _a.SquareLength3(), _b.SquareLength() ) );
			SqrRoot( lengthSqr );
			if( lengthSqr.IsZero() )
			{
				return 0.0f;
			}
			dotProduct = _mm_div_ss( dotProduct.V, lengthSqr.V );
			return _mm_shuffle_ps( dotProduct.V, dotProduct.V, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void UnitDot( RedScalar& _a, const RedVector4& _b, const RedVector4& _c )
		{
			RedScalar dotProduct = Dot( _b, _c );
			RedScalar lengthSqr( Mul( _b.SquareLength4(), _c.SquareLength4() ) );
			SqrRoot( lengthSqr );
			if( lengthSqr.IsZero() )
			{
				_a.SetZeros();
				return;
			}
			dotProduct = _mm_div_ss( dotProduct.V, lengthSqr.V );
			_a.V = _mm_shuffle_ps( dotProduct.V, dotProduct.V, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar UnitDot( const RedVector4& _a, const RedVector4& _b )
		{
			RedScalar dotProduct = Dot( _a, _b );
			RedScalar lengthSqr( Mul( _a.SquareLength4(), _b.SquareLength4() ) );
			SqrRoot( lengthSqr );
			if( lengthSqr.IsZero() )
			{
				return 0.0f;
			}
			dotProduct = _mm_div_ss( dotProduct.V, lengthSqr.V );
			return _mm_shuffle_ps( dotProduct.V, dotProduct.V, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void UnitDot3( RedScalar& _a, const RedVector4& _b, const RedVector4& _c )
		{
			RedScalar dotProduct = Dot3( _b, _c );
			RedScalar lengthSqr( Mul( _b.SquareLength3(), _c.SquareLength3() ) );
			SqrRoot( lengthSqr );
			if( lengthSqr.IsZero() )
			{
				_a.SetZeros();
				return;
			}
			dotProduct = _mm_div_ss( dotProduct.V, lengthSqr.V );
			_a.V = _mm_shuffle_ps( dotProduct.V, dotProduct.V, _MM_SHUFFLE( 0, 0, 0, 0 ) );	
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar UnitDot3( const RedVector4& _a, const RedVector4& _b )
		{
			RedScalar dotProduct = Dot3( _a, _b );
			RedScalar lengthSqr( Mul( _a.SquareLength3(), _b.SquareLength3() ) );
			SqrRoot( lengthSqr );
			if( lengthSqr.IsZero() )
			{
				return 0.0f;
			}
			dotProduct = _mm_div_ss( dotProduct.V, lengthSqr.V );
			return _mm_shuffle_ps( dotProduct.V, dotProduct.V, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		}

		//////////////////////////////////////////////////////////////////////////
		// 'CROSS PRODUCT' Functions
		//////////////////////////////////////////////////////////////////////////
		void Cross( RedScalar& _a, const RedVector2& _b, const RedVector2& _c )
		{
			SIMDVector vA = _mm_shuffle_ps( _c.V, _c.V, _MM_SHUFFLE( 0, 1, 0, 1 ) );
			SIMDVector vB = _mm_mul_ps( _b.V, vA );
			_a.V = _mm_sub_ps( _mm_shuffle_ps( vB, vB, _MM_SHUFFLE( 0, 0, 0, 0 ) ), _mm_shuffle_ps( vB, vB, _MM_SHUFFLE( 1, 1, 1, 1 ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar Cross( const RedVector2& _a, const RedVector2& _b )
		{
			SIMDVector vA = _mm_shuffle_ps( _b.V, _b.V, _MM_SHUFFLE( 0, 1, 0, 1 ) );
			SIMDVector vB = _mm_mul_ps( _a.V, vA );
			return _mm_sub_ps( _mm_shuffle_ps( vB, vB, _MM_SHUFFLE( 0, 0, 0, 0 ) ), _mm_shuffle_ps( vB, vB, _MM_SHUFFLE( 1, 1, 1, 1 ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void Cross( RedVector3& _a, const RedVector3& _b, const RedVector3& _c )
		{
			_a.V = _mm_sub_ps( 
				_mm_mul_ps(_b.V, _mm_shuffle_ps(_c.V, _c.V, _MM_SHUFFLE(3, 0, 2, 1))), 
				_mm_mul_ps(_c.V, _mm_shuffle_ps(_b.V, _b.V, _MM_SHUFFLE(3, 0, 2, 1)))
				);
			_a.V = _mm_shuffle_ps(_a.V, _a.V, _MM_SHUFFLE(3, 0, 2, 1 ));
			_a.W = 1.0f;
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 Cross( const RedVector3& _a, const RedVector3& _b )
		{
			RedVector3 c;
			c.V = _mm_sub_ps(
				_mm_mul_ps(_a.V, _mm_shuffle_ps(_b.V, _b.V, _MM_SHUFFLE(3, 0, 2, 1))), 
				_mm_mul_ps(_b.V, _mm_shuffle_ps(_a.V, _a.V, _MM_SHUFFLE(3, 0, 2, 1)))
				);
			c.V = _mm_shuffle_ps(c.V, c.V, _MM_SHUFFLE(3, 0, 2, 1 ));
			c.W = 1.0f;

			return c;
		}

		//////////////////////////////////////////////////////////////////////////
		void Cross( RedVector4& _a, const RedVector4& _b, const RedVector3& _c )
		{
			_a.V = _mm_sub_ps(
				_mm_mul_ps(_b.V, _mm_shuffle_ps(_c.V, _c.V, _MM_SHUFFLE(3, 0, 2, 1))),
				_mm_mul_ps(_c.V, _mm_shuffle_ps(_b.V, _b.V, _MM_SHUFFLE(3, 0, 2, 1)))
				);
			_a.V = _mm_shuffle_ps(_a.V, _a.V, _MM_SHUFFLE(3, 0, 2, 1 ));
			_a.W = 1.0f;
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Cross( const RedVector4& _a, const RedVector3& _b )
		{
			RedVector4 c;
			c.V = _mm_sub_ps(
				_mm_mul_ps(_a.V, _mm_shuffle_ps(_b.V, _b.V, _MM_SHUFFLE(3, 0, 2, 1))),
				_mm_mul_ps(_b.V, _mm_shuffle_ps(_a.V, _a.V, _MM_SHUFFLE(3, 0, 2, 1)))
				);
			c.V = _mm_shuffle_ps(c.V, c.V, _MM_SHUFFLE(3, 0, 2, 1 ));
			c.W = 1.0f;

			return c;
		}

		//////////////////////////////////////////////////////////////////////////
		void Cross( RedVector4& _a, const RedVector4& _b, const RedVector4& _c )
		{
			_a.V = _mm_sub_ps(
				_mm_mul_ps(_b.V, _mm_shuffle_ps(_c.V, _c.V, _MM_SHUFFLE(3, 0, 2, 1))),
				_mm_mul_ps(_c.V, _mm_shuffle_ps(_b.V, _b.V, _MM_SHUFFLE(3, 0, 2, 1)))
				);
			_a.V = _mm_shuffle_ps(_a.V, _a.V, _MM_SHUFFLE(3, 0, 2, 1 ));
			_a.W = 1.0f;
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Cross( const RedVector4& _a, const RedVector4& _b )
		{
			RedVector4 c;
			c.V = _mm_sub_ps(
				_mm_mul_ps(_a.V, _mm_shuffle_ps(_b.V, _b.V, _MM_SHUFFLE(3, 0, 2, 1))),
				_mm_mul_ps(_b.V, _mm_shuffle_ps(_a.V, _a.V, _MM_SHUFFLE(3, 0, 2, 1)))
				);
			c.V = _mm_shuffle_ps(c.V, c.V, _MM_SHUFFLE(3, 0, 2, 1 ));
			c.W = 1.0f;

			return c;
		}
		
		//////////////////////////////////////////////////////////////////////////
		// 'DOT PRODUCT' Functions
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void Dot( RedScalar& _a, const RedVector2& _b, const RedVector2& _c )
		{
			__m128 r1 = _mm_and_ps( _mm_mul_ps(_b.V, _c.V), XY_MASK );
			__m128 r2 = _mm_hadd_ps(r1, r1);
			__m128 r3 = _mm_hadd_ps(r2, r2);
			_a.V = r3;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedScalar Dot( const RedVector2& _a, const RedVector2& _b )
		{
			__m128 r1 = _mm_and_ps( _mm_mul_ps(_a.V, _b.V), XY_MASK );
			__m128 r2 = _mm_hadd_ps(r1, r1);
			__m128 r3 = _mm_hadd_ps(r2, r2);
			return r3;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void Dot( RedScalar& _a, const RedVector3& _b, const RedVector3& _c )
		{
			__m128 r1 = _mm_and_ps( _mm_mul_ps(_b.V, _c.V), XYZ_MASK );
			__m128 r2 = _mm_hadd_ps(r1, r1);
			__m128 r3 = _mm_hadd_ps(r2, r2);
			_a.V = r3;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedScalar Dot( const RedVector3& _a, const RedVector3& _b )
		{
			__m128 r1 = _mm_and_ps( _mm_mul_ps(_a.V, _b.V), XYZ_MASK );
			__m128 r2 = _mm_hadd_ps(r1, r1);
			__m128 r3 = _mm_hadd_ps(r2, r2);
			return r3;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void Dot( RedScalar& _a, const RedVector4& _b, const RedVector3& _c )
		{
			__m128 r1 = _mm_and_ps( _mm_mul_ps(_b.V, _c.V), XYZ_MASK );
			__m128 r2 = _mm_hadd_ps(r1, r1);
			__m128 r3 = _mm_hadd_ps(r2, r2);
			_a.V = r3;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedScalar Dot( const RedVector4& _a, const RedVector3& _b )
		{
			__m128 r1 = _mm_and_ps( _mm_mul_ps(_a.V, _b.V), XYZ_MASK );
			__m128 r2 = _mm_hadd_ps(r1, r1);
			__m128 r3 = _mm_hadd_ps(r2, r2);
			return r3;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void Dot( RedScalar& _a, const RedVector4& _b, const RedVector4& _c )
		{
			__m128 r1 = _mm_mul_ps(_b.V, _c.V);
			__m128 r2 = _mm_hadd_ps(r1, r1);
			__m128 r3 = _mm_hadd_ps(r2, r2);
			_a.V = r3;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedScalar Dot( const RedVector4& _a, const RedVector4& _b )
		{
			__m128 r1 = _mm_mul_ps(_a.V, _b.V);
			__m128 r2 = _mm_hadd_ps(r1, r1);
			__m128 r3 = _mm_hadd_ps(r2, r2);
			return r3;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void Dot3( RedScalar& _a, const RedVector4& _b, const RedVector4& _c )
		{
			__m128 r1 = _mm_and_ps( _mm_mul_ps(_b.V, _c.V), XYZ_MASK );
			__m128 r2 = _mm_hadd_ps(r1, r1);
			__m128 r3 = _mm_hadd_ps(r2, r2);
			_a.V = r3;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedScalar Dot3( const RedVector4& _a, const RedVector4& _b )
		{
			__m128 r1 = _mm_and_ps( _mm_mul_ps(_a.V, _b.V), XYZ_MASK );
			__m128 r2 = _mm_hadd_ps(r1, r1);
			__m128 r3 = _mm_hadd_ps(r2, r2);
			return r3;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void CalculatePerpendicularVector( const RedVector4& _in, RedVector4& _out )
		{
			int min = 0;
			int eleA = 1;
			int eleB = 2;

			float vA = Red::Math::MAbs( _in.X );
			float vB = Red::Math::MAbs( _in.Y );
			float vC = Red::Math::MAbs( _in.Z );

			if( vB < vA )
			{
				eleA = 0;
				min = 1;
				vA = vB;
			}

			if( vC < vA )
			{
				eleB = min;
				min = 2;
			}

			_out.SetZeros();
			_out.f[eleA] = _in.f[eleB];
			_out.f[eleB] = _in.f[eleA];
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void AxisRotateVector( RedVector4& vec, const RedVector4& normAxis, float angle )
		{
			RedVector4 redAxis;
			redAxis.Set( normAxis.X, normAxis.Y, normAxis.Z, 0.0f );
			RedQuaternion rotQuat;
			rotQuat.SetAxisAngle( redAxis, angle );

			RedVector4 redVec;
			RedVector4 redRotated;

			redVec.Set( vec.X, vec.Y, vec.Z, 0.0f );
			redRotated.RotateDirection( rotQuat, redVec );
			vec = redRotated;
		}

	}
}