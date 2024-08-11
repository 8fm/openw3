/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

//////////////////////////////////////////////////////////////////////

enum ECookingPlatform : Int32;
enum ERenderingPass : Int32;
enum EMaterialDebugMode : Int32;
class RenderingContext;

/// Data type
enum EMaterialDataType
{
	MDT_Float,			// Single scalar
	MDT_Float2,			// 2D scalar
	MDT_Float3,			// 3D scalar
	MDT_Float4,			// 4D scalar
	MDT_Float4x4,		// 4x4 Matrix
	MDT_Float4x3,		// 4x3 Matrix ( TR matrix )
	MDT_Float3x3,		// 3x3 Matrix ( Rotation matrix )
	MDT_Bool,
	MDT_Uint,			// int - sv_coverage
	MDT_Int,
	MDT_Int2,
	MDT_Int3,
	MDT_Int4,
};

BEGIN_ENUM_RTTI( EMaterialDataType );
	ENUM_OPTION( MDT_Float );
	ENUM_OPTION( MDT_Float2 );
	ENUM_OPTION( MDT_Float3 );
	ENUM_OPTION( MDT_Float4 );
	ENUM_OPTION( MDT_Float4x4 );
	ENUM_OPTION( MDT_Float4x3 );
	ENUM_OPTION( MDT_Float3x3 );
	ENUM_OPTION( MDT_Bool );
	ENUM_OPTION( MDT_Uint );
	ENUM_OPTION( MDT_Int );
	ENUM_OPTION( MDT_Int2 );
	ENUM_OPTION( MDT_Int3 );
	ENUM_OPTION( MDT_Int4 );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////

/// Sampler type
enum EMaterialSamplerType
{
	MST_Texture,		// 2D Texture
	MST_Cubemap,		// Cube map
	MST_Volume,			// 3D Volume
	MST_TextureArray,	// Texture array
};

BEGIN_ENUM_RTTI( EMaterialSamplerType )
	ENUM_OPTION( MST_Texture );
	ENUM_OPTION( MST_Cubemap );
	ENUM_OPTION( MST_Volume );
	ENUM_OPTION( MST_TextureArray );
END_ENUM_RTTI()

//////////////////////////////////////////////////////////////////////

/// Param type
enum EMaterialParamType
{
	MPT_Float,			// Single scalar
	MPT_Float2,			// 2D scalar
	MPT_Float3,			// 3D scalar
	MPT_Float4,			// 4D scalar
	MPT_Float4x4,		// 4x4 Matrix
	MPT_Float4x3,		// 4x3 Matrix ( TR matrix )
	MPT_Float3x3,		// 3x3 Matrix ( Rotation matrix )
	MPT_Bool,
	MPT_Uint,			// int - sv_coverage
	MPT_Texture,		// 2D Texture
	MPT_Cubemap,		// Cube map
	MPT_Volume,			// 3D Volume
	MPT_TextureArray,	// Texture array
};

BEGIN_ENUM_RTTI( EMaterialParamType )
	ENUM_OPTION( MPT_Float );
	ENUM_OPTION( MPT_Float2 );
	ENUM_OPTION( MPT_Float3 );
	ENUM_OPTION( MPT_Float4 );
	ENUM_OPTION( MPT_Float4x4 );
	ENUM_OPTION( MPT_Float4x3 );
	ENUM_OPTION( MPT_Float3x3 );
	ENUM_OPTION( MPT_Bool );
	ENUM_OPTION( MPT_Texture );
	ENUM_OPTION( MPT_Cubemap );
	ENUM_OPTION( MPT_Volume );
	ENUM_OPTION( MPT_TextureArray );
END_ENUM_RTTI()

//////////////////////////////////////////////////////////////////////

/// Material shader target
enum EMaterialShaderTarget
{
	MSH_VertexShader = 1,
	MSH_PixelShader = 2,
};

BEGIN_ENUM_RTTI( EMaterialShaderTarget );
	ENUM_OPTION( MSH_VertexShader );
	ENUM_OPTION( MSH_PixelShader );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////

/// Feedback rendertarget data type
enum ERenderFeedbackDataType
{
	RFDT_Depth,
	RFDT_Color
};

BEGIN_ENUM_RTTI( ERenderFeedbackDataType );
	ENUM_OPTION( RFDT_Depth );
	ENUM_OPTION( RFDT_Color );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////

/// Type of material vertex factory
enum EMaterialVertexFactory : CEnum::TValueType
{
	MVF_Invalid=0,							//!< Invalid vertex factory, do not draw
	MVF_Terrain,							//!< Terrain mesh ( height map sampling )
	MVF_MeshStatic,							//!< Generic static mesh vertex factory
	MVF_MeshSkinned,						//!< Generic skinned mesh vertex factory	
	MVF_ParticleBilboard,					//!< Simple bilboarded particles
	MVF_ParticleBilboardRain,				//!< Simple rain particles
	MVF_ParticleParallel,					//!< Particles parallel to emitter
	MVF_ParticleMotionBlur,					//!< Particles oriented in movement direction with stretching ability
	MVF_ParticleSphereAligned,				//!< Particles billboarded along sphere surface
	MVF_ParticleVerticalFixed,				//!< Particles billboarded along sphere surface, aligned to Z axis
	MVF_ParticleTrail,						//!< Particles producing motion trail effect
	MVF_ParticleFacingTrail,				//!< Particles producing camera facing motion trail effect
	MVF_ParticleScreen,						//!< Particles produced in screen space
	MVF_ParticleBeam,						//!< Particles composing beam effects
	MVF_ApexWithoutBones,					//!< Apex without bones (simulated cloth, non-fractured destruction)
	MVF_ApexWithBones,						//!< Apex with bones (non-simulated cloth, fractured chunks)
	MVF_TesselatedTerrain,					//!< Tesselated terrain
	MVF_TerrainSkirt,						//!< Terrain skirt
	MVF_MeshDestruction,					//!< Skinned mesh with fadeout data
	MVF_Max
};

BEGIN_ENUM_RTTI( EMaterialVertexFactory );
	ENUM_OPTION( MVF_Terrain );
	ENUM_OPTION( MVF_MeshStatic );
	ENUM_OPTION( MVF_MeshSkinned );	
	ENUM_OPTION( MVF_ParticleBilboard );
	ENUM_OPTION( MVF_ParticleBilboardRain );
	ENUM_OPTION( MVF_ParticleParallel );
	ENUM_OPTION( MVF_ParticleMotionBlur );
	ENUM_OPTION( MVF_ParticleSphereAligned );
	ENUM_OPTION( MVF_ParticleVerticalFixed );
	ENUM_OPTION( MVF_ParticleTrail );
	ENUM_OPTION( MVF_ParticleFacingTrail );
	ENUM_OPTION( MVF_ParticleScreen );
	ENUM_OPTION( MVF_ParticleBeam );
	ENUM_OPTION( MVF_ApexWithoutBones );
	ENUM_OPTION( MVF_ApexWithBones );
	ENUM_OPTION( MVF_MeshDestruction );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////

