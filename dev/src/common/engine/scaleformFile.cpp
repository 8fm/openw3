/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "../core/dataError.h"
#include "../core/depot.h"

#include "swfResource.h"
#include "swfTexture.h"
#include "scaleformTextureCacheImage.h"
#include "textureCache.h"
#include "renderer.h"
#include "renderScaleform.h"

#include "guiGlobals.h"
#include "scaleformFile.h"

#include "../core/fileSys.h"
#include "../core/resourceDefManager.h"

#ifdef USE_SCALEFORM

#include <Render/ImageFiles/JPEG_ImageFile.h>
#include <Render/ImageFiles/PNG_ImageFile.h>
#include <Render/ImageFiles/DDS_ImageFile.h>
#include "userProfile.h"

static const String SWF_EXTENSION( TXT(".swf") );
static const String GFX_EXTENSION( TXT(".gfx") );
static const String REDSWF_EXTENSION( TXT(".redswf") );

//////////////////////////////////////////////////////////////////////////
// CScaleformUrlBuilder
//////////////////////////////////////////////////////////////////////////
void CScaleformUrlBuilder::BuildURL( SF::String* ppath, const LocationInfo& loc )
{
	ASSERT( ppath );
	if ( !ppath )
	{
		return;
	}

	// Convert to lowercase since the GFx resource library is case sensitive
	// and avoid potentially having the same SWF multiple times memory (it's happened before)
	const ::String fileName = FLASH_UTF8_TO_TXT( loc.FileName.ToLower() );

	if ( IsPathAbsolute( loc.FileName ) )
	{
		String debugFilePath( FLASH_UTF8_TO_TXT( loc.FileName.ToCStr() ) );
		RED_UNUSED( debugFilePath );
		GUI_ERROR(TXT("GFx URL filename contains an absolute path. Not loading for security reasons: '%ls'"), debugFilePath.AsChar() );

		*ppath = "";
	}
	else
	{

		::String absolutePath;
		if (GDeferredInit)
			absolutePath = GDeferredInit->DataPath();
		else
			GDepot->GetAbsolutePath( absolutePath );

		// TMPHACK: Need to sort USMs, SWFs, and loose images etc
		// Parent path is the path of a SWF that itself is loading something, which should
		// already be in *absolute* depot path form already. But before we were always getting the depot path and tacking on gameplay/gui (then gameplay/gui_new)
		// but to unify this, we need to stop the hacks. So just accept that if you load swf/hud_module.swf from depot/.../hud/hud.swf then parent path will be deopt/.../hud/swf/hud_module.swf
		// so just say hud_module.swf. Hopefully nobody needs to then use ".." back a dir notation.
		const Bool isEngine = fileName.EqualsNC(TXT("engine\\swf\\videoplayer.gfx")) || ( !loc.ParentPath.IsEmpty() && Red::System::StringSearch( loc.ParentPath.ToCStr(), "engine\\swf\\" ) );
		const Bool emptyParent = loc.ParentPath.IsEmpty();
		
		// if "engine SWF" or the parent path is empty, we'll use the complete file path without tacking on gameplay\gui_new for now
		// E.g., fonts and other stuff loaded by depotPaths vs. loaded by SWFs since they'll give the proper full depotPath
		if ( !isEngine && !emptyParent )
		{
			absolutePath += TXT("gameplay\\gui_new\\");
		}
		// kind of a common hack to not use the parent path for images or USM locations from the fullscreen player or menus

		if ( !absolutePath.EndsWith(TXT("/")) && !absolutePath.EndsWith(TXT("\\")) )
		{
			absolutePath += TXT("\\");
		}

		if ( fileName.EndsWith(TXT(".usm")) )
		{
			if ( isEngine )
			{
				// Don't mess with the path if DLC
				if ( !fileName.BeginsWith( TXT("dlc\\"))  )
				{
					absolutePath += TXT("movies\\");
				}
			}
			else
			{
				GUI_ERROR(TXT("USM '%ls' should be played by the fullscreen player"), fileName.AsChar() );
				*ppath = "";
				return;
			}
		}

		::String urlPath = String::Printf( TXT("%ls%ls"), absolutePath.AsChar(), fileName.AsChar() );	

		urlPath.ReplaceAll( TXT("/"), TXT("\\") );

		// Some scripts do things like read the LoaderInfo URL and expect .swf
		urlPath.Replace( GFX_EXTENSION, SWF_EXTENSION, true );

		*ppath = SF::String( urlPath.AsChar() );
	}
}

