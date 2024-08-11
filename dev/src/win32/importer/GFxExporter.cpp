/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#pragma optimize("",off)

#include "../../common/redSystem/guid.h"
#include "../../common/redSystem/utility.h"
#include "../../common/gpuApiUtils/gpuApiMemory.h"

#include "../../common/core/memory.h"
#include "../../common/core/feedback.h"

#include "GFxExporter.h"

static Bool GShowFeedback = false;

#define ERR_MSG( fmt, ... )\
do\
{\
	ERR_IMPORTER( fmt, ## __VA_ARGS__ );\
	if ( GShowFeedback )\
	{\
		GFeedback->ShowError( fmt, ## __VA_ARGS__ );\
	}\
}while(0)

static Bool RunProcess( const String& exeName, const String& cmdLine );
static Bool GFxImportTexture( const wchar_t* importPath, GpuApi::eTextureImportFormat format, /*out*/void*& dst, /*out*/Uint32& rowPitch, /*out*/Uint32& depthPitch, /*out*/GpuApi::TextureDesc& desc );

CGFxExporter::CGFxExporter()
{
}

Bool CGFxExporter::DoExport( const SExportOptions& exportOptions, SExportInfo& outExportInfo, Bool showFeedback /*= true*/ )
{
	Red::System::ScopedFlag<Bool> scopedFlag( GShowFeedback = showFeedback, false );

	String imageQualityOpt;
	switch( exportOptions.m_imageQuality )
	{
	case ImageQuality_Fast:
		imageQualityOpt = TXT("-quick");
		break;
	case ImageQuality_Normal:
		imageQualityOpt = TXT("-quality_normal");
		break;
	case ImageQuality_Production:
		imageQualityOpt = TXT("-quality_production");
		break;
	case ImageQuality_Highest:
		imageQualityOpt = TXT("-quality_highest");
		break;
	default:
		ERR_MSG( TXT("Unknown image quality '%u'"), (Uint32)exportOptions.m_imageQuality );
		return false;
		break;
	}

	const String exeName = exportOptions.m_gfxExportExePath;

	const CFilePath sourceFilePath( exportOptions.m_sourceFilePath.ToLower() );
	const CFilePath linkageFilePath( exportOptions.m_linkageFilePath.ToLower() );

	const Uint32 swfHash = static_cast< Uint32 >( Red::CNameHash::Hash( exportOptions.m_linkageFilePath.ToLower().AsChar() ).GetValue() );

	String linkageNameSuffix = linkageFilePath.GetFileName();
	linkageNameSuffix.ReplaceAll( TXT("_"), TXT("-") ); // convert to hyphens so exported images can parse the underscore to find their SWF
	// TODO: With shipping import settings can have a linkage name with just the hash
	const String linkageName = String::Printf(TXT("%ls{%08x}"), linkageNameSuffix.AsChar(), swfHash );

	const String swfName = sourceFilePath.GetFileName();
	const String exportPath = exportOptions.m_dumpDirectoryPath + swfName + TXT("\\");
	const String swfCompressOpts = exportOptions.m_doSwfCompression ? TXT("-c") : TXT("");

	// TBD: -sq might help with texture stretching?
	const String imageOpts = String::Printf( TXT("DDS -d1c -d5 %ls -pack -ptresize mult4 -share_images"), imageQualityOpt.AsChar() );

	// "-ne" to not use export names, otherwise they can break the hash naming convention
	const String exportCmdLine = String::Printf( TXT("%ls %ls %ls -i %ls -ne -list -lwr -d %ls -o %ls -p %ls"), 
		exeName.AsChar(),
		exportOptions.m_sourceFilePath.AsChar(),
		swfCompressOpts.AsChar(),
		imageOpts.AsChar(),
		exportPath.AsChar(), exportPath.AsChar(),
		linkageName.AsChar() );

	if ( ! GSystemIO.CreatePath( exportPath.AsChar() ) )
	{
		ERR_MSG( TXT("Can't create temp export path '%ls'"), exportPath.AsChar() );
		return nullptr;
	}

	Char cwd[ MAX_PATH ] = { 0 };
	::GetCurrentDirectory( MAX_PATH, cwd );
	LOG_IMPORTER( TXT("Running '%ls' ['%ls']"), exportCmdLine.AsChar(), cwd );
	if ( ! RunProcess( exeName, exportCmdLine ) )
	{
		ERR_MSG( TXT("Failed to run '%ls'"), exportCmdLine.AsChar() );
		return nullptr;
	}

	TDynArray< String > textureFilePaths;
	String fileBlob;
	if ( GFileManager->LoadFileToString( exportPath + swfName + TXT(".lst"), fileBlob, true ) )
	{
		textureFilePaths = fileBlob.Split( TXT("\r\n") );
	}

	outExportInfo.m_linkageName = linkageName + TXT(".gfx");
	outExportInfo.m_imageExportString = imageOpts;

	const String gfxPath = exportPath + swfName + TXT(".gfx");

	IFile* gfxReader = GFileManager->CreateFileReader( gfxPath, FOF_AbsolutePath | FOF_Buffered );
	if ( ! gfxReader )
	{
		return false;
	}

	outExportInfo.m_strippedSwf.Allocate( static_cast< DataBuffer::TSize >( gfxReader->GetSize() ) );
	gfxReader->Serialize( outExportInfo.m_strippedSwf.GetData(), outExportInfo.m_strippedSwf.GetSize() );
	delete gfxReader;
	gfxReader = nullptr;

	for ( Uint32 i = 0; i < textureFilePaths.Size(); ++i )
	{
		String& absoluteFilePath = textureFilePaths[ i ];
		absoluteFilePath.ReplaceAll( TXT('/'), TXT('\\') );
		CFilePath texturePath( absoluteFilePath );

		outExportInfo.m_textureInfos.PushBack( CSwfResource::TextureInfo() );
		CSwfResource::TextureInfo& texInfo = outExportInfo.m_textureInfos.Back();
		texInfo.m_linkageName = texturePath.GetFileNameWithExt();
		if ( ! ImportSwfTexture( absoluteFilePath, texInfo ) )
		{
			ERR_MSG( TXT("Failed to import texture '%ls' for file '%ls'"), absoluteFilePath.AsChar(), exportOptions.m_sourceFilePath.AsChar() );
			return false;
		}
	}

	if ( exportOptions.m_doFullExports && ! GetSwfExportInfo( gfxPath, outExportInfo.m_fonts, outExportInfo.m_header ) )
	{
		ERR_MSG( TXT("Could not get export info for GFx SWF '%ls'."), gfxPath.AsChar() );
		return false;
	}

	return true;
}

Bool CGFxExporter::ImportSwfTexture( const String& absolutePath, CSwfResource::TextureInfo& outTextureInfo )
{
	void* srcData = NULL;
	Uint32 rowPitch, depthPitch;
	GpuApi::TextureDesc desc;

	size_t lastDotIndex;
	if ( !absolutePath.FindCharacter( TXT('.'), lastDotIndex, true ) )
	{
		ERR_MSG( TXT("Error parsing resource filename.") );
		return false;
	}
	ASSERT( lastDotIndex < absolutePath.Size(), TXT(" No separator for file extension found, invalid filename ") );
	const String ext = absolutePath.MidString( lastDotIndex + 1, absolutePath.Size() - lastDotIndex );
	if ( ext != TXT("dds") )
	{
		ERR_MSG( TXT("Not a DDS '%ls'"), absolutePath.AsChar() );
		return false;
	}

	if ( ! GFxImportTexture( absolutePath.AsChar(), GpuApi::TIF_DDS, srcData, rowPitch, depthPitch, desc ) )
	{
		ERR_MSG( TXT("Texture import failed") );
		return false;
	}

	ASSERT( srcData );

	Uint32 pixelSize = 0;
	ETextureRawFormat rawFormat;
	if ( desc.format == GpuApi::TEXFMT_Float_R32G32B32A32 )
	{
		// Load as HDR image
		rawFormat = TRF_HDR;
		pixelSize = 16;
	}
	else if ( desc.format == GpuApi::TEXFMT_Float_R32 )
	{
		// Load as HDR image
		rawFormat = TRF_HDRGrayscale;
		pixelSize = 4;
	}
	else if ( desc.format == GpuApi::TEXFMT_A8L8 || desc.format == GpuApi::TEXFMT_Uint_16_norm )
	{
		// Load as gray scale image
		rawFormat = TRF_AlphaGrayscale;
		pixelSize = 2;
	}
	else if ( desc.format ==  GpuApi::TEXFMT_A8 || desc.format == GpuApi::TEXFMT_L8 )
	{
		// Load as gray scale image
		rawFormat = TRF_Grayscale;
		pixelSize = 1;
	}
	else
	{
		// Load as True Color image
		rawFormat = TRF_TrueColor;
		pixelSize = 4;
	}

	CName textureGroupName;
	switch ( desc.format )
	{
	case GpuApi::TEXFMT_BC1:
		textureGroupName = RED_NAME( GUIWithoutAlpha );
		break;
	case GpuApi::TEXFMT_BC3:
		textureGroupName = RED_NAME( GUIWithAlpha );
		break;
	default:
		break;
	}

	if ( ! textureGroupName )
	{
		ERR_MSG( TXT("Unhandled texture group for format '%u'"), (Uint32)desc.format );
		return false;
	}

	outTextureInfo.m_textureGroupName = textureGroupName;
	outTextureInfo.m_textureRawFormat = rawFormat;

	// depthPitch is how big the buffer should be, but rowPitch is the required pitch for the DDS,
	// so just fake it and tell the MipMap the pitch is depthPitch/height so it'll allocate a buffer of height*deptPitch/height size
	// as the rowPitch would be too large
	outTextureInfo.m_mipMap = CBitmapTexture::MipMap( desc.width, desc.height, depthPitch / desc.height );
	outTextureInfo.m_mipMap.m_pitch = rowPitch;
	Red::System::MemoryCopy( outTextureInfo.m_mipMap.m_data.GetData(), srcData, depthPitch );

	// free data that was used to initialize the texture
	//GpuApi::FreeTextureData( srcData );

	RED_MEMORY_FREE( MemoryPool_Default, MC_BufferBitmapFlash, srcData );

	srcData = NULL;

	return true;
}

#ifdef USE_SCALEFORM
Bool CGFxExporter::GetSwfExportInfo( const String& absolutePath, TDynArray< SSwfFontDesc >& outSwfFontDescs, SSwfHeaderInfo& outSwfHeaderInfo )
{
	GFx::Loader loader;
	loader.SetAS3Support( SF::Ptr<GFx::AS3Support>( *SF_NEW GFx::AS3Support ) );

	Uint32 loadConstants = GFx::Loader::LoadDisableImports | GFx::Loader::LoadWaitFrame1 | GFx::Loader::LoadDisableSWF;
	SF::Ptr< GFx::MovieDef > movieDef = loader.CreateMovie( FLASH_TXT_TO_UTF8( absolutePath.AsChar() ), loadConstants );
	if ( ! movieDef )
	{
		ERR_MSG( TXT("GetSwfExportInfo: Failed to load Flash SWF '%ls'"), absolutePath.AsChar() );
		return false;
	}

	outSwfHeaderInfo.m_frameRate = movieDef->GetFrameRate();
	outSwfHeaderInfo.m_frameHeight = movieDef->GetFrameRect().Height();
	outSwfHeaderInfo.m_frameWidth = movieDef->GetFrameRect().Width();
	outSwfHeaderInfo.m_frameCount = movieDef->GetFrameCount();
	outSwfHeaderInfo.m_height = movieDef->GetHeight();
	outSwfHeaderInfo.m_width = movieDef->GetWidth();
	outSwfHeaderInfo.m_version = movieDef->GetVersion();
	outSwfHeaderInfo.m_compressed = ( movieDef->GetSWFFlags() & GFx::MovieInfo::SWF_Compressed ) != 0;

	struct SFontVisitor : GFx::MovieDef::ResourceVisitor, Red::System::NonCopyable
	{
		TDynArray< SSwfFontDesc >& m_swfFontDescRef;

		virtual void Visit( GFx::MovieDef* pmovieDef, GFx::Resource* presource, GFx::ResourceId id, const SFChar* pexportName ) override
		{
			RED_ASSERT( pmovieDef );
			RED_ASSERT( presource );

			RED_ASSERT( presource->GetResourceType() == GFx::Resource::RT_Font );
			if ( presource->GetResourceType() == GFx::Resource::RT_Font )
			{
				GFx::FontResource* fontResource = static_cast< GFx::FontResource* >( presource );
				String fontName = FLASH_UTF8_TO_TXT( fontResource->GetName() );
				Uint32 numGlyphs = fontResource->GetGlyphShapeCount();
				const Bool italic = fontResource->IsItalic();
				const Bool bold = fontResource->IsBold();

				m_swfFontDescRef.PushBack( SSwfFontDesc( fontName, numGlyphs, italic, bold ) );
			}
		}

		SFontVisitor( TDynArray< SSwfFontDesc >& swfFontDesc )
			: m_swfFontDescRef( swfFontDesc )
		{
		}
	};

	SFontVisitor fontVisitor( outSwfFontDescs );
	movieDef->VisitResources( &fontVisitor, GFx::MovieDef::ResVisit_Fonts );

	return true;
}

#if 0
Bool CGFxExporter::GetSwfImportInfos( const String& absolutePath, TDynArray< SSwfImportInfo >& outSwfImports )
{
	GFx::Loader loader;
	loader.SetAS3Support( SF::Ptr<GFx::AS3Support>( *SF_NEW GFx::AS3Support ) );

	Uint32 loadConstants = GFx::Loader::LoadWaitFrame1 | GFx::Loader::LoadDisableSWF;
	SF::Ptr< GFx::MovieDef > movieDef = loader.CreateMovie( FLASH_TXT_TO_UTF8( absolutePath.AsChar() ), loadConstants );
	if ( ! movieDef )
	{
		ERR_IMPORTER( TXT("GetSwfImportInfos: Failed to load Flash SWF '%ls'"), absolutePath.AsChar() );
		return false;
	}

	struct SImportVisitor : GFx::MovieDef::ImportVisitor, Red::System::NonCopyable
	{
		TDynArray< SSwfImportInfo >& m_swfImportsRef;

		virtual void Visit( GFx::MovieDef* pparentDef, GFx::MovieDef* pimportDef, const char* pimportedMovieFilename ) override
		{
			m_swfImportsRef.PushBack( SSwfImportInfo( ANSI_TO_UNICODE( pimportedMovieFilename ), ANSI_TO_UNICODE( pimportDef->GetFileURL() ) ) );
		}

		SImportVisitor( TDynArray< SSwfImportInfo >& outSwfImports )
			: m_swfImportsRef( outSwfImports )
		{}
	};

	SImportVisitor importVisitor( outSwfImports );
	movieDef->VisitImportedMovies( &importVisitor );

	return true;
}
#endif // if 0

#endif // USE_SCALEFORM

static Bool RunProcess( const String& exeName, const String& cmdLine )
{
	STARTUPINFOW siStartupInfo;
	PROCESS_INFORMATION piProcessInfo;
	memset(&siStartupInfo, 0, sizeof(siStartupInfo));
	memset(&piProcessInfo, 0, sizeof(piProcessInfo));
	siStartupInfo.cb = sizeof(siStartupInfo);

	//CreateProcess CAN modify commandLine
	wchar_t * pwszParam = new wchar_t[cmdLine.Size() + 1];
	wcscpy_s(pwszParam, cmdLine.Size() + 1, cmdLine.AsChar()); 	
	DWORD retCode = 1;
	if (CreateProcessW(exeName.AsChar(), pwszParam, 0, 0, false, CREATE_DEFAULT_ERROR_MODE, 0, 0, &siStartupInfo, &piProcessInfo))
	{
		//wait for exit
		retCode = WaitForSingleObject(piProcessInfo.hProcess, INFINITE);		
	}	
	CloseHandle(piProcessInfo.hProcess);
	CloseHandle(piProcessInfo.hThread);
	delete[] pwszParam;

	return retCode == 0;
}

// No D3D device in the cooker, so copy/paste/modify the GpuApi function here to use some function that doesn't even need it.
#ifndef NO_TEXTURE_IMPORT

// Appending "GFx" because Koenig lookup will make us use the ones in the GpuApi (and not going to rely on explictly using global namespace, which is fragile here)

static GpuApi::eTextureFormat GFxMap( DXGI_FORMAT format );

static Bool GFxImportTexture( const wchar_t* importPath, GpuApi::eTextureImportFormat format, /*out*/void*& dst, /*out*/Uint32& rowPitch, /*out*/Uint32& depthPitch, /*out*/GpuApi::TextureDesc& desc )
{
	// Read bitmap info
	DirectX::TexMetadata metadata;
	Red::System::MemorySet( &metadata, 0, sizeof( metadata ) );

	HRESULT hr = S_FALSE;
	switch ( format )
	{
	case GpuApi::TIF_DDS:
		hr = DirectX::GetMetadataFromDDSFile( importPath, DirectX::DDS_FLAGS_NONE, metadata );		
		break;
	case GpuApi::TIF_TGA:
		hr = DirectX::GetMetadataFromTGAFile( importPath, metadata );
		break;
	case GpuApi::TIF_WIC:
		// PNGs (and possibly others?) can be imported in BGR order, but WIC_FLAGS_FORCE_RGB forces it into RGB, which is good for us!
		hr = DirectX::GetMetadataFromWICFile( importPath, DirectX::WIC_FLAGS_FORCE_RGB, metadata );
		break;
	}

	if ( FAILED( hr ) )
	{			
		ERR_MSG( TXT("Unknown texture import format!") );
		return false;
	}

	// Texture size should be power of 2 !

	Bool isPow2Width = Red::Math::IsPow2( static_cast< Int32 >( metadata.width ) );
	Bool isPow2Height = Red::Math::IsPow2( static_cast< Int32 >( metadata.height ) );

	if ( format == GpuApi::TIF_DDS )
	{
		if ( metadata.width < 1 || metadata.height < 1 || (metadata.width & 3) != 0 || (metadata.height & 3 ) != 0 )
		{
			ERR_MSG( TXT("DDS Texture size should be a multiple of four") );
			return false;
		}
	}
	else if ( !isPow2Width || !isPow2Height )
	{
		ERR_MSG( TXT("Texture size should be power of two") );
		return false;
	}

	DirectX::ScratchImage image;
	hr = S_FALSE;
	switch ( format )
	{
	case GpuApi::TIF_DDS:
		hr = DirectX::LoadFromDDSFile( importPath, DirectX::DDS_FLAGS_NONE, NULL, image );
		break;
	case GpuApi::TIF_TGA:
		hr = DirectX::LoadFromTGAFile( importPath, NULL, image );
		break;
	case GpuApi::TIF_WIC:
		hr = DirectX::LoadFromWICFile( importPath, DirectX::WIC_FLAGS_FORCE_RGB, NULL, image );
		break;
	}

	if ( FAILED( hr ) )
	{
		ERR_MSG( TXT("Unable to create texture from file") );
		return false;
	}

	const DirectX::Image* loadedImage = image.GetImage(0, 0, 0);
	desc.width	= static_cast< GpuApi::Uint32 >( metadata.width );
	desc.height = static_cast< GpuApi::Uint32 >( metadata.height );
	desc.format = GFxMap( metadata.format );
	rowPitch	= static_cast< GpuApi::Uint32 >( loadedImage->rowPitch );
	depthPitch	= static_cast< GpuApi::Uint32 >( loadedImage->slicePitch );

//	GPUAPI_ASSERT( !dst, TXT( "Destination memory already allocated" ) );
	dst = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_BufferBitmapFlash, loadedImage->slicePitch, 16 );
//	GPUAPI_ASSERT( dst, TXT( "Destination memory can't be allocated" ) );

	Red::System::MemoryCopy( dst, loadedImage->pixels, static_cast< GpuApi::Uint32 >( loadedImage->slicePitch ) );

	return true;
}

static GpuApi::eTextureFormat GFxMap( DXGI_FORMAT format )
{
	switch ( format )
	{
	case DXGI_FORMAT_A8_UNORM:				return GpuApi::TEXFMT_A8;
	case DXGI_FORMAT_R8_UNORM:				return GpuApi::TEXFMT_L8;
	case DXGI_FORMAT_R8_UINT:				return GpuApi::TEXFMT_R8_Uint;
	case DXGI_FORMAT_R8G8B8A8_UNORM:		return GpuApi::TEXFMT_R8G8B8A8;
	case DXGI_FORMAT_R8G8_TYPELESS:			return GpuApi::TEXFMT_A8L8;
	case DXGI_FORMAT_R8G8_UNORM:			return GpuApi::TEXFMT_A8L8;
	case DXGI_FORMAT_R16_UNORM:				return GpuApi::TEXFMT_Uint_16_norm;
	case DXGI_FORMAT_R16_UINT:				return GpuApi::TEXFMT_Uint_16;
	case DXGI_FORMAT_R16G16_UINT:			return GpuApi::TEXFMT_R16G16_Uint;
	case DXGI_FORMAT_R10G10B10A2_UNORM:		return GpuApi::TEXFMT_Float_R10G10B10A2;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:	return GpuApi::TEXFMT_Float_R16G16B16A16;
	case DXGI_FORMAT_R11G11B10_FLOAT:		return GpuApi::TEXFMT_Float_R11G11B10;
	case DXGI_FORMAT_R16G16_FLOAT:			return GpuApi::TEXFMT_Float_R16G16;
	case DXGI_FORMAT_R32G32_FLOAT:			return GpuApi::TEXFMT_Float_R32G32; //dex
	case DXGI_FORMAT_R32G32B32A32_FLOAT:	return GpuApi::TEXFMT_Float_R32G32B32A32;
	case DXGI_FORMAT_R32_FLOAT:				return GpuApi::TEXFMT_Float_R32;
	case DXGI_FORMAT_R16_FLOAT:				return GpuApi::TEXFMT_Float_R16;
	case DXGI_FORMAT_R24G8_TYPELESS:		return GpuApi::TEXFMT_D24S8;
	case DXGI_FORMAT_BC1_UNORM:				return GpuApi::TEXFMT_BC1;
	case DXGI_FORMAT_BC2_UNORM:				return GpuApi::TEXFMT_BC2;
	case DXGI_FORMAT_BC3_UNORM:				return GpuApi::TEXFMT_BC3;
	case DXGI_FORMAT_BC4_UNORM:				return GpuApi::TEXFMT_BC4;
	case DXGI_FORMAT_BC5_UNORM:				return GpuApi::TEXFMT_BC5;
	case DXGI_FORMAT_BC6H_UF16:				return GpuApi::TEXFMT_BC6H;
	case DXGI_FORMAT_BC7_UNORM:				return GpuApi::TEXFMT_BC7;
	default:							ERR_IMPORTER( TXT("invalid DXGI format") ); return GpuApi::TEXFMT_Max;
	}
}

#endif