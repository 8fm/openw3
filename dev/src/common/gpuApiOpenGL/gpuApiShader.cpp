/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "gpuApi.h"
#include "../gpuApiUtils/gpuApiMemory.h"
#include "../redSystem/hash.h"

#ifdef GPU_API_DEBUG_PATH
	#include <wchar.h>
#endif
#include <string>
#include "../redSystem/crt.h"
#include "../gpuApiUtils/gpuApiShaderParsing.h"

// HACK HACK

namespace GpuApi
{	
	void AddRef( const ShaderRef &shader )
	{
		GPUAPI_ASSERT( GetDeviceData().m_Shaders.IsInUse(shader) );
		GetDeviceData().m_Shaders.IncRefCount( shader );
	}

	Int32 Release( const ShaderRef &shader )
	{
		GPUAPI_ASSERT( shader );

		SDeviceData &dd = GetDeviceData();
		GPUAPI_ASSERT( dd.m_Shaders.IsInUse(shader) );
		GPUAPI_ASSERT( dd.m_Shaders.GetRefCount(shader) >= 1 );
		if ( 0 == dd.m_Shaders.DecRefCount( shader ) )
		{
			SShaderData &data = dd.m_Shaders.Data( shader );

			// Release resources
			GPUAPI_ASSERT( 0 != data.m_shader );

			if ( data.m_shader != 0 )

			{
				OGL_CHK( glDeleteProgram( data.m_shader ) );
				data.m_shader = 0;
			}

			// Destroy shit
			dd.m_Shaders.Destroy( shader );
		}

		return 0;
	}

	eShaderLanguage GetShaderLanguage()
	{
		return SL_GLSL;
	}

	Bool GetShaderTargetAndEntryPoint( eShaderType shaderType, const AnsiChar*& outShaderTarget, const AnsiChar*& outMainFunction )
	{
		switch ( shaderType )
		{
		case GpuApi::PixelShader:
			outMainFunction = "main";
			outShaderTarget = "gl_fs";
			break;
		case GpuApi::VertexShader:
			outMainFunction = "main";
			outShaderTarget = "gl_vs";
			break;
		case GpuApi::DomainShader:
			outMainFunction = "main";
			outShaderTarget = "gl_ds";
			break;
		case GpuApi::ComputeShader:
			outMainFunction = "main";
			outShaderTarget = "gl_cs";
			break;
		case GpuApi::GeometryShader:
			outMainFunction = "main";
			outShaderTarget = "gl_gs";
			break;
		case GpuApi::HullShader:
			outMainFunction = "main";
			outShaderTarget = "gl_hs";
			break;
		default:
			GPUAPI_HALT( "Invalid shader type" );
			return false;
		}
		return true;
	}

	Uint64 GetShaderHash( const char* code, const char* mainFunction, ShaderDefine* defines, const char* fileName )
	{
//		// Compile shader code
//		ID3DBlob* errorBuffer = NULL;
//		ID3DBlob* codeBuffer = NULL;
//
//		Uint32 flags = D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY;
//#ifdef _DEBUG
//		flags |= D3DCOMPILE_DEBUG;
//#endif
//
//		ShaderIncludeHandler includeHandler;
//
//#ifdef RED_PLATFORM_DURANGO
//		char* preprocessedCode = nullptr;
//		size_t size;
//		includeHandler.Preprocess( code, &preprocessedCode, size );
//
//		HRESULT hRet = D3DPreprocess( preprocessedCode, Red::System::StringLength( preprocessedCode )+1, 
//			fileName, 
//			(D3D_SHADER_MACRO*)defines, 
//			nullptr, 
//			&codeBuffer, &errorBuffer);
//
//		GPU_API_FREE( GpuMemoryPool_ShaderInclude, MC_Temporary, preprocessedCode );
//#else
//		HRESULT hRet = D3DPreprocess( code, Red::System::StringLength( code )+1, 
//			fileName, 
//			(D3D_SHADER_MACRO*)defines, 
//			(ID3DInclude*)&includeHandler, 
//			&codeBuffer, &errorBuffer);
//#endif
//
//		RED_UNUSED(hRet);

		Uint64 shaderHash = 0;

		//if ( codeBuffer )
		//{
		//	void* codeBuf = codeBuffer->GetBufferPointer();
		//	size_t codeSize = codeBuffer->GetBufferSize();
		//	
		//	shaderHash = Red::System::CalculateHash64( codeBuf, static_cast< GpuApi::Uint32 >( codeSize ), shaderHash );
		//	shaderHash = Red::System::CalculateHash64( mainFunction, static_cast< GpuApi::Uint32 >( Red::System::StringLength(mainFunction) ), shaderHash );
		//	SAFE_RELEASE( codeBuffer );
		//}

		//SAFE_RELEASE( errorBuffer );

		return shaderHash;
	}