/// Type of texture filtering
enum ETextureFilteringMin
{
	TFMin_Point = 0,			// Point filtering used aka NEAREST
	TFMin_Linear,			// Bilinear interpolation filtering  
	TFMin_Anisotropic,		// Anisotropic texture filtering 
	TFMin_AnisotropicLow,	// Anisotropic texture low filtering 
};

BEGIN_ENUM_RTTI( ETextureFilteringMin );
ENUM_OPTION( TFMin_Point );
ENUM_OPTION( TFMin_Linear );
ENUM_OPTION( TFMin_Anisotropic );
ENUM_OPTION( TFMin_AnisotropicLow );
END_ENUM_RTTI();

/// Type of texture filtering
enum ETextureFilteringMag
{
	TFMag_Point = 0,			// Point filtering used aka NEAREST
	TFMag_Linear,			// Bilinear interpolation filtering  
};

BEGIN_ENUM_RTTI( ETextureFilteringMag );
ENUM_OPTION( TFMag_Point );
ENUM_OPTION( TFMag_Linear );
END_ENUM_RTTI();

/// Type of texture filtering
enum ETextureFilteringMip
{
	TFMip_None = 0,				// No filtering
	TFMip_Point,			// Point filtering used aka NEAREST
	TFMip_Linear,			// Bilinear interpolation filtering  
};

BEGIN_ENUM_RTTI( ETextureFilteringMip );
ENUM_OPTION( TFMip_None );
ENUM_OPTION( TFMip_Point );
ENUM_OPTION( TFMip_Linear );
END_ENUM_RTTI();

/// Texture addressing mode
enum ETextureAddressing
{
	TA_Wrap = 0,			// Tile the texture at every integer junction
	TA_Mirror,			// Similar to wrap, except that the texture is flipped at every integer junction
	TA_Clamp,			// Texture coordinates outside the range [0.0, 1.0] are set to the texture color at 0.0 or 1.0, respectively
	TA_MirrorOnce,		// Takes the absolute value of the texture coordinate (thus, mirroring around 0), and then clamps to the maximum value
};

BEGIN_ENUM_RTTI( ETextureAddressing );
ENUM_OPTION( TA_Wrap );
ENUM_OPTION( TA_Mirror );
ENUM_OPTION( TA_Clamp );
ENUM_OPTION( TA_MirrorOnce );
END_ENUM_RTTI();

enum ETextureComparisonFunction : CEnum::TValueType
{
	TCF_None = 0,
	TCF_Less, //todo: add more if you need
	TCF_Equal,
	TCF_LessEqual,
	TCF_Greater,
	TCF_NotEqual,
	TCF_GreaterEqual,
	TCF_Always,
};

BEGIN_ENUM_RTTI( ETextureComparisonFunction );
ENUM_OPTION( TCF_None );
ENUM_OPTION( TCF_Less );
ENUM_OPTION( TCF_Equal );
ENUM_OPTION( TCF_LessEqual );
ENUM_OPTION( TCF_Greater );
ENUM_OPTION( TCF_NotEqual );
ENUM_OPTION( TCF_GreaterEqual );
ENUM_OPTION( TCF_Always );
END_ENUM_RTTI();



RED_INLINE Bool DoesMaterialVertexFactorySupportDissolve( EMaterialVertexFactory factory )
{
	switch ( factory )
	{
	case MVF_MeshStatic:
	case MVF_MeshSkinned:
	case MVF_MeshDestruction:
	case MVF_ApexWithoutBones:
	case MVF_ApexWithBones:
		return true;

	default:
		return false;
	}
}

RED_INLINE Bool DoesMaterialVertexFactorySupportUVDissolve( EMaterialVertexFactory factory )
{
	switch ( factory )
	{
	case MVF_MeshStatic:
	case MVF_MeshSkinned:
		return true;

	default:
		return false;
	}
}

RED_INLINE Bool DoesMaterialVertexFactorySupportClippingEllipse( EMaterialVertexFactory factory )
{
	switch ( factory )
	{
	case MVF_MeshStatic:
	case MVF_MeshSkinned:
	case MVF_ApexWithBones:
		return true;

	default:
		return false;
	}
}


/// Material rendering context
class MaterialRenderingContext
{
public:
	const RenderingContext*		m_renderingContext;		// Base rendering context context
	EMaterialVertexFactory		m_vertexFactory;		// Vertex factory for material
	Bool						m_selected;				// Render as selected
	Bool						m_uvDissolveSeparateUV;	// UV-space dissolve uses different UV stream ( only MeshSkinned/MeshStatic )
	Bool						m_useInstancing;		// Apply instancing data
	Bool						m_useParticleInstancing;// Vertex color needed
	Bool						m_hasExtraStreams;		// We have vertex color or second UV in data stream ( only MeshGeometry vertex factory )
	Bool						m_hasVertexCollapse;	// Vertex collapse
	Bool						m_discardingPass;		// A separated pass with discard instruction (no hi/early-z)
	Bool						m_pregeneratedMaps;		// Compile shader for use with pregenerated texture maps
	Bool						m_proxyMaterialCast;	// Compile shader for proxy generation
	EMaterialDebugMode			m_materialDebugMode;	// Debug mode for materials ( PC )
	Bool						m_lowQuality;			// Low quality version of material
	mutable Uint32				m_cachedID;				// Cached the ID of the context to avoid recalculation
	
public:
	// Constructor
	MaterialRenderingContext( const RenderingContext& renderingContext );

	// Get unique bucket ID
	Uint32 CalcID() const;
	
	// Get human readable rendering context description
	String ToString() const;
};

//////////////////////////////////////////////////////////////////////

struct SamplerStateInfo
{
	ETextureFilteringMin		m_filteringMin;
	ETextureFilteringMag		m_filteringMag;
	ETextureFilteringMip		m_filteringMip;
	ETextureAddressing			m_addressU;
	ETextureAddressing			m_addressV;
	ETextureAddressing			m_addressW;
	ETextureComparisonFunction	m_comparisonFunc;
	Uint32						m_register;

	RED_INLINE Bool operator==( const SamplerStateInfo& rhs ) const
	{
		return	m_filteringMin		== rhs.m_filteringMin	&&
			m_filteringMag		== rhs.m_filteringMag	&&
			m_filteringMip		== rhs.m_filteringMip	&&
			m_addressU			== rhs.m_addressU		&&
			m_addressV			== rhs.m_addressV		&&
			m_addressW			== rhs.m_addressW		&&
			m_comparisonFunc	== rhs.m_comparisonFunc;
	}

	void Serialize( IFile& file )
	{
		if ( file.IsReader() || file.IsWriter() )
		{
			file.SerializeSimpleType( &m_filteringMin, sizeof( m_filteringMin ) );
			file.SerializeSimpleType( &m_filteringMag, sizeof( m_filteringMag ) );
			file.SerializeSimpleType( &m_filteringMip, sizeof( m_filteringMip ) );
			file.SerializeSimpleType( &m_addressU, sizeof( m_addressU ) );
			file.SerializeSimpleType( &m_addressV, sizeof( m_addressV ) );
			file.SerializeSimpleType( &m_addressW, sizeof( m_addressW ) );
			file.SerializeSimpleType( &m_comparisonFunc, sizeof( m_comparisonFunc ) );
			file.SerializeSimpleType( &m_register, sizeof( m_register ) );
		}
	}
};

