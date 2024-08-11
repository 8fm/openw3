namespace RedMath
{
	namespace SIMD
	{
		//////////////////////////////////////////////////////////////////////////
		// '+' Addition Functions.
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		void Add( RedScalar& _a, const RedScalar& _b, const RedScalar& _c )
		{
			_a.V = _mm_add_ps( _b.V, _c.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void Add( RedVector2& _a, const RedVector2& _b, const RedScalar& _c )
		{
			_a.V = _mm_add_ps( _b.V, _c.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void Add( RedVector3& _a, const RedVector3& _b, const RedScalar& _c )
		{
			_a.V = _mm_add_ps( _b.V, _mm_and_ps( _c.V, XYZ_MASK ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void Add( RedVector4& _a, const RedVector4& _b, const RedScalar& _c )
		{
			_a.V = _mm_add_ps( _b.V, _c.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void Add( RedVector2& _a, const RedVector2& _b, const RedVector2& _c )
		{
			_a.V = _mm_add_ps( _b.V, _c.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void Add( RedVector3& _a, const RedVector3& _b, const RedVector3& _c )
		{
			_a.V = _mm_add_ps( _b.V, _mm_and_ps( _c.V, XYZ_MASK ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void Add( RedVector4& _a, const RedVector4& _b, const RedVector3& _c )
		{
			_a.V = _mm_add_ps( _b.V, _mm_and_ps( _c.V, XYZ_MASK ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void Add( RedVector4& _a, const RedVector4& _b, const RedVector4& _c )
		{
			_a.V = _mm_add_ps( _b.V, _c.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void Add3( RedVector4& _a, const RedVector4& _b, const RedVector4& _c )
		{
			_a.V = _mm_add_ps( _b.V, _mm_and_ps( _c.V, XYZ_MASK ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar Add( const RedScalar& _a, const RedScalar& _b )
		{
			return _mm_add_ps( _a.V, _b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2 Add( const RedVector2& _a, const RedScalar& _b )
		{
			return RedVector2( _mm_add_ps( _a.V, _b.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 Add( const RedVector3& _a, const RedScalar& _b )
		{
			return RedVector3( _mm_add_ps( _a.V, _mm_and_ps( _b.V, XYZ_MASK ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Add( const RedVector4& _a, const RedScalar& _b )
		{
			return RedVector4( _mm_add_ps( _a.V, _b.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2 Add( const RedVector2& _a, const RedVector2& _b )
		{
			return RedVector2( _mm_add_ps( _a.V, _b.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 Add( const RedVector3& _a, const RedVector3& _b )
		{
			return RedVector3( _mm_add_ps( _a.V, _mm_and_ps( _b.V, XYZ_MASK ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Add( const RedVector4& _a, const RedVector3& _b )
		{
			return RedVector4( _mm_add_ps( _a.V, _mm_and_ps( _b.V, XYZ_MASK ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Add( const RedVector4& _a, const RedVector4& _b )
		{
			return RedVector4( _mm_add_ps( _a.V, _b.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Add3( const RedVector4& _a, const RedVector4& _b )
		{
			return RedVector4( _mm_add_ps( _a.V, _mm_and_ps( _b.V, XYZ_MASK ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetAdd( RedScalar& _a, const RedScalar&_b )
		{
			_a.V = _mm_add_ps( _a.V, _b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetAdd( RedVector2& _a, const RedScalar&_b )
		{
			_a.V = _mm_add_ps( _a.V, _b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetAdd( RedVector3& _a, const RedScalar&_b )
		{
			_a.V = _mm_add_ps( _a.V, _mm_and_ps( _b.V, XYZ_MASK ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetAdd( RedVector4& _a, const RedScalar&_b )
		{
			_a.V = _mm_add_ps( _a.V, _b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetAdd( RedVector2& _a, const RedVector2&_b )
		{
			_a.V = _mm_add_ps( _a.V, _b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetAdd( RedVector3& _a, const RedVector3& _b )
		{
			_a.V = _mm_add_ps( _a.V, _mm_and_ps( _b.V, XYZ_MASK ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetAdd( RedVector4& _a, const RedVector3& _b )
		{
			_a.V = _mm_add_ps( _a.V, _mm_and_ps( _b.V, XYZ_MASK ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetAdd( RedVector4& _a, const RedVector4& _b )
		{
			_a.V = _mm_add_ps( _a.V, _b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetAdd3( RedVector4& _a, const RedVector4& _b )
		{
			_a.V = _mm_add_ps( _mm_and_ps( _a.V, XYZ_MASK ), _mm_and_ps( _b.V, XYZ_MASK ) );
		}

		//////////////////////////////////////////////////////////////////////////
		// '-' Subtraction Functions
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		void Sub( RedScalar& _a, const RedScalar& _b, const RedScalar& _c )
		{
			_a.V = _mm_sub_ps( _b.V, _c.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void Sub( RedVector2& _a, const RedVector2& _b, const RedScalar& _c )
		{
			_a.V = _mm_sub_ps( _b.V, _c.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void Sub( RedVector3& _a, const RedVector3& _b, const RedScalar& _c )
		{
			_a.V = _mm_sub_ps( _b.V, _mm_and_ps( _c.V, XYZ_MASK ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void Sub( RedVector4& _a, const RedVector4& _b, const RedScalar& _c )
		{
			_a.V = _mm_sub_ps( _b.V, _c.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void Sub( RedVector2& _a, const RedVector2& _b, const RedVector2& _c )
		{
			_a.V = _mm_sub_ps( _b.V, _c.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void Sub( RedVector3& _a, const RedVector3& _b, const RedVector3& _c )
		{
			_a.V = _mm_sub_ps( _b.V, _mm_and_ps( _c.V, XYZ_MASK ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void Sub( RedVector4& _a, const RedVector4& _b, const RedVector3& _c )
		{
			_a.V = _mm_sub_ps( _b.V, _mm_and_ps( _c.V, XYZ_MASK ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void Sub( RedVector4& _a, const RedVector4& _b, const RedVector4& _c )
		{
			_a.V = _mm_sub_ps( _b.V, _c.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void Sub3( RedVector4& _a, const RedVector4& _b, const RedVector4& _c )
		{
			_a.V = _mm_sub_ps( _b.V, _mm_and_ps( _c.V, XYZ_MASK ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar Sub( const RedScalar& _a, const RedScalar& _b )
		{
			return _mm_sub_ps( _a.V, _b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2 Sub( const RedVector2& _a, const RedScalar& _b )
		{
			return RedVector2( _mm_sub_ps( _a.V, _b.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 Sub( const RedVector3& _a, const RedScalar& _b )
		{
			return RedVector3( _mm_sub_ps( _a.V, _b.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Sub( const RedVector4& _a, const RedScalar& _b )
		{
			return RedVector4( _mm_sub_ps( _a.V, _b.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2 Sub( const RedVector2& _a, const RedVector2& _b )
		{
			return RedVector2( _mm_sub_ps( _a.V, _b.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 Sub( const RedVector3& _a, const RedVector3& _b )
		{
			return RedVector3( _mm_sub_ps( _a.V, _mm_and_ps( _b.V, XYZ_MASK ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Sub( const RedVector4& _a, const RedVector3& _b )
		{
			return RedVector4( _mm_sub_ps( _a.V, _mm_and_ps( _b.V, XYZ_MASK ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Sub( const RedVector4& _a, const RedVector4& _b )
		{
			return RedVector4( _mm_sub_ps( _a.V, _b.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Sub3( const RedVector4& _a, const RedVector4& _b )
		{
			return RedVector4( _mm_sub_ps( _a.V, _mm_and_ps( _b.V, XYZ_MASK ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetSub( RedScalar& _a, const RedScalar& _b )
		{
			_a.V = _mm_sub_ps( _a.V, _b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetSub( RedVector2& _a, const RedScalar& _b )
		{
			_a.V = _mm_sub_ps( _a.V, _b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetSub( RedVector3& _a, const RedScalar& _b )
		{
			_a.V = _mm_sub_ps( _a.V, _mm_and_ps( _b.V, XYZ_MASK ) );
		}

		void SetSub( RedVector4& _a, const RedScalar& _b )
		{
			_a.V = _mm_sub_ps( _a.V, _b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetSub( RedVector2& _a, const RedVector2& _b )
		{
			_a.V = _mm_sub_ps( _a.V, _b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetSub( RedVector3& _a, const RedVector3& _b )
		{
			_a.V = _mm_sub_ps( _a.V, _mm_and_ps( _b.V, XYZ_MASK ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetSub( RedVector4& _a, const RedVector3& _b )
		{
			_a.V = _mm_sub_ps( _a.V, _mm_and_ps( _b.V, XYZ_MASK ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetSub( RedVector4& _a, const RedVector4& _b )
		{
			_a.V = _mm_sub_ps( _a.V, _b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetSub3( RedVector4& _a, const RedVector4& _b )
		{
			_a.V = _mm_sub_ps( _a.V, _mm_and_ps( _b.V, XYZ_MASK ) );
		}

		//////////////////////////////////////////////////////////////////////////
		// '*' Multiplication Functions.
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		void Mul( RedScalar& _a, const RedScalar& _b, const RedScalar& _c )
		{
			_a.V = _mm_mul_ps( _b.V, _c.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void Mul( RedVector2& _a, const RedVector2& _b, const RedScalar& _c )
		{
			_a.V = _mm_mul_ps( _b.V, _c.V );
		}

		//////////////////////////////////////////////////////////////////////////
		extern void Mul( RedVector3& _a, const RedVector3& _b, const RedScalar& _c )
		{
			_a.V = _mm_mul_ps( _b.V, _mm_add_ps( _mm_and_ps( _c.V, XYZ_MASK ), _mm_setr_ps( 0.0f, 0.0f, 0.0f, 1.0f ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		extern void Mul( RedVector4& _a, const RedVector4& _b, const RedScalar& _c )
		{
			_a.V = _mm_mul_ps( _b.V, _c.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void Mul( RedVector2& _a, const RedVector2& _b, const RedVector2& _c )
		{
			_a.V = _mm_mul_ps( _b.V, _c.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void Mul( RedVector3& _a, const RedVector3& _b, const RedVector3& _c )
		{
			_a.V = _mm_mul_ps( _b.V, _c.V );
		}
		//////////////////////////////////////////////////////////////////////////
		void Mul( RedVector4& _a, const RedVector4& _b, const RedVector3& _c )
		{
			_a.V = _mm_mul_ps( _b.V, _c.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void Mul( RedVector4& _a, const RedVector4& _b, const RedVector4& _c )
		{
			_a.V = _mm_mul_ps( _b.V, _c.V );
		}

		void Mul3( RedVector4& _a, const RedVector4& _b, const RedVector4& _c )
		{
			_a.V = _mm_mul_ps( _b.V, _mm_add_ps( _mm_and_ps( _c.V, XYZ_MASK ), _mm_setr_ps( 0.0f, 0.0f, 0.0f, 1.0f ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar Mul( const RedScalar& _a, const RedScalar& _b )
		{
			return _mm_mul_ps( _a.V, _b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2 Mul( const RedVector2& _a, const RedScalar& _b )
		{
			return RedVector2( _mm_mul_ps( _a.V, _b.V ) );
		}	

		//////////////////////////////////////////////////////////////////////////
		RedVector3 Mul( const RedVector3& _a, const RedScalar& _b )
		{
			return RedVector3( _mm_mul_ps( _a.V, _b.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Mul( const RedVector4& _a, const RedScalar& _b )
		{
			return RedVector4( _mm_mul_ps( _a.V, _b.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2 Mul( const RedVector2& _a, const RedVector2& _b )
		{
			return RedVector2( _mm_mul_ps( _a.V, _b.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 Mul( const RedVector3& _a, const RedVector3& _b )
		{
			return RedVector3( _mm_mul_ps( _a.V, _b.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Mul( const RedVector4& _a, const RedVector3& _b )
		{
			return RedVector4( _mm_mul_ps( _a.V, _b.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Mul( const RedVector4& _a, const RedVector4& _b )
		{
			return RedVector4( _mm_mul_ps( _a.V, _b.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Mul3( const RedVector4& _a, const RedVector4& _b )
		{
			return RedVector4( _mm_mul_ps( _a.V, _mm_add_ps( _mm_and_ps( _b.V, XYZ_MASK ), _mm_setr_ps( 0.0f, 0.0f, 0.0f, 1.0f  ) ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetMul( RedScalar& _a, const RedScalar& _b )
		{
			_a.V = _mm_mul_ps( _a.V, _b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetMul( RedVector2& _a, const RedScalar& _b )
		{
			_a.V = _mm_mul_ps( _a.V, _b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetMul( RedVector3& _a, const RedScalar& _b )
		{
			_a.V = _mm_mul_ps( _a.V, _mm_and_ps( _b.V, XYZ_MASK ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetMul( RedVector4& _a, const RedScalar& _b )
		{
			_a.V = _mm_mul_ps( _a.V, _b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetMul( RedVector2& _a, const RedVector2& _b )
		{
			_a.V = _mm_mul_ps( _a.V, _b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetMul( RedVector3& _a, const RedVector3& _b )
		{
			_a.V = _mm_mul_ps( _a.V, _b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetMul( RedVector4& _a, const RedVector3& _b )
		{
			_a.V = _mm_add_ps( _mm_mul_ps( _mm_and_ps( _a.V, XYZ_MASK ), _b.V ), _mm_and_ps( _a.V, W_MASK ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetMul( RedVector4& _a, const RedVector4& _b )
		{
			_a.V = _mm_mul_ps( _a.V, _b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetMul3( RedVector4& _a, const RedVector4& _b )
		{
			_a.V = _mm_mul_ps( _a.V, _mm_add_ps( _mm_and_ps( _b.V, XYZ_MASK ), _mm_setr_ps( 0.0f, 0.0f, 0.0f, 1.0f ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		// '/' Divisor Function
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		void Div( RedScalar& _a, const RedScalar& _b, const RedScalar& _c )
		{
			_a.V = _mm_div_ps( _b.V, _c.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void Div( RedVector2& _a, const RedVector2& _b, const RedScalar& _c )
		{
			_a.V = _mm_div_ps( _b.V, _c.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void Div( RedVector3& _a, const RedVector3& _b, const RedScalar& _c )
		{
			_a.V = _mm_div_ps( _b.V, _mm_add_ps( _mm_and_ps( _c.V, XYZ_MASK ), _mm_setr_ps( 0.0f, 0.0f, 0.0f, 1.0f ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void Div( RedVector4& _a, const RedVector4& _b, const RedScalar& _c )
		{
			_a.V = _mm_div_ps( _b.V, _c.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void Div( RedVector2& _a, const RedVector2& _b, const RedVector2& _c )
		{
			_a.V = _mm_div_ps( _b.V, _c.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void Div( RedVector3& _a, const RedVector3& _b, const RedVector3& _c )
		{
			_a.V = _mm_div_ps( _b.V, _c.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void Div( RedVector4& _a, const RedVector4& _b, const RedVector3& _c )
		{
			_a.V = _mm_div_ps( _b.V, _c.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void Div( RedVector4& _a, const RedVector4& _b, const RedVector4& _c )
		{
			_a.V = _mm_div_ps( _b.V, _c.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void Div3( RedVector4& _a, const RedVector4& _b, const RedVector4& _c )
		{
			_a.V = _mm_div_ps( _b.V, _mm_add_ps( _mm_and_ps( _c.V, XYZ_MASK ), _mm_setr_ps( 0.0f, 0.0f, 0.0f, 1.0f ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar Div( const RedScalar& _a, const RedScalar& _b )
		{
			return _mm_div_ps( _a.V, _b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2 Div( const RedVector2& _a, const RedScalar& _b )
		{
			return RedVector2( _mm_div_ps( _a.V, _b.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 Div( const RedVector3& _a, const RedScalar& _b )
		{
			return RedVector3( _mm_div_ps( _a.V, _b.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Div( const RedVector4& _a, const RedScalar& _b )
		{
			return RedVector4( _mm_div_ps( _a.V, _b.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2 Div( const RedVector2& _a, const RedVector2& _b )
		{
			return RedVector2( _mm_div_ps( _a.V, _b.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 Div( const RedVector3& _a, const RedVector3& _b )
		{
			return RedVector3( _mm_div_ps( _a.V, _b.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Div( const RedVector4& _a, const RedVector3& _b )
		{
			return RedVector4( _mm_div_ps( _a.V, _b.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Div( const RedVector4& _a, const RedVector4& _b )
		{
			return RedVector4( _mm_div_ps( _a.V, _b.V ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Div3( const RedVector4& _a, const RedVector4& _b )
		{
			return RedVector4( _mm_div_ps( _a.V, _mm_add_ps( _mm_and_ps( _b.V, XYZ_MASK ), _mm_setr_ps( 0.0f, 0.0f, 0.0f, 1.0f ) ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetDiv( RedScalar& _a, const RedScalar& _b )
		{
			_a.V = _mm_div_ps( _a.V, _b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetDiv( RedVector2& _a, const RedScalar& _b )
		{
			_a.V = _mm_div_ps( _a.V, _b.V );
		}


		//////////////////////////////////////////////////////////////////////////
		void SetDiv( RedVector3& _a, const RedScalar& _b )
		{
			_a.V = _mm_div_ps( _a.V, _mm_add_ps( _mm_and_ps( _b.V, XYZ_MASK ), _mm_setr_ps( 0.0f, 0.0f, 0.0f, 1.0f ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetDiv( RedVector4& _a, const RedScalar& _b )
		{
			_a.V = _mm_div_ps( _a.V, _b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetDiv( RedVector2& _a, const RedVector2& _b )
		{
			_a.V = _mm_div_ps( _a.V, _b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetDiv( RedVector3& _a, const RedVector3& _b )
		{
			_a.V = _mm_div_ps( _a.V, _b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetDiv( RedVector4& _a, const RedVector3& _b )
		{
			_a.V = _mm_div_ps( _a.V,_b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetDiv( RedVector4& _a, const RedVector4& _b )
		{
			_a.V = _mm_div_ps( _a.V, _b.V );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetDiv3( RedVector4& _a, const RedVector4& _b )
		{
			_a.V = _mm_div_ps( _a.V, _mm_add_ps( _mm_and_ps( _b.V, XYZ_MASK ), _mm_setr_ps( 0.0f, 0.0f, 0.0f, 1.0f) ) );
		}

		// TODO:
		// 		RED_INLINE void Min( RedScalar& _a, const RedVector2& _b, const RedVector2& _c );
		// 		RED_INLINE RedScalar Min( const RedVector2& _a, const RedVector2& _b );
		// 		RED_INLINE void Min( RedVector3& _a, const RedVector3& _b, const RedVector3& _c );
		// 		RED_INLINE RedVector3 Min( const RedVector3& _a, const RedVector3& _b );
		// 		RED_INLINE void Min( RedVector4& _a, const RedVector4& _b, const RedVector3& _c );
		// 		RED_INLINE RedVector4 Min( const RedVector4& _a, const RedVector3& _b );
		void Min( RedVector4& _a, const RedVector4& _b, const RedVector4& _c )
		{
			_a.V = _mm_min_ps(_b.V, _c.V);
		}

		RedVector4 Min( const RedVector4& _a, const RedVector4& _b )
		{
			return RedVector4(_mm_min_ps(_a.V, _b.V));
		}

		// TODO:
		// 		RED_INLINE void Max( RedScalar& _a, const RedVector2& _b, const RedVector2& _c );
		// 		RED_INLINE RedScalar Max( const RedVector2& _a, const RedVector2& _b );
		// 		RED_INLINE void Max( RedVector3& _a, const RedVector3& _b, const RedVector3& _c );
		// 		RED_INLINE RedVector3 Max( const RedVector3& _a, const RedVector3& _b );
		// 		RED_INLINE void Max( RedVector4& _a, const RedVector4& _b, const RedVector3& _c );
		// 		RED_INLINE RedVector4 Max( const RedVector4& _a, const RedVector3& _b );
		RED_INLINE void Max( RedVector4& _a, const RedVector4& _b, const RedVector4& _c )
		{
			_a.V = _mm_max_ps(_b.V, _c.V);
		}

		RED_INLINE RedVector4 Max( const RedVector4& _a, const RedVector4& _b )
		{
			return RedVector4(_mm_max_ps(_a.V, _b.V));
		}
	}
}