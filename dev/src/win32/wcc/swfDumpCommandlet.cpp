#include "build.h"
#include "wccVersionControl.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/dependencyLinker.h"
#include "../../common/core/depot.h"
#include "../../common/core/garbageCollector.h"

#include "../../common/engine/swfResource.h"
#include "../../common/engine/swfTexture.h"
#include "../../common/engine/bitmapTexture.h"
#include "../../common/redIO/redIO.h"

//////////////////////////////////////////////////////////////////////////

class CSwfDumpCommandlet: public ICommandlet
{
	DECLARE_ENGINE_CLASS( CSwfDumpCommandlet, ICommandlet, 0 );

private:
	struct SSaveFormatExtEntry
	{
		const Char*					m_ext;
		GpuApi::eTextureSaveFormat	m_textureSaveFormat;
	};

private:
	static const SSaveFormatExtEntry s_saveFormatExts[];
	static const Char* GetExtForSaveFormat( GpuApi::eTextureSaveFormat saveFormat );

private:
	struct STextureData 
	{
		CBitmapTexture::MipMap	m_mipCopy;
		Uint32					m_format;

		STextureData();
		~STextureData();
	};

public:
	CSwfDumpCommandlet();

public:
	// Executes commandlet command
	virtual Bool Execute( const CommandletOptions& options );

	void CreateBatFile( const String& path );

	// Returns commandlet one-liner
	virtual const Char* GetOneLiner() const
	{
		return TXT("Dump SWF stuff");
	}

	// Prints commandlet help
	virtual void PrintHelp() const
	{
		LOG_WCC( TXT("Dumps SWF resources. Use wcc swfdump [out directory]") );
	}

private:
	Bool CSwfDumpCommandlet::GetTextureData( const CSwfTexture* swfTexture, STextureData& outTextureData );
	void DumpSwf( CSwfResource* swfResource, GpuApi::eTextureSaveFormat saveFormat, const String& outPath );
};

DEFINE_SIMPLE_RTTI_CLASS( CSwfDumpCommandlet, ICommandlet );
IMPLEMENT_ENGINE_CLASS( CSwfDumpCommandlet );


CSwfDumpCommandlet::STextureData::STextureData()
	: m_format( 0 )
{}

CSwfDumpCommandlet::STextureData::~STextureData()
{
}

const CSwfDumpCommandlet::SSaveFormatExtEntry CSwfDumpCommandlet::s_saveFormatExts[] = {
	{ TXT(".dds"), GpuApi::SAVE_FORMAT_DDS },
	{ TXT(".bmp"), GpuApi::SAVE_FORMAT_BMP },
	{ TXT(".png"), GpuApi::SAVE_FORMAT_PNG },
	{ TXT(".jpg"), GpuApi::SAVE_FORMAT_JPG },
	{ TXT(".tga"), GpuApi::SAVE_FORMAT_TGA },
};

const Char* CSwfDumpCommandlet::GetExtForSaveFormat( GpuApi::eTextureSaveFormat saveFormat )
{
	for ( Uint32 i = 0; i < ARRAY_COUNT_U32(s_saveFormatExts); ++i )
	{
		if ( saveFormat == s_saveFormatExts[i].m_textureSaveFormat )
		{
			return s_saveFormatExts[i].m_ext;
		}
	}

	return nullptr;
}

CSwfDumpCommandlet::CSwfDumpCommandlet()
{
	m_commandletName = CName( TXT("swfdump") );
}

Bool CSwfDumpCommandlet::Execute( const CommandletOptions& options )
{
	// Options
	String fromAbsPath;


	String toAbsolutePath;
	const TDynArray< String >& freeOpts = options.GetFreeArguments();
	if ( !freeOpts.Empty() )
	{
		toAbsolutePath = freeOpts[0];
		toAbsolutePath.ReplaceAll( TXT("/"), TXT("\\") );
		if ( !toAbsolutePath.Empty() && !toAbsolutePath.EndsWith( TXT("\\") ) )
		{
			toAbsolutePath += TXT("\\");
		}
	}

	if ( toAbsolutePath.Empty() )
	{
		toAbsolutePath = GFileManager->GetDataDirectory() + TXT("flashDump\\");
	}

	TDynArray < String > paths;
	GFileManager->FindFiles( GFileManager->GetDataDirectory() + TXT("gameplay\\gui_new\\"), TXT("*.redswf"), paths, true );
	for( Uint32 j = 0; j < paths.Size(); j++ )
	{
		String localPath;
		GDepot->ConvertToLocalPath( paths[j], localPath );
		paths[j] = localPath;
	}

	SYSTEMTIME time;
	::GetSystemTime( &time );

	const String timestamp = String::Printf( TXT("%02d_%02d_%04d"),
		(Int32)time.wDay, (Int32)time.wMonth, (Int32)time.wYear );

	toAbsolutePath += timestamp + TXT("\\");

	// Report what we'll do
	LOG_WCC( TXT("Dumping flash files to:       %s"), toAbsolutePath.AsChar() );
	GFileManager->CreatePath( toAbsolutePath );

	for ( const String& depotPath : paths )
	{
		THandle< CSwfResource > swfResource = LoadResource< CSwfResource >( depotPath );
		if ( ! swfResource )
		{
			ERR_WCC(TXT("Failed to load '%ls'"), depotPath.AsChar() );
			continue;
		}
	
		DumpSwf( swfResource.Get(), GpuApi::SAVE_FORMAT_PNG, toAbsolutePath );
		swfResource->Discard();

		// Make sure to process discards, since easy to run out of texture memory!
		SGarbageCollector::GetInstance().CollectNow();
	}

	CreateBatFile( toAbsolutePath+TXT("\\updateTech.bat") );

	LOG_WCC( TXT("Done!") );
	return true;
}


Bool CSwfDumpCommandlet::GetTextureData( const CSwfTexture* swfTexture, STextureData& outTextureData )
{
	if ( ! swfTexture || swfTexture->GetMipCount() < 1 )
	{
		return false;
	}

	GpuApi::eTextureFormat format = swfTexture->GetPlatformSpecificCompression();
	outTextureData.m_mipCopy = swfTexture->GetMips()[0];
	outTextureData.m_format = static_cast< Uint32 >( format );

	// Programming guide for DDS
	// http://msdn.microsoft.com/en-us/library/windows/desktop/bb943991(v=vs.85).aspx
	Uint32 blockSize = 0;
	switch (format)
	{
	case GpuApi::TEXFMT_BC1:
		blockSize = 8;
		break;
	case GpuApi::TEXFMT_BC3:
		blockSize = 16;
		break;
	default:
		RED_HALT( "Unsupported texture format %u", (Uint32)format );
		break;
	}

	if ( blockSize < 1 )
	{
		return false;
	}

	outTextureData.m_mipCopy.m_pitch = Max<Uint32>( 1, (outTextureData.m_mipCopy.m_width + 3 )/4 ) * blockSize;

	return true;
}

void CSwfDumpCommandlet::DumpSwf( CSwfResource* swfResource, GpuApi::eTextureSaveFormat saveFormat, const String& outPath )
{
	LOG_WCC(TXT("Dumping '%ls'..."), swfResource->GetDepotPath().AsChar() );
	CFilePath filePath( swfResource->GetDepotPath() );
	const String& swfOutPath = outPath + filePath.GetFileName() + TXT("\\");
	
	const auto& swfTextures = swfResource->GetTextures();
	for ( Uint32 i = 0; i < swfTextures.Size(); ++i )
	{
		GFileManager->CreatePath( swfOutPath );

		CSwfTexture* swfTexture = swfTextures[ i ];
		STextureData textureData;
		if ( GetTextureData( swfTexture, textureData ) )
		{
			textureData.m_mipCopy.m_data.Load();
			Uint8* srcData = static_cast< Uint8* >( textureData.m_mipCopy.m_data.GetData() );
			const size_t srcDataSize = textureData.m_mipCopy.m_data.GetSize();
			const size_t width = textureData.m_mipCopy.m_width;
			const size_t height = textureData.m_mipCopy.m_height;
			const GpuApi::eTextureFormat format = static_cast< GpuApi::eTextureFormat >( textureData.m_format );
			const size_t ddsPitch = textureData.m_mipCopy.m_pitch; // Already converted for DDS

			GpuApi::TextureDataDesc textureToSave;
			textureToSave.data = &srcData;
			textureToSave.slicePitch = srcDataSize;
			textureToSave.width = width;
			textureToSave.height = height;
			textureToSave.format = format;
			textureToSave.rowPitch = ddsPitch;

			String fileName = swfTexture->GetLinkageName();

			const Char* ext = GetExtForSaveFormat( saveFormat );
			if ( ext )
			{
				fileName.Replace(TXT(".dds"), ext, true );
			}

			GpuApi::SaveTextureToFile( textureToSave, String::Printf(TXT("%ls%ls"), swfOutPath.AsChar(), fileName.AsChar() ).AsChar(), saveFormat );
		}
	}
}

void CSwfDumpCommandlet::CreateBatFile( const String& path )
{
	String helperText = TXT("set DD=%DATE:~0,2%\n");
	helperText += TXT("set MM=%DATE:~3,2%\n");
	helperText += TXT("set YYYY=%DATE:~6,4%\n");
	helperText += TXT("set FOLDER=%YYYY%-%MM%-%DD%\n");
	helperText += TXT("set OUTPATH=\"t:\\techart\\GUI\\\"\n");
	helperText += TXT("pushd %OUTPATH%\n");
	helperText += TXT("mkdir %FOLDER%\n");
	helperText += TXT("popd\n");
	helperText += TXT("%systemroot%\\System32\\xcopy /s *.* %OUTPATH%%FOLDER%\n");

	GFileManager->SaveStringToFile( path, helperText );
}