/// Information about material parameter used by rendering technique
class MaterialUsedParameter
{
public:
	CName										m_name;				//!< Name of parameter
	Uint8										m_register;			//!< Target registers

public:
	// Default constructor
	RED_INLINE MaterialUsedParameter()
	{};

	// Constructor
	RED_INLINE MaterialUsedParameter( const CName& name, Uint8 reg )
		: m_name( name )
		, m_register( reg )
	{
	}

	// Compare
	RED_INLINE Bool operator==( const MaterialUsedParameter& param ) const
	{
		return ( m_name == param.m_name ) && ( m_register == param.m_register );
	}

public:
	// Serialization
	friend IFile& operator<<( IFile& file, MaterialUsedParameter& param )
	{
		file << param.m_name;
		file << param.m_register;
		return file;
	}
};

typedef TDynArray< MaterialUsedParameter, MC_MaterialParameters > TMaterialUsedParameterArray;

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// Material code chunk
class CodeChunk
{
protected:
	TDynArray< AnsiChar, MC_BufferShader >	m_code;
	Bool									m_const;

public:
	static CodeChunk EMPTY;		// Empty code chunk

public:
	// Default constructor
	RED_INLINE CodeChunk()
		: m_const( true )
	{};

	// Copy Constructor
	RED_INLINE CodeChunk( const CodeChunk& chunk )
		: m_code( chunk.m_code )
		, m_const( chunk.m_const )
	{};

	// Move constructor
	RED_INLINE CodeChunk( CodeChunk&& chunk )
	{
		m_const = chunk.m_const;
		m_code = Move( chunk.m_code );
	};

	// Construct from float
	RED_INLINE CodeChunk( Float value )
		: m_const( true )
	{
		AnsiChar text[ 64 ];
		Red::System::SNPrintF( text, ARRAY_COUNT( text ), "(%f)", value );
		Uint32 length = static_cast< Uint32 >( Red::System::StringLength( text ) );
		m_code.Resize( length+1 );
		Red::System::MemoryCopy( m_code.TypedData(), text, length+1 );	
	}	

	// Construct from uint
	RED_INLINE CodeChunk( Uint32 value )
		: m_const( true )
	{
		AnsiChar text[ 64 ];
		Red::System::SNPrintF( text, ARRAY_COUNT( text ), "(%d)", value );
		Uint32 length = static_cast< Uint32 >( Red::System::StringLength( text ) );
		m_code.Resize( length+1 );
		Red::System::MemoryCopy( m_code.TypedData(), text, length+1 );	
	}	

	// Construct from vector
	RED_INLINE CodeChunk( const Vector& value )
		: m_const( true )
	{
		AnsiChar text[ 64 ];
		Red::System::SNPrintF( text, ARRAY_COUNT( text ), "float4( %f, %f, %f, %f )", value.X, value.Y, value.Z, value.W );
		Uint32 length = static_cast< Uint32 >( Red::System::StringLength( text ) );
		m_code.Resize( length+1 );
		Red::System::MemoryCopy( m_code.TypedData(), text, length+1 );		
	}

	// Construct from string
	RED_INLINE CodeChunk( const AnsiChar* code, Bool isConst = true )
		: m_const( isConst )
	{
		ASSERT( code );
		Uint32 length = static_cast< Uint32 >( Red::System::StringLength( code ) );
		m_code.Resize( length+1 );
		Red::System::MemoryCopy( m_code.TypedData(), code, length+1 );
	}

	// Convert to string
	RED_INLINE const AnsiChar* AsChar() const
	{
		const AnsiChar* text = m_code.TypedData();
		return text ? text : "";
	}
	
	// Check if not empty
	RED_INLINE operator Bool() const
	{
		return !IsEmpty();
	}

	// Check if empty, explicit way
	RED_INLINE Bool IsEmpty() const
	{
		return m_code.Size() <= 1;
	}

	// Check if code is const
	RED_INLINE Bool IsConst() const
	{
		return m_const;
	}

	// Change code constness
	RED_INLINE void SetConst( Bool isConst )
	{
		m_const = isConst;
	}

	// Move assignment operator
	RED_INLINE CodeChunk& operator=( CodeChunk&& chunk )
	{
		m_const = chunk.m_const;
		m_code = Move( chunk.m_code );

		return *this;
	}

	// Copy assignment operator
	RED_INLINE CodeChunk& operator=( const CodeChunk& chunk )
	{
		m_const = chunk.m_const;
		m_code = chunk.m_code;

		return *this;
	}

	// Compare value
	RED_INLINE Bool operator==( const AnsiChar* dupa ) const
	{
		ASSERT( dupa );
		return 0 == Red::System::StringCompareNoCase( dupa, AsChar() );
	}

	// Compare value
	RED_INLINE Bool operator!=( const AnsiChar* dupa ) const
	{
		ASSERT( dupa );
		return 0 != Red::System::StringCompareNoCase( dupa, AsChar() );
	}

	// Compare value
	RED_INLINE Bool operator==( const CodeChunk& dupa ) const
	{
		return 0 == Red::System::StringCompareNoCase( AsChar(), dupa.AsChar() );
	}

	// Compare value
	RED_INLINE Bool operator!=( const CodeChunk& dupa ) const
	{
		return 0 != Red::System::StringCompareNoCase( AsChar(), dupa.AsChar() );
	}

public:
	// Create string from formatted expression
	static CodeChunk Printf( Bool isConst, const AnsiChar* format, ... )
	{
		va_list arglist;
		va_start(arglist, format);

		int len = static_cast< int >( Red::System::StringLength(format) + 2048 );

		// GET NEAREST POWER OF 2
		len -= 1;
		len |= len >> 16;
		len |= len >> 8;
		len |= len >> 4;
		len |= len >> 2;
		len |= len >> 1;
		len += 1; 

		AnsiChar* formattedBuf = static_cast<AnsiChar*>(RED_MEMORY_ALLOCATE( MemoryPool_Strings, MC_String,len) );
		int n = Red::System::VSNPrintF( formattedBuf, len, format, arglist );
		ASSERT( n <= len );
		va_end(arglist);
		CodeChunk ret(formattedBuf, isConst);
		RED_MEMORY_FREE( MemoryPool_Strings, MC_String, formattedBuf );
		return ret;
	}

public:
	// Swizzle
	RED_INLINE CodeChunk x() const { return Printf( m_const, "(%s).x", AsChar() ); }
	RED_INLINE CodeChunk y() const { return Printf( m_const, "(%s).y", AsChar() ); }
	RED_INLINE CodeChunk z() const { return Printf( m_const, "(%s).z", AsChar() ); }
	RED_INLINE CodeChunk w() const { return Printf( m_const, "(%s).w", AsChar() ); }

