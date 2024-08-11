
#ifndef COMMON_FX_H_INCLUDED
#define COMMON_FX_H_INCLUDED

/// Common header for all dynamically compiled shaders
/// It's esential to recompile all shaders after changing any of those lines

// Realize global constants in compiled shaders

#define REG( _name, _reg, _type ) _type _name;
#define REGI( _name, _reg, _type ) _type _name : register( i##_reg );
#define REGB( _name, _reg, _type ) _type _name : register( b##_reg );
#define REG_ARRAY( _name,_reg, _type, _size )  _type _name[ _size ] : register( c##_reg );
#define SYS_SAMPLER( _name,_reg )	SamplerState	s_##_name	: register( s##_reg ); \
									Texture2D		t_##_name	: register( t##_reg );
#define SYS_TEXTURE_NO_SAMPLER( _name, _reg ) Texture2D		t_##_name	: register( t##_reg );
#define HALF_PIXEL_OFFSET 0.0f

// =====================================================================
// There are differences in sample functions names between HLSL and PSSL. Use ONLY macros below.
#define SYS_SAMPLE( _name, _coord )					t_##_name.Sample( s_##_name, _coord )
#define SYS_SAMPLE_TEXEL( _name, _coord )			t_##_name[ _coord ]
#define SAMPLE( _texture, _sampler, _coord )		_texture.Sample( _sampler, _coord )
#define GATHER_RED( _texture, _sampler, _coord )	_texture.GatherRed( _sampler, _coord )
#define GATHER_GREEN( _texture, _sampler, _coord )	_texture.GatherGreen( _sampler, _coord )
#define GATHER_BLUE( _texture, _sampler, _coord )	_texture.GatherBlue( _sampler, _coord )
#define GATHER_ALPHA( _texture, _sampler, _coord )	_texture.GatherAlpha( _sampler, _coord )

#ifdef __PSSL__
	#define SYS_SAMPLE_LEVEL( _name, _coord, _level )	t_##_name.SampleLOD( s_##_name, _coord, _level )
	#define SAMPLE_LEVEL( _texture, _sampler, _coord, _level )	_texture.SampleLOD( _sampler, _coord, _level )
	#define SAMPLE_CMP_LEVEL0( _texture, _sampler, _coord, _compareValue ) _texture.SampleCmpLOD0( _sampler, _coord, _compareValue )
	#define SAMPLE_GRADIENT( _texture, _sampler, _coord, _ddx, _ddy ) _texture.SampleGradient( _sampler, _coord, _ddx, _ddy )
	#define SYS_STATIC 
	#define SAMPLE_MIPMAPS( _texture, _miplevel, _location ) _texture.MipMaps[ _miplevel ][ _location ]
#else
	#define SYS_SAMPLE_LEVEL( _name, _coord, _level )	t_##_name.SampleLevel( s_##_name, _coord, _level )
	#define SAMPLE_LEVEL( _texture, _sampler, _coord, _level )	_texture.SampleLevel( _sampler, _coord, _level )
	#define SAMPLE_CMP_LEVEL0( _texture, _sampler, _coord, _compareValue ) _texture.SampleCmpLevelZero( _sampler, _coord, _compareValue )
	#define SAMPLE_GRADIENT( _texture, _sampler, _coord, _ddx, _ddy ) _texture.SampleGrad( _sampler, _coord, _ddx, _ddy )
	#define SYS_STATIC static
	#define SAMPLE_MIPMAPS( _texture, _miplevel, _location ) _texture.mips[_miplevel][ _location ]
#endif

// ============================
// Constant buffer block defines

#ifdef __PSSL__
	#define START_CB( _name, _reg )	ConstantBuffer	_name : register( b##_reg )	 {		// PSSL
#else
	#define START_CB( _name, _reg )	cbuffer			_name : register( b##_reg )	 {		// DX11
#endif

#define END_CB  }																	// PSSL + DX11

// ============================
// PER RENDERING API SEMANTICS DEFINES

