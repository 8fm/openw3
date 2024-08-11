/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "shaderCompilerDurango.h"

Bool ShaderCompilerDurango::Compile( const AnsiChar* code, Uint32 codeLength, DataBuffer& shaderData, const Uint64& shaderHash )
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

	const String inFile = m_dumpDir + String::Printf( TXT( "\\%s%i.xbone.%s.in" ), m_context.m_shaderFileName.AsChar(), m_id, m_shortName.AsChar() );
	const String outFile = m_dumpDir + String::Printf( TXT( "\\%s%i.xbone.%s.out" ), m_context.m_shaderFileName.AsChar(), m_id, m_shortName.AsChar() );
	const String updbFile = m_dumpDir + String::Printf( TXT("\\%s%i.xbone.%s.updb" ), m_context.m_shaderFileName.AsChar(), m_id, m_shortName.AsChar() );
	const String assemblyFile = m_dumpDir + String::Printf( TXT( "\\%s%i.xbone.assembly" ), m_context.m_shaderFileName.AsChar(), m_id );
	const String errorFile = m_dumpDir + String::Printf( TXT( "\\..\\compilerErrors\\%s%i.xbone.%s.error" ), m_context.m_shaderFileName.AsChar(), m_id, m_shortName.AsChar() );

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
		ERR_WCC( TXT("Error preparing inputFile: %s"), inFile.AsChar() );
		return false;
	}

	TParameterDictionary params;
	RED_VERIFY( params.Insert( GetParameterName( INPUT_FILE ), inFile ) );
	RED_VERIFY( params.Insert( GetParameterName( OUTPUT_FILE ), outFile ) );

	RED_VERIFY( params.Insert( GetParameterName( INCLUDES_DIRECTORY ), GpuApi::GetShaderIncludePath() ) );
	RED_VERIFY( params.Insert( GetParameterName( SDB_DIR ), sdbDir ) );

	RED_VERIFY( params.Insert( GetParameterName( UPDB_FILE ), updbFile ) );
	RED_VERIFY( params.Insert( GetParameterName( ASSEMBLY_FILE ), assemblyFile ) );

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
		ERR_WCC( TXT("Shader cook error: %ls"), runner.GetFullCommandLine().AsChar() );
		DumpError( runner.GetFullCommandLine() );
		runner.LogOutput( errorFile );
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

	GSystemIO.DeleteFile( inFile.AsChar() );
	GSystemIO.DeleteFile( outFile.AsChar() );
	return true;
}

String ShaderCompilerDurango::GetCommandline( const TParameterDictionary& params )
{
	// Create command line
	String commandLine = String::EMPTY;

	// Entry point
	const String& entryPoint = params[ GetParameterName( ENTRY_POINT ) ];
	commandLine += String::Printf( TXT( "/E %s " ), entryPoint.AsChar() );

	// Shader target
	const String& shaderTarget = params[ GetParameterName( SHADER_TARGET ) ];
	commandLine += String::Printf( TXT( "/T %s " ), shaderTarget.AsChar() );

	// Include path
	const String& includesDir = params[ GetParameterName( INCLUDES_DIRECTORY ) ];
	commandLine += String::Printf( TXT( "/I %s " ), includesDir.AsChar() );

	// Updb
	const String& updbFile = params[ GetParameterName( UPDB_FILE ) ];
	commandLine += String::Printf( TXT( "/Fd %s " ), updbFile.AsChar() );

	// Strip unnecessary data (unnecessary for FINAL ofc)
	// Note! __XBOX_FULL_PRECOMPILE_PROMISE means that the shaders can not be rebuilt during runtime
	commandLine += String::Printf( TXT("/Qstrip_reflect /Qstrip_priv /Qstrip_debug /O3 /Ges /D__XBOX_FULL_PRECOMPILE_PROMISE=1 ") );

	// Include compilation macros
	for ( Uint32 def_i=0; def_i<m_defines.Size(); ++def_i )
	{
		const GpuApi::ShaderDefine &def = m_defines[def_i];
		if ( IsCompilerShaderDefine( def ) )
		{
			commandLine += String::Printf( TXT("/D%s=%s "), ANSI_TO_UNICODE(def.Name), ANSI_TO_UNICODE(def.Definition) );
		}		
	}

	// Strip the shader stages that are not going to be used
	switch ( m_context.m_shaderType )
	{
	case GpuApi::VertexShader:
		if ( m_context.m_shadersMask & HullShader )
		{
			//// Have hull shader, will be bound to LS (geom shader doesn't matter)
			//commandLine += String::Printf(TXT("/D__XBOX_DISABLE_PRECOMPILE_ES=1 /D__XBOX_DISABLE_PRECOMPILE_VS=1 "));
		}
		else if ( m_context.m_shadersMask & GeometryShader )
		{
			//// Doesn't have hull shader but have geometry shader -> will be bound to ES
			//commandLine += String::Printf(TXT("/D__XBOX_DISABLE_PRECOMPILE_VS=1 /D__XBOX_DISABLE_PRECOMPILE_LS=1 "));
		}
		else
		{
			// No hull, no geom, will be bound to VS
			commandLine += String::Printf(TXT("/D__XBOX_DISABLE_PRECOMPILE_ES=1 /D__XBOX_DISABLE_PRECOMPILE_LS=1 "));
		}
		break;
	//case GpuApi::DomainShader:
	//	if ( m_context.m_shadersMask & GeometryShader )
	//	{
	//		// Have geom shader, will be bound to DS-ES
	//		commandLine += String::Printf(TXT("/D__XBOX_DISABLE_PRECOMPILE_VS=1 "));
	//	}
	//	else
	//	{
	//		// doesn't have geom, will be bound to DS-VS
	//		commandLine += String::Printf(TXT("/D__XBOX_DISABLE_PRECOMPILE_ES=1 "));
	//	}
	//	break;
	}


	// !!! UNDOCUMENTED RISKY STUFF !!!
	{
		if( m_context.m_bufferChunkType == GpuApi::BCT_Max )
		{
			// strip the shader IL code, after this it's impossible to recompile the shader
			commandLine += String::Printf(TXT("/D__XBOX_DISABLE_DXBC=1 "));
		}

		{
			//strip the filenames that are stored in the shaders
			commandLine += String::Printf(TXT("/D__XBOX_DISABLE_SHADER_NAME_EMPLACEMENT=1 "));
		}
	}

	// assembly file
	if ( m_context.m_dumpAssembly )
	{
		const String& assemblyFile = params[ GetParameterName( ASSEMBLY_FILE ) ];
		commandLine += String::Printf( TXT( "/Fc %s " ), assemblyFile.AsChar() );
	}

	// Streamout (when it will be supported)
	const String& streamOutDesc = params[ GetParameterName( STREAMOUT_DESC ) ];
	commandLine += streamOutDesc;

	// Files
	const String& inFile = params[ GetParameterName( INPUT_FILE ) ];
	const String& outFile = params[ GetParameterName( OUTPUT_FILE ) ];
	commandLine += String::Printf( TXT( "/Fo %s %s " ), outFile.AsChar(), inFile.AsChar() );

	return commandLine;
}

String ShaderCompilerDurango::GetStreamOutDesc()
{
	// not supported for preprocessing yet
	return String::EMPTY;
}

Bool ShaderCompilerDurango::GetShaderTargetAndEntryPoint( StringAnsi& outShaderTarget, StringAnsi& outEntryPoint )
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