	RED_INLINE CodeChunk xx() const { return Printf( m_const, "(%s).xx", AsChar() ); }
	RED_INLINE CodeChunk xy() const { return Printf( m_const, "(%s).xy", AsChar() ); }
	RED_INLINE CodeChunk xz() const { return Printf( m_const, "(%s).xz", AsChar() ); }
	RED_INLINE CodeChunk xw() const { return Printf( m_const, "(%s).xw", AsChar() ); }
	RED_INLINE CodeChunk yx() const { return Printf( m_const, "(%s).yx", AsChar() ); }
	RED_INLINE CodeChunk yy() const { return Printf( m_const, "(%s).yy", AsChar() ); }
	RED_INLINE CodeChunk yz() const { return Printf( m_const, "(%s).yz", AsChar() ); }
	RED_INLINE CodeChunk yw() const { return Printf( m_const, "(%s).yw", AsChar() ); }
	RED_INLINE CodeChunk zx() const { return Printf( m_const, "(%s).zx", AsChar() ); }
	RED_INLINE CodeChunk zy() const { return Printf( m_const, "(%s).zy", AsChar() ); }
	RED_INLINE CodeChunk zz() const { return Printf( m_const, "(%s).zz", AsChar() ); }
	RED_INLINE CodeChunk zw() const { return Printf( m_const, "(%s).zw", AsChar() ); }
	RED_INLINE CodeChunk wx() const { return Printf( m_const, "(%s).wx", AsChar() ); }
	RED_INLINE CodeChunk wy() const { return Printf( m_const, "(%s).wy", AsChar() ); }
	RED_INLINE CodeChunk wz() const { return Printf( m_const, "(%s).wz", AsChar() ); }
	RED_INLINE CodeChunk ww() const { return Printf( m_const, "(%s).ww", AsChar() ); }
	
	RED_INLINE CodeChunk xyz() const { return Printf( m_const, "(%s).xyz", AsChar() ); }
	RED_INLINE CodeChunk xxx() const { return Printf( m_const, "(%s).xxx", AsChar() ); }
	RED_INLINE CodeChunk yyy() const { return Printf( m_const, "(%s).yyy", AsChar() ); }
	RED_INLINE CodeChunk zzz() const { return Printf( m_const, "(%s).zzz", AsChar() ); }
	RED_INLINE CodeChunk www() const { return Printf( m_const, "(%s).www", AsChar() ); }	

	RED_INLINE CodeChunk xyxy() const { return Printf( m_const, "(%s).xyxy", AsChar() ); }

	RED_INLINE CodeChunk xyzw() const { return Printf( m_const, "(%s).xyzw", AsChar() ); }
	RED_INLINE CodeChunk xxxx() const { return Printf( m_const, "(%s).xxxx", AsChar() ); }
	RED_INLINE CodeChunk yyyy() const { return Printf( m_const, "(%s).yyyy", AsChar() ); }
	RED_INLINE CodeChunk zzzz() const { return Printf( m_const, "(%s).zzzz", AsChar() ); }
	RED_INLINE CodeChunk wwww() const { return Printf( m_const, "(%s).wwww", AsChar() ); }
	
	// Math
	RED_INLINE CodeChunk operator+( const CodeChunk& other ) const { return Printf( m_const && other.m_const, "(%s) + (%s)", AsChar(), other.AsChar() ); }
	RED_INLINE CodeChunk operator-( const CodeChunk& other ) const { return Printf( m_const && other.m_const, "(%s) - (%s)", AsChar(), other.AsChar() ); }
	RED_INLINE CodeChunk operator/( const CodeChunk& other ) const { return Printf( m_const && other.m_const, "(%s) / (%s)", AsChar(), other.AsChar() ); }
	RED_INLINE CodeChunk operator*( const CodeChunk& other ) const { return Printf( m_const && other.m_const, "(%s) * (%s)", AsChar(), other.AsChar() ); }
	RED_INLINE CodeChunk operator>( const CodeChunk& other ) const { return Printf( m_const && other.m_const, "(%s) > (%s)", AsChar(), other.AsChar() ); }
	RED_INLINE CodeChunk operator<( const CodeChunk& other ) const { return Printf( m_const && other.m_const, "(%s) < (%s)", AsChar(), other.AsChar() ); }

	// Math with float
	RED_INLINE CodeChunk operator+( const Float other ) const { return Printf( m_const, "(%s) + %f", AsChar(), other ); }
	RED_INLINE CodeChunk operator-( const Float other ) const { return Printf( m_const, "(%s) - %f", AsChar(), other ); }
	RED_INLINE CodeChunk operator/( const Float other ) const { return Printf( m_const, "(%s) / %f", AsChar(), other ); }
	RED_INLINE CodeChunk operator*( const Float other ) const { return Printf( m_const, "(%s) * %f", AsChar(), other ); }
	

	RED_INLINE CodeChunk operator%( const Uint32 other ) const { return Printf( m_const, "(%s) %% %u", AsChar(), other ); }

	// Unary operators
	RED_INLINE CodeChunk Saturate() const { return Printf( m_const, "saturate(%s)", AsChar() ); }
	RED_INLINE CodeChunk Length() const { return Printf( m_const, "length(%s)", AsChar() ); }
	RED_INLINE CodeChunk Normalize() const { return Printf( m_const, "normalize(%s)", AsChar() ); }
	RED_INLINE CodeChunk operator-() const { return Printf( m_const, "(-(%s))", AsChar() ); }

	// Lookup
	RED_INLINE CodeChunk operator[]( const CodeChunk& index ) const { return CodeChunk::Printf( m_const && index.m_const, "(%s)[ %s ]", AsChar(), index.AsChar() ); }
};

