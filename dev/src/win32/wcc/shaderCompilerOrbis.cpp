/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "shaderCompilerOrbis.h"

#ifdef _WIN64
// need to include GpuApi mapping functions to compute StreamOut declaration
#include <gnm.h>
#include <gnmx.h>
#include "../../common/gpuApiGnm/gpuApiMapping.h"
#endif

Bool ShaderCompilerOrbis::Compile(const AnsiChar* code, Uint32 codeLength, DataBuffer& shaderData, const Uint64& shaderHash )
{
	if ( !GSystemIO.FileExist( m_dumpDir.AsChar() ) )
	{
		Bool dirCreationStatus = GSystemIO.CreateDirectory( m_dumpDir.AsChar() );
		WARN_WCC( TXT("Dump directory '%s' does not exist. Attempting to create... %s"), m_dumpDir.AsChar(), dirCreationStatus ? TXT("SUCCEEDED") : TXT("FAILED") );
	}
	const String sdbDir = m_dumpDir + TXT( "\\" ) + TXT( "sdbs" );
	if ( !GSystemIO.FileExist( sdbDir.AsChar() ) )
	{
		RED_VERIFY( GSystemIO.CreateDirectory( sdbDir.AsChar() ) );
	}

	const String inFile = m_dumpDir + String::Printf( TXT( "\\%s%i.ps4.%s.in" ), m_context.m_shaderFileName.AsChar(), m_id, m_shortName.AsChar() );
	const String outFile = m_dumpDir + String::Printf( TXT( "\\%s%i.ps4.%s.out" ), m_context.m_shaderFileName.AsChar(), m_id, m_shortName.AsChar() );
	const String errorFile = m_dumpDir + String::Printf( TXT( "\\..\\compilerErrors\\%s%i.ps4.%s.error" ), m_context.m_shaderFileName.AsChar(), m_id, m_shortName.AsChar() );

	// Cleanup
	if ( GSystemIO.FileExist( inFile.AsChar() ) )
	{
		RED_VERIFY( GSystemIO.DeleteFile( inFile.AsChar() ) );
	}
	if ( GSystemIO.FileExist( outFile.AsChar() ) )
	{
		RED_VERIFY( GSystemIO.DeleteFile( outFile.AsChar() ) );
	}

	// Create fake input file
	if ( !PrepareInputFile( inFile, code, codeLength ) )
	{
		ERR_WCC( TXT("Error preparing inputFile") );
		return false;
	}

	TParameterDictionary params;
	RED_VERIFY( params.Insert( GetParameterName( INPUT_FILE ), inFile ) );
	RED_VERIFY( params.Insert( GetParameterName( OUTPUT_FILE ), outFile ) );

	RED_VERIFY( params.Insert( GetParameterName( INCLUDES_DIRECTORY ), TXT("shaders\\include") ) );
	RED_VERIFY( params.Insert( GetParameterName( SDB_DIR ), sdbDir ) );

#ifdef _WIN64
	// Compute Stream Out declaration
	RED_VERIFY( params.Insert( GetParameterName( STREAMOUT_DESC ), GetStreamOutDesc() ) );
#endif

	RED_VERIFY( params.Insert( GetParameterName( SHADER_TARGET ), ANSI_TO_UNICODE( m_shaderTarget.AsChar() ) ) );
	RED_VERIFY( params.Insert( GetParameterName( ENTRY_POINT ), ANSI_TO_UNICODE( m_entryPoint.AsChar() ) ) );

	const String commandline = GetCommandline( params );

	CProcessRunner runner;
	if ( !runner.Run( m_appName, commandline, TXT(".") ) || !runner.WaitForFinish( 30000 ) || runner.GetExitCode() != 0 )
	{
		ERR_WCC( TXT("Shader cook error: %s"), runner.GetFullCommandLine().AsChar() );
		DumpError( runner.GetFullCommandLine() );
		runner.LogOutput( errorFile );
		runner.Terminate();
		return false;
	}

	runner.Terminate();
	
	// Grab the output
	Red::IO::CNativeFileHandle outputFileHandle;
	if ( !outputFileHandle.Open( outFile.AsChar(), Red::IO::eOpenFlag_Read ) )
	{
		ERR_WCC( TXT("Shader cook error: %s"), runner.GetFullCommandLine().AsChar() );
		DumpError( runner.GetFullCommandLine() );
		runner.LogOutput( errorFile );
		return false;
	}

	// Read compiled shader content
	DataBuffer::TSize size = static_cast< DataBuffer::TSize>( outputFileHandle.GetFileSize() );
	shaderData.Allocate( size );

	Uint32 dummyOut;
	if ( !outputFileHandle.Read( shaderData.GetData(), shaderData.GetSize(), dummyOut ) )
	{
		ERR_WCC( TXT("Error reading results from output file: %s"), runner.GetFullCommandLine().AsChar() );
		return false;
	}
	RED_VERIFY( outputFileHandle.Close() );

	DumpShaderInfo( shaderHash, inFile );

	// Leave the files in the shaders dump - we need this for debugging
	//GSystemIO.DeleteFile( inFile.AsChar() );
	//GSystemIO.DeleteFile( outFile.AsChar() );
	return true;
}

