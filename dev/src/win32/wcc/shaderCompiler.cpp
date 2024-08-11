#include "build.h"

#include "shaderCompiler.h"
#include "shaderCompilerPC.h"
#ifndef WCC_LITE
#include "shaderCompilerOrbis.h"
#include "shaderCompilerDurango.h"
#endif
#include "processRunner.h"
#include "materialCooker.h"
#include "../../common/core/dependencyMapper.h"
#include "../../common/redIO/redIO.h"
#include "../../common/redIO/redIOCommon.h"
#include "../../common/redSystem/crt.h"

volatile LONG IShaderCompiler::s_id = 0;

IShaderCompiler* IShaderCompiler::Create( ECookingPlatform platform, const SMaterialCookingContext& context, const String& dumpFileName, const String& dumpDirPath )
{
	IShaderCompiler* res = nullptr;
	switch ( platform )
	{
	case PLATFORM_PC:
		res = new ShaderCompilerPC( context, dumpFileName, dumpDirPath );
		break;
#ifndef WCC_LITE
	case PLATFORM_PS4:
		res = new ShaderCompilerOrbis( context, dumpFileName, dumpDirPath );
		break;
	case PLATFORM_XboxOne:
		res = new ShaderCompilerDurango( context, dumpFileName, dumpDirPath );
		break;
#endif
	default:
		ERR_WCC( TXT("Platform unsupported") );
		return nullptr;
	}

	RED_ASSERT( res );
	res->Initialize();


	return res;
}

void IShaderCompiler::Initialize()
{
	ResolveDefines();
	
	// Get entry point and target
	RED_VERIFY( GetShaderTargetAndEntryPoint( m_shaderTarget, m_entryPoint ) );
}

Bool IShaderCompiler::PrepareInputFile( const String& inFile, const AnsiChar* code, Uint32 codeLength )
{
	Red::IO::CNativeFileHandle inputFileHandle;
	if ( !inputFileHandle.Open( inFile.AsChar(), Red::IO::eOpenFlag_WriteNew ) )
	{
		WARN_WCC( TXT("Unable to create fake input file '%s'."), inFile.AsChar() );
		return false;
	}

	RED_ASSERT( inputFileHandle.IsValid() );
	
	Uint32 dummyOut = 0;
	Bool res = inputFileHandle.Write( code, codeLength, dummyOut );
	RED_VERIFY( inputFileHandle.Close() );
	return res;
}

Bool IShaderCompiler::DumpShaderInfo( Uint64 shaderHash, const String& inFile /*=String::EMPTY*/ )
{
	if ( !m_dumpFilenameValid )
	{
		return false;
	}

	Red::IO::CNativeFileHandle dumpFileHandle;
	if ( !dumpFileHandle.Open( m_dumpFilename.AsChar(), Red::IO::eOpenFlag_Write | Red::IO::eOpenFlag_Create | Red::IO::eOpenFlag_Append ) )
	{
		return false;
	}

	String definesList;
	for ( Uint32 i = 0; i < m_defines.Size(); ++i )
	{
		definesList += String::Printf( TXT( "'%ls'='%ls', " ), ANSI_TO_UNICODE( m_defines[i].Name ), ANSI_TO_UNICODE( m_defines[i].Definition ) );
	}
	String s = String::Printf( TXT( "Shader [%ls] with defines [%ls] compiled to hash [%" ) RED_PRIWu64 TXT( "]" ), m_context.m_shaderFileName.AsChar(), definesList.AsChar(), shaderHash );
	if ( !inFile.Empty() )
	{
		s += String::Printf( TXT(" from input file [%ls]"), inFile.AsChar() );
	}
	s += TXT("\r\n");

	Uint32 dummyOut;
	AnsiChar* ansiStr = UNICODE_TO_ANSI( s.TypedData() );
	Uint32 dataSize = s.GetLength() * sizeof( AnsiChar );
	Bool res = dumpFileHandle.Write( ansiStr, dataSize, dummyOut );
	RED_VERIFY( dumpFileHandle.Close() );
	return res;
}

