/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "gpuApi.h"
#include "../gpuApiUtils/gpuApiMemory.h"
#include "../gpuApiUtils/gpuApiShaderParsing.h"

#ifdef GPU_API_DEBUG_PATH
	#include <wchar.h>
#endif
#include "../redSystem/crt.h"

#ifdef _DEBUG
#define GPU_API_SHADER_DEBUG
#endif

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
		Int32 refCount = dd.m_Shaders.DecRefCount( shader );
		if ( 0 == refCount )
		{
			// Can just safely destroy, DX internal ref counting will make it safe
			Destroy( shader );
		}
		return refCount;
	}

	void Destroy(const ShaderRef& shader)
	{
		SDeviceData &dd = GetDeviceData();
		SShaderData &data = dd.m_Shaders.Data( shader );

		// Release resources
		GPUAPI_ASSERT( nullptr != data.m_pShader );

		if ( data.m_pShader )
		{
			ULONG refcount = data.m_pShader->Release();
#ifdef NO_GPU_ASSERTS
			RED_UNUSED( refcount );
#endif
			GPUAPI_ASSERT( refcount == 0, TXT( "shader leak" ) );
			data.m_pShader = nullptr;
		}

		SAFE_RELEASE( data.m_byteCode );

		// Release any DX input layouts used by this shader
		if ( data.m_type == VertexShader )
		{
			ReleaseInputLayouts( shader );
		}

		// Destroy shit
		dd.m_Shaders.Destroy( shader );
	}

	ID3D11PixelShader* GetD3DPixelShader( const ShaderRef& ref )
	{
		GPUAPI_ASSERT( !(ref && ( !GetDeviceData().m_Shaders.Data(ref).m_pShader || GetDeviceData().m_Shaders.Data(ref).m_type != PixelShader ) ) );
		return ref ? static_cast< ID3D11PixelShader* >( GetDeviceData().m_Shaders.Data(ref).m_pShader ) : nullptr;
	}

	ID3D11ComputeShader* GetD3DComputeShader( const ShaderRef& ref )
	{
		GPUAPI_ASSERT( !(ref && ( !GetDeviceData().m_Shaders.Data(ref).m_pShader || GetDeviceData().m_Shaders.Data(ref).m_type != ComputeShader ) ) );
		return ref ? static_cast< ID3D11ComputeShader* >( GetDeviceData().m_Shaders.Data(ref).m_pShader ) : nullptr;
	}

	Bool GetShaderTargetAndEntryPoint( eShaderType shaderType, const AnsiChar*& outShaderTarget, const AnsiChar*& outMainFunction )
	{
		switch ( shaderType )
		{
		case GpuApi::PixelShader:
			outMainFunction = "ps_main";
			outShaderTarget = "ps_5_0";
			break;
		case GpuApi::VertexShader:
			outMainFunction = "vs_main";
			outShaderTarget = "vs_5_0";
			break;
		case GpuApi::DomainShader:
			outMainFunction = "ds_main";
			outShaderTarget = "ds_5_0";
			break;
		case GpuApi::ComputeShader:
			outMainFunction = "cs_main";
			outShaderTarget = "cs_5_0";
			break;
		case GpuApi::GeometryShader:
			outMainFunction = "gs_main";
			outShaderTarget = "gs_5_0";
			break;
		case GpuApi::HullShader:
			outMainFunction = "hs_main";
			outShaderTarget = "hs_5_0";
			break;
		default:
			GPUAPI_HALT( "Invalid shader type" );
			return false;
		}
		return true;
	}

	static void FillStreamOutDeclaration( D3D11_SO_DECLARATION_ENTRY& declaration, const VertexPacking::PackingElement& element, VertexPacking::PackingElement* adjustedElement )
	{
		// Translate usage
		const char* semanticName;
		Uint32 semanticIndex;
		if ( !MapPackingElementToSemanticAndIndex( element, semanticName, semanticIndex ) )
		{
			semanticName = nullptr;
			semanticIndex = 0;
		}


		Uint8 componentCount = 0;
		switch ( element.m_type )
		{
		case VertexPacking::PT_Float1:
		case VertexPacking::PT_UShort1:
		case VertexPacking::PT_Short1:
		case VertexPacking::PT_UByte1:
		case VertexPacking::PT_Index16:
		case VertexPacking::PT_Index32:
		case VertexPacking::PT_UInt1:
		case VertexPacking::PT_Int1:
			componentCount = 1;
			break;

		case VertexPacking::PT_Float2:
		case VertexPacking::PT_Float16_2:
		case VertexPacking::PT_UShort2:
		case VertexPacking::PT_Short2:
		case VertexPacking::PT_UInt2:
		case VertexPacking::PT_Int2:
			componentCount = 2;
			break;

		case VertexPacking::PT_Float3:
		case VertexPacking::PT_UInt3:
		case VertexPacking::PT_Int3:
			componentCount = 3;
			break;

		case VertexPacking::PT_Float4:
		case VertexPacking::PT_Float16_4:
		case VertexPacking::PT_UShort4:
		case VertexPacking::PT_UShort4N:
		case VertexPacking::PT_Short4:
		case VertexPacking::PT_Short4N:
		case VertexPacking::PT_UInt4:
		case VertexPacking::PT_Int4:
		case VertexPacking::PT_Color:
		case VertexPacking::PT_UByte4:
		case VertexPacking::PT_UByte4N:
		case VertexPacking::PT_Byte4N:
		case VertexPacking::PT_Dec4:
			componentCount = 4;
			break;
		}

		// Initialize the element list
		declaration.SemanticName = semanticName;
		declaration.SemanticIndex = semanticIndex;
		declaration.StartComponent = 0;
		declaration.ComponentCount = componentCount;


		// Fill adjusted element. Since SO only seems to work on 32-bit register values, some requested types will not work.
		// So, we translate those to acceptable types and return them as "suggested" formats. This should then be usable as
		// an vertex format for drawing from the resulting SO buffer.
		if ( adjustedElement )
		{
			*adjustedElement = element;

			switch ( adjustedElement->m_type )
			{
			case VertexPacking::PT_UShort1:
			case VertexPacking::PT_UByte1:
				adjustedElement->m_type = VertexPacking::PT_UInt1;
				break;

			case VertexPacking::PT_Short1:
				adjustedElement->m_type = VertexPacking::PT_Int1;
				break;

			case VertexPacking::PT_Float16_2:
				adjustedElement->m_type = VertexPacking::PT_Float2;
				break;
			case VertexPacking::PT_UShort2:
				adjustedElement->m_type = VertexPacking::PT_UInt2;
				break;
			case VertexPacking::PT_Short2:
				adjustedElement->m_type = VertexPacking::PT_Int2;
				break;

			case VertexPacking::PT_Float16_4:
			case VertexPacking::PT_Color:
			case VertexPacking::PT_Short4N:
			case VertexPacking::PT_UShort4N:
			case VertexPacking::PT_UByte4N:
			case VertexPacking::PT_Byte4N:
			case VertexPacking::PT_Dec4:
				adjustedElement->m_type = VertexPacking::PT_Float4;
				break;

			case VertexPacking::PT_UShort4:
			case VertexPacking::PT_UByte4:
				adjustedElement->m_type = VertexPacking::PT_UInt4;
				break;
			case VertexPacking::PT_Short4:
				adjustedElement->m_type = VertexPacking::PT_Int4;
				break;
			}
		}
	}


	// Ownership of compiledCode is taken. Caller should not use the passed-in pointer, unless it explicitly holds an extra reference.
	static Uint32 CreateGeometryShaderWithSOInternal( ID3DBlob* compiledCode, const char* debugName, Uint32 debugNameLength, const VertexLayoutDesc& outputDesc, GpuApi::VertexLayoutDesc* adjustedDesc )
	{
		SDeviceData &dd = GetDeviceData();

		// Process output layout
		D3D11_SO_DECLARATION_ENTRY outputLayout[ GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS ];
		Uint32 outputLayoutSize = 0;
		Uint32 bufferStrides[ GPUAPI_VERTEX_LAYOUT_MAX_SLOTS ] = { 0 };
		Uint8 bufferStridesSize = 0;

		for ( Uint32 i = 0; i < GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS; ++i )
		{
			if ( outputDesc.m_elements[ i ].IsEmpty() )
			{
				break;
			}

			FillStreamOutDeclaration( outputLayout[ i ], outputDesc.m_elements[ i ], adjustedDesc ? &adjustedDesc->m_elements[ i ] : nullptr );

			outputLayout[ i ].Stream = 0;

			Uint8 slot = outputDesc.m_elements[ i ].m_slot;
			outputLayout[ i ].OutputSlot = slot;

			bufferStrides[ slot ] += outputLayout[ i ].ComponentCount * 4;

			if ( bufferStridesSize < slot + 1 )
			{
				bufferStridesSize = slot + 1;
			}

			++outputLayoutSize;
		}

		// Create shader
		ID3D11DeviceChild* shader = nullptr;
		HRESULT hRet = GetDevice()->CreateGeometryShaderWithStreamOutput( compiledCode->GetBufferPointer(), compiledCode->GetBufferSize(), outputLayout, outputLayoutSize, bufferStrides, bufferStridesSize, D3D11_SO_NO_RASTERIZED_STREAM, nullptr, ( ID3D11GeometryShader**) &shader );
#ifdef NO_GPU_ASSERTS
		RED_UNUSED( hRet );
#endif
		GPUAPI_ASSERT( SUCCEEDED( hRet ) );

		if ( !shader )
		{
			SAFE_RELEASE( compiledCode );
			return ShaderRef::Null();
		}

#ifdef GPU_API_DEBUG_PATH
		if ( shader )
		{
			shader->SetPrivateData( WKPDID_D3DDebugObjectName, debugNameLength, debugName );
		}
#endif

		// Create GpuApi shader
		Uint32 newShaderId = dd.m_Shaders.Create( 1 );
		if ( !newShaderId )
		{
			GPUAPI_HALT( "Failed to create gpuapi shader" );
			SAFE_RELEASE( compiledCode );
			return ShaderRef::Null();
		}
		GPUAPI_ASSERT( dd.m_Shaders.IsInUse( newShaderId ) );

		// Initialize new buffer
		SShaderData &data = dd.m_Shaders.Data( newShaderId );
		data.m_type = GeometryShader;
		data.m_pShader = shader;
		data.m_byteCode = compiledCode;

		// Finalize
		return newShaderId;
	}

	ShaderRef CreateGeometryShaderWithSOFromSource( const char* code, const char* mainFunction, const char* shaderTarget, ShaderDefine* defines, Uint32 numDefines, const char* fileName, const VertexLayoutDesc& outputDesc, GpuApi::VertexLayoutDesc* adjustedDesc /*nullptr*/ )
	{
		// Compile shader code
		ID3DBlob* errorBuffer = nullptr;
		ID3DBlob* codeBuffer = nullptr;
		ID3DBlob* stripBuffer = nullptr;

		Uint32 flags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
		Uint32 stripFlags = D3DCOMPILER_STRIP_REFLECTION_DATA | D3DCOMPILER_STRIP_PRIVATE_DATA;
#ifdef GPU_API_SHADER_DEBUG
		flags |= D3DCOMPILE_DEBUG;
#else
		stripFlags |= D3DCOMPILER_STRIP_DEBUG_INFO;
#endif // GPU_API_SHADER_DEBUG
		

		AnsiChar* preprocessedCode;
		Uint32 preprocessedLength;
		if ( !GpuApi::Preprocess( code, defines, numDefines, fileName, preprocessedCode, preprocessedLength ) )
		{
			GPUAPI_ASSERT( false );
			return ShaderRef::Null();
		}

		HRESULT hRet = D3DCompile(	preprocessedCode, preprocessedLength, fileName, nullptr, nullptr,
									mainFunction, shaderTarget, flags, 0, &codeBuffer, &errorBuffer );

		// Print error message to log
		if ( errorBuffer )
		{
			// Has the shader compilation failed ?
			if ( FAILED( hRet ) )
			{
				GPUAPI_ERROR
				(
					TXT( "\nShader error(s):\n" )
					TXT( "%" ) RED_PRIWas
					TXT( "\n==================================================" ),
					errorBuffer->GetBufferPointer()
				);
			} 
#ifdef GPU_API_SHADER_DEBUG
			else
			{
				GPUAPI_LOG_WARNING
				(
					TXT( "\nShader warning(s):\n" )
					TXT( "%" ) RED_PRIWas
					TXT( "\n==================================================" ),
					errorBuffer->GetBufferPointer()
				);
			}
#endif // GPU_API_SHADER_DEBUG
			// Cleanup
			errorBuffer->Release();
		}

		GPU_API_FREE( GpuMemoryPool_ShaderInclude, MC_Temporary, preprocessedCode );

		// No shader compiled so far, exit
		if ( FAILED( hRet ) )
		{
			return ShaderRef::Null();
		}

		const Uint32 len = static_cast< Uint32 >( Red::System::StringLength(fileName)+1 );

		hRet = D3DStripShader( codeBuffer->GetBufferPointer(), codeBuffer->GetBufferSize(), stripFlags, &stripBuffer );
		SAFE_RELEASE( codeBuffer );
		if ( FAILED( hRet ) )
		{
			// Show error message
			GPUAPI_ERROR( TXT("Error stripping HLSL shader in %s for %s:"), mainFunction, shaderTarget );
			return ShaderRef::Null();
		}

		Uint32 newShaderId = CreateGeometryShaderWithSOInternal( stripBuffer, fileName, len, outputDesc, adjustedDesc );

		// Finalize
		return ShaderRef( newShaderId );
	}

	ShaderRef CreateGeometryShaderWithSOFromBinary( const void* shaderBuffer, Uint32 shaderBufferSize, const VertexLayoutDesc& outputDesc, GpuApi::VertexLayoutDesc* adjustedDesc /*nullptr*/ )
	{
		ID3DBlob* codeBufferBlob = nullptr;
		HRESULT hRet = D3DCreateBlob( shaderBufferSize, &codeBufferBlob );
		if ( FAILED( hRet ) )
		{
			GPUAPI_HALT(  "Failed to allocate memory for gpuapi shader" );
			return ShaderRef::Null();
		}
		Red::System::MemoryCopy( codeBufferBlob->GetBufferPointer(), shaderBuffer, shaderBufferSize );

		// Create shader
		Uint32 newShaderId = CreateGeometryShaderWithSOInternal( codeBufferBlob, "shaderFromCache", 15, outputDesc, adjustedDesc );

		// Finalize
		return ShaderRef( newShaderId );
	}


	// Ownership of compiledCode is taken. Caller should not use the passed-in pointer, unless it explicitly holds an extra reference.
	Uint32 CreateShaderInternal( ID3DBlob* compiledCode, eShaderType shaderType, const char* debugName, Uint32 debugNameLength )
	{
		GPUAPI_ASSERT( compiledCode );
		GPUAPI_ASSERT( compiledCode->GetBufferPointer() );
		GPUAPI_ASSERT( compiledCode->GetBufferSize() );

		SDeviceData &dd = GetDeviceData();

		ID3D11DeviceChild* shader = nullptr;

		HRESULT hRet;
		// Create shader
		switch (shaderType)
		{
		case VertexShader:
			{
				hRet = GetDevice()->CreateVertexShader( compiledCode->GetBufferPointer(), compiledCode->GetBufferSize(), nullptr, ( ID3D11VertexShader**) &shader );
				GPUAPI_ASSERT( SUCCEEDED( hRet ) );
				break;
			}
		case PixelShader:
			{
				hRet = GetDevice()->CreatePixelShader( compiledCode->GetBufferPointer(), compiledCode->GetBufferSize(), nullptr, ( ID3D11PixelShader**) &shader );
				GPUAPI_ASSERT( SUCCEEDED( hRet ) );
				break;
			}
		case GeometryShader:
			{
				hRet = GetDevice()->CreateGeometryShader( compiledCode->GetBufferPointer(), compiledCode->GetBufferSize(), nullptr, ( ID3D11GeometryShader**) &shader );
				GPUAPI_ASSERT( SUCCEEDED( hRet ) );
				break;
			}
		case HullShader:
			{
				hRet = GetDevice()->CreateHullShader( compiledCode->GetBufferPointer(), compiledCode->GetBufferSize(), nullptr, ( ID3D11HullShader**) &shader );
				GPUAPI_ASSERT( SUCCEEDED( hRet ) );
				break;
			}
		case DomainShader:
			{
				hRet = GetDevice()->CreateDomainShader( compiledCode->GetBufferPointer(), compiledCode->GetBufferSize(), nullptr, ( ID3D11DomainShader**) &shader );
				GPUAPI_ASSERT( SUCCEEDED( hRet ) );
				break;
			}
		case ComputeShader:
			{
				hRet = GetDevice()->CreateComputeShader( compiledCode->GetBufferPointer(), compiledCode->GetBufferSize(), nullptr, ( ID3D11ComputeShader**) &shader );
				GPUAPI_ASSERT( SUCCEEDED( hRet ) );
				break;
			}
		}

		if ( !shader )
		{
			SAFE_RELEASE( compiledCode );
			return ShaderRef::Null();
		}

#ifdef GPU_API_DEBUG_PATH
		GPUAPI_ASSERT( shader );
		shader->SetPrivateData( WKPDID_D3DDebugObjectName, debugNameLength, debugName );
#endif

		// Create GpuApi shader
		Uint32 newShaderId = dd.m_Shaders.Create( 1 );
		if ( !newShaderId )
		{
			GPUAPI_HALT( "Failed to create gpuapi shader" );
			SAFE_RELEASE( compiledCode );
			return ShaderRef::Null();
		}
		GPUAPI_ASSERT( dd.m_Shaders.IsInUse( newShaderId ) );

		// Initialize new buffer
		SShaderData &data = dd.m_Shaders.Data( newShaderId );
		data.m_type = shaderType;
		data.m_pShader = shader;
		data.m_byteCode = compiledCode;

		return newShaderId;
	}

	ShaderRef CreateShaderFromSource( eShaderType shaderType, const char* code, const char* mainFunction, const char* shaderTarget, ShaderDefine* defines, Uint32 numDefines, const char* fileName )
	{
		// Compile shader code
		ID3DBlob* errorBuffer = nullptr;
		ID3DBlob* codeBuffer = nullptr;
		ID3DBlob* stripBuffer = nullptr;

		Uint32 flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
		Uint32 stripFlags = D3DCOMPILER_STRIP_REFLECTION_DATA | D3DCOMPILER_STRIP_PRIVATE_DATA;
#ifdef GPU_API_SHADER_DEBUG
		flags |= D3DCOMPILE_DEBUG;
#else
		stripFlags |= D3DCOMPILER_STRIP_DEBUG_INFO;
#endif // GPU_API_SHADER_DEBUG

		AnsiChar* preprocessedCode;
		Uint32 preprocessedLength;
		if ( !GpuApi::Preprocess( code, defines, numDefines, fileName, preprocessedCode, preprocessedLength ) )
		{
			GPUAPI_ASSERT( false );
			return ShaderRef::Null();
		}
		
		D3D_SHADER_MACRO shaderMacros[16];
		Uint32 numShaderMacros = 0;
#ifdef RED_PLATFORM_DURANGO		
		for ( Uint32 def_i=0; def_i<numDefines; ++def_i )
		{
			const GpuApi::ShaderDefine &def = defines[def_i];
			if ( IsCompilerShaderDefine( def ) )
			{
				RED_ASSERT( numShaderMacros + 1 < ARRAY_COUNT(shaderMacros) );
				if ( numShaderMacros + 1 < ARRAY_COUNT(shaderMacros) )
				{
					shaderMacros[numShaderMacros].Name = def.Name;
					shaderMacros[numShaderMacros].Definition = def.Definition;
					++numShaderMacros;
				}
			}
		}
#endif
		RED_ASSERT( numShaderMacros < ARRAY_COUNT(shaderMacros) );
		shaderMacros[numShaderMacros].Name = nullptr;
		shaderMacros[numShaderMacros].Definition = nullptr;
		++numShaderMacros;

		HRESULT hRet = D3DCompile(	preprocessedCode, preprocessedLength, fileName, shaderMacros, nullptr,
									mainFunction, shaderTarget, flags, 0, &codeBuffer, &errorBuffer );

		// Print error message to log
		if ( errorBuffer )
		{
			// Has the shader compilation failed ?
			if ( FAILED( hRet ) )
			{
				GPUAPI_ERROR
				(
					TXT( "\nShader error(s):\n" )
					TXT( "%" ) RED_PRIWas
					TXT( "\n==================================================" ),
					errorBuffer->GetBufferPointer()
				);
			} 
#ifdef GPU_API_SHADER_DEBUG
			else
			{
				GPUAPI_LOG_WARNING
				(
					TXT( "\nShader warning(s):\n" )
					TXT( "%" ) RED_PRIWas
					TXT( "\n==================================================" ),
					errorBuffer->GetBufferPointer()
				);
			}
#endif // GPU_API_SHADER_DEBUG
			// Cleanup
			errorBuffer->Release();
		}

		GPU_API_FREE( GpuMemoryPool_ShaderInclude, MC_Temporary, preprocessedCode );

		// No shader compiled so far, exit
		if ( FAILED( hRet ) )
		{
			return ShaderRef::Null();
		}

		const Uint32 len = static_cast< Uint32 >( Red::System::StringLength(fileName)+1 );

		hRet = D3DStripShader( codeBuffer->GetBufferPointer(), codeBuffer->GetBufferSize(), stripFlags, &stripBuffer );
		SAFE_RELEASE( codeBuffer );

		if ( FAILED( hRet ) )
		{
			// Show error message
			GPUAPI_ERROR( TXT("Error stripping HLSL shader in %s for %s:"), mainFunction, shaderTarget );
			return ShaderRef::Null();
		}

		Uint32 newShaderId = CreateShaderInternal( stripBuffer, shaderType, fileName, len );

		// Finalize
		return ShaderRef( newShaderId );
	}

	ShaderRef CreateShaderFromBinary( eShaderType shaderType, const void* shaderBuffer, Uint32 shaderBufferSize, const char* debugName )
	{
		HRESULT hRet;
		ID3DBlob* codeBufferBlob = nullptr;
		hRet = D3DCreateBlob( shaderBufferSize, &codeBufferBlob );
		if ( FAILED( hRet ) )
		{
			GPUAPI_HALT( "Failed to allocate memory for gpuapi shader" );
			return ShaderRef::Null();
		}
		Red::System::MemoryCopy( codeBufferBlob->GetBufferPointer(), shaderBuffer, shaderBufferSize );

		// Create shader
		Uint32 newShaderId = CreateShaderInternal( codeBufferBlob, shaderType, "shaderFromCache", 15 );
		if ( newShaderId == ShaderRef::Null() )
		{
			return ShaderRef::Null();
		}
		
		ShaderRef ret( newShaderId );

		if (debugName != nullptr)
		{
			SetShaderDebugPath( ret, debugName );
		}

		// Finalize
		return ret;
	}

	eShaderLanguage GetShaderLanguage()
	{
		return SL_HLSL;
	}

	Uint32 GetShaderCodeSize( const ShaderRef& shader )
	{
		SDeviceData &dd = GetDeviceData();

		if ( !shader.isNull() )
		{
			SShaderData &data = dd.m_Shaders.Data( shader );
			return static_cast< Uint32 >( data.m_byteCode->GetBufferSize() );
		}

		return 0;
	}

	void CopyShaderCode( const ShaderRef& shader, void* targetCode )
	{
		SDeviceData &dd = GetDeviceData();

		GPUAPI_ASSERT( shader );
		if ( !shader.isNull() )
		{
			SShaderData &data = dd.m_Shaders.Data( shader );
			GPUAPI_ASSERT( data.m_byteCode->GetBufferSize() > 0 );
			Red::System::MemoryCopy( targetCode, data.m_byteCode->GetBufferPointer(), data.m_byteCode->GetBufferSize() );
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
					GetDeviceContext()->VSSetShader(nullptr, nullptr, 0);
					dd.m_VertexShader = ShaderRef::Null();
				}
				else
				{
					GPUAPI_ASSERT( GetDeviceData().m_Shaders.Data(shader).m_type == VertexShader );
					GetDeviceContext()->VSSetShader( (ID3D11VertexShader*) GetDeviceData().m_Shaders.Data(shader).m_pShader, nullptr, 0 );
					dd.m_VertexShader = shader;
				}
			}
			break;
		case GpuApi::PixelShader:
			{
				SDeviceData &dd = GetDeviceData();
				if (shader.isNull())
				{
					GetDeviceContext()->PSSetShader(nullptr, nullptr, 0);
					dd.m_PixelShader = ShaderRef::Null();
				}
				else
				{
					GPUAPI_ASSERT( GetDeviceData().m_Shaders.Data(shader).m_type == PixelShader );
					GetDeviceContext()->PSSetShader( (ID3D11PixelShader*) GetDeviceData().m_Shaders.Data(shader).m_pShader, nullptr, 0 );
					dd.m_PixelShader = shader;
				}
			}
			break;
		case GpuApi::GeometryShader:
			{
				SDeviceData &dd = GetDeviceData();
				if (shader.isNull())
				{
					GetDeviceContext()->GSSetShader(nullptr, nullptr, 0);
					dd.m_GeometryShader = ShaderRef::Null();
				}
				else
				{
					GPUAPI_ASSERT( GetDeviceData().m_Shaders.Data(shader).m_type == GeometryShader );
					GetDeviceContext()->GSSetShader( (ID3D11GeometryShader*) GetDeviceData().m_Shaders.Data(shader).m_pShader, nullptr, 0 );
					dd.m_GeometryShader = shader;
				}
			}
			break;
		case GpuApi::HullShader:
			{
				SDeviceData &dd = GetDeviceData();
				if (shader.isNull())
				{
					GetDeviceContext()->HSSetShader(nullptr, nullptr, 0);
					dd.m_HullShader = ShaderRef::Null();
				}
				else
				{
					GPUAPI_ASSERT( GetDeviceData().m_Shaders.Data(shader).m_type == HullShader );
					GetDeviceContext()->HSSetShader( (ID3D11HullShader*) GetDeviceData().m_Shaders.Data(shader).m_pShader, nullptr, 0 );
					dd.m_HullShader = shader;
				}
			}
			break;
		case GpuApi::DomainShader:
			{
				SDeviceData &dd = GetDeviceData();
				if (shader.isNull())
				{
					GetDeviceContext()->DSSetShader(nullptr, nullptr, 0);
					dd.m_DomainShader = ShaderRef::Null();
				}
				else
				{
					GPUAPI_ASSERT( GetDeviceData().m_Shaders.Data(shader).m_type == DomainShader );
					GetDeviceContext()->DSSetShader( (ID3D11DomainShader*) GetDeviceData().m_Shaders.Data(shader).m_pShader, nullptr, 0 );
					dd.m_DomainShader = shader;
				}
			}
			break;
		case GpuApi::ComputeShader:
			{
				SDeviceData &dd = GetDeviceData();
				if (shader.isNull())
				{
					GetDeviceContext()->CSSetShader(nullptr, nullptr, 0);
					dd.m_ComputeShader = ShaderRef::Null();
				}
				else
				{
					GPUAPI_ASSERT( GetDeviceData().m_Shaders.Data(shader).m_type == ComputeShader );
					GetDeviceContext()->CSSetShader( (ID3D11ComputeShader*) GetDeviceData().m_Shaders.Data(shader).m_pShader, nullptr, 0 );
					dd.m_ComputeShader = shader;
				}
			}
			break;
		default:
			break;
		}
	}
}