/// Operators
namespace CodeChunkOperators
{
	// Operators
	static RED_INLINE CodeChunk Abs( const CodeChunk& a ) { return CodeChunk::Printf( a.IsConst(), "abs(%s)", a.AsChar() ); }
	static RED_INLINE CodeChunk All( const CodeChunk& a ) { return CodeChunk::Printf( a.IsConst(), "all(%s)", a.AsChar() ); }
	static RED_INLINE CodeChunk Any( const CodeChunk& a ) { return CodeChunk::Printf( a.IsConst(), "any(%s)", a.AsChar() ); }
	static RED_INLINE CodeChunk Saturate( const CodeChunk& a ) { return CodeChunk::Printf( a.IsConst(), "saturate(%s)", a.AsChar() ); }
	static RED_INLINE CodeChunk Length( const CodeChunk& a ) { return CodeChunk::Printf( a.IsConst(), "length(%s)", a.AsChar() ); }
	static RED_INLINE CodeChunk Normalize( const CodeChunk& a ) { return CodeChunk::Printf( a.IsConst(), "normalize(%s)", a.AsChar() ); }
	static RED_INLINE CodeChunk Add( const CodeChunk& a, const CodeChunk& b ) { return CodeChunk::Printf( a.IsConst() && b.IsConst(), "add( %s, %s )", a.AsChar(), b.AsChar() ); }
	static RED_INLINE CodeChunk Mul( const CodeChunk& a, const CodeChunk& b ) { return CodeChunk::Printf( a.IsConst() && b.IsConst(), "mul( %s, %s )", a.AsChar(), b.AsChar() ); }
	static RED_INLINE CodeChunk Div( const CodeChunk& a, const CodeChunk& b ) { return CodeChunk::Printf( a.IsConst() && b.IsConst(), "div( %s, %s )", a.AsChar(), b.AsChar() ); }
	static RED_INLINE CodeChunk Min( const CodeChunk& a, const CodeChunk& b ) { return CodeChunk::Printf( a.IsConst() && b.IsConst(), "min( %s, %s )", a.AsChar(), b.AsChar() ); }
	static RED_INLINE CodeChunk Max( const CodeChunk& a, const CodeChunk& b ) { return CodeChunk::Printf( a.IsConst() && b.IsConst(), "max( %s, %s )", a.AsChar(), b.AsChar() ); }
	static RED_INLINE CodeChunk Dot2( const CodeChunk& a, const CodeChunk& b ) { return CodeChunk::Printf( a.IsConst() && b.IsConst(), "dot( (%s).xy, (%s).xy )", a.AsChar(), b.AsChar() ); }
	static RED_INLINE CodeChunk Dot3( const CodeChunk& a, const CodeChunk& b ) { return CodeChunk::Printf( a.IsConst() && b.IsConst(), "dot( (%s).xyz, (%s).xyz )", a.AsChar(), b.AsChar() ); }	
	static RED_INLINE CodeChunk Dot4( const CodeChunk& a, const CodeChunk& b ) { return CodeChunk::Printf( a.IsConst() && b.IsConst(), "dot( (%s).xyzw, (%s).xyzw )", a.AsChar(), b.AsChar() ); }
	static RED_INLINE CodeChunk Cross( const CodeChunk& a, const CodeChunk& b ) { return CodeChunk::Printf( a.IsConst() && b.IsConst(), "cross( (%s).xyz, (%s).xyz )", a.AsChar(), b.AsChar() ); }
	static RED_INLINE CodeChunk Pow( const CodeChunk& a, const CodeChunk& b ) { return CodeChunk::Printf( a.IsConst() && b.IsConst(), "pow( %s, %s )", a.AsChar(), b.AsChar() ); }
	static RED_INLINE CodeChunk Sqrt( const CodeChunk& a ) { return CodeChunk::Printf( a.IsConst(), "sqrt( %s )", a.AsChar() ); }
	static RED_INLINE CodeChunk Log2( const CodeChunk& a) { return CodeChunk::Printf( a.IsConst(), "log2( %s )", a.AsChar() ); }
	static RED_INLINE CodeChunk Exp( const CodeChunk& a) { return CodeChunk::Printf( a.IsConst(), "exp( %s )", a.AsChar() ); }
	static RED_INLINE CodeChunk Exp2( const CodeChunk& a) { return CodeChunk::Printf( a.IsConst(), "exp2( %s )", a.AsChar() ); }
	static RED_INLINE CodeChunk Clamp( const CodeChunk& x, const CodeChunk& a, const CodeChunk& b ) { return CodeChunk::Printf( x.IsConst() && a.IsConst() && b.IsConst(), "clamp( %s, %s, %s)", x.AsChar(), a.AsChar(), b.AsChar() ); } 
	static RED_INLINE CodeChunk Lerp( const CodeChunk& x, const CodeChunk& y, const CodeChunk& s ) { return CodeChunk::Printf( x.IsConst() && y.IsConst() && s.IsConst(), "lerp( %s, %s, %s)", x.AsChar(), y.AsChar(), s.AsChar() ); } 
	static RED_INLINE CodeChunk SmoothStep( const CodeChunk& _min, const CodeChunk& _max, const CodeChunk& x ) { return CodeChunk::Printf( _min.IsConst() && _max.IsConst() && x.IsConst(), "smoothstep( %s, %s, %s)", _min.AsChar(), _max.AsChar(), x.AsChar() ); } 
	static RED_INLINE CodeChunk Mad( const CodeChunk& a, const CodeChunk& b, const CodeChunk& c ) { return CodeChunk::Printf( a.IsConst() && b.IsConst() && c.IsConst(), "(%s) * (%s) + (%s)", a.AsChar(), b.AsChar(), c.AsChar() ); } 
	static RED_INLINE CodeChunk Sin( const CodeChunk& a ) { return CodeChunk::Printf( a.IsConst(), "sin( %s )", a.AsChar() ); }
	static RED_INLINE CodeChunk Cos( const CodeChunk& a ) { return CodeChunk::Printf( a.IsConst(), "cos( %s )", a.AsChar() ); }
	static RED_INLINE CodeChunk Tan( const CodeChunk& a ) { return CodeChunk::Printf( a.IsConst(), "tan( %s )", a.AsChar() ); }
	static RED_INLINE CodeChunk ArcSin( const CodeChunk& a ) { return CodeChunk::Printf( a.IsConst(), "asin( %s )", a.AsChar() ); }
	static RED_INLINE CodeChunk ArcCos( const CodeChunk& a ) { return CodeChunk::Printf( a.IsConst(), "acos( %s )", a.AsChar() ); }
	static RED_INLINE CodeChunk ArcTan( const CodeChunk& a ) { return CodeChunk::Printf( a.IsConst(), "atan( %s )", a.AsChar() ); }
	static RED_INLINE CodeChunk ArcTan2( const CodeChunk& y, const CodeChunk& x ) { return CodeChunk::Printf( y.IsConst() && x.IsConst(), "atan2( %s, %s )", y.AsChar(), x.AsChar() ); }
	static RED_INLINE CodeChunk Frac( const CodeChunk& a) { return CodeChunk::Printf( a.IsConst(), "frac( %s )", a.AsChar() ); }
	static RED_INLINE CodeChunk Floor( const CodeChunk& a) { return CodeChunk::Printf( a.IsConst(), "floor( %s )", a.AsChar() ); }
	static RED_INLINE CodeChunk Sign( const CodeChunk& a) { return CodeChunk::Printf( a.IsConst(), "sign( %s )", a.AsChar() ); }
	static RED_INLINE CodeChunk Reflect( const CodeChunk& v, const CodeChunk& norm ) { return v.xyz() - CodeChunk( 2.0f ) * Dot3( v, norm ) * norm.xyz(); }
	static RED_INLINE CodeChunk Float2( const CodeChunk& a ) { return CodeChunk::Printf( a.IsConst(), "float2( %s )", a.AsChar() ); }
	static RED_INLINE CodeChunk Float2( const CodeChunk& a, const CodeChunk& b ) { return CodeChunk::Printf( a.IsConst() && b.IsConst(), "float2( %s, %s )", a.AsChar(), b.AsChar() ); }
	static RED_INLINE CodeChunk Float3( const CodeChunk& a ) { return CodeChunk::Printf( a.IsConst(), "float3( %s )", a.AsChar() ); }
	static RED_INLINE CodeChunk Float3( const CodeChunk& a, const CodeChunk& b ) { return CodeChunk::Printf( a.IsConst() && b.IsConst(), "float3( %s, %s )", a.AsChar(), b.AsChar() ); }
	static RED_INLINE CodeChunk Float3( const CodeChunk& a, const CodeChunk& b, const CodeChunk& c ) { return CodeChunk::Printf( a.IsConst() && b.IsConst() && c.IsConst(), "float3( %s, %s, %s )", a.AsChar(), b.AsChar(), c.AsChar() ); }
	static RED_INLINE CodeChunk Float4( const CodeChunk& a ) { return CodeChunk::Printf( a.IsConst(), "float4( %s )", a.AsChar() ); }
	static RED_INLINE CodeChunk Float4( const CodeChunk& a, const CodeChunk& b ) { return CodeChunk::Printf( a.IsConst() && b.IsConst(), "float4( %s, %s )", a.AsChar(), b.AsChar() ); }
	static RED_INLINE CodeChunk Float4( const CodeChunk& a, const CodeChunk& b, const CodeChunk& c ) { return CodeChunk::Printf( a.IsConst() && b.IsConst() && c.IsConst(), "float4( %s, %s, %s )", a.AsChar(), b.AsChar(), c.AsChar() ); }
	static RED_INLINE CodeChunk Float4( const CodeChunk& a, const CodeChunk& b, const CodeChunk& c, const CodeChunk& d ) { return CodeChunk::Printf( a.IsConst() && b.IsConst() && c.IsConst() && d.IsConst(),  "float4( %s, %s, %s, %s )", a.AsChar(), b.AsChar(), c.AsChar(), d.AsChar() ); }
	static RED_INLINE CodeChunk DDX( const CodeChunk& uv ) { return CodeChunk::Printf( false, "ddx( %s )", uv.AsChar() ); }
	static RED_INLINE CodeChunk DDY( const CodeChunk& uv ) { return CodeChunk::Printf( false, "ddy( %s )", uv.AsChar() ); }
	