Bool IShaderCompiler::DumpError( const String& error )
{
	if ( !m_dumpFilenameValid )
	{
		return false;
	}

	Red::IO::CNativeFileHandle errorFileHandle;
	if ( !errorFileHandle.Open( m_errorsFilename.AsChar(), Red::IO::eOpenFlag_Write| Red::IO::eOpenFlag_Create | Red::IO::eOpenFlag_Append ) )
	{
		return false;
	}
	String newlinedError = error + TXT("\n");
	StringAnsi ansiError( UNICODE_TO_ANSI( newlinedError.AsChar() ) );

	Uint32 dummyOut;
	Bool res = errorFileHandle.Write( ansiError.TypedData(), ansiError.GetLength(), dummyOut );
	RED_VERIFY( errorFileHandle.Close() );
	return res;
}

void IShaderCompiler::SetupFilesAndPaths( const String& dumpDirPath )
{
	// Prepare fileNames
	switch ( m_context.m_shaderType )
	{
	case GpuApi::VertexShader:		m_shortName = TXT( "vs" ); break;
	case GpuApi::PixelShader:		m_shortName = TXT( "ps" ); break;
	case GpuApi::GeometryShader:	m_shortName = TXT( "gs" ); break;
	case GpuApi::HullShader:		m_shortName = TXT( "hs" ); break;
	case GpuApi::DomainShader:		m_shortName = TXT( "ds" ); break;
	case GpuApi::ComputeShader:		m_shortName = TXT( "cs" ); break;
	}

	m_dumpDir = dumpDirPath + m_shortName;
}

void IShaderCompiler::ResolveDefines()
{
	const GpuApi::eShaderType& shaderType = m_context.m_shaderType;

	if ( shaderType == GpuApi::GeometryShader || shaderType == GpuApi::HullShader || shaderType == GpuApi::DomainShader )
	{
		m_defines.PushBack( GpuApi::ShaderDefine() );
		m_defines.Back().Name = "VERTEXSHADER";
		m_defines.Back().Definition = "1";
	}

	switch ( shaderType )
	{
	case GpuApi::VertexShader:
		m_defines.PushBack( GpuApi::ShaderDefine() );
		m_defines.Back().Name = "VERTEXSHADER";
		m_defines.Back().Definition = "1";
		break;
		
	case GpuApi::PixelShader:
		m_defines.PushBack( GpuApi::ShaderDefine() );
		m_defines.Back().Name = "PIXELSHADER";
		m_defines.Back().Definition = "1";
		break;

	case GpuApi::GeometryShader:
		m_defines.PushBack( GpuApi::ShaderDefine() );
		m_defines.Back().Name = "GEOMETRYSHADER";
		m_defines.Back().Definition = "1";
		break;

	case GpuApi::HullShader:
		m_defines.PushBack( GpuApi::ShaderDefine() );
		m_defines.Back().Name = "HULLSHADER";
		m_defines.Back().Definition = "1";
		break;

	case GpuApi::DomainShader:
		m_defines.PushBack( GpuApi::ShaderDefine() );
		m_defines.Back().Name = "DOMAINSHADER";
		m_defines.Back().Definition = "1";
		break;

	case GpuApi::ComputeShader:
		m_defines.PushBack( GpuApi::ShaderDefine() );
		m_defines.Back().Name = "COMPUTESHADER";
		m_defines.Back().Definition = "1";
		break;
	}

	AddPlatformSpecificDefines();

	const TMaterialCompilerDefinesList& rawDefines = m_context.m_definitions.GetDefines();
	for ( Uint32 i = 0; i < rawDefines.Size(); ++i )
	{
		m_defines.PushBack( GpuApi::ShaderDefine() );
		m_defines.Back().Name = rawDefines[i].m_first.AsChar();
		m_defines.Back().Definition = rawDefines[i].m_second.AsChar();
	}
}

String IShaderCompiler::GetParameterName( EParameterType paramType )
{
	switch ( paramType )
	{
	case STREAMOUT_DESC:		return TXT("streamOutDesc");
	case INPUT_FILE:			return TXT("inFile");
	case OUTPUT_FILE:			return TXT("outFile");
	case ENTRY_POINT:			return TXT("entryPoint");
	case SHADER_TARGET:			return TXT("shaderTarget");
	case SDB_DIR:				return TXT("sdbDir");
	case INCLUDES_DIRECTORY:	return TXT("includesDir");
	case UPDB_FILE:				return TXT("updbFile");
	case ASSEMBLY_FILE:			return TXT("assemblyFile");

	default:
		ERR_WCC( TXT( "Unknown param" ) );
	}
	return String::EMPTY;
}