//////////////////////////////////////////////////////////////////////////
// CScaleformFileWrapper
//////////////////////////////////////////////////////////////////////////
CScaleformFileReaderWrapper::CScaleformFileReaderWrapper( const String& filePath, Bool buffered /*=true*/ )
	: m_file( nullptr )
{
	m_filePath = FLASH_TXT_TO_UTF8( filePath.AsChar() );

	// Should fix the config paths in platform*.cpp to use backslashes, but risky right now...
	String stupidPathsUsingForwardSlashes = filePath;
#if defined( RED_PLATFORM_DURANGO ) || defined( RED_PLATFORM_ORBIS )
	stupidPathsUsingForwardSlashes.ReplaceAll(TXT("\\"),TXT("/"));
#endif

	// FIXME: hack to get the bumpers loaded loose
	if (GDeferredInit || filePath.EndsWith( TXT( "bumpers.usm" ) ) || filePath.EndsWith( TXT( "\\loading.usm" ) ) )
	{
		m_file = GFileManager->CreateFileReader (stupidPathsUsingForwardSlashes, FOF_AbsolutePath);
		return;
	}

	String depotPath;
	if ( !GDepot->ConvertToLocalPath( stupidPathsUsingForwardSlashes, depotPath ) )
	{
#ifndef RED_PLATFORM_ORBIS // FIXME: Somehow crashes with .usm files in the logging??!
		GUI_ERROR( TXT("Failed to convert path '%ls' to a depot path."), filePath.AsChar() );
		return;
#endif
	}

	CDiskFile* diskFile = GDepot->FindFile( depotPath );
	if ( ! diskFile )
	{
#ifndef RED_PLATFORM_ORBIS // FIXME: Somehow crashes with .usm files in the logging??!
		GUI_ERROR( TXT("Failed to open file '%ls' for reading."), filePath.AsChar() );
#endif
		return;
	}

	//buffered/Not buffered?
	m_file = diskFile->CreateReader();
	if ( ! m_file )
	{	
#ifndef RED_PLATFORM_ORBIS // FIXME: Somehow crashes with .usm files in the logging??!
		GUI_ERROR( TXT("Failed to open file '%ls' for reading."), filePath.AsChar() );
#endif
	}
}

CScaleformFileReaderWrapper::CScaleformFileReaderWrapper( IFile* file )
	: m_file( file )
{
	if ( file )
	{
		m_filePath = UNICODE_TO_ANSI( file->GetFileNameForDebug() );
		if ( m_filePath.Empty() )
		{
			// Some stuff uses this, prevent it from crashing
			m_filePath = "unknown";
		}
	}
}

CScaleformFileReaderWrapper::~CScaleformFileReaderWrapper()
{
	delete m_file;
	m_file = nullptr;
}

const SFChar* CScaleformFileReaderWrapper::GetFilePath()
{
	return m_filePath.AsChar();
}

SFBool CScaleformFileReaderWrapper::IsValid()
{
	return m_file != nullptr;
}

SFBool CScaleformFileReaderWrapper::IsWritable()
{
	return false;
}

SFInt CScaleformFileReaderWrapper::Tell()
{
	return m_file ? static_cast< SFInt >( m_file->GetOffset() ) : -1;
}