	static RED_INLINE CodeChunk Tex2D( const CodeChunk& sampler, const CodeChunk& uv ) { return CodeChunk::Printf( false, "SYS_SAMPLE( %s, (%s).xy )", sampler.AsChar(), uv.AsChar() ); }
	static RED_INLINE CodeChunk Tex2D( const CodeChunk& texture, const CodeChunk& sampler, const CodeChunk& uv ) { return CodeChunk::Printf( false, "%s.Sample( %s, (%s).xy )", texture.AsChar(), sampler.AsChar(), uv.AsChar() ); }
	static RED_INLINE CodeChunk Tex2DArray( const CodeChunk& sampler, const CodeChunk& uv, const CodeChunk& arrayIndex ) { return CodeChunk::Printf( false, "t_%s.Sample( s_%s, float3( (%s).xy, %s ) )", sampler.AsChar(), sampler.AsChar(), uv.AsChar(), arrayIndex.AsChar() ); }
	static RED_INLINE CodeChunk Tex2DArray( const CodeChunk& texture, const CodeChunk& sampler, const CodeChunk& uv, const CodeChunk& arrayIndex ) { return CodeChunk::Printf( false, "%s.Sample( %s, float3( (%s).xy, %s ) )", texture.AsChar(), sampler.AsChar(), uv.AsChar(), arrayIndex.AsChar() ); }
	static RED_INLINE CodeChunk Tex2Dproj( const CodeChunk& sampler, const CodeChunk& uv ) { return CodeChunk::Printf( false, "t_%s.Sample( s_%s, (%s).xy/(%s).w )", sampler.AsChar(), sampler.AsChar(), uv.AsChar(), uv.AsChar() ); }
	static RED_INLINE CodeChunk Tex2Dproj( const CodeChunk& texture, const CodeChunk& sampler, const CodeChunk& uv ) { return CodeChunk::Printf( false, "%s.Sample( %s, (%s).xy/(%s).w )", texture.AsChar(), sampler.AsChar(), uv.AsChar(), uv.AsChar() ); }
	static RED_INLINE CodeChunk Tex2Dlod( const CodeChunk& sampler, const CodeChunk& uv ) { return CodeChunk::Printf( false, "SYS_SAMPLE_LEVEL( %s, (%s).xy, (%s).z )", sampler.AsChar(), uv.AsChar(), uv.AsChar() ); }
	static RED_INLINE CodeChunk Tex2Dlod( const CodeChunk& texture, const CodeChunk& sampler, const CodeChunk& uv ) { return CodeChunk::Printf( false, "SAMPLE_LEVEL( %s, %s, (%s).xy, (%s).z )", texture.AsChar(), sampler.AsChar(), uv.AsChar(), uv.AsChar() ); }
	static RED_INLINE CodeChunk Tex2Dgrad( const CodeChunk& sampler, const CodeChunk& uv, const CodeChunk& ddx, const CodeChunk& ddy  ) { return CodeChunk::Printf( false, "SAMPLE_GRADIENT( t_%s, s_%s, (%s).xy, (%s).xy, (%s).xy )", sampler.AsChar(), sampler.AsChar(), uv.AsChar(), ddx.AsChar(), ddy.AsChar() ); }
	static RED_INLINE CodeChunk Tex2Dgrad( const CodeChunk& texture, const CodeChunk& sampler, const CodeChunk& uv, const CodeChunk& ddx, const CodeChunk& ddy  ) { return CodeChunk::Printf( false, "SAMPLE_GRADIENT( %s, %s, (%s).xy, %s.xy, %s.xy )", texture.AsChar(), sampler.AsChar(), uv.AsChar(), ddx.AsChar(), ddy.AsChar() ); }
	static RED_INLINE CodeChunk Tex2Dbias( const CodeChunk& sampler, const CodeChunk& uv, const CodeChunk& bias ) { return CodeChunk::Printf( false, "t_%s.SampleBias( s_%s, (%s).xy, %s.x )", sampler.AsChar(), sampler.AsChar(), uv.AsChar(), bias.AsChar() ); }
	static RED_INLINE CodeChunk Tex2Dbias( const CodeChunk& texture, const CodeChunk& sampler, const CodeChunk& uv, const CodeChunk& bias ) { return CodeChunk::Printf( false, "%s.SampleBias( %s, (%s).xy, %s.x )", texture.AsChar(), sampler.AsChar(), uv.AsChar(), bias.AsChar() ); }
	static RED_INLINE CodeChunk Tex2Dload( const CodeChunk& texture, const CodeChunk& uv, const CodeChunk& level ) { return CodeChunk::Printf( false, "t_%s.Load( float3( (%s).xy, (%s).x ) )", texture.AsChar(), uv.AsChar(), level.AsChar() ); }
	static RED_INLINE CodeChunk TexCUBE( const CodeChunk& sampler, const CodeChunk& uvw ) { return CodeChunk::Printf( false, "t_%s.Sample( s_%s, (%s).xyz )", sampler.AsChar(), sampler.AsChar(), uvw.AsChar() ); }
	static RED_INLINE CodeChunk TexCUBE( const CodeChunk& texture, const CodeChunk& sampler, const CodeChunk& uvw ) { return CodeChunk::Printf( false, "%s.Sample( %s, (%s).xyz )", texture.AsChar(), sampler.AsChar(), uvw.AsChar() ); }
	static RED_INLINE CodeChunk TexCUBEbias( const CodeChunk& sampler, const CodeChunk& uvw, const CodeChunk& bias ) { return CodeChunk::Printf( false, "t_%s.SampleBias( s_%s, (%s).xyz, %s.x )", sampler.AsChar(), sampler.AsChar(), uvw.AsChar(), bias.AsChar() ); }
	static RED_INLINE CodeChunk TexCUBEbias( const CodeChunk& texture, const CodeChunk& sampler, const CodeChunk& uvw, const CodeChunk& bias ) { return CodeChunk::Printf( false, "%s.SampleBias( %s, (%s).xyz, %s.x )", texture.AsChar(), sampler.AsChar(), uvw.AsChar(), bias.AsChar() ); }
	static RED_INLINE CodeChunk TexCUBElod( const CodeChunk& sampler, const CodeChunk& uvw, const CodeChunk& lod ) { return CodeChunk::Printf( false, "SYS_SAMPLE_LEVEL( %s, (%s).xyz, %s.x )", sampler.AsChar(), uvw.AsChar(), lod.AsChar() ); }
	static RED_INLINE CodeChunk TexCUBElod( const CodeChunk& texture, const CodeChunk& sampler, const CodeChunk& uvw, const CodeChunk& lod ) { return CodeChunk::Printf( false, "SAMPLE_LEVEL( %s, %s, (%s).xyz, %s.x )", texture.AsChar(), sampler.AsChar(), uvw.AsChar(), lod.AsChar() ); }
	static RED_INLINE CodeChunk Tex3D( const CodeChunk& sampler, const CodeChunk& uvw ) { return CodeChunk::Printf( false, "t_%s.Sample( s_%s, (%s).xyz )", sampler.AsChar(), sampler.AsChar(), uvw.AsChar() ); }
	static RED_INLINE CodeChunk Tex3D( const CodeChunk& texture, const CodeChunk& sampler, const CodeChunk& uvw ) { return CodeChunk::Printf( false, "%s.Sample( %s, (%s).xyz )", texture.AsChar(), sampler.AsChar(), uvw.AsChar() ); }
	static RED_INLINE CodeChunk ConditionalOperator( const CodeChunk& condition, const CodeChunk& codeForTrue, const CodeChunk& codeForFalse ) 
	{
		return CodeChunk::Printf( condition.IsConst() && codeForTrue.IsConst() && codeForFalse.IsConst(),
			"StaticBranch(%s, %s, %s)", condition.AsChar(), codeForTrue.AsChar(), codeForFalse.AsChar() ); 
	}

