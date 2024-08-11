
namespace RedMath
{
	namespace SIMD
	{
		//////////////////////////////////////////////////////////////////////////
		RedVector2::RedVector2()
			: V( _mm_setzero_ps() )
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2::RedVector2( const RedScalar& _v )
			: V( _v.V )
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2::RedVector2( const RedVector2& _v )
			: V( _v.V )
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2::RedVector2( Red::System::Float _x, Red::System::Float _y )
			: V( _mm_setr_ps( _x, _y, _x, _y ) )
		{
		
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2::RedVector2( const Red::System::Float* _f )
			: V( _mm_load_ps( _f ) )
		{
			
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2::RedVector2( SIMDVector _v )
			: V( _v )
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2::~RedVector2()
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2& RedVector2::operator = ( const RedScalar& _v )
		{
			V = _v.V;
			return *this;
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2& RedVector2::operator = ( const RedVector2& _v )
		{
			V = _v.V;
			return *this;
		}

		//////////////////////////////////////////////////////////////////////////
		const Red::System::Float* RedVector2::AsFloat() const
		{
			return &X;
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector2::Set( Red::System::Float _x, Red::System::Float _y )
		{
			V = _mm_setr_ps( _x, _y, _x, _y );
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector2::Set( const RedScalar& _v )
		{
			V = _v.V; 
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector2::Set( const RedVector2& _v )
		{
			V = _v.V;
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector2::Set( const Red::System::Float* _f )
		{
			V =  _mm_load_ps( _f );
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector2::SetZeros()
		{
			V = _mm_setzero_ps();
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector2::SetOnes()
		{
			V = _mm_set1_ps( 1.0f );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2& RedVector2::Negate()
		{
			V = _mm_xor_ps( V, SIGN_MASK );
			return *this;
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2 RedVector2::Negated() const
		{
			SIMDVector negVal = _mm_xor_ps( V, SIGN_MASK );
			return RedVector2( negVal );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedScalar RedVector2::Length() const
		{
			SIMDVector vA = _mm_mul_ps( V, V );
			SIMDVector vB = _mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 1, 1, 1, 1 ) ) );
			vB = _mm_sqrt_ss( vB );
			return _mm_shuffle_ps( vB, vB, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedScalar RedVector2::SquareLength() const
		{
			SIMDVector vA = _mm_mul_ps( V, V );
			SIMDVector vB = _mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 1, 1, 1, 1 ) ) );
			return _mm_shuffle_ps( vB, vB, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2& RedVector2::Normalize()
		{
			SIMDVector xVal = _mm_and_ps( V, X_MASK );
			xVal = _mm_mul_ss( xVal, xVal );
			SIMDVector yVal = _mm_shuffle_ps( _mm_and_ps( V, Y_MASK ), _mm_setzero_ps(), _MM_SHUFFLE( 0, 0, 0, 1 ) );
			yVal = _mm_mul_ss( yVal, yVal );

			SIMDVector length = _mm_sqrt_ss( _mm_add_ss( xVal, yVal ) );
			SIMDVector hasLength = _mm_cmpeq_ss( length, _mm_setzero_ps() );
			hasLength = _mm_shuffle_ps( hasLength, hasLength, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			SIMDVector unitLength = _mm_div_ss( _mm_set1_ps( 1.0f ), length );
			unitLength = _mm_shuffle_ps( unitLength, unitLength, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			V = _mm_andnot_ps( hasLength, _mm_mul_ps( V, unitLength) );
			return *this;
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2 RedVector2::Normalized() const
		{
			SIMDVector xVal = _mm_and_ps( V, X_MASK );
			xVal = _mm_mul_ss( xVal, xVal );
			SIMDVector yVal = _mm_shuffle_ps( _mm_and_ps( V, Y_MASK ), _mm_setzero_ps(), _MM_SHUFFLE( 0, 0, 0, 1 ) );
			yVal = _mm_mul_ss( yVal, yVal );

			SIMDVector length = _mm_sqrt_ps( _mm_add_ps( xVal, yVal ) );
			SIMDVector hasLength = _mm_cmpeq_ss( length, _mm_setzero_ps() );
			hasLength = _mm_shuffle_ps( hasLength, hasLength, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			SIMDVector unitLength = _mm_div_ps( _mm_set1_ps( 1.0f ), length );
			unitLength = _mm_shuffle_ps( unitLength, unitLength, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			return RedVector2( _mm_andnot_ps( hasLength, _mm_mul_ps( V, unitLength ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2& RedVector2::NormalizeFast()
		{
			SIMDVector xVal = _mm_and_ps( V, X_MASK );
			xVal = _mm_mul_ss( xVal, xVal );
			SIMDVector yVal = _mm_shuffle_ps( _mm_and_ps( V, Y_MASK ), _mm_setzero_ps(), _MM_SHUFFLE( 0, 0, 0, 1 ) );
			yVal = _mm_mul_ss( yVal, yVal );
			SIMDVector sqr = _mm_add_ps( xVal, yVal );
			SIMDVector hasLength = _mm_cmpeq_ss( sqr, _mm_setzero_ps() );
			SIMDVector length = _mm_rsqrt_ss( sqr );
			hasLength = _mm_shuffle_ps( hasLength, hasLength, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			length = _mm_shuffle_ps( length, length, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			V = _mm_andnot_ps( hasLength, _mm_mul_ps( V, length ) );
			return *this;
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2 RedVector2::NormalizedFast() const
		{
			SIMDVector xVal = _mm_and_ps( V, X_MASK );
			xVal = _mm_mul_ss( xVal, xVal );
			SIMDVector yVal = _mm_shuffle_ps( _mm_and_ps( V, Y_MASK ), _mm_setzero_ps(), _MM_SHUFFLE( 0, 0, 0, 1 ) );
			yVal = _mm_mul_ss( yVal, yVal );
			SIMDVector sqr = _mm_add_ps( xVal, yVal );
			SIMDVector hasLength = _mm_cmpeq_ss( sqr, _mm_setzero_ps() );
			SIMDVector length = _mm_rsqrt_ss( sqr );
			hasLength = _mm_shuffle_ps( hasLength, hasLength, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			length = _mm_shuffle_ps( length, length, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			return _mm_andnot_ps( hasLength, _mm_mul_ps( V, length ) );
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector2::IsAlmostEqual( const RedVector2& _v, const SIMDVector _epsilon  ) const
		{
			SIMDVector vA = _mm_sub_ps( V, _v.V );
			SIMDVector vB = _mm_max_ps( _mm_sub_ps( _mm_setzero_ps(), vA), vA );
			RedVector2 test( _mm_cmple_ps( vB, _epsilon ) );
			return ( test.Xi != 0 && test.Yi != 0 );
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector2::IsZero() const
		{
			RedVector2 test (_mm_cmpeq_ps( _mm_setzero_ps(), V ) );
			return ( test.Xi != 0 && test.Yi != 0 );
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector2::IsAlmostZero( const SIMDVector _epsilon ) const
		{
			SIMDVector absVal = _mm_max_ps(_mm_sub_ps(_mm_setzero_ps(), V), V);
			RedVector2 test( _mm_cmple_ps( absVal, _epsilon ) );
			return ( test.Xi != 0 && test.Yi != 0 );
		}
	}
}