/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "shaderCompilerPC.h"

#include "d3d11_1.h"
#include "d3dcompiler.h"

#pragma comment (lib, "d3dcompiler.lib")

Bool DumpErrorsFile( const String& errorFilePath, const StringAnsi& error )
{
	Red::IO::CNativeFileHandle errorFileHandle;
	Uint32 dummyOut;

	if ( !errorFileHandle.Open( errorFilePath.AsChar(), Red::IO::eOpenFlag_Write | Red::IO::eOpenFlag_Truncate | Red::IO::eOpenFlag_Create ) )
	{
		WARN_WCC( TXT("Unable to create error file '%s'"), errorFilePath.AsChar() );
		return false;
	}

	Uint32 dataSize = static_cast< Uint32 >( error.DataSize() );
	RED_VERIFY( errorFileHandle.Write( error.TypedData(), dataSize, dummyOut ) );
	return true;
}

Bool ShaderCompilerPC::Compile(const AnsiChar* code, Uint32 codeLength, DataBuffer& shaderData, const Uint64& shaderHash )
{
	// Compile shader
	ID3DBlob* outShader = nullptr;
	ID3DBlob* outError = nullptr;
	ID3DBlob* outStrip = nullptr;
	Uint32 flags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
	Uint32 flags1 = 0;
	Uint32 stripFlags = D3DCOMPILER_STRIP_REFLECTION_DATA | D3DCOMPILER_STRIP_DEBUG_INFO | D3DCOMPILER_STRIP_PRIVATE_DATA;

	HRESULT hRet = D3DCompile(	code, codeLength, m_context.m_shaderFileNameAnsi, nullptr, nullptr, 
		m_entryPoint.AsChar(), m_shaderTarget.AsChar(), flags, flags1, &outShader, &outError );

	// Failed to compile
	if ( FAILED( hRet ) )
	{
		// Show error message
		WARN_WCC( TXT("Error compiling HLSL shader in %s for %s:"), ANSI_TO_UNICODE( m_entryPoint.AsChar() ), ANSI_TO_UNICODE( m_shaderTarget.AsChar() ) );

		// Print error message to log
		if ( outError )
		{
			const String errorFile = m_dumpDir + String::Printf( TXT( "\\..\\compilerErrors\\%s.pc.%s.error" ), m_context.m_shaderFileName.AsChar(), m_shortName.AsChar() );
			StringAnsi errorString( (AnsiChar*)outError->GetBufferPointer() );
			DumpErrorsFile( errorFile, errorString );
			outError->Release();
		}

		// Do not compile
		return false;
	}

	DumpShaderInfo( shaderHash );

	hRet = D3DStripShader( outShader->GetBufferPointer(), outShader->GetBufferSize(), stripFlags, &outStrip );
	if ( FAILED( hRet ) )
	{
		// Show error message
		WARN_WCC( TXT("Error stripping HLSL shader in %s for %s:"), ANSI_TO_UNICODE( m_entryPoint.AsChar() ), ANSI_TO_UNICODE( m_shaderTarget.AsChar() ) );
		return false;
	}

	// Extract data
	DataBuffer::TSize sizeToAllocate = (DataBuffer::TSize)outStrip->GetBufferSize();
	shaderData.Allocate( sizeToAllocate );
	Red::System::MemoryCopy( shaderData.GetData(), outStrip->GetBufferPointer(), outStrip->GetBufferSize() );

	// Cleanup
	if ( outShader )
	{
		outShader->Release();
		outShader = nullptr;
	}

	if( outStrip )
	{
		outStrip->Release();
		outStrip = nullptr;
	}

	// Compiled
	return true;
}

Bool ShaderCompilerPC::GetShaderTargetAndEntryPoint( StringAnsi& outShaderTarget, StringAnsi& outEntryPoint )
{
	switch ( m_context.m_shaderType )
	{
	case GpuApi::PixelShader:
		outEntryPoint = "ps_main";
		outShaderTarget = "ps_5_0";
		break;
	case GpuApi::VertexShader:
		outEntryPoint = "vs_main";
		outShaderTarget = "vs_5_0";
		break;
	case GpuApi::DomainShader:
		outEntryPoint = "ds_main";
		outShaderTarget = "ds_5_0";
		break;
	case GpuApi::ComputeShader:
		outEntryPoint = "cs_main";
		outShaderTarget = "cs_5_0";
		break;
	case GpuApi::GeometryShader:
		outEntryPoint = "gs_main";
		outShaderTarget = "gs_5_0";
		break;
	case GpuApi::HullShader:
		outEntryPoint = "hs_main";
		outShaderTarget = "hs_5_0";
		break;
	default:
		RED_HALT( "Invalid shader type" );
		return false;
	}

	return true;
}