	// Math with float
	static RED_INLINE CodeChunk operator+( const Float other, const CodeChunk& a ) { return CodeChunk::Printf( a.IsConst(), "%f + (%s)", other, a.AsChar() ); }
	static RED_INLINE CodeChunk operator-( const Float other, const CodeChunk& a ) { return CodeChunk::Printf( a.IsConst(), "%f - (%s)", other, a.AsChar() ); }
	static RED_INLINE CodeChunk operator/( const Float other, const CodeChunk& a ) { return CodeChunk::Printf( a.IsConst(), "%f / (%s)", other, a.AsChar() ); }
	static RED_INLINE CodeChunk operator*( const Float other, const CodeChunk& a ) { return CodeChunk::Printf( a.IsConst(), "%f * (%s)", other, a.AsChar() ); }

};

enum ETessellationDomain
{
	TD_Triangle,
	TD_Quad,
	TD_Line,
};

BEGIN_ENUM_RTTI( ETessellationDomain );
ENUM_OPTION( TD_Triangle );
ENUM_OPTION( TD_Quad );
ENUM_OPTION( TD_Line );
END_ENUM_RTTI();

/// Material shader compiler interface
class IMaterialShaderCompiler
{
	Uint32								m_nameCount;				// Autogenerated name count

public:
	IMaterialShaderCompiler() : m_nameCount(0) {};

	virtual ~IMaterialShaderCompiler() {};

	// Get implemented shader type
	virtual EMaterialShaderTarget GetShaderTarget() const=0;

	// Allocate shader local variable
	virtual CodeChunk Var( EMaterialDataType type, const CodeChunk& value )=0;

	// Allocate shader local variable
	virtual CodeChunk Var( EMaterialDataType type, Uint32 arrayCount, const CodeChunk& value )=0;

	// Allocate shader const
	virtual CodeChunk ConstReg( EMaterialDataType type, const AnsiChar* name )=0;

	// Allocate shader sampler
	virtual CodeChunk ConstSampler( EMaterialSamplerType type, const AnsiChar* name, Int32 constRegister )=0;

	// Allocate shader external parameter
	virtual CodeChunk Param( EMaterialDataType type, Int32 *allocatedRegister=NULL, const CodeChunk& name = CodeChunk::EMPTY, Int32 arrayCount = 0 )=0;

	// Allocate shader sampler
	virtual CodeChunk Texture( EMaterialSamplerType type, Int32 *allocatedRegister=NULL, const CodeChunk& name = CodeChunk::EMPTY )=0;

	// Allocate sampler state
	virtual CodeChunk SamplerState( ETextureAddressing addressU,
								ETextureAddressing addressV,
								ETextureAddressing addressW,
								ETextureFilteringMin filteringMin,
								ETextureFilteringMag filteringMag,
								ETextureFilteringMip filteringMip,
								ETextureComparisonFunction comparisonFunction )=0;

	// Macrodefinition
	virtual CodeChunk Macro( const CodeChunk& macro )=0;

	// Include a .fx file
	virtual CodeChunk Include( const CodeChunk& path )=0;

	// Allocate shader input
	virtual CodeChunk Input( EMaterialDataType type, const CodeChunk& semantic )=0;

	// Declare shader output
	virtual CodeChunk Output( EMaterialDataType type, const CodeChunk& semantic, const CodeChunk& outputValue )=0;

	// Request shader internal data
	virtual CodeChunk Data( const CodeChunk& semantic )=0;

	// Discard pixel fragment when value < 0
	virtual void Discard( const CodeChunk& value )=0;

	// Tessellation setup
	virtual void Tessellate( const CodeChunk& triangleSize, const CodeChunk& displacementMap, const CodeChunk& displacementScale, ETessellationDomain tessellationDomain, Bool vertexFactoryOverride )=0;

	// Print statement
	virtual void Statement( const CodeChunk& value, Bool prepend = false )=0;

	// Allocate automatic name
	CodeChunk AutomaticName() { return CodeChunk::Printf( true, "Var%i", m_nameCount++ ); }

	// Compiles default code for given pass. Return value indicates if default code have been generated - otherwise explicit implementation is needed.
	virtual Bool CompileRenderPassSimple( ERenderingPass pass )=0;

	// Compiles deferred shading GBuffer.
	virtual Bool CompileDeferredShadingGBuffer( const CodeChunk &diffuse, const CodeChunk &normal, const CodeChunk &vertexNormal, const CodeChunk &specularity, const CodeChunk &glossinessFactor, const CodeChunk &translucencyFactor, const CodeChunk &ambientOcclusion, const CodeChunk &subsurfaceScattering, const CodeChunk &materialFlagsMaskEncoded = CodeChunk(0.f) /*GBUFF_MATERIAL_MASK_DEFAULT*/ )=0;