String ShaderCompilerOrbis::GetCommandline( const TParameterDictionary& params )
{
	// Create command line
	String commandLine = String::EMPTY;

	// Entry point
	const String& entryPoint = params[ GetParameterName( ENTRY_POINT ) ];
	commandLine += String::Printf( TXT( "-entry %s " ), entryPoint.AsChar() );

	// Shader target
	const String& shaderTarget = params[ GetParameterName( SHADER_TARGET ) ];
	commandLine += String::Printf( TXT( "-profile %s " ), shaderTarget.AsChar() );

	const String& streamOutDesc = params[ GetParameterName( STREAMOUT_DESC ) ];
	commandLine += streamOutDesc;

	// Include path
	const String& includesDir = params[ GetParameterName( INCLUDES_DIRECTORY ) ];
	commandLine += String::Printf( TXT( "-I%s " ), includesDir.AsChar() );

	commandLine += TXT( "-cache " );
	// commandLine += TXT( "-nofastmath " ); <- ENABLE THIS FOR A CHECK IF YOU HAVE WEIRD GLITCHES THAT YOU CAN'T FIGURE OUT. IF IT FIXES THE PROBLEM, LOOKUP __invariant IN THE DOCS

	// pdbs path
	const String& sdbDir = params[ GetParameterName( SDB_DIR ) ];
	commandLine += String::Printf( TXT( "-cachedir %s " ), sdbDir.AsChar() );

	// Generate debug info. Will make shaders bigger and slower, but debugging is a priority right now.
	//commandLine += TXT( "-debug " );

	// Files
	const String& inFile = params[ GetParameterName( INPUT_FILE ) ];
	const String& outFile = params[ GetParameterName( OUTPUT_FILE ) ];

	commandLine += String::Printf( TXT( "-o %s %s " ), outFile.AsChar(), inFile.AsChar() );

	return commandLine;
}

