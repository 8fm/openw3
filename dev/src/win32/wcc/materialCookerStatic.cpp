/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "materialCooker.h"
#include "wccStats.h"

#include "../../common/renderer/renderHelpers.h"
#include "..\..\common\engine\shaderCacheManager.h"
#include "..\..\common\engine\materialCompilerDefines.h"

static CStaticShadersStats m_shaderCacheStats;

struct ShaderToCompile
{
	String						m_shaderFileName;
	CMaterialCompilerDefines	m_localDefines;
	Uint32						m_shaderTypeFlags;
	GpuApi::eBufferChunkType	m_bufferChunkType;

	RED_INLINE ShaderToCompile( const String& name, const CMaterialCompilerDefines& localDefines, Uint32 shaderTypeFlags, GpuApi::eBufferChunkType bct = GpuApi::BCT_Max )
		: m_shaderFileName( name )
		, m_localDefines( localDefines )
		, m_shaderTypeFlags( shaderTypeFlags )
		, m_bufferChunkType( bct )
	{};
};

// must match method in renderInterface.cpp
namespace RenderInterfaceStubs
{
	Uint32 GetInitialMSAALevel( Uint32 *outMSAA_X, Uint32 *outMSAA_Y )
	{
		Uint32 msaa = 1;//Max<Uint32>( 1, SRenderSettingsManager::GetInstance().GetSettings().m_msaaLevel );
		Uint32 msaaX = 1;
		Uint32 msaaY = msaa / msaaX;

		if ( nullptr != outMSAA_X )
			*outMSAA_X = msaaX;
		if ( nullptr != outMSAA_Y )
			*outMSAA_Y = msaaY;
		return msaa;
	}

	static CMaterialCompilerDefines BuildMaterialCompilerDefinesMSAABase()
	{
		CMaterialCompilerDefines defines;

		Uint32 msaaLevelX, msaaLevelY;
		GetInitialMSAALevel( &msaaLevelX, &msaaLevelY );
		Uint32 msaaLevel = msaaLevelX * msaaLevelY;

		defines.Add( ("MSAA_NUM_SAMPLES_X"),	StringAnsi::Printf( ("%u"), msaaLevelX ) );
		defines.Add( ("MSAA_NUM_SAMPLES_Y"),	StringAnsi::Printf( ("%u"), msaaLevelY ) );
		defines.Add( ("MSAA_NUM_SAMPLES"),		StringAnsi::Printf( ("%u"), msaaLevel ) );
		defines.Add( ("MSAA_NUM_SAMPLES_INV"),	StringAnsi::Printf( ("%.12f"), (Float)(1.f / msaaLevel) ) );

		return defines;
	}
};

