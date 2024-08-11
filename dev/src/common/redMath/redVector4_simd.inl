
namespace RedMath
{
	namespace SIMD
	{
		//////////////////////////////////////////////////////////////////////////
		RedVector4::RedVector4()
			: V( _mm_setzero_ps() )
		{
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4::RedVector4( const RedScalar& _v )
			: V( _v.V)
		{
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4::RedVector4( const RedVector3& _v )
			: V( _v.V )
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4::RedVector4( const RedVector3& _v, Red::System::Float _w )
			: V( _mm_add_ps( _mm_and_ps( _v.V, XYZ_MASK ), _mm_setr_ps( 0.0f, 0.0f, 0.0f, _w ) ) )
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4::RedVector4( const RedVector4& _v )
			: V( _v.V )
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4::RedVector4( const Red::System::Float* _f )
			: V( _mm_load_ps( _f ) )
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4::RedVector4( Red::System::Float _x, Red::System::Float _y, Red::System::Float _z, Red::System::Float _w )
			: V( _mm_setr_ps( _x, _y, _z, _w ) )
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4::RedVector4( SIMDVector _v )
			: V( _v )
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4::~RedVector4()
		{

		}

		//////////////////////////////////////////////////////////////////////////
		const Red::System::Float* RedVector4::AsFloat() const
		{
			return &X;
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4& RedVector4::operator=( const RedScalar& _v )
		{
			V = _v.V;
			return (*this);
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4& RedVector4::operator=( const RedVector3& _v )
		{
			V = _v.V;
			return (*this);
 		}
 

		//////////////////////////////////////////////////////////////////////////		
		void RedVector4::Set( const RedScalar& _v )
		{
			V = _v.V;
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector4::Set( const RedVector3& _v )
		{
			V = _v.V;
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector4::Set( const RedVector3& _v, Red::System::Float _w )
		{
			V = _mm_add_ps( _mm_and_ps( _v.V, XYZ_MASK ), _mm_setr_ps( 0.0f, 0.0f, 0.0f, _w ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector4::Set( const RedVector4& _v )
		{
			V = _v.V;
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector4::Set( Red::System::Float _x, Red::System::Float _y, Red::System::Float _z, Red::System::Float _w  )
		{
			V = _mm_setr_ps( _x, _y, _z, _w );
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector4::Set( const Red::System::Float* _f )
		{
			V = _mm_load_ps( _f );
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector4::SetZeros()
		{
			V = _mm_setzero_ps();
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector4::SetOnes()
		{
			V = _mm_set1_ps( 1.0f );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4& RedVector4::Negate()
		{
			V = _mm_xor_ps( V, SIGN_MASK );
			return (*this);
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 RedVector4::Negated() const
		{
			SIMDVector negVal = _mm_xor_ps( V, SIGN_MASK );
			return RedVector4( negVal );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 RedVector4::Abs() const
		{
			SIMDVector absVal = _mm_max_ps( _mm_sub_ps( _mm_setzero_ps(), V), V);
			return RedVector4( absVal );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::Sum3() const
		{
			return _mm_add_ss( _mm_add_ss( _mm_shuffle_ps( V, V, _MM_SHUFFLE( 3, 0, 0, 0 ) ), _mm_shuffle_ps( V, V, _MM_SHUFFLE( 3, 1, 1, 1 ) ) ), _mm_shuffle_ps( V, V, _MM_SHUFFLE( 3, 2, 2, 2 ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::Sum4() const
		{
			return _mm_add_ss( 
				_mm_add_ss( _mm_shuffle_ps( V, V, _MM_SHUFFLE( 0, 0, 0, 0 ) ), 
				_mm_shuffle_ps( V, V, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), 
				_mm_add_ss( _mm_shuffle_ps( V, V, _MM_SHUFFLE( 2, 2, 2, 2 ) ),
				_mm_shuffle_ps( V, V, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ) 
				);
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::Length3() const
		{
			SIMDVector vA = _mm_mul_ps( V, V );
			SIMDVector vB = _mm_add_ss( _mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 3, 0, 0, 0 ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 3, 1, 1, 1 ) ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 3, 2, 2, 2 ) ) );
			vB = _mm_sqrt_ss( vB );
			return _mm_shuffle_ps( vB, vB, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::Length4() const
		{
			SIMDVector vA = _mm_mul_ps( V, V );
			SIMDVector vB = _mm_add_ss( 
				_mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ), 
				_mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), 
				_mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 2, 2, 2, 2 ) ),
				_mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ) 
				);
			vB = _mm_sqrt_ss( vB );
			return _mm_shuffle_ps( vB, vB, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::SquareLength3() const
		{
			SIMDVector vA = _mm_mul_ps( V, V );
			SIMDVector vB = _mm_add_ss( _mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 2, 2, 2, 2 ) ) );
			return _mm_shuffle_ps( vB, vB, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::SquareLength4() const
		{
			SIMDVector vA = _mm_mul_ps( V, V );
			SIMDVector vB = _mm_add_ss( 
				_mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ), 
				_mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), 
				_mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 2, 2, 2, 2 ) ),
				_mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ) 
				);
			return _mm_shuffle_ps( vB, vB, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		}
		
		//////////////////////////////////////////////////////////////////////////
		RedVector4& RedVector4::Normalize4()
		{
			SIMDVector vA = _mm_mul_ps( V, V );
			vA =  _mm_add_ss( 
				_mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ), 
				_mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), 
				_mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 2, 2, 2, 2 ) ),
				_mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ) 
				);
			SIMDVector length = _mm_sqrt_ss( vA );
			length = _mm_shuffle_ps( length, length, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			SIMDVector hasLength = _mm_cmpeq_ss( length, _mm_setzero_ps() );
			hasLength = _mm_shuffle_ps( hasLength, hasLength, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			SIMDVector unitLength = _mm_div_ps( _mm_set1_ps( 1.0f ), length );
			V = _mm_andnot_ps( hasLength, _mm_mul_ps( V, unitLength) );
			return (*this);
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 RedVector4::Normalized4() const
		{
			SIMDVector vA = _mm_mul_ps( V, V );
			vA =  _mm_add_ss( 
				_mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ), 
				_mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), 
				_mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 2, 2, 2, 2 ) ),
				_mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ) 
				);
			SIMDVector length = _mm_sqrt_ss( vA );
			length = _mm_shuffle_ps( length, length, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			SIMDVector hasLength = _mm_cmpeq_ss( length, _mm_setzero_ps() );
			hasLength = _mm_shuffle_ps( hasLength, hasLength, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			SIMDVector unitLength = _mm_div_ps( _mm_set1_ps( 1.0f ), length );
			return RedVector3( _mm_andnot_ps( hasLength, _mm_mul_ps( V, unitLength) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4& RedVector4::Normalize3()
		{
			SIMDVector vA = _mm_mul_ps( V, V );
			vA = _mm_add_ss( _mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 2, 2, 2, 2 ) ) );
			SIMDVector length = _mm_sqrt_ss( vA );
			length = _mm_shuffle_ps( length, length, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			SIMDVector hasLength = _mm_cmpeq_ss( length, _mm_setzero_ps() );
			hasLength = _mm_shuffle_ps( hasLength, hasLength, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			SIMDVector unitLength = _mm_div_ps( _mm_set1_ps( 1.0f ), length );
			V = _mm_andnot_ps( hasLength, _mm_mul_ps( _mm_and_ps( V, XYZ_MASK ), unitLength) );
			return (*this);
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 RedVector4::Normalized3() const
		{
			SIMDVector vA = _mm_mul_ps( V, V );
			vA = _mm_add_ss( _mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 2, 2, 2, 2 ) ) );
			SIMDVector length = _mm_sqrt_ss( vA );
			length = _mm_shuffle_ps( length, length, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			SIMDVector hasLength = _mm_cmpeq_ss( length, _mm_setzero_ps() );
			hasLength = _mm_shuffle_ps( hasLength, hasLength, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			SIMDVector unitLength = _mm_div_ps( _mm_set1_ps( 1.0f ), length );
			return RedVector3( _mm_andnot_ps( hasLength, _mm_mul_ps( _mm_and_ps( V, XYZ_MASK ), unitLength) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4& RedVector4::NormalizeFast3()
		{
			SIMDVector vA = _mm_mul_ps( V, V );
			vA = _mm_add_ss( _mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 2, 2, 2, 2 ) ) );
			SIMDVector hasLength = _mm_cmpeq_ss( vA, _mm_setzero_ps() );
			vA = _mm_rsqrt_ss( vA );
			hasLength = _mm_shuffle_ps( hasLength, hasLength, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			V = _mm_andnot_ps( hasLength, _mm_mul_ps( _mm_and_ps( V, XYZ_MASK ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ) ) );
			return (*this);
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 RedVector4::NormalizedFast3() const
		{
			SIMDVector vA = _mm_mul_ps( V, V );
			vA = _mm_add_ss( _mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 2, 2, 2, 2 ) ) );
			SIMDVector hasLength = _mm_cmpeq_ss( vA, _mm_setzero_ps() );
			vA = _mm_rsqrt_ss( vA );
			hasLength = _mm_shuffle_ps( hasLength, hasLength, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			return RedVector3( _mm_andnot_ps( hasLength, _mm_mul_ps( _mm_and_ps( V, XYZ_MASK ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ) ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4& RedVector4::NormalizeFast4()
		{
			SIMDVector vA = _mm_mul_ps( V, V );
			vA =  _mm_add_ss( 
				_mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ), 
				_mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), 
				_mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 2, 2, 2, 2 ) ),
				_mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ) 
				);
			SIMDVector hasLength = _mm_cmpeq_ss( vA, _mm_setzero_ps() );
			vA = _mm_rsqrt_ss( vA );
			hasLength = _mm_shuffle_ps( hasLength, hasLength, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			V = _mm_andnot_ps( hasLength, _mm_mul_ps( V, _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ) ) );
			return (*this);
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 RedVector4::NormalizedFast4() const
		{
			SIMDVector vA = _mm_mul_ps( V, V );
			vA =  _mm_add_ss( 
				_mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ), 
				_mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), 
				_mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 2, 2, 2, 2 ) ),
				_mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ) 
				);
			SIMDVector hasLength = _mm_cmpeq_ss( vA, _mm_setzero_ps() );
			vA = _mm_rsqrt_ss( vA );
			hasLength = _mm_shuffle_ps( hasLength, hasLength, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			return RedVector3( _mm_andnot_ps( hasLength, _mm_mul_ps( V, _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ) ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector4::IsNormalized4( const SIMDVector _epsilon  ) const
		{
			SIMDVector vA = _mm_mul_ps( V, V );
			vA = _mm_add_ss( 
				_mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ), 
				_mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), 
				_mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 2, 2, 2, 2 ) ),
				_mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ) 
				);
			vA = _mm_sub_ss( vA, EX.V );
			vA = _mm_max_ss( _mm_sub_ss( _mm_setzero_ps(), vA ), vA);
			vA = _mm_cmple_ss( vA, _epsilon );
			return ( *( (Red::System::Uint32*)&vA ) != 0 );
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector4::IsNormalized3( const SIMDVector _epsilon ) const
		{
			SIMDVector vA = _mm_mul_ps( V, V );
			vA = _mm_add_ss( _mm_add_ss( _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 2, 2, 2, 2 ) ) );
			vA = _mm_sub_ss( vA, EX.V );
			vA = _mm_max_ss( _mm_sub_ss( _mm_setzero_ps(), vA ), vA);
			vA = _mm_cmple_ss( vA, _epsilon );
			return ( *( (Red::System::Uint32*)&vA ) != 0 );
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector4::IsNormalized3( float _epsilon ) const
		{
			SIMDVector e = _mm_set_ps1(_epsilon);
			return IsNormalized3(e);
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector4::IsNormalized4( float _epsilon ) const
		{
			SIMDVector e = _mm_set_ps1(_epsilon);
			return IsNormalized4(e);
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector4::IsAlmostEqual( const RedVector4& _v, float _epsilon ) const
		{
			SIMDVector e = _mm_set_ps1(_epsilon);
			return IsAlmostEqual(_v, e);
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector4::IsAlmostEqual( const RedVector4& _v, const SIMDVector _epsilon ) const
		{
			SIMDVector vA = _mm_sub_ps( V, _v.V );
			SIMDVector vB = _mm_max_ps( _mm_sub_ps( _mm_setzero_ps(), vA), vA );
			RedVector4 test ( _mm_cmple_ps( vB, _epsilon ) );
			return ( test.Xi != 0 && test.Yi != 0 && test.Zi != 0 && test.Wi != 0 );
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector4::IsAlmostZero( const SIMDVector _epsilon ) const
		{
			SIMDVector absVal = _mm_max_ps(_mm_sub_ps(_mm_setzero_ps(), V), V);
			RedVector4 test( _mm_cmple_ps( absVal, _epsilon ) );
			return ( test.Xi != 0 && test.Yi != 0 && test.Zi != 0 && test.Wi != 0 );
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector4::IsZero() const
		{
			RedVector4 test( _mm_cmpeq_ps( _mm_setzero_ps(), V ) );
			return ( test.Xi != 0 && test.Yi != 0 && test.Zi != 0 && test.Wi != 0 );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::Upper3() const
		{
			SIMDVector vA = _mm_max_ss( _mm_shuffle_ps( V, V, _MM_SHUFFLE( 0, 0, 0, 0 ) ), _mm_max_ss( _mm_shuffle_ps( V, V, _MM_SHUFFLE( 1, 1, 1, 1 ) ), _mm_shuffle_ps( V, V, _MM_SHUFFLE( 2, 2, 2, 2 ) ) ) );
			return _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::Upper4() const
		{
			SIMDVector vA = _mm_max_ss( 
				_mm_max_ss( _mm_shuffle_ps( V, V, _MM_SHUFFLE( 0, 0, 0, 0 ) ), _mm_shuffle_ps( V, V, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ),
				_mm_max_ss( _mm_shuffle_ps( V, V, _MM_SHUFFLE( 2, 2, 2, 2 ) ), _mm_shuffle_ps( V, V, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ) 
				);
			return _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::Lower3() const
		{
			SIMDVector vA = _mm_min_ss( _mm_shuffle_ps( V, V, _MM_SHUFFLE( 0, 0, 0, 0 ) ), _mm_min_ss( _mm_shuffle_ps( V, V, _MM_SHUFFLE( 1, 1, 1, 1 ) ), _mm_shuffle_ps( V, V, _MM_SHUFFLE( 2, 2, 2, 2 ) ) ) );
			return _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::Lower4() const
		{
			SIMDVector vA = _mm_min_ss( 
				_mm_min_ss( _mm_shuffle_ps( V, V, _MM_SHUFFLE( 0, 0, 0, 0 ) ), _mm_shuffle_ps( V, V, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ),
				_mm_min_ss( _mm_shuffle_ps( V, V, _MM_SHUFFLE( 2, 2, 2, 2 ) ), _mm_shuffle_ps( V, V, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ) 
				);
			return _mm_shuffle_ps( vA, vA, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 RedVector4::ZeroElement( Red::System::Uint32 _i ) const
		{
			RedVector4 res(*this);
			res.f[_i] = 0.0f;
			return res;
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::AsScalar( Red::System::Uint32 _i ) const
		{
			return f[_i];
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 RedVector4::AsVector3() const
		{
			RedVector3 res(V);
			res.W = 1.0f;
			return res;
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector4::IsOk() const
		{
			return ( ( Xi & __inf[0] ) != __inf[0] && 
					 ( Yi & __inf[0] ) != __inf[0] &&
					 ( Zi & __inf[0] ) != __inf[0] &&
					 ( Wi & __inf[0] ) != __inf[0] );
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector4::Store( Red::System::Float* _f ) const
		{
			_mm_store_ps( _f, V );
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector4::Lerp( const RedVector4& _a, const float _weight )
		{
			Set( Add( Mul( *this, 1.0f - _weight ), Mul( _a, _weight ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 RedVector4::Lerp( const RedVector4& _a, const RedVector4& _b, const float _weight )
		{
			return Add( Mul( _a, 1.0f - _weight ), Mul( _b, _weight ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4& RedVector4::SetTransformedPos( const RedQsTransform& _trans, const RedVector4& _v )
		{
			RedVector4 scaled;
			RedVector4 rotated; 					
			scaled = Mul( _v, _trans.Scale );
			rotated.RotateDirection( _trans.Rotation, scaled );
			Set( Add( rotated, _trans.Translation ) ); 
			
			return *this;
		}

		// TODO: optimize
		//////////////////////////////////////////////////////////////////////////
		void RedVector4::InverseRotateDirection( const RedMatrix3x3& _m, const RedVector4& _v )
		{
			float v0 = _v.X;
			float v1 = _v.Y;
			float v2 = _v.Z;

			X = _m.Row0.X * v0 + _m.Row1.X * v1 + _m.Row2.X * v2;
			Y = _m.Row0.Y * v0 + _m.Row1.Y * v1 + _m.Row2.Y * v2;
			Z = _m.Row0.Z * v0 + _m.Row1.Z * v1 + _m.Row2.Z * v2;
			W = 0;
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector4::InverseRotateDirection( const RedQuaternion& _quat, const RedVector4& _v )
		{
			RedScalar qReal( _quat.Quat.W );
			RedScalar q2minus1( Sub( Mul ( qReal, qReal ),  0.5f ).V );

			RedVector4 ret;
			ret = Mul ( _v, q2minus1 );

			RedScalar imagDotDir( Dot3(_quat.GetImaginary(), _v ) );
			SetAdd( ret, Mul( _quat.GetImaginary(), imagDotDir ) );

			RedVector4 imagCrossDir;
			imagCrossDir = Cross( _v, _quat.GetImaginary() );
			SetAdd( ret, Mul ( imagCrossDir, qReal ) );

			SetAdd( ret, ret );
			Set(ret);
		}

		// TODO: optimize
		//////////////////////////////////////////////////////////////////////////
		void RedVector4::SetTransformedInversePos(const RedQsTransform& _t, const RedVector4& _v )
		{
			RedVector4 temp( _v );
			SetSub( temp, _t.Translation );
			RedMatrix3x3 tempMat;
			tempMat.BuildFromQuaternion( _t.Rotation.Quat );

			float v0 = temp.X;
			float v1 = temp.Y;
			float v2 = temp.Z;
			RedVector4& t = *this;
			t.X = ( tempMat.Row0.X * v0 ) + ( tempMat.Row1.X * v1 ) + ( tempMat.Row2.X * v2 );
			t.Y = ( tempMat.Row0.Y * v0 ) + ( tempMat.Row1.Y * v1 ) + ( tempMat.Row2.Y * v2 );
			t.Z = ( tempMat.Row0.Z * v0 ) + ( tempMat.Row1.Z * v1 ) + ( tempMat.Row2.Z * v2 );
			t.W = 0.0f;

			X *= 1.0f / _t.Scale.X;
			Y *= 1.0f / _t.Scale.Y;
			Z *= 1.0f / _t.Scale.Z;
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::DistanceSquaredTo( const RedVector4& _t ) const
		{
			return Sub( *this,  _t ).SquareLength3();
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::DistanceTo2D( const RedVector4& _t ) const
		{
			RedVector2 delta;
			delta.V = Sub( *this, _t ).V;
			return delta.Length();
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::DistanceTo( const RedVector4& _t ) const
		{
			return Sub( *this, _t ).Length3();
		}
	
		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::SquareLength2() const 
		{ 
			return ( X * X ) + ( Y * Y ); 
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector4::RotateDirection( const RedQuaternion& _quat, const RedVector4& _direction )
		{
			RedScalar qreal( _quat.Quat.W );
			RedScalar q2minus1( Sub( Mul( qreal, qreal ), 0.5f ) );

			RedVector4 ret;
			ret = Mul( _direction, q2minus1 );

			RedScalar imagDotDir = Dot3(_quat.GetImaginary(), _direction );
			SetAdd( ret, Mul( _quat.GetImaginary(), imagDotDir ) ); 

			RedVector4 imagCrossDir = Cross( _quat.GetImaginary(), _direction );
			SetAdd( ret,  Mul( imagCrossDir, qreal ) );

			SetAdd( ret, ret);
			Set(ret);
		}
	}
}