	//static void FillStreamOutDeclaration( D3D11_SO_DECLARATION_ENTRY& declaration, const VertexPacking::PackingElement& element, VertexPacking::PackingElement* adjustedElement )
	//{
	//	// Translate usage
	//	const char* semanticName;
	//	Uint32 semanticIndex;
	//	if ( !MapPackingElementToSemanticAndIndex( element, semanticName, semanticIndex ) )
	//	{
	//		semanticName = nullptr;
	//		semanticIndex = 0;
	//	}


	//	Uint8 componentCount = 0;
	//	switch ( element.m_type )
	//	{
	//	case VertexPacking::PT_Float1:
	//	case VertexPacking::PT_UShort1:
	//	case VertexPacking::PT_Short1:
	//	case VertexPacking::PT_UByte1:
	//	case VertexPacking::PT_Index16:
	//	case VertexPacking::PT_Index32:
	//	case VertexPacking::PT_UInt1:
	//	case VertexPacking::PT_Int1:
	//		componentCount = 1;
	//		break;

	//	case VertexPacking::PT_Float2:
	//	case VertexPacking::PT_Float16_2:
	//	case VertexPacking::PT_UShort2:
	//	case VertexPacking::PT_Short2:
	//	case VertexPacking::PT_UInt2:
	//	case VertexPacking::PT_Int2:
	//		componentCount = 2;
	//		break;

	//	case VertexPacking::PT_Float3:
	//	case VertexPacking::PT_UInt3:
	//	case VertexPacking::PT_Int3:
	//		componentCount = 3;
	//		break;

	//	case VertexPacking::PT_Float4:
	//	case VertexPacking::PT_Float16_4:
	//	case VertexPacking::PT_UShort4:
	//	case VertexPacking::PT_Short4:
	//	case VertexPacking::PT_Short4N:
	//	case VertexPacking::PT_UInt4:
	//	case VertexPacking::PT_Int4:
	//	case VertexPacking::PT_Color:
	//	case VertexPacking::PT_UByte4:
	//	case VertexPacking::PT_UByte4N:
	//	case VertexPacking::PT_Dec4:
	//		componentCount = 4;
	//		break;
	//	}

	//	// Initialize the element list
	//	declaration.SemanticName = semanticName;
	//	declaration.SemanticIndex = semanticIndex;
	//	declaration.StartComponent = 0;
	//	declaration.ComponentCount = componentCount;


	//	// Fill adjusted element. Since SO only seems to work on 32-bit register values, some requested types will not work.
	//	// So, we translate those to acceptable types and return them as "suggested" formats. This should then be usable as
	//	// an vertex format for drawing from the resulting SO buffer.
	//	if ( adjustedElement )
	//	{
	//		*adjustedElement = element;

	//		switch ( adjustedElement->m_type )
	//		{
	//		case VertexPacking::PT_UShort1:
	//		case VertexPacking::PT_UByte1:
	//			adjustedElement->m_type = VertexPacking::PT_UInt1;
	//			break;

	//		case VertexPacking::PT_Short1:
	//			adjustedElement->m_type = VertexPacking::PT_Int1;
	//			break;