SF::SInt64 CScaleformFileReaderWrapper::LTell()
{
	return m_file ? static_cast< SF::SInt64 >( m_file->GetOffset() ) : -1;
}

SFInt CScaleformFileReaderWrapper::GetLength()
{
	return m_file ? static_cast< SFInt >( m_file->GetSize() ) : -1;
}

SF::SInt64 CScaleformFileReaderWrapper::LGetLength()
{
	return m_file ? static_cast< SF::SInt64 >( m_file->GetSize() ) : -1;
}

SFInt CScaleformFileReaderWrapper::GetErrorCode()
{
	return 0;
}

SFInt CScaleformFileReaderWrapper::Write( const SF::UByte *pbufer, SFInt numBytes )
{
	RED_UNUSED( pbufer );
	RED_UNUSED( numBytes );
	return -1;
}

SFInt CScaleformFileReaderWrapper::Read( SF::UByte *pbufer, SFInt numBytes )
{
	if ( m_file )
	{
		Uint64 curOffset = m_file->GetOffset();
		if( curOffset + numBytes > m_file->GetSize() )
			numBytes = (SFInt)(m_file->GetSize() - curOffset);
		m_file->Serialize( pbufer, numBytes );
		Uint64 newOffset = m_file->GetOffset();
		Uint64 bytesRead = newOffset - curOffset;
		return static_cast< SFInt >( bytesRead );
	}

	return -1;
}

SFInt CScaleformFileReaderWrapper::SkipBytes( SFInt numBytes )
{
	if ( m_file )
	{
		const Uint64 curOffset = m_file->GetOffset();
		const Uint64 skipPos = curOffset + numBytes;
		m_file->Seek( skipPos );
		const Uint64 newOffset = m_file->GetOffset();
		const Uint64 bytesSkipped = newOffset - curOffset;
		return static_cast< SFInt >( bytesSkipped );
	}

	return -1;
}

SFInt CScaleformFileReaderWrapper::BytesAvailable()
{
	if ( ! m_file )
	{
		return -1;
	}

	Uint64 pos = m_file->GetOffset();
	Uint64 size = m_file->GetSize();

	const Uint64 bytesAvail = size - pos;
	return  static_cast< SFInt >( bytesAvail );
}

SFBool CScaleformFileReaderWrapper::Flush()
{
	return true;
}

SFInt CScaleformFileReaderWrapper::Seek( SFInt offset, SFInt origin/*=Seek_Set*/ )
{
	if ( ! m_file )
	{
		return -1;
	}

	const Uint64 fileSize = m_file->GetSize();
	if ( fileSize == 0 )
	{
		return 0;
	}

	Uint64 seekPos = 0;
	switch (origin)
	{
	case Seek_Set:
		seekPos = static_cast< Uint64 >( offset );
		break;
	case Seek_Cur:
		seekPos = m_file->GetOffset() + offset;
		break;
	case Seek_End:
		seekPos = fileSize - 1 + offset;
		break;
	default:
		break;
	}

	m_file->Seek( seekPos );

	return static_cast< SFInt >( m_file->GetOffset() );
}

SF::SInt64 CScaleformFileReaderWrapper::LSeek( SF::SInt64 offset, SFInt origin/*=Seek_Set*/ )
{
	if ( ! m_file )
	{
		return -1;
	}

	const Uint64 fileSize = m_file->GetSize();
	if ( fileSize == 0 )
	{
		return 0;
	}

	Uint64 seekPos = 0;
	switch (origin)
	{
	case Seek_Set:
		seekPos = static_cast< Uint64 >( offset );
		break;
	case Seek_Cur:
		seekPos = m_file->GetOffset() + offset;
		break;
	case Seek_End:
		seekPos = fileSize - 1 + offset;
		break;
	default:
		break;
	}

	m_file->Seek( seekPos );

	return static_cast< SF::SInt64 >( m_file->GetOffset() );
}

