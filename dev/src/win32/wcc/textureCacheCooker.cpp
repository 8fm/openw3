/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "textureCacheCooker.h"
#include "processRunner.h"
#include "../../common/core/dataBuffer.h"
#include "../../common/core/dependencyMapper.h"

#include "../../../internal/TexCookTool/texCookTool.h"


struct TexCookModule
{
	HMODULE m_handle;
	TexCookTool::ProcessDataFunc Process;
	TexCookTool::ReleaseDataFunc ReleaseData;
	TexCookTool::VerifyFunc Verify;
	TexCookTool::GetOutputStringFunc GetOutputString;

	TexCookModule()
		: m_handle( nullptr )
		, Process( nullptr )
		, ReleaseData( nullptr )
		, Verify( nullptr )
		, GetOutputString( nullptr )
	{}

	operator Bool() const
	{
		return m_handle != nullptr
			&& Process != nullptr
			&& ReleaseData != nullptr
			&& Verify != nullptr
			&& GetOutputString != nullptr;
	}
};

// HACK : To avoid reloading the same DLL for every texture, and also support the potential for multiple platforms in one go,
// we'll just keep a table of loaded modules.
// We don't worry about releasing it, because all library handles are released when the process exits anyways.
static THashMap< String, TexCookModule > s_loadedModules;

//////////////////////////////////////////////////////////////////////////////////////////////////////


static TexCookModule LoadTexCookModule( const String& toolPath )
{
	TexCookModule module;
	if ( !s_loadedModules.Find( toolPath, module ) )
	{
		// As suggested at http://stackoverflow.com/questions/4058303/silently-catch-windows-error-popups-when-calling-loadlibrary
		// Change error mode, so we don't get popups if loading fails (maybe can't find a dependency DLL or something)
		UINT oldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );
		SetErrorMode( oldErrorMode | SEM_FAILCRITICALERRORS );

		// Need to give absolute path for LOAD_WITH_ALTERED_SEARCH_PATH.
		String absolutePath = GFileManager->GetBaseDirectory() + toolPath;
		module.m_handle = LoadLibraryEx( absolutePath.AsChar(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH );

		// Restore error mode.
		SetErrorMode( oldErrorMode );

		if ( module.m_handle == nullptr )
		{
			ERR_WCC( TXT("Failed to load %ls"), absolutePath.AsChar() );
			return TexCookModule();
		}

		module.Process			= (TexCookTool::ProcessDataFunc)GetProcAddress( module.m_handle, "ProcessData" );
		module.ReleaseData		= (TexCookTool::ReleaseDataFunc)GetProcAddress( module.m_handle, "ReleaseData" );
		module.Verify			= (TexCookTool::VerifyFunc)GetProcAddress( module.m_handle, "Verify" );
		module.GetOutputString	= (TexCookTool::GetOutputStringFunc)GetProcAddress( module.m_handle, "GetOutputString" );

		if ( module.Process == nullptr )
		{
			ERR_WCC( TXT("Failed to find Process in %ls"), toolPath.AsChar() );
			FreeLibrary( module.m_handle );
			return TexCookModule();
		}
		if ( module.ReleaseData == nullptr )
		{
			ERR_WCC( TXT("Failed to find ReleaseData in %ls"), toolPath.AsChar() );
			FreeLibrary( module.m_handle );
			return TexCookModule();
		}
		if ( module.Verify == nullptr )
		{
			ERR_WCC( TXT("Failed to find Verify in %ls"), toolPath.AsChar() );
			FreeLibrary( module.m_handle );
			return TexCookModule();
		}
		if ( module.GetOutputString == nullptr )
		{
			ERR_WCC( TXT("Failed to find GetOutputString in %ls"), toolPath.AsChar() );
			FreeLibrary( module.m_handle );
			return TexCookModule();
		}

		if ( !module.Verify( sizeof( TexCookTool::SourceData ), GpuApi::TEXFMT_Max ) )
		{
			ERR_WCC( TXT("TexCookTool::Verify (%ls) failed. Maybe you've made some changes to GpuApi texture formats, and need to rebuild the tool?"), toolPath.AsChar() );
			FreeLibrary( module.m_handle );
			return TexCookModule();
		}

		s_loadedModules[ toolPath ] = module;
	}

	return module;
}

