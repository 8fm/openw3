
namespace RedMath
{
	namespace SIMD
	{
		//////////////////////////////////////////////////////////////////////////
		RedVector3::RedVector3()
			: V( _mm_add_ps( _mm_setzero_ps(), _mm_setr_ps( 0.0f, 0.0f, 0.0f, 1.0f )) )
		{
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3::RedVector3( const RedScalar& _v )
			: V( _mm_add_ps( _mm_and_ps( _v.V, XYZ_MASK ), _mm_setr_ps( 0.0f, 0.0f, 0.0f, 1.0f ) ) )
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3::RedVector3( const RedVector3& _v )
			: V( _v.V )
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3::RedVector3( Red::System::Float _x, Red::System::Float _y, Red::System::Float _z )
			: V( _mm_setr_ps( _x, _y, _z, 1.0f ) )
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3::RedVector3( const Red::System::Float* _f )
			: V( _mm_add_ps( _mm_and_ps( _mm_load_ps( _f ), XYZ_MASK ), _mm_setr_ps( 0.0f, 0.0f, 0.0f, 1.0f ) ) )
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3::RedVector3( SIMDVector _v )
			: V( _v )
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3::~RedVector3()
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3& RedVector3::operator = ( const RedScalar& _v )
		{
			V = _mm_add_ps( _mm_and_ps( _v.V, XYZ_MASK ), _mm_setr_ps( 0.0f, 0.0f, 0.0f, 1.0f ) );
			return (*this);
		}

		//////////////////////////////////////////////////////////////////////////
		const Red::System::Float* RedVector3::AsFloat() const
		{
			return &X;
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector3::Store( Red::System::Float* _f ) const
		{
			_mm_store_ps( _f, V );
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector3::Set( Red::System::Float _x, Red::System::Float _y, Red::System::Float _z )
		{
			V = _mm_setr_ps( _x, _y, _z, 1.0f );
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector3::Set( const RedScalar& _v )
		{
			V = _mm_add_ps( _mm_and_ps( _v.V, XYZ_MASK ), _mm_setr_ps( 0.0f, 0.0f, 0.0f, 1.0f ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector3::Set( const RedVector3& _v )
		{
			V = _v.V;
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector3::Set( const Red::System::Float* _f )
		{
			V = _mm_add_ps( _mm_and_ps( _mm_load_ps( _f ), XYZ_MASK ), _mm_setr_ps( 0.0f, 0.0f, 0.0f, 1.0f ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector3::SetZeros()
		{
			V = _mm_add_ps( _mm_setzero_ps(), _mm_setr_ps( 0.0f, 0.0f, 0.0f, 1.0f ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector3::SetOnes()
		{
			V = _mm_set_ps1( 1.0f );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3& RedVector3::Negate()
		{
			V = _mm_xor_ps( V, _mm_and_ps( SIGN_MASK, XYZ_MASK ) );
			return (*this);
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 RedVector3::Negated() const
		{
			SIMDVector negVal = _mm_xor_ps( V, _mm_and_ps( SIGN_MASK, XYZ_MASK ) );
			return RedVector3( negVal );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 RedVector3::Abs() const
		{
			SIMDVector absVal = _mm_max_ps( _mm_sub_ps( _mm_setzero_ps(), V), V);
			return RedVector3( absVal );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector3::Length() const
		{
			SIMDVector vA = _mm_mul_ps( V, V );
			SIMDVector vB = _mm_add_ss( _mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 3, 0, 0, 0 ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 3, 1, 1, 1 ) ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 3, 2, 2, 2 ) ) );
			vB = _mm_sqrt_ss( vB );
			return _mm_shuffle_ps( vB, vB, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector3::SquareLength() const
		{
			SIMDVector vA = _mm_mul_ps( V, V );
			SIMDVector vB = _mm_add_ss( _mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 2, 2, 2, 2 ) ) );
			return _mm_shuffle_ps( vB, vB, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3& RedVector3::Normalize()
		{
			SIMDVector vA = _mm_mul_ps( V, V );
			vA = _mm_add_ss( _mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 2, 2, 2, 2 ) ) );
			SIMDVector length = _mm_sqrt_ss( vA );
			length = _mm_shuffle_ps( length, length, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			SIMDVector hasLength = _mm_cmpeq_ss( length, _mm_setzero_ps() );
			hasLength = _mm_shuffle_ps( hasLength, hasLength, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			SIMDVector unitLength = _mm_div_ps( _mm_set1_ps( 1.0f ), length );
			V = _mm_add_ps( _mm_and_ps( _mm_andnot_ps( hasLength, _mm_mul_ps( V, unitLength) ), XYZ_MASK ), _mm_setr_ps( 0.0f, 0.0f, 0.0f, 1.0f ) );
			return (*this);
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 RedVector3::Normalized() const
		{
			SIMDVector vA = _mm_mul_ps( V, V );
			vA = _mm_add_ss( _mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 2, 2, 2, 2 ) ) );
			SIMDVector length = _mm_sqrt_ss( vA );
			length = _mm_shuffle_ps( length, length, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			SIMDVector hasLength = _mm_cmpeq_ss( length, _mm_setzero_ps() );
			hasLength = _mm_shuffle_ps( hasLength, hasLength, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			SIMDVector unitLength = _mm_div_ps( _mm_set1_ps( 1.0f ), length );
			return RedVector3( _mm_add_ps( _mm_and_ps( _mm_andnot_ps( hasLength, _mm_mul_ps( V, unitLength) ), XYZ_MASK ), _mm_setr_ps( 0.0f, 0.0f, 0.0f, 1.0f ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3& RedVector3::NormalizeFast()
		{
			SIMDVector vA = _mm_mul_ps( V, V );
			vA = _mm_add_ss( _mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 2, 2, 2, 2 ) ) );
			SIMDVector hasLength = _mm_cmpeq_ss( vA, _mm_setzero_ps() );
			vA = _mm_rsqrt_ss( vA );
			hasLength = _mm_shuffle_ps( hasLength, hasLength, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			V = _mm_add_ps( _mm_and_ps( _mm_andnot_ps( hasLength, _mm_mul_ps( V, _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ) ) ), XYZ_MASK ), _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f ) );
			return (*this);
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 RedVector3::NormalizedFast()
		{
			SIMDVector vA = _mm_mul_ps( V, V );
			vA = _mm_add_ss( _mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 2, 2, 2, 2 ) ) );
			SIMDVector hasLength = _mm_cmpeq_ss( vA, _mm_setzero_ps() );
			vA = _mm_rsqrt_ss( vA );
			hasLength = _mm_shuffle_ps( hasLength, hasLength, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			return RedVector3(  _mm_add_ps( _mm_and_ps( _mm_andnot_ps( hasLength, _mm_mul_ps( V, _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ) ) ), XYZ_MASK ), _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector3::IsAlmostEqual( const RedVector3& _v, const SIMDVector _epsilon ) const
		{
			SIMDVector vA = _mm_sub_ps( V, _v.V );
			SIMDVector vB = _mm_max_ps( _mm_sub_ps( _mm_setzero_ps(), vA), vA );
			RedVector3 test( _mm_cmple_ps( vB, _epsilon ) );
			return ( test.Xi != 0 && test.Yi != 0 && test.Zi != 0 );
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector3::IsAlmostEqual( const RedVector3& _v, float _epsilon ) const
		{
			SIMDVector e = _mm_set_ps1(_epsilon);
			return IsAlmostEqual(_v, e);
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector3::IsZero() const
		{
			RedVector3 test( _mm_cmpeq_ps( _mm_setzero_ps(), V ) );
			return ( test.Xi != 0 && test.Yi != 0 && test.Zi != 0 );
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector3::IsAlmostZero( const SIMDVector _epsilon ) const
		{
			SIMDVector absVal = _mm_max_ps(_mm_sub_ps(_mm_setzero_ps(), V), V);
			RedVector3 test( _mm_cmple_ps( absVal, _epsilon ) );
			return ( test.Xi != 0 && test.Yi != 0 && test.Zi != 0 );
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector3::IsOk() const
		{
			return ( ( Xi & __inf[0] ) != __inf[0] && 
				( Yi & __inf[0] ) != __inf[0] &&
				( Zi & __inf[0] ) != __inf[0] &&
				( Wi & __inf[0] ) != __inf[0] );
		}
	}
}