#ifdef __PSSL__
	#define SYS_POSITION					S_POSITION
	#define SYS_TARGET_OUTPUT0				S_TARGET_OUTPUT0
	#define SYS_TARGET_OUTPUT1				S_TARGET_OUTPUT1
	#define SYS_TARGET_OUTPUT2				S_TARGET_OUTPUT2
	#define SYS_TARGET_OUTPUT3				S_TARGET_OUTPUT3
	#define SYS_TARGET_OUTPUT4				S_TARGET_OUTPUT4
	#define SYS_TARGET_OUTPUT5				S_TARGET_OUTPUT5
	#define SYS_TARGET_OUTPUT6				S_TARGET_OUTPUT6
	#define SYS_TARGET_OUTPUT7				S_TARGET_OUTPUT7
	#define SYS_DEPTH_OUTPUT				S_DEPTH_OUTPUT
	#define SYS_VERTEX_ID					S_VERTEX_ID
	#define SYS_INSTANCE_ID					S_INSTANCE_ID
	#define SYS_PRIMITIVE_ID				S_PRIMITIVE_ID
	#define SYS_GSINSTANCE_ID				S_GSINSTANCE_ID
	#define SYS_OUTPUT_CONTROL_POINT_ID		S_OUTPUT_CONTROL_POINT_ID
	#define SYS_EDGE_TESS_FACTOR			S_EDGE_TESS_FACTOR
	#define SYS_INSIDE_TESS_FACTOR			S_INSIDE_TESS_FACTOR
	#define SYS_DOMAIN_LOCATION				S_DOMAIN_LOCATION
	#define SYS_FRONT_FACE					S_FRONT_FACE
	#define SYS_COVERAGE					S_COVERAGE
	#define SYS_CLIP_DISTANCE0				S_CLIP_DISTANCE0
	#define SYS_CLIP_DISTANCE1				S_CLIP_DISTANCE1
	#define SYS_CULL_DISTANCE0				S_CULL_DISTANCE0
	#define SYS_CULL_DISTANCE1				S_CULL_DISTANCE1
	#define SYS_RENDER_TARGET_ARRAY_INDEX	S_RENDER_TARGET_INDEX
	#define SYS_VIEWPORT_ARRAY_INDEX		S_VIEWPORT_INDEX
	#define SYS_DISPATCH_THREAD_ID			S_DISPATCH_THREAD_ID
	#define SYS_GROUP_ID					S_GROUP_ID
	#define SYS_GROUP_INDEX					S_GROUP_INDEX
	#define SYS_GROUP_THREAD_ID				S_GROUP_THREAD_ID

	// textures
	#define TEXTURE2D						Texture2D
	#define TEXTURE2D_MS					MS_Texture2D
	#define TEXTURE2D_ARRAY					Texture2D_Array
	#define TEXTURE2D_ARRAY_MS				MS_Texture2D_Array
	#define TEXTURECUBE						TextureCube
	#define TEXTURECUBE_ARRAY				TextureCube_Array
	#define RW_TEXTURE2D					RW_Texture2D
	#define RW_TEXTURE2D_ARRAY				RW_Texture2D_Array
	#define BYTEBUFFER						ByteBuffer
	#define RW_BYTEBUFFER					RW_ByteBuffer
	#define DATABUFFER(_type)				DataBuffer< _type >
	#define RW_DATABUFFER(_type)			RW_DataBuffer< _type >
	#define STRUCTBUFFER(_type)				RegularBuffer< _type >
	#define RW_STRUCTBUFFER(_type)			RW_RegularBuffer< _type >

	// hull,domain,geometry,compute shaders
	//#define MAX_VERTEX_COUNT				MAX_VERTEX_COUNT
	#define GS_INPUT_TRIANGLE				Triangle
	#define GS_INPUT_POINT					Point
	#define GS_BUFFER_POINT					PointBuffer
	#define GS_BUFFER_LINE					LineBuffer
	#define GS_BUFFER_TRIANGLE				TriangleBuffer
	//#define DOMAIN_PATCH_TYPE				DOMAIN_PATCH_TYPE
	#define HS_PARTITIONING					PARTITIONING_TYPE
	#define HS_OUTPUT_TOPOLOGY				OUTPUT_TOPOLOGY_TYPE
	#define HS_OUTPUT_CONTROL_POINTS		OUTPUT_CONTROL_POINTS
	#define HS_PATCH_CONSTANT_FUNC			PATCH_CONSTANT_FUNC
	#define HS_MAX_TESS_FACTOR				MAX_TESS_FACTOR

	#define NUMTHREADS						NUM_THREADS
	#define GROUPSHARED						thread_group_memory
	#define	NOINTERPOLATION					nointerp

	// synchronization
	#define INTERLOCKED_ADD( _dest, _val, _prev )	AtomicAdd( _dest, _val, _prev )
	#define INTERLOCKED_MIN( _dest, _val )	AtomicMin( _dest, _val )
	#define INTERLOCKED_MAX( _dest, _val )	AtomicMax( _dest, _val )

	#define GROUP_BARRIER_GROUP_SYNC		ThreadGroupMemoryBarrierSync()

	// Bitwise ops
	#define REVERSE_BITS					ReverseBits
	
	// attributes
	#define EARLY_DEPTH_STENCIL				FORCE_EARLY_DEPTH_STENCIL
	
	// parameter modifiers
	#define nointerpolation					nointerp
	#define noperspective					nopersp
	#define PARAM_NOINTERP					nointerp
	#define PARAM_NOPERSP					nopersp
	