// For consoles, we should pre-tile the data so we can just drop it directly into memory when we're loading.
// We don't want to include the console-specific stuff directly here, so we pass it off to an external tool.
static Bool CookWithExternalTool( const ITextureBakerSource& textureSource, CTextureBakerOutput& output, const String& toolPath )
{
	TexCookModule tool = LoadTexCookModule( toolPath );
	if ( !tool.m_handle )
	{
		ERR_WCC( TXT("Invalid TexCookTool %ls"), toolPath.AsChar() );
		return false;
	}

	TexCookTool::SourceData sourceData;

	sourceData.mipCount = textureSource.GetMipCount();
	sourceData.texCount = textureSource.GetSliceCount();
	sourceData.baseWidth = textureSource.GetBaseWidth();
	sourceData.baseHeight = textureSource.GetBaseHeight();
	sourceData.texFormat = textureSource.GetTextureFormat();
	sourceData.texType = textureSource.GetTextureType();

	TDynArray< Uint32 > sizes( sourceData.texCount * sourceData.mipCount );
	TDynArray< Uint32 > pitches( sourceData.texCount * sourceData.mipCount );
	TDynArray< const void* > data( sourceData.texCount * sourceData.mipCount );
	for ( Uint16 tex_i = 0; tex_i < sourceData.texCount; ++tex_i )
	{
		for ( Uint16 mip_i = 0; mip_i < sourceData.mipCount; ++mip_i )
		{
			Uint32 idx = TexCookTool::GetTexMipIndex( mip_i, tex_i, sourceData.mipCount );

			data[ idx ] = textureSource.GetMipData( mip_i, tex_i );
			if ( data[ idx ] == nullptr )
			{
				ERR_WCC( TXT("No texture data for mip %u slice %u"), mip_i, tex_i );
				return false;
			}

			sizes[ idx ] = textureSource.GetMipDataSize( mip_i, tex_i );
			pitches[ idx ] = textureSource.GetMipPitch( mip_i, tex_i );
		}
	}

	sourceData.data = data.TypedData();
	sourceData.sizes = sizes.TypedData();
	sourceData.pitches = pitches.TypedData();

	TDynArray< Uint32 >& mipOffsets = output.GetMipOffsets();
	mipOffsets.Resize( sourceData.mipCount );

	Uint32 resultBaseAlignment = 0;
	Uint32 resultDataSize = 0;
	void* resultData = nullptr;

	if ( !tool.Process( sourceData, mipOffsets.TypedData(), resultData, resultDataSize, resultBaseAlignment ) )
	{
		ERR_WCC( TXT("%ls Process() failed. Output:\n%ls"), toolPath.AsChar(), tool.GetOutputString() );
		return false;
	}

	output.SetDataAlignment( resultBaseAlignment );
	output.WriteData( resultData, resultDataSize );

	tool.ReleaseData( resultData );


	return true;
}


//////////////////////////////////////////////////////////////////////////


// On PC, we just copy the data over.
static Bool TextureCookFunctionPC( const ITextureBakerSource& textureSource, CTextureBakerOutput& output )
{
	const Uint16 mipCount = textureSource.GetMipCount();
	const Uint16 texCount = textureSource.GetSliceCount();

	auto& mipOffsets = output.GetMipOffsets();
	mipOffsets.Resize( mipCount );

	// Lay out in the same ordering as consoles (mip0tex0, mip0tex1, ... mip0texN, mip1tex0, ...)
	for ( Uint16 mip_i = 0; mip_i < mipCount; ++mip_i )
	{
		mipOffsets[ mip_i ] = output.GetTotalDataSize();

		for ( Uint16 tex_i = 0; tex_i < texCount; ++tex_i )
		{
			const void* mipData = textureSource.GetMipData( mip_i, tex_i );
			if ( mipData == nullptr )
			{
				ERR_WCC( TXT("No texture data for mip %u slice %u"), mip_i, tex_i );
				return false;
			}

			Uint32 mipSize = textureSource.GetMipDataSize( mip_i, tex_i );
			output.WriteData( mipData, mipSize );
		}
	}
	output.SetDataAlignment( 16 );		// On PC, we can use practically any alignment, since we don't load in-place

	return true;
}

#ifndef WCC_LITE

static Bool TextureCookFunctionPS4( const ITextureBakerSource& textureSource, CTextureBakerOutput& output )
{
	return CookWithExternalTool( textureSource, output, TXT("tools\\TexCookTool\\TexCookPS4.dll") );
}


static Bool TextureCookFunctionXboxOne( const ITextureBakerSource& textureSource, CTextureBakerOutput& output )
{
	return CookWithExternalTool( textureSource, output, TXT("tools\\TexCookTool\\TexCookXboxOne.dll") );
}

#endif

//////////////////////////////////////////////////////////////////////////


CWccTextureCacheCooker::CWccTextureCacheCooker()
	: CTextureCacheCooker()
{
}

CWccTextureCacheCooker::~CWccTextureCacheCooker()
{
}


CAsyncTextureBaker::CookFunctionPtr CWccTextureCacheCooker::GetDefaultCookFunction( const ECookingPlatform platform )
{
	if ( platform == PLATFORM_PC )
	{
		return TextureCookFunctionPC;
	}
#ifndef WCC_LITE
	else if ( platform == PLATFORM_XboxOne )
	{
		return TextureCookFunctionXboxOne;
	}
	else if ( platform == PLATFORM_PS4 )
	{
		return TextureCookFunctionPS4;
	}
#endif

	RED_HALT( "Unknown cooking platform: %d", platform );
	return nullptr;
}