	//		case VertexPacking::PT_Float16_2:
	//			adjustedElement->m_type = VertexPacking::PT_Float2;
	//			break;
	//		case VertexPacking::PT_UShort2:
	//			adjustedElement->m_type = VertexPacking::PT_UInt2;
	//			break;
	//		case VertexPacking::PT_Short2:
	//			adjustedElement->m_type = VertexPacking::PT_Int2;
	//			break;

	//		case VertexPacking::PT_Float16_4:
	//		case VertexPacking::PT_Color:
	//		case VertexPacking::PT_Short4N:
	//		case VertexPacking::PT_UByte4N:
	//			adjustedElement->m_type = VertexPacking::PT_Float4;
	//			break;

	//		case VertexPacking::PT_UShort4:
	//		case VertexPacking::PT_UByte4:
	//			adjustedElement->m_type = VertexPacking::PT_UInt4;
	//			break;
	//		case VertexPacking::PT_Short4:
	//		case VertexPacking::PT_Dec4:
	//			adjustedElement->m_type = VertexPacking::PT_Int4;
	//			break;
	//		}
	//	}
	//}

//	static Uint32 CreateGeometryShaderWithSOInternal( ID3DBlob* compiledCode, const char* debugName, Uint32 debugNameLength, const VertexLayoutDesc& outputDesc, GpuApi::VertexLayoutDesc* adjustedDesc )
//	{
//		SDeviceData &dd = GetDeviceData();
//
//		ID3D11DeviceChild* shader = NULL;
//
//		// Create GpuApi shader
//		Uint32 newShaderId = dd.m_Shaders.Create( 0 );
//		if ( !newShaderId )
//		{
//			GPUAPI_HALT( TXT( "Failed to create gpuapi shader" ) );
//			return ShaderRef::Null();
//		}
//		GPUAPI_ASSERT( dd.m_Shaders.IsInUse( newShaderId ) );
//
//		// Process output layout
//		D3D11_SO_DECLARATION_ENTRY outputLayout[ GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS ];
//		Uint32 outputLayoutSize = 0;
//		Uint32 bufferStrides[ GPUAPI_VERTEX_LAYOUT_MAX_SLOTS ] = {0};
//		Uint8 bufferStridesSize = 0;
//
//		for ( Uint32 i = 0; i < GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS; ++i )
//		{
//			if ( outputDesc.m_elements[ i ].IsEmpty() )
//			{
//				break;
//			}
//
//			FillStreamOutDeclaration( outputLayout[ i ], outputDesc.m_elements[ i ], adjustedDesc ? &adjustedDesc->m_elements[ i ] : nullptr );
//
//			outputLayout[ i ].Stream = 0;
//
//			Uint8 slot = outputDesc.m_elements[ i ].m_slot;
//			outputLayout[ i ].OutputSlot = slot;
//
//			bufferStrides[ slot ] += outputLayout[ i ].ComponentCount * 4;
//
//			if ( bufferStridesSize < slot + 1 )
//			{
//				bufferStridesSize = slot + 1;
//			}
//
//			++outputLayoutSize;
//		}
//
//		// Create shader
//		HRESULT hRet = GetDevice()->CreateGeometryShaderWithStreamOutput( compiledCode->GetBufferPointer(), compiledCode->GetBufferSize(), outputLayout, outputLayoutSize, bufferStrides, bufferStridesSize, D3D11_SO_NO_RASTERIZED_STREAM, NULL, ( ID3D11GeometryShader**) &shader );
//		GPUAPI_ASSERT( SUCCEEDED( hRet ) );
//
//#ifdef GPU_API_DEBUG_PATH
//		if (shader)
//		{
//			shader->SetPrivateData( WKPDID_D3DDebugObjectName, debugNameLength, debugName );
//		}
//#endif
//		// Initialize new buffer
//		SShaderData &data = dd.m_Shaders.Data( newShaderId );
//		data.m_type = GeometryShader;
//		data.m_pShader = shader;
//		data.m_byteCode = compiledCode;
//
//		// Finalize
//		return newShaderId;
//	}