SFBool CScaleformFileReaderWrapper::ChangeSize( int newSize )
{
	RED_UNUSED( newSize );
	return false;
}

SFInt CScaleformFileReaderWrapper::CopyFromStream( SF::File *pstream, SFInt byteSize )
{
	RED_UNUSED( pstream );
	RED_UNUSED( byteSize );
	return -1;
}

SFBool CScaleformFileReaderWrapper::Close()
{
	if ( m_file )
	{
		delete m_file;
		m_file = nullptr;
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
// CScaleformSwfResourceFileWrapper
//////////////////////////////////////////////////////////////////////////
CScaleformSwfResourceFileWrapper::CMutex CScaleformSwfResourceFileWrapper::st_loadedSwfLock;
THashMap< String, CSwfResource* > CScaleformSwfResourceFileWrapper::st_loadedSwfMap;
CScaleformSwfResourceFileWrapper::CScaleformSwfResourceFileWrapper( CSwfResource* swfResource )
	: m_swfResource( swfResource )
{
	if ( m_swfResource )
	{
		const String& swfLinkageName = m_swfResource->GetLinkageName();

		if ( ! swfLinkageName.Empty() )
		{
			CScopedLock scopedLock( st_loadedSwfLock );

			// The file wrapper doesn't match the resourcelib refcount when using a threaded task manager.
			// So it's possible for us to be closing a CScaleformSwfResourceFileWrapper but it's expected that the resource is still alive
			// So add to the root set under the st_loadedSwfLock, the root set itself is refcounted this way. Even though the root set has its own
			// mutex, lock it under the st_loadedSwfLock so it's in a consistent state when trying to access it. Loading job threads block the GC, so
			// adding and removing it from the root set isn't a race.

			const Bool wasInRootSet = m_swfResource->IsInRootSet();
			m_swfResource->AddToRootSet();

			if ( !wasInRootSet && !st_loadedSwfMap.Insert( swfLinkageName, m_swfResource ) )
			{
				DATA_HALT( DES_Major, m_swfResource, TXT("SWF linkage name collision '%ls'"), swfLinkageName.AsChar() );
			}
		}
		else
		{
			DATA_HALT( DES_Major, m_swfResource, TXT("GUI"), TXT("SWF linkage name empty") );
		}

		const DataBuffer& swfBuffer = m_swfResource->GetDataBuffer();
		pFile = *SF_NEW SF::MemoryFile( FLASH_TXT_TO_UTF8( swfResource->GetDepotPath().AsChar() ),
			static_cast< SF::UByte* >( swfBuffer.GetData() ),
			static_cast< SFInt >( swfBuffer.GetSize() ) );
	}
}

CScaleformSwfResourceFileWrapper::~CScaleformSwfResourceFileWrapper()
{
	// Note the GpuApi texture ref will still be kept alive through the Scaleform TextureImage, released when it's released.
	// So we just use the CSwfResource to get the swf data and create textures
	if ( m_swfResource )
	{
		{
			CScopedLock scopedLock( st_loadedSwfLock );

			auto findIt = st_loadedSwfMap.Find( m_swfResource->GetLinkageName() );

			m_swfResource->RemoveFromRootSet();
			const Bool isInRootSet = m_swfResource->IsInRootSet();

			// Make sure we're erasing this swf resource in case there was a export name collision
			// instead of keeping some bool around that it failed to be inserted.
			if ( !isInRootSet && findIt != st_loadedSwfMap.End() && findIt->m_second == m_swfResource )
			{
				st_loadedSwfMap.Erase( findIt );
			}
		}
	}
}

const THandle< CSwfTexture >& CScaleformSwfResourceFileWrapper::GetExportedTexture( const String& textureLinkageName )
{
	const String swfLinkageName = textureLinkageName.StringBefore( TXT("_"), true ) + GFX_EXTENSION;
	CScopedLock scopedLock( st_loadedSwfLock );
	CSwfResource* swfResource = nullptr;
	st_loadedSwfMap.Find( swfLinkageName, swfResource );
	if ( swfResource )
	{
		return swfResource->GetTextureExport( textureLinkageName );
	}
	else
	{
		HALT( "swfLinkageName '%ls' missing", swfLinkageName.AsChar() );
	}

	return CSwfResource::NULL_SWF_TEXTURE_HANDLE;
}

//////////////////////////////////////////////////////////////////////////
// CScaleformFileOpener
//////////////////////////////////////////////////////////////////////////
SF::File* CScaleformFileOpener::OpenFile( const SFChar* purl, SFInt flags, SFInt mode )
{
	const String fileName( FLASH_UTF8_TO_TXT(purl) );

	if ( fileName.EndsWith( TXT(".sav") ) )
	{
		if ( GUserProfileManager )
		{
			IFile* file = GUserProfileManager->CreateScreenshotDataReader();
			if ( ! file )
			{
				ERR_ENGINE(TXT("User profile manager returned no data for sav '%ls'"), fileName.AsChar() );
				return nullptr;
			}

			return SF_NEW CScaleformFileReaderWrapper( file );
		}
		ERR_ENGINE(TXT("No user profile manager available for sav '%ls'"), fileName.AsChar() );
		return nullptr;
	}
	else if ( fileName.EndsWith( SWF_EXTENSION ) )
	{
		CSwfResource* swfResource = OpenSwf( fileName );
		return swfResource ? SF_NEW CScaleformSwfResourceFileWrapper( swfResource ) : NULL;
	}
	else if ( fileName.EndsWith( GFX_EXTENSION ) )
	{
		return nullptr;
	}
	else if ( fileName.EndsWith( REDSWF_EXTENSION ) )
	{
		RED_HALT( "Attempted to load a CSwfResource without converting to .swf extension" );
		return nullptr;
	}
	else
	{
		// For the moment. Just return a USM subclass of file, otherwise normal. Use url builder above to change the URL, not here.
		const SFInt supportedFlags = SF::FileConstants::Open_Read | SF::FileConstants::Open_Buffered;
		const SFInt supportedMode = SF::FileConstants::Mode_Read;

		if ( (flags & supportedFlags) != flags )
		{
			GUI_ERROR( TXT("Unsupported file open flags '%d' on file '%ls'"), flags, fileName.AsChar() );
			return nullptr;
		}

		// FIXME: Ignore for now - Scaleform is overgeneral in the modes it requests anyway
		// 		if ( mode != supportedMode )
		// 		{
		// 			GUI_ERROR( TXT("Unsupported file open mode '%d'"), mode, fileName.AsChar() );
		// 			return nullptr;
		// 		}

		const Bool buffered = (flags & SF::FileConstants::Open_Buffered) != 0;
		return SF_NEW CScaleformFileReaderWrapper( fileName, buffered );
		//		GUI_ERROR(TXT("Unsupported file extension '%ls' for direct file opening. (Use protocol source 'img://' for images)"), fileName.AsChar() );
		//		return nullptr;
	}
}

CSwfResource* CScaleformFileOpener::OpenSwf( const String& url )
{
	::String absolutePath;
	if (GDepot)
		GDepot->GetAbsolutePath( absolutePath );

	const Uint32 urlLen = url.GetLength();
	const Uint32 absLen = absolutePath.GetLength();
	ASSERT( urlLen >= absLen, TXT("URL is too short") );
	::String depotPath = urlLen >= absLen ? url.RightString( urlLen - absLen ) : String::EMPTY;
	depotPath.Replace( SWF_EXTENSION, REDSWF_EXTENSION, true );

	CSwfResource* swfResource = nullptr;

	if (GDepot)
	{
		swfResource = Cast<CSwfResource>( GDepot->LoadResource( depotPath ) );
	}
	else
	{
		Red::TScopedPtr<IFile> reader( GFileManager->CreateFileReader (depotPath, FOF_AbsolutePath) );
		CDependencyLoader loader( *reader, NULL );

		DependencyLoadingContext loadingContext;
		RED_VERIFY ( loader.LoadObjects( loadingContext ) );
		loader.PostLoad();

		swfResource = Cast<CSwfResource>(loadingContext.m_loadedRootObjects[0]);
	}


	// #ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
	// 	swfResource = Cast< CSwfResource >( GDepot->FindResource( depotPath ) );
	// 	if ( swfResource )
	// 	{
	// 		swfResource->AddToRootSet(); // Add to root set *NOW* because Reload() calls the GC
	// 		//TMPSHIT: For now just force reload for iterations
	// 		swfResource->Reload( false );
	// 		
	// 		// Old resource is discarded upon Reload
	// 		swfResource = Cast< CSwfResource >( GDepot->FindResource( depotPath ) );
	// 		RED_ASSERT( swfResource );
	// 		if ( swfResource )
	// 		{
	// 			// Our file wrapper will do this, put it in the root set because of Reload()
	// 			swfResource->RemoveFromRootSet();
	// 		}
	// 	}
	// 	else
	// 	{
	// 		swfResource = Cast< CSwfResource >( GDepot->LoadResource( depotPath ) );
	// 	}
	// #else
	//swfResource = Cast< CSwfResource >( GDepot->LoadResource( depotPath ) );
	//#endif // !NO_FILE_SOURCE_CONTROL_SUPPORT

	if ( ! swfResource )
	{
		GUI_ERROR( TXT("Failed to open SWF resource '%ls' ('%ls')"), depotPath.AsChar(), url.AsChar() );
	}

	return swfResource;
}

#ifndef RED_FINAL_BUILD
SF::SInt64 CScaleformFileOpener::GetFileModifyTime( const SFChar* purl )
{
	//TMPSHIT: Debug why SF keeps getting the same file modification time from the filesystem
	static SF::SInt64 test = 0;
	return test++;
}
#endif

//////////////////////////////////////////////////////////////////////////
// CScaleformImageCreator
//////////////////////////////////////////////////////////////////////////
CScaleformImageCreator::CScaleformImageCreator( SF::Ptr<SF::Render::TextureManager>  textureManager )
	: GFx::ImageCreator( textureManager ),
	  m_lastUsedPrefixPathIndex( 0 )
{		
	if ( !Red::System::StringSearch( SGetCommandLine(), TXT("-oldgui") ) )
	{
		m_depotPathPrefix = TXT("gameplay\\gui_new\\");
	}
	else
	{
		m_depotPathPrefix = TXT("gameplay\\gui\\");
	}
}

SF::Render::Image* CScaleformImageCreator::LoadProtocolImage( const GFx::ImageCreateInfo& info, const SF::String& url )
{
	// NOTE: For now. Need to load async. And could have just done this is CGuiFileOpener::OpenFile instead of here and in
	// CGuiUrlBuilder::BuildURL, but CGuiFileOpener::OpenFile shouldn't change the URL because of how the GFx resource library works

	SF::String filePath( url );

	filePath.StripProtocol();
	String filePathStriped  = ANSI_TO_UNICODE( filePath );

	String fullFilePath = m_depotPathPrefix + filePathStriped;

	SF::Ptr<SF::Render::Image> image = LoadImage( info, fullFilePath.AsChar() );	
	if( !image )
	{
		const Uint32 contentCount = m_additionalContentPathPrefix.Size();
		if( contentCount > 0 )
		{
			Uint32 startIndex = m_lastUsedPrefixPathIndex; 
			Uint32 index = startIndex; 
			do 
			{  
				fullFilePath = m_additionalContentPathPrefix[index] + filePathStriped;
				image = LoadImage( info, fullFilePath.AsChar() );
				if( image )
				{
					m_lastUsedPrefixPathIndex = index;
					break;
				}

				index = (index + 1 ) % contentCount;
			} while ( index != startIndex);
		}

	}

	return image;
}

SF::Ptr<SF::Render::Image> CScaleformImageCreator::LoadImage( const GFx::ImageCreateInfo& info, const Char* filePath )
{
	SF::Ptr<SF::Render::Image> image;

	// First, check if this is in the texture cache
	{
		// get as normal pah
		CTextureCacheQuery query = GTextureCache->FindNonResourceEntry( filePath );
		if ( query )
		{
			// Loading from the cache, we assume the texture must be read-only, we don't need to generate mips, and can't be a render target.
			// This way, we can create it as an immutable texture, from cooked data.
			Uint32 unsupportedFlags = SF::Render::ImageUse_ReadOnly_Mask | SF::Render::ImageUse_GenMipmaps | SF::Render::ImageUse_RenderTarget;
			RED_ASSERT( ( info.Use & unsupportedFlags ) == 0, TXT("Scaleform requesting texture from image '%ls', with unsupported use flags. Do we need to generate mips offline maybe?"), filePath );

			Uint32 use = info.Use;
			use &= ~unsupportedFlags;					// Make sure the unsupported flags are not present
			use |= SF::Render::ImageUse_InitOnly;		// Must initialize now

			SF::Ptr<SF::Render::TextureManager> mgr = GetTextureManager();
			if ( mgr )
			{
				// Fine to create this on stack. Because of InitOnly, the resulting Texture won't keep a reference to the image.
				CScaleformTextureCacheImage cacheImage( query );

				SF::Ptr<SF::Render::Texture> ptexture = *mgr->CreateTexture( cacheImage.GetFormat(), cacheImage.GetMipmapCount(), cacheImage.GetSize(), use, &cacheImage );
				if ( ptexture )
				{
					image = SF_NEW SF::Render::TextureImage( cacheImage.GetFormat(), cacheImage.GetSize(), use, ptexture, nullptr );
				}
			}
		}
	}

	if ( !image )
	{
		String absolutePath;
		GDepot->GetAbsolutePath( absolutePath );

		SF::String newUrl;
		newUrl = absolutePath.AsChar();
		newUrl += filePath;

		image = LoadImageFile( info, newUrl );
	}

	return image;
}

void CScaleformImageCreator::AddAdditionalContentDirectory( const String& contentDir )
{
	m_additionalContentPathPrefix.PushBackUnique( contentDir );
	m_lastUsedPrefixPathIndex = 0;
}

void CScaleformImageCreator::RemoveAdditionalContentDirectory(  const String& contentDir )
{
	String* foundContentDir = m_additionalContentPathPrefix.FindPtr( contentDir );
	if( foundContentDir )
	{
		m_additionalContentPathPrefix.Erase( foundContentDir );
		m_lastUsedPrefixPathIndex = 0;
	}
}

SF::Render::Image* CScaleformImageCreator::LoadExportedImage( const GFx::ImageCreateExportInfo& info, const SF::String& url )
{
	const String textureExportName = ANSI_TO_UNICODE( url.GetFilename().ToCStr() );
	const THandle< CSwfTexture >& swfTexture = CScaleformSwfResourceFileWrapper::GetExportedTexture( textureExportName );
	if ( ! swfTexture )
	{
		return nullptr;
	}

	// TBD: These texture won't show up in AMP anymore and probably won't use the image heap either
	IRenderScaleform* renderSystemScalefrom = GRender->GetRenderScaleform();
	if ( renderSystemScalefrom )
	{
		return renderSystemScalefrom->CreateImage( swfTexture->GetRenderResource(), info.TargetSize.Width, info.TargetSize.Height, info.Use );
	}

	return nullptr;
	//	return TBaseClass::LoadExportedImage( info, url );
}

#endif // USE_SCALEFORM