Bool CMaterialCooker::CompileStaticShaders( const MaterialCookingOptions& options )
{
	TDynArray< CMaterialCompilerDefines > systemDefines;
	TDynArray< ShaderToCompile > shadersToCompile;

	{
		// Initialize system defines for platform
		new ( systemDefines ) CMaterialCompilerDefines();

		#define RENDER_SHADER_GEN(var,name,defines) new ( shadersToCompile ) ShaderToCompile( name, defines, VertexShader | PixelShader )
		#define RENDER_SHADER_TESS_GEN(var,name,defines) new ( shadersToCompile ) ShaderToCompile( name, defines, VertexShader | HullShader | DomainShader | PixelShader )
		#define RENDER_SHADER_COMPUTE(var,name,defines) new ( shadersToCompile ) ShaderToCompile( name, defines, ComputeShader )
		#define RENDER_SHADER_GEOM_GEN(var,name,defines) new ( shadersToCompile ) ShaderToCompile( name, defines, VertexShader | GeometryShader | PixelShader )
		#define RENDER_SHADER_SO_GEN(var,name,defines,bctOut) new ( shadersToCompile ) ShaderToCompile( name, defines, VertexShader | GeometryShader, bctOut )
		#define RENDER_SHADER_DEFINES_NOOPT()		CMaterialCompilerDefines()
		#define RENDER_SHADER_DEFINES_BASE()		CMaterialCompilerDefines().Add( "__XBOX_WAVESIM_ITERATION_N", "1" ).Add( "STENCIL_TEX_FETCH_CHANNEL", (ECookingPlatform::PLATFORM_XboxOne == options.m_platform ? "0" : "1") )
		#define RENDER_SHADER_MSAA_DEFINES_BASE()	RenderInterfaceStubs::BuildMaterialCompilerDefinesMSAABase()

		#ifndef NO_EDITOR
			#define RENDER_SHADER_GEN_EDITOR RENDER_SHADER_GEN
		#else
			#define RENDER_SHADER_GEN_EDITOR(var,name,defines)
		#endif

		#include "..\..\common\engine\renderShaders.h"

		#undef RENDER_SHADER_GEN
		#undef RENDER_SHADER_TESS_GEN
		#undef RENDER_SHADER_COMPUTE
		#undef RENDER_SHADER_GEOM_GEN
		#undef RENDER_SHADER_SO_GEN
		#undef RENDER_SHADER_DEFINES_NOOPT
		#undef RENDER_SHADER_DEFINES_BASE
		#undef RENDER_SHADER_MSAA_DEFINES_BASE
	}

#if 0 // GFx 3
	{
		{
			String filename( TXT("guishaders") );
			String vertexShader( TXT("VertexShader") );
			String pixelShader( TXT("PixelShader") );

			for ( Uint32 i = 0; i < 12; ++i )
			{
				CMaterialCompilerDefines defines;

				String valueStr = String::Printf( TXT("%d"), i );
				defines.Add( vertexShader, valueStr );

				if ( VertexShaderTextTable[i] )
				{
					// Compile
					if ( !CompileVertexShader( m_shaderCache, options, filename, defines, VertexShaderTextTable[i]) )
					{
						// We cannot continue compilation
						break;
					}
				}
			}

			for ( Uint32 i = 0; i < 18; ++i )
			{
				CMaterialCompilerDefines defines;

				String valueStr = String::Printf( TXT("%d"), i );
				defines.Add( pixelShader, valueStr );

				if ( PixelShaderInitTable[i] )
				{
					// Compile
					if ( !CompilePixelShader( m_shaderCache, options, filename, defines, PixelShaderInitTable[i] ) )
					{
						// We cannot continue compilation
						break;
					}
				}
			}
		}
	}
#endif // #if 0

	// add the shader file name to fastPath to compile only this shader
	TDynArray< String > fastPath;
	/*
	fastPath.PushBack( TXT("dynamicDecalGen.fx") );
	fastPath.PushBack( TXT("morphedMeshGen.fx") );
	//*/

	// add the shader file name to shadersToSkip to skip compiling this shader
	TDynArray< String > shadersToSkip;

	// init stats
	if( options.m_collectStats )
	{
		m_shaderCacheStats.Reserve( shadersToCompile.Size() );
	}
	
	// Compile shaders
	for ( Uint32 i = 0; i < shadersToCompile.Size(); ++i )
	{
		const ShaderToCompile& shaderToCompile = shadersToCompile[i];

		Bool skipShader = false;

		if ( !fastPath.Empty() )
		{
			skipShader = true;
			for ( Uint32 j = 0; j < fastPath.Size(); ++j )
			{
				size_t index = 0;
				if ( shaderToCompile.m_shaderFileName.FindSubstring( fastPath[j], index ) )
				{
					skipShader = false;
					break;
				}
			}
		}
		for ( Uint32 j = 0; j < shadersToSkip.Size(); ++j )
		{
			size_t index = 0;
			if ( shaderToCompile.m_shaderFileName.FindSubstring( shadersToSkip[j], index ) )
			{
				skipShader = true;
				break;
			}
		}
		if ( skipShader )
		{
			continue;
		}

		// Compile
		if ( !CompileStaticShader( options, shaderToCompile.m_shaderFileName, shaderToCompile.m_localDefines, systemDefines, shaderToCompile.m_shaderTypeFlags, shaderToCompile.m_bufferChunkType ) )
		{
			return false;
		}
		LOG_WCC( TXT("StaticShaders: %u/%u compiled"), i + 1, shadersToCompile.Size() );
	}

	if( options.m_collectStats )
	{
		m_shaderCacheStats.DumpStatsToFile( String::Printf( TXT("StaticShadersStats_%s.csv"), CookingPlatformToString( options.m_platform ) ) );
	}
		
	return true;
}