	ShaderRef CreateGeometryShaderWithSOFromSource( const char* code, const char* mainFunction, const char* shaderTarget, ShaderDefine* defines, const char* fileName, const VertexLayoutDesc& outputDesc, GpuApi::VertexLayoutDesc* adjustedDesc /*nullptr*/ )
	{
		return ShaderRef::Null();

//		// Compile shader code
//		ID3DBlob* errorBuffer = NULL;
//		ID3DBlob* codeBuffer = NULL;
//
//		Uint32 flags = D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY;
//#ifdef _DEBUG
//		flags |= D3DCOMPILE_DEBUG;
//#endif
//
//		ShaderIncludeHandler includeHandler;
//
//#ifdef RED_PLATFORM_DURANGO
//		char* preprocessedCode = nullptr;
//		size_t size;
//		includeHandler.Preprocess( code, &preprocessedCode, size );
//
//		HRESULT hRet = D3DCompile( preprocessedCode, Red::System::StringLength( preprocessedCode )+1, 
//			fileName, 
//			(D3D_SHADER_MACRO*)defines, 
//			nullptr,
//			mainFunction, shaderTarget, 
//			flags, 0, 
//			&codeBuffer, &errorBuffer );
//
//		GPU_API_FREE( GpuMemoryPool_ShaderInclude, MC_Temporary, preprocessedCode );
//
//#else
//		HRESULT hRet = D3DCompile( code, Red::System::StringLength( code )+1, 
//			fileName, 
//			(D3D_SHADER_MACRO*)defines, 
//			(ID3DInclude*)&includeHandler, 
//			mainFunction, shaderTarget, 
//			flags, 0, 
//			&codeBuffer, &errorBuffer );
//#endif
//
//		// Print error message to log
//		if ( errorBuffer )
//		{
//			// Has the shader compilation failed ?
//			if ( FAILED( hRet ) )
//			{
//				GPUAPI_ERROR
//				(
//					TXT( "\nShader error(s):\n" )
//					TXT( "%" ) RED_PRIWas
//					TXT( "\n==================================================" ),
//					errorBuffer->GetBufferPointer()
//				);
//			} 
//#ifdef _DEBUG
//			else
//			{
//				GPUAPI_WARNING
//				(
//					TXT( "\nShader warning(s):\n" )
//					TXT( "%" ) RED_PRIWas
//					TXT( "\n==================================================" ),
//					errorBuffer->GetBufferPointer()
//				);
//			}
//#endif
//			// Cleanup
//			errorBuffer->Release();
//		}
//
//		// No shader compiled so far, exit
//		if ( FAILED( hRet ) )
//		{
//			return ShaderRef::Null();
//		}
//
//
//		const Uint32 len = static_cast< Uint32 >( Red::System::StringLength(fileName)+1 );
//
//		Uint32 newShaderId = CreateGeometryShaderWithSOInternal( codeBuffer, fileName, len, outputDesc, adjustedDesc );
//
//		// Finalize
//		return ShaderRef( newShaderId );
	}

	ShaderRef CreateGeometryShaderWithSOFromBinary( const void* shaderBuffer, Uint32 shaderBufferSize, const VertexLayoutDesc& outputDesc, GpuApi::VertexLayoutDesc* adjustedDesc /*nullptr*/ )
	{
		return ShaderRef::Null();

		//HRESULT hRet;
		//ID3DBlob* codeBufferBlob = NULL;
		//hRet = D3DCreateBlob( shaderBufferSize, &codeBufferBlob );
		//if ( FAILED( hRet ) )
		//{
		//	GPUAPI_HALT( TXT( "Failed to allocate memory for gpuapi shader" ) );
		//	return ShaderRef::Null();
		//}
		//Red::System::MemoryCopy( codeBufferBlob->GetBufferPointer(), shaderBuffer, shaderBufferSize );

		//// Create shader
		//Uint32 newShaderId = CreateGeometryShaderWithSOInternal( codeBufferBlob, "shaderFromCache", 15, outputDesc, adjustedDesc );

		//// Finalize
		//return ShaderRef( newShaderId );
	}