String ShaderCompilerOrbis::GetStreamOutDesc()
{
	if ( m_context.m_shaderType != GpuApi::GeometryShader )
	{
		return String::EMPTY;
	}

	if ( m_context.m_bufferChunkType == GpuApi::BCT_Max  )
	{
		return String::EMPTY;
	}

	String streamOutDesc = String::EMPTY;
	const GpuApi::VertexPacking::PackingElement* outputLayout = GpuApi::GetPackingForFormat( m_context.m_bufferChunkType );

	// first compute the stride of each stream
	Uint32 strides [GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS] = {0};

	for ( Uint32 i = 0; i < GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS; ++i )
	{
		if ( outputLayout[ i ].IsEmpty() )
		{
			break;
		}

		Uint8 buff_number = outputLayout[i].m_slot;
		GpuApi::VertexPacking::ePackingType packing_type = outputLayout[i].m_type;

		strides [buff_number] += GpuApi::VertexPacking::GetPackingTypeSize(packing_type);
	}

	for ( Uint32 stream_i = 0; stream_i < GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS; ++stream_i )
	{
		if ( strides[stream_i] > 0 )
		{
			streamOutDesc += String::Printf( TXT( "-gsstream %d,%d" ), stream_i, strides[ stream_i ] );
			Uint32 offset = 0;

			for ( Uint32 i = 0; i < GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS; ++i )
			{
				if ( outputLayout[ i ].IsEmpty() )
				{
					break;
				}

				if ( outputLayout[i].m_slot == stream_i )
				{
					const AnsiChar* semanticName;
					Uint32 semanticIndex;
					if ( !GpuApi::MapPackingElementToSemanticAndIndex( outputLayout[ i ], semanticName, semanticIndex ) )
					{
						// we MUST have a valid semanticName for this...
						break;
					}

					const Char* swizzleMask = TXT( "xyzw" );

					switch ( outputLayout[i].m_type )
					{
					case GpuApi::VertexPacking::PT_Float1:
					case GpuApi::VertexPacking::PT_UShort1:
					case GpuApi::VertexPacking::PT_Short1:
					case GpuApi::VertexPacking::PT_UByte1:
					case GpuApi::VertexPacking::PT_Index16:
					case GpuApi::VertexPacking::PT_Index32:
					case GpuApi::VertexPacking::PT_UInt1:
					case GpuApi::VertexPacking::PT_Int1:
						swizzleMask = TXT( "x" );
						break;

					case GpuApi::VertexPacking::PT_Float2:
					case GpuApi::VertexPacking::PT_Float16_2:
					case GpuApi::VertexPacking::PT_UShort2:
					case GpuApi::VertexPacking::PT_Short2:
					case GpuApi::VertexPacking::PT_UInt2:
					case GpuApi::VertexPacking::PT_Int2:
						swizzleMask = TXT( "xy" );
						break;

					case GpuApi::VertexPacking::PT_Float3:
					case GpuApi::VertexPacking::PT_UInt3:
					case GpuApi::VertexPacking::PT_Int3:
						swizzleMask = TXT( "xyz" );
						break;

					case GpuApi::VertexPacking::PT_Float4:
					case GpuApi::VertexPacking::PT_Float16_4:
					case GpuApi::VertexPacking::PT_UShort4:
					case GpuApi::VertexPacking::PT_UShort4N:
					case GpuApi::VertexPacking::PT_Short4:
					case GpuApi::VertexPacking::PT_Short4N:
					case GpuApi::VertexPacking::PT_UInt4:
					case GpuApi::VertexPacking::PT_Int4:
					case GpuApi::VertexPacking::PT_Color:
					case GpuApi::VertexPacking::PT_UByte4:
					case GpuApi::VertexPacking::PT_UByte4N:
					case GpuApi::VertexPacking::PT_Byte4N:
					case GpuApi::VertexPacking::PT_Dec4:
						swizzleMask = TXT( "xyzw" );
						break;
					}

					streamOutDesc += String::Printf( TXT( ",%d:%d:%s%d.%s" ), offset, /*stream_i*/0, ANSI_TO_UNICODE( semanticName ), semanticIndex, swizzleMask );
					GpuApi::VertexPacking::ePackingType packing_type = outputLayout[ i ].m_type;
					offset += GpuApi::VertexPacking::GetPackingTypeSize( packing_type );
				}
			}
		}
	}

	// add space at the end in order not to mix streamOut with next parameter in commandline
	streamOutDesc += TXT(" ");

	return streamOutDesc;
}

void ShaderCompilerOrbis::AddPlatformSpecificDefines()
{
	m_defines.PushBack( GpuApi::ShaderDefine() );
	m_defines.Back().Name = "__PSSL__";
	m_defines.Back().Definition = "1";
}

Bool ShaderCompilerOrbis::GetShaderTargetAndEntryPoint( StringAnsi& outShaderTarget, StringAnsi& outEntryPoint )
{
	switch ( m_context.m_shaderType )
	{
	case GpuApi::PixelShader:
		outEntryPoint = "ps_main";
		outShaderTarget = "sce_ps_orbis";
		break;
	case GpuApi::VertexShader:
		outEntryPoint = "vs_main";
		if ( m_context.m_shadersMask & HullShader )
		{
			// Have hull shader, will be bound to LS (geom shader doesn't matter)
			outShaderTarget = "sce_vs_ls_orbis";
		}
		else if ( m_context.m_shadersMask & GeometryShader )
		{
			// Doesn't have hull shader but have geometry shader -> will be bound to ES
			outShaderTarget = "sce_vs_es_orbis";
		}
		else
		{
			// No hull, no geom, will be bound to VS
			outShaderTarget = "sce_vs_vs_orbis";
		}
		break;
	case GpuApi::DomainShader:
		outEntryPoint = "ds_main";
		if ( m_context.m_shadersMask & GeometryShader )
		{
			// Have geom shader, will be bound to DS-ES
			outShaderTarget = "sce_ds_es_orbis";
		}
		else
		{
			// doesn't have geom, will be bound to DS-VS
			outShaderTarget = "sce_ds_vs_orbis";
		}
		break;
	case GpuApi::ComputeShader:
		outEntryPoint = "cs_main";
		outShaderTarget = "sce_cs_orbis";
		break;
	case GpuApi::GeometryShader:
		outEntryPoint = "gs_main";
		outShaderTarget = "sce_gs_orbis";
		break;
	case GpuApi::HullShader:
		outEntryPoint = "hs_main";
		outShaderTarget = "sce_hs_orbis";
		break;
	default:
		ERR_WCC( TXT("Error getting shader target and entry point") );
		return false;
	}
	return true;
}