Bool CMaterialCooker::CompileStaticShader( const MaterialCookingOptions& options, const String& shaderFileName, const CMaterialCompilerDefines& localDefines, const TDynArray< CMaterialCompilerDefines >& systemDefines, Uint32 shaderTypeFlags, GpuApi::eBufferChunkType bct)
{
	if ( !options.m_staticShaderCacheCooker || !options.m_shaderCacheCooker )
	{
		return false;
	}
	
	// Find file
	String absoluteFilePath = String( GpuApi::GetShaderRootPath() ) + shaderFileName;
	IFile* file = GFileManager->CreateFileReader( absoluteFilePath, FOF_AbsolutePath );
	if ( !file )
	{
		RED_LOG_ERROR( Shaders, TXT("Missing shader file: '%s'"), shaderFileName.AsChar() );
		return false;
	}
	else
	{
		RED_LOG( Shaders, TXT( "Cooking shader file '%s'" ), shaderFileName.AsChar() );
	}

	// Load code
	size_t fileSize = static_cast< size_t >( file->GetSize() );
	RED_ASSERT( (Uint64)fileSize == file->GetSize(), TXT("Unexpectedly large file '%s'"), file->GetFileNameForDebug() );
	AnsiChar* code = new AnsiChar[ fileSize+1 ];
	file->Serialize( code, fileSize );
	code[ fileSize ] = 0;
	delete file;

	// Add stats record
	if( options.m_collectStats )
	{
		m_shaderCacheStats.GenerateShaderStat( shaderFileName, fileSize );
	}
	
	// Compile the shader pair for every system setting
	for ( Uint32 i = 0; i < systemDefines.Size(); ++i )
	{
		// Compile complete defines list, system defines goes first
		CMaterialCompilerDefines defines( systemDefines[ i ] );
		defines.Add( localDefines );

		String definesString = String::EMPTY;
		const TMaterialCompilerDefinesList& defList = defines.GetDefines();
		for ( Uint32 i = 0; i < defList.Size(); ++i )
		{
			definesString += String::Printf( TXT( "'%s' -> '%s', " ), ANSI_TO_UNICODE( defList[ i ].m_first.AsChar() ), ANSI_TO_UNICODE( defList[i].m_second.AsChar() ) );
		}

		Uint32 shaderType = 0;
		for ( Uint32 j = 1; j < ShaderTypeFlagsMax; j = j << 1, ++shaderType )
		{
			eShaderTypeFlags shaderTypeFlag = (eShaderTypeFlags)j;

			if ( shaderTypeFlags & shaderTypeFlag )
			{
				const Char* shaderName = GetShaderName( shaderTypeFlag );

				// Compile shader
				SMaterialCookingContext context;
				context.m_shaderFileName		= shaderFileName;
				context.m_shaderFileNameAnsi	= UNICODE_TO_ANSI( shaderFileName.AsChar() );
				context.m_shaderType			= (GpuApi::eShaderType)shaderType;
				context.m_shadersMask			= shaderTypeFlags;
				context.m_definitions			= defines;
				context.m_dumpAssembly			= options.m_dumpAssembly;
				context.m_bufferChunkType		= bct;
				context.m_isStaticShader		= true;

				SMaterialCookingOutput output;
				ECompilationResult compilationResult = CompileShader( options, code, context, output );
				switch ( compilationResult )
				{
				case ECR_CompilationSuccessful:
					{
						RED_LOG( Shaders, TXT("Compiled %s for shader file '%s', hash: [%") RED_PRIWu64 TXT("] defines: [%s] contentCRC: [%") RED_PRIWu64 TXT("]"), shaderName, shaderFileName.AsChar(), output.m_fileNameHash, definesString.AsChar(), output.m_shaderHash );
						StaticShaderEntry* entry = new StaticShaderEntry();
						entry->m_hash = output.m_fileNameHash;
						entry->m_contentCRC = output.m_shaderHash;
						entry->m_data = output.m_shaderData;
						options.m_staticShaderCacheCooker->AddShader( entry->m_hash, entry );
						if( options.m_collectStats )
						{
							// populate stat size
							m_shaderCacheStats.AddStat( shaderFileName, entry->m_data.GetSize() );
						}
						break;
					}
				case ECR_CompilationFailed:
					{
						ERR_WCC( TXT("Error compiling %s for shader file '%s', defines: [%s]"), shaderName, shaderFileName.AsChar(), definesString.AsChar() );
						break;
					}
				case ECR_FoundInCache:
					{
						RED_LOG( Shaders, TXT("[%") RED_PRIWu64 TXT("] %s for shader file '%s', defines: [%s] was already found in cache"), output.m_shaderHash, shaderName, shaderFileName.AsChar(), definesString.AsChar() );
						break;
					}
				}
			}
		}
	}

	// Shader compiled
	return true;
}