	// Compiles deferred shading GBuffer for forward shaded stuff.
	virtual Bool CompileDeferredShadingForwardGBuffer( const CodeChunk &normal )=0;

	// Applies fragment clipping if needed
	virtual void CompileOptionalFragmentClipping( const MaterialRenderingContext &context, const CodeChunk& maskValue )=0;

	// Outputs MSAA coverage mask
	virtual void CompileMSAACoverageMask( const CodeChunk &alphaValue )=0;
	
	// Output code that branches if the two-sided lighting is required
	virtual CodeChunk PrepareForTwoSidedLighting( const CodeChunk& normal, const CodeChunk& worldNormal )=0;

	// Returns information whether compiler provides given feedback data
	virtual Bool IsFeedbackDataFetchSupported( ERenderFeedbackDataType feedbackType ) const=0;

	// Returns scene depth (if supported)
	virtual void CompileFeedbackDataFetch( ERenderFeedbackDataType feedbackType, CodeChunk &outData, const CodeChunk *coordOffset = NULL )=0;

	// Applies gamma into given value
	virtual CodeChunk ApplyGammaToLinearExponent( EMaterialDataType dataType, const CodeChunk &code, bool inverse, bool allowClampZero )=0;
};

/// Material compiler
class IMaterialCompiler
{
private:
	const MaterialRenderingContext*		m_context;			//!< Compilation context
	ECookingPlatform m_platform;

public:
	IMaterialCompiler( const MaterialRenderingContext& context, ECookingPlatform platform ) 
		: m_context( &context ), m_platform( platform ) {}

	virtual ~IMaterialCompiler() {}

	ECookingPlatform GetPlatform() const { return m_platform; }

	// Get compilation context
	const MaterialRenderingContext& GetContext() const { return *m_context; }

	// Get vertex shader compiler
	virtual IMaterialShaderCompiler* GetVertexShaderCompiler() = 0;

	// Get pixel shader compiler
	virtual IMaterialShaderCompiler* GetPixelShaderCompiler() = 0;

	virtual Uint64 GetVSCodeCRC() const = 0;
	virtual Uint64 GetPSCodeCRC() const = 0;

	// Connect vertex shader with pixel shader
	virtual void Connect( EMaterialDataType type, const CodeChunk& vsSource, const CodeChunk& psTarget, const CodeChunk& semantic ) = 0;

	// Create used parameter definition
	virtual void Param( const CName& name, Uint8 reg, EMaterialShaderTarget shaderTarget = MSH_PixelShader )=0;

	String GetMaterialParamTypeName(EMaterialParamType param)
	{
		switch(param)
		{
		case MPT_Float:
			return TXT("float");
		case MPT_Float2:
			return TXT("float2");
		case MPT_Float3:
			return TXT("float3");
		case MPT_Float4:
			return TXT("float4");
		case MPT_Texture:
			return TXT("sampler2D");
		default:
			ASSERT(false); //not implemented yet
			return TXT("");
		}
	}

	virtual Bool Function( const String& name, const String& code, const THashMap<String, EMaterialParamType>& params, EMaterialShaderTarget shaderTarget = MSH_PixelShader )=0;
};

/// Pixel shader compilator wrapper
#define PS_VAR_FLOAT( x ) compiler.GetPS().Var( MDT_Float, x )
#define PS_VAR_FLOAT2( x ) compiler.GetPS().Var( MDT_Float2, x )
#define PS_VAR_FLOAT3( x ) compiler.GetPS().Var( MDT_Float3, x )
#define PS_VAR_FLOAT4( x ) compiler.GetPS().Var( MDT_Float4, x )
#define PS_DATA( x ) compiler.GetPS().Data( x )
#define PS_CONST_FLOAT( name ) compiler.GetPS().ConstReg( MDT_Float, #name )
#define PS_CONST_FLOAT2( name ) compiler.GetPS().ConstReg( MDT_Float2, #name )
#define PS_CONST_FLOAT3( name ) compiler.GetPS().ConstReg( MDT_Float3, #name )
#define PS_CONST_FLOAT4( name ) compiler.GetPS().ConstReg( MDT_Float4, #name )
#define PS_CONST_FLOAT44( name ) compiler.GetPS().ConstReg( MDT_Float4x4, #name )
#define PS_CONST_FLOAT43( name ) compiler.GetPS().ConstReg( MDT_Float4x3, #name )
#define PS_CONST_FLOAT33( name ) compiler.GetPS().ConstReg( MDT_Float3x3, #name )
#define PS_SAMPLER_2D( registerIndex ) compiler.GetPS().Sampler( MST_Texture, registerIndex )
#define PS_SAMPLER_CUBE( registerIndex ) compiler.GetPS().Sampler( MST_Cubemap, registerIndex )
#define PS_SAMPLER_3D( registerIndex ) compiler.GetPS().Sampler( MST_Volume, registerIndex )
#define PS_SAMPLER_2D_ARRAY( registerIndex ) compiler.GetPS().Sampler( MST_TextureArray, registerIndex )

#define SHADER_VAR_FLOAT( x, target ) compiler.GetShader(target).Var( MDT_Float, x )
#define SHADER_VAR_FLOAT2( x, target ) compiler.GetShader(target).Var( MDT_Float2, x )
#define SHADER_VAR_FLOAT3( x, target ) compiler.GetShader(target).Var( MDT_Float3, x )
#define SHADER_VAR_FLOAT4( x, target ) compiler.GetShader(target).Var( MDT_Float4, x )
#define SHADER_DATA( x, target ) compiler.GetShader(target).Data( x )
#define SHADER_CONST_FLOAT( name, target ) compiler.GetShader(target).ConstReg( MDT_Float, #name )
#define SHADER_CONST_FLOAT2( name, target ) compiler.GetShader(target).ConstReg( MDT_Float2, #name )
#define SHADER_CONST_FLOAT3( name, target ) compiler.GetShader(target).ConstReg( MDT_Float3, #name )
#define SHADER_CONST_FLOAT4( name, target ) compiler.GetShader(target).ConstReg( MDT_Float4, #name )
#define SHADER_CONST_FLOAT44( name, target ) compiler.GetShader(target).ConstReg( MDT_Float4x4, #name )
#define SHADER_CONST_FLOAT43( name, target ) compiler.GetShader(target).ConstReg( MDT_Float4x3, #name )
#define SHADER_CONST_FLOAT33( name, target ) compiler.GetShader(target).ConstReg( MDT_Float3x3, #name )
#define SHADER_SAMPLER_2D( registerIndex, target ) compiler.GetShader(target).Sampler( MST_Texture, registerIndex )
#define SHADER_SAMPLER_CUBE( registerIndex, target ) compiler.GetShader(target).Sampler( MST_Cubemap, registerIndex )
#define SHADER_SAMPLER_3D( registerIndex, target ) compiler.GetShader(target).Sampler( MST_Volume, registerIndex )
#define SHADER_SAMPLER_2D_ARRAY( registerIndex, target ) compiler.GetShader(target).Sampler( MST_TextureArray, registerIndex )

#endif