	inline static GLenum Map( eShaderType shaderType )
	{
		switch (shaderType)
		{
		case VertexShader:				return GL_VERTEX_SHADER;
		case PixelShader:				return GL_FRAGMENT_SHADER;
		case GeometryShader:			return GL_GEOMETRY_SHADER;
		case HullShader:				return GL_TESS_CONTROL_SHADER;
		case DomainShader:				return GL_TESS_EVALUATION_SHADER; 
		case ComputeShader:				return GL_COMPUTE_SHADER;
		default:
			RED_ASSERT( !"Uknown shadery type" );
		}
		return GL_NONE;
	}

	// MOJOShader seems to remove new line after #version XXX directive
	// This totally non  hacky functions inserts new line \n after version

	// lolwut - isblank only since C++11. Is this file not C11 standard ?
	static bool isblank( char c ) { return (c == ' ') || (c == '\t'); }

	void FixVersionNewLine( char* code )
	{
		// Find first '#'
		char* hash = strchr( code, '#' );
		if( hash == nullptr )	return;

		// Find following 'version'
		char* version = strstr( hash, "version" );
		if( version == nullptr )	return;

		char* ptr = version + 7;

#define EAT(a)				while( *ptr && (a) ) { ++ptr; } if( ! *ptr ) return
#define CHECK_ENDING(a)														\
	if( strncmp( ptr, #a, Red::System::StringLengthCompileTime(#a) ) == 0 )	\
		{																	\
		endVersion = ptr + Red::System::StringLengthCompileTime(#a);		\
		}

		// Eat spaces
		EAT( isblank(*ptr) );

		// Eat digits of version
		EAT( isdigit( *ptr ) );

		char* endVersion = ptr;

		// Eat spaces
		EAT( isblank(*ptr) );

		// Version endings
		CHECK_ENDING( core );
		CHECK_ENDING( compatibility );

		// Insert missing new line
		if( isblank( *endVersion ) )
		{
			*endVersion = '\n';
		}

#undef EAT
#undef CHECK_ENDING

	}

	char* FixExtensionNewLine( char* code )
	{
		char* ext = strstr( code, "extension" );
		if( ext == nullptr )	return nullptr;

		char* enabled = strstr( ext, "enable" );
		if( enabled == nullptr )	return nullptr;

		enabled += Red::System::StringLengthCompileTime( "enable" );
		if( isblank( *enabled ) )
		{
			*enabled = '\n';
		}

		return enabled + 1;
	}

	void FixNewLine( char* code )
	{
		FixVersionNewLine( code );

		char * ptr = code;
		while( ptr )
		{
			ptr = FixExtensionNewLine( ptr );
		}

	}

	ShaderRef CreateShaderFromSource( eShaderType shaderType, const char* code, const char* mainFunction, const char* shaderTarget, ShaderDefine* defines, Uint32 numDefines, const char* fileName )
	{
		// Create shader
		const GLuint shaderID = OGL_CHK( glCreateShader( Map(shaderType) ) );
		if( shaderID == 0 )
		{
			GPUAPI_HALT( "Failed to create gpuapi shader" );
			return ShaderRef::Null();
		}

		AnsiChar* preprocessedCode;
		Uint32 preprocessedLength;
		
		// Build proper source code (include all the things!!!)
		if ( !GpuApi::Preprocess( code, defines, numDefines, fileName, preprocessedCode, preprocessedLength ) )
		{
			GPUAPI_HALT( "Coudn't parse shader source" );
			return ShaderRef::Null();
		}

		FixNewLine( preprocessedCode );

		// Attach source and compile
		OGL_CHK( glShaderSource(shaderID, 1, &preprocessedCode, NULL) );
		OGL_CHK( glCompileShader(shaderID) );

		GPU_API_FREE( GpuMemoryPool_ShaderInclude, MC_Temporary, preprocessedCode );

		// Check compilation status
		GLint compiled = GL_FALSE;
		OGL_CHK( glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compiled) );
		if( !compiled )
		{
			GLchar errorMessage[1024];
			OGL_CHK( glGetShaderInfoLog( shaderID , 1024 , nullptr , errorMessage ) );
			OGL_CHK( glDeleteShader(shaderID) );

			GPUAPI_ERROR( TXT("Shader compiling failed with error message: %hs"), errorMessage );
			return ShaderRef::Null();
		}

		// Create new shader program
		const GLuint program = OGL_CHK( glCreateProgram() );
		if( !program )
		{
			OGL_CHK( glDeleteShader(shaderID) );

			GPUAPI_HALT( "Failed to create gpuapi program" );
			return ShaderRef::Null();
		}

		// Make this program seperable
		OGL_CHK( glProgramParameteri(program, GL_PROGRAM_SEPARABLE, GL_TRUE) );

		// Attach program and link
		OGL_CHK( glAttachShader(program, shaderID) );

		////////////////////////////////////////////////////////////////////////////
		// TODO: Here must goes all pre Link-Time bindings if any ( or use source
		// code based (binding location=N) semantics
		////////////////////////////////////////////////////////////////////////////

		// Set this program be able to use local-machine cashe
		OGL_CHK( glProgramParameteri( program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE ) );

		// Link whole program
		OGL_CHK( glLinkProgram(program) );

		// Detach and delete shader
		OGL_CHK( glDetachShader(program, shaderID) );
		OGL_CHK( glDeleteShader(shaderID) );

		// Check link status
		GLint result = GL_FALSE;
		OGL_CHK( glGetProgramiv( program, GL_LINK_STATUS, &result ) );
		if ( result == GL_FALSE )
		{
			GLchar errorMessage[1024];
			OGL_CHK( glGetProgramInfoLog( program, 1024, nullptr, &errorMessage[0] ) );

			OGL_CHK( glDeleteProgram(program) );

			GPUAPI_ERROR( TXT("Shader linking failed with error message: %hs"), errorMessage );

			return ShaderRef::Null();
		}

		SDeviceData &dd = GetDeviceData();
		const Uint32 newShaderId = dd.m_Shaders.Create( 0 );
		if ( !newShaderId )
		{
			GPUAPI_HALT( "Failed to create gpuapi shader" );
			return ShaderRef::Null();
		}

		// Initialize new buffer
		SShaderData &data = dd.m_Shaders.Data( newShaderId );
		data.m_type = shaderType;
		data.m_shader = program;

#ifdef GPU_API_DEBUG_PATH
		if (shader)
		{
			const Uint32 debugNameLength = static_cast< Uint32 >( Red::System::StringLength(fileName)+1 );
			mamcpy( data.m_debugPath, filename, debugNameLength );
#error copy too much?
		}
#endif

		// Finalize
		return ShaderRef( newShaderId );
	}

	ShaderRef CreateShaderFromBinary( eShaderType shaderType, const void* shaderBuffer, Uint32 shaderBufferSize, const char* debugName )
	{
		// Create new shader program
		const GLuint program = OGL_CHK( glCreateProgram() );
		if( !program )
		{
			GPUAPI_HALT( "Failed to create gpuapi program" );
			return ShaderRef::Null();
		}

		// Upload data
		{
			GLenum formatType	= *(GLenum*)( shaderBuffer );
			void* dataPtr		= (void*)( (Uint8*)shaderBuffer + sizeof( GLenum ) );

			OGL_CHK( glProgramBinary( program, formatType, dataPtr, shaderBufferSize - sizeof(GLenum ) ) );

			GLint link = GL_FALSE;
			OGL_CHK( glGetProgramiv( program, GL_LINK_STATUS, &link ) );

			if( ! link )
			{
				OGL_CHK( glDeleteProgram(program) );

				GPUAPI_HALT( "Failed to create gpuapi program from binary" );
				return ShaderRef::Null();
			}

		}

		SDeviceData &dd = GetDeviceData();
		const Uint32 newShaderId = dd.m_Shaders.Create( 0 );
		if ( !newShaderId )
		{
			GPUAPI_HALT( "Failed to create gpuapi shader" );
			return ShaderRef::Null();
		}

		// Initialize new buffer
		SShaderData &data = dd.m_Shaders.Data( newShaderId );
		data.m_type = shaderType;
		data.m_shader = program;

#ifdef GPU_API_DEBUG_PATH
		if (shader && debugName)
		{
			const Uint32 debugNameLength = static_cast< Uint32 >( Red::System::StringLength(debugName)+1 );
			mamcpy( data.m_debugPath, debugName, debugNameLength );
#error copy too much?
		}
#endif

		return ShaderRef( newShaderId );
	}

	Uint32 GetShaderCodeSize( const ShaderRef& shader )
	{
		SDeviceData &dd = GetDeviceData();

		if ( !shader.isNull() )
		{
			SShaderData &data = dd.m_Shaders.Data( shader );

			GLint len = 0;
			OGL_CHK( glGetProgramiv( data.m_shader, GL_PROGRAM_BINARY_LENGTH, &len ) );
			
			// Add 4 bytes for storing GLenumformatType later

			return len + sizeof( GLenum );
		}

		return 0;
	}

	void CopyShaderCode( const ShaderRef& shader, void* targetCode )
	{
		SDeviceData &dd = GetDeviceData();

		if( Uint32 codeSize = GetShaderCodeSize(shader) )
		{
			SShaderData &data = dd.m_Shaders.Data( shader );

			GLenum*	formatType	= (GLenum*)( targetCode );
			void* dataPtr		= (Uint8*)targetCode + sizeof( GLenum );

			OGL_CHK( glGetProgramBinary( data.m_shader, codeSize, nullptr, formatType, dataPtr ) );
		}
	}

	void SetShader( const ShaderRef& shader , eShaderType shaderType )
	{
		switch (shaderType)
		{
		case GpuApi::VertexShader:
			{
				SDeviceData &dd = GetDeviceData();
				if (shader.isNull())
				{
					dd.m_VertexShader = ShaderRef::Null();
				}
				else
				{
					GPUAPI_ASSERT( GetDeviceData().m_Shaders.Data(shader).m_type == VertexShader );
					dd.m_VertexShader = shader;
				}
			}
			break;
		case GpuApi::PixelShader:
			{
				SDeviceData &dd = GetDeviceData();
				if (shader.isNull())
				{
					dd.m_PixelShader = ShaderRef::Null();
				}
				else
				{
					GPUAPI_ASSERT( GetDeviceData().m_Shaders.Data(shader).m_type == PixelShader );
					dd.m_PixelShader = shader;
				}
			}
			break;
		case GpuApi::GeometryShader:
			{
				SDeviceData &dd = GetDeviceData();
				if (shader.isNull())
				{
					dd.m_GeometryShader = ShaderRef::Null();
				}
				else
				{
					GPUAPI_ASSERT( GetDeviceData().m_Shaders.Data(shader).m_type == GeometryShader );
					dd.m_GeometryShader = shader;
				}
			}
			break;
		case GpuApi::HullShader:
			{
				SDeviceData &dd = GetDeviceData();
				if (shader.isNull())
				{
					dd.m_HullShader = ShaderRef::Null();
				}
				else
				{
					GPUAPI_ASSERT( GetDeviceData().m_Shaders.Data(shader).m_type == HullShader );
					dd.m_HullShader = shader;
				}
			}
			break;
		case GpuApi::DomainShader:
			{
				SDeviceData &dd = GetDeviceData();
				if (shader.isNull())
				{
					dd.m_DomainShader = ShaderRef::Null();
				}
				else
				{
					GPUAPI_ASSERT( GetDeviceData().m_Shaders.Data(shader).m_type == DomainShader );
					dd.m_DomainShader = shader;
				}
			}
			break;
		case GpuApi::ComputeShader:
			{
				SDeviceData &dd = GetDeviceData();
				if (shader.isNull())
				{
					dd.m_ComputeShader = ShaderRef::Null();
				}
				else
				{
					GPUAPI_ASSERT( GetDeviceData().m_Shaders.Data(shader).m_type == ComputeShader );
					dd.m_ComputeShader = shader;
				}
			}
			break;
		default:
			break;
		}
	}
}