#else

	#define SYS_POSITION					SV_Position
	#define SYS_TARGET_OUTPUT0				SV_Target0
	#define SYS_TARGET_OUTPUT1				SV_Target1
	#define SYS_TARGET_OUTPUT2				SV_Target2
	#define SYS_TARGET_OUTPUT3				SV_Target3
	#define SYS_TARGET_OUTPUT4				SV_Target4
	#define SYS_TARGET_OUTPUT5				SV_Target5
	#define SYS_TARGET_OUTPUT6				SV_Target6
	#define SYS_TARGET_OUTPUT7				SV_Target7
	#define SYS_DEPTH_OUTPUT				SV_Depth
	#define SYS_VERTEX_ID					SV_VertexID
	#define SYS_INSTANCE_ID					SV_InstanceID
	#define SYS_PRIMITIVE_ID				SV_PrimitiveID
	#define SYS_GSINSTANCE_ID				SV_GSInstanceID
	#define SYS_OUTPUT_CONTROL_POINT_ID		SV_OutputControlPointID
	#define SYS_EDGE_TESS_FACTOR			SV_TessFactor
	#define SYS_INSIDE_TESS_FACTOR			SV_InsideTessFactor
	#define SYS_DOMAIN_LOCATION				SV_DomainLocation
	#define SYS_FRONT_FACE					SV_IsFrontFace
	#define SYS_COVERAGE					SV_Coverage
	#define SYS_CLIP_DISTANCE0				SV_ClipDistance0
	#define SYS_CLIP_DISTANCE1				SV_ClipDistance1
	#define SYS_CULL_DISTANCE0				SV_CullDistance0
	#define SYS_CULL_DISTANCE1				SV_CullDistance1
	#define SYS_RENDER_TARGET_ARRAY_INDEX	SV_RenderTargetArrayIndex	
	#define SYS_VIEWPORT_ARRAY_INDEX		SV_ViewportArrayIndex
	#define SYS_DISPATCH_THREAD_ID			SV_DispatchThreadID
	#define SYS_GROUP_ID					SV_GroupID
	#define SYS_GROUP_INDEX					SV_GroupIndex
	#define SYS_GROUP_THREAD_ID				SV_GroupThreadID
	
	// textures
	#define TEXTURE2D						Texture2D
	#define TEXTURE2D_MS					Texture2DMS
	#define TEXTURE2D_ARRAY					Texture2DArray
	#define TEXTURE2D_ARRAY_MS				Texture2DMSArray
	#define TEXTURECUBE						TextureCube
	#define TEXTURECUBE_ARRAY				TextureCubeArray
	#define RW_TEXTURE2D					RWTexture2D
	#define RW_TEXTURE2D_ARRAY				RWTexture2DArray
	#define BYTEBUFFER						ByteAddressBuffer
	#define RW_BYTEBUFFER					RWByteAddressBuffer
	#define DATABUFFER(_type)				Buffer< _type >
	#define RW_DATABUFFER(_type)			RWBuffer< _type >
	#define	STRUCTBUFFER(_type)				StructuredBuffer< _type >
	#define RW_STRUCTBUFFER(_type)			RWStructuredBuffer< _type >

	// hull,domain,geometry,compute shaders
	#define MAX_VERTEX_COUNT				maxvertexcount
	#define GS_INPUT_TRIANGLE				triangle
	#define GS_INPUT_POINT					point
	#define GS_BUFFER_POINT					PointStream
	#define GS_BUFFER_LINE					LineStream
	#define GS_BUFFER_TRIANGLE				TriangleStream
	#define DOMAIN_PATCH_TYPE				domain
	#define HS_PARTITIONING					partitioning
	#define HS_OUTPUT_TOPOLOGY				outputtopology
	#define HS_OUTPUT_CONTROL_POINTS		outputcontrolpoints
	#define HS_PATCH_CONSTANT_FUNC			patchconstantfunc
	#define HS_MAX_TESS_FACTOR				maxtessfactor
	#define NUMTHREADS						numthreads
	#define GROUPSHARED						groupshared
	#define	NOINTERPOLATION					nointerpolation

	// synchronization
	#define INTERLOCKED_ADD( _dest, _val, _prev )	InterlockedAdd( _dest, _val, _prev )
	#define INTERLOCKED_MIN( _dest, _val )	InterlockedMin( _dest, _val )
	#define INTERLOCKED_MAX( _dest, _val )	InterlockedMax( _dest, _val )

	#define GROUP_BARRIER_GROUP_SYNC		GroupMemoryBarrierWithGroupSync()

	// Bitwise ops
	#define REVERSE_BITS					reversebits
	
	// attributes
	#define EARLY_DEPTH_STENCIL				earlydepthstencil

	// parameter modifiers
	#define PARAM_NOINTERP					nointerpolation
	#define PARAM_NOPERSP					noperspective
	
#endif

// ============================

#define PI				3.14159265358979323846264338327f
#define HALF_PI			1.570796326794896619231321691635f
#define DEG2RAD( x ) ( ((x) / 180.0f) * PI )

// Wrapper for custom user defined constants
#define custom_register( x ) : register(c##x)

// Render states, defined by compiler
//#define RS_TERRAIN_TOOL_ACTIVE
//#define RS_USE_SHADOW_MASK
//#define RS_PASS_NO_LIGHTING
//#define RS_PASS_HIT_PROXIES
//#define RS_PASS_DEFERRED_LIGHTING
//#define RS_PASS_BASE_LIGHTING
//#define RS_PASS_POINT_LIGHTING
//#define RS_PASS_SPOT_LIGHTING
//#define RS_PASS_SHADOW_DEPTH
//#define RS_PASS_LIGHT_PRESPASS
//#define RS_TWOSIDED

// Material states, defined by compiler
//#define MS_SELECTED

#endif
