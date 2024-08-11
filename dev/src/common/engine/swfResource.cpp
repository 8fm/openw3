/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "../core/dataError.h"
#include "../core/cooker.h"
#include "swfResource.h"
#include "guiGlobals.h"

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CSwfResource );
IMPLEMENT_ENGINE_CLASS( SSwfFontDesc );
IMPLEMENT_ENGINE_CLASS( SSwfHeaderInfo );

//////////////////////////////////////////////////////////////////////////
// CSwfResource
//////////////////////////////////////////////////////////////////////////
const THandle< CSwfTexture > CSwfResource::NULL_SWF_TEXTURE_HANDLE;

const Char* CSwfResource::RAW_SWF_LINKAGE_NAME = TXT("Raw SWF");

CSwfResource::CSwfResource()
	: m_dataBuffer( TDataBufferAllocator< MC_BufferFlash >::GetInstance() )
	, m_sourceSwf( MC_BufferFlash )
{
}

CSwfResource::~CSwfResource()
{
	m_dataBuffer.Clear();
	m_textureMap.ClearFast();
}

void CSwfResource::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	m_dataBuffer.Serialize( file );
	m_sourceSwf.Serialize( file, true, false );
}

void CSwfResource::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	for ( auto it = m_textures.Begin(); it != m_textures.End(); ++it )
	{
		CSwfTexture* texture = *it;
		if ( ! m_textureMap.Insert( texture->GetLinkageName(), texture ) )
		{
			DATA_HALT( DES_Major, this, TXT("GUI"), TXT("Duplicate SWF texture export entry '%ls'"), texture->GetLinkageName().AsChar() );
		}
	}

#ifndef NO_DATA_ASSERTS
	// Sanity check the Flash cooker and entire cook process
	// This check should probably be in the dependencyLoader itself
	if ( HasFlag( OF_WasCooked ) )
	{
		for ( Uint32 i = 0; i < m_textures.Size(); ++i )
		{
			if ( ! m_textures[ i ]->HasFlag( OF_WasCooked ) )
			{
				DATA_HALT( DES_Uber, this, TXT("GUI"), TXT("Cooked SWF resource has uncooked SWF texture export '%ls'"), m_textures[i]->GetLinkageName().AsChar() );
			}
		}
	}

	if ( m_linkageName.Empty() )
	{
		DATA_HALT( DES_Major, this, TXT("GUI"), TXT("SWF linkage name empty. No exported textures will work for this SWF") );
	}

	for ( Uint32 i = 0; i < m_textures.Size(); ++i )
	{
		const String GFX_EXTENSION( TXT(".gfx") );
		const String swfLinkageName = m_textures[ i ]->GetLinkageName().StringBefore( TXT("_"), true ) + GFX_EXTENSION;
		if ( swfLinkageName != m_linkageName )
		{
			DATA_HALT( DES_Major, this, TXT("GUI"), TXT("SWF texture export '%ls' links to SWF '%ls' instead of '%ls'"), m_textures[i]->GetLinkageName().AsChar(), swfLinkageName.AsChar(), m_linkageName.AsChar() );
		}
	}

	if ( m_dataBuffer.GetSize() == 0 )
	{
		DATA_HALT( DES_Major, this, TXT("GUI"), TXT("SWF file is empty. Possibly corrupted or failed resave") );
	}
	else if ( ! VerifySwf( m_dataBuffer ) )
	{
		DATA_HALT( DES_Major, this, TXT("GUI"), TXT("SWF file is corrupt") );
	}

#ifndef NO_RESOURCE_IMPORT
	if( CheckNewerImportFileExists() )
	{
		String updateTime = ToString( GFileManager->GetFileTime( m_importFile ) );
		String importTime = ToString( m_importFileTimeStamp );
		DATA_HALT( DES_Minor, this, TXT("GUI"), TXT("Needs reimporting. Updated: '%ls'. Imported: '%ls'"), updateTime.AsChar(), importTime.AsChar() );
	}
#endif // NO_RESOURCE_IMPORT

#endif // ! NO_DATA_ASSERTS
}

void CSwfResource::CleanupSourceData()
{
	TBaseClass::CleanupSourceData();
	m_sourceSwf.Clear();
}

#ifndef NO_DATA_VALIDATION
void CSwfResource::OnCheckDataErrors() const 
{
	// Any point of this override anymore?
	TBaseClass::OnCheckDataErrors();
}
#endif // !NO_DATA_VALIDATION

#ifndef NO_RESOURCE_COOKING
void CSwfResource::OnCook( ICookerFramework& cooker )
{
	// don't support it in the normal pipeline. Somebody could abuse this for "uncompressed" images instead
	// of properly creating assets
	if ( m_linkageName.BeginsWith( RAW_SWF_LINKAGE_NAME ) )
	{
		cooker.CookingError( this, TXT("Cooking raw SWF. Import through the editor!") );
		
		// ensure can't be used
		m_dataBuffer.Clear();
		m_textures.Clear();
		m_fonts.Clear();
	}

	TBaseClass::OnCook( cooker );
}
#endif

void CSwfResource::GetAdditionalInfo( TDynArray< String >& info ) const 
{
	TBaseClass::GetAdditionalInfo( info );

	info.PushBack( String::Printf( TXT("%u texture(s)"), m_textures.Size() ) );
	info.PushBack( String::Printf( TXT("%u font(s)"), m_fonts.Size() ) );
	info.PushBack( String::Printf( TXT("%dx%d [%.1f fps]"), (Int32)m_header.m_width, (Int32)m_header.m_height, m_header.m_frameRate ) );
}

#ifdef RED_LOGGING_ENABLED
#define FCC(h,s) ((AnsiChar)(((h)>>(s))&0xFF))
#define FCC2(h,s) (isprint(FCC((h),(s))) ? FCC((h),(s)) : '.')
static String GetFourCCForLog(Uint32 header)
{
	const String str = String::Printf(TXT("%c%c%c%c"), FCC2(header,0), FCC2(header,8), FCC2(header,16), FCC2(header,24));
	return str;
}
#endif

Bool CSwfResource::VerifySwf( const DataBuffer& dataBuffer )
{
	if ( ! dataBuffer.GetData() || dataBuffer.GetSize() < sizeof(Uint32) )
	{
		return false;
	}

	// SWF file format is always little endian.
	Uint32 header = *(Uint32*)dataBuffer.GetData();

	header &= 0x00FFFFFF; // clear Flash version from header

	static const Uint32 swfMagic = 0x00535746;
	static const Uint32 swcMagic = 0x00535743;
	static const Uint32 swzMagic = 0x0053575A;
	static const Uint32 gfxMagic = 0x00584647;
	static const Uint32 gfcMagic = 0x00584643;

#ifdef USE_SCALEFORM
	if ( header == swzMagic )
	{
		GUI_WARN( TXT("Scaleform can't load an LZMA compressed (7-Zip) SWF") );
	}
#endif

	const Bool hasMagic = ( header == swfMagic || header == swcMagic || header == swzMagic ||
							header == gfxMagic || header == gfcMagic );
	
	ASSERT( hasMagic, TXT("SWF file magic signature not recognized!") );
	if ( ! hasMagic )
	{
		GUI_ERROR( TXT("SWF file magic signature '%ls' (0x%08X) not recognized"), GetFourCCForLog(header).AsChar(), header );
	}
	
	GUI_LOG(TXT("SWF file magic '%ls' (0x%08X)"), GetFourCCForLog(header).AsChar(), header );

	return hasMagic;
}

// LEGACY!!
Bool CSwfResource::VerifySwf( const LatentDataBuffer& dataBuffer )
{
	if ( ! dataBuffer.GetData() || dataBuffer.GetSize() < sizeof(Uint32) )
	{
		return false;
	}

	// SWF file format is always little endian.
	Uint32 header = *(Uint32*)dataBuffer.GetData();

	header &= 0x00FFFFFF; // clear Flash version from header

	static const Uint32 swfMagic = 0x00535746;
	static const Uint32 swcMagic = 0x00535743;
	static const Uint32 swzMagic = 0x0053575A;
	static const Uint32 gfxMagic = 0x00584647;
	static const Uint32 gfcMagic = 0x00584643;

#ifdef USE_SCALEFORM
	if ( header == swzMagic )
	{
		GUI_WARN( TXT("Scaleform can't load an LZMA compressed (7-Zip) SWF") );
	}
#endif

	const Bool hasMagic = ( header == swfMagic || header == swcMagic || header == swzMagic ||
		header == gfxMagic || header == gfcMagic );

	ASSERT( hasMagic, TXT("SWF file magic signature not recognized!") );
	if ( ! hasMagic )
	{
		GUI_ERROR( TXT("SWF file magic signature not recognized") );
	}

	return hasMagic;
}
//////////////////////////////////////////////////////////////////////////

const THandle< CSwfTexture >& CSwfResource::GetTextureExport( const String& textureLinkageName ) const
{
	const THandle< CSwfTexture >* pTextureHandle = m_textureMap.FindPtr( textureLinkageName );
	if ( pTextureHandle )
	{
		return *pTextureHandle;
	}

	return NULL_SWF_TEXTURE_HANDLE;
}

#ifndef NO_RESOURCE_IMPORT
CSwfResource* CSwfResource::Create( const FactoryInfo& data )
{
	CSwfResource* swfResource = data.m_reuse;
	if ( ! swfResource )
	{
		swfResource = ::CreateObject< CSwfResource >( data.m_parent );
	}

	swfResource->m_linkageName = data.m_linkageName;
	swfResource->m_fonts= data.m_fonts;
	swfResource->m_header = data.m_header;
	swfResource->m_dataBuffer.MoveHandle( data.m_dataBuffer );
	swfResource->m_sourceSwf.MoveHandle( data.m_sourceSwf );
	swfResource->m_imageImportOptions = data.m_imageImportOptions;
	swfResource->m_textures.ClearFast(); // Clear existing textures for REimport.

	for ( Uint32 i = 0; i < data.m_textureInfos.Size(); ++i )
	{
		const CSwfResource::TextureInfo& texInfo = data.m_textureInfos[ i ];

		CSwfTexture::FactoryInfo facTexInfo;
		facTexInfo.m_linkageName = texInfo.m_linkageName;
		facTexInfo.m_parent = swfResource;
		CSwfTexture* swfTexture = CSwfTexture::Create( facTexInfo );
		if ( ! swfTexture )
		{
			swfResource->Discard();
			return nullptr;
		}

		if ( ! swfTexture->InitFromCompressedMip( texInfo.m_mipMap, texInfo.m_textureGroupName, texInfo.m_textureRawFormat ) )
		{
			swfTexture->Discard();
			swfResource->Discard();
			return nullptr;
		}

		swfResource->m_textures.PushBack( swfTexture );
	}

	return swfResource;
}

CSwfTexture* CSwfResource::GetThumbnailTextureSource() const
{
	if ( ! m_textures.Empty() )
	{
		return m_textures[0];
	}

	return nullptr;
}

Bool CSwfResource::SetCookedData( CSwfResource* swfResource, const CookInfo& data, ECookingPlatform cookingPlatform )
{
	if ( ! swfResource )
	{
		ERR_ENGINE(TXT("Null SWF resource!"));
		return false;
	}

	TDynArray< THandle< CSwfTexture > > textures;

	ICooker* textureCooker = ICooker::FindCooker( ClassID< CSwfTexture >(), cookingPlatform );
	if ( ! textureCooker )
	{
		ERR_ENGINE(TXT("Could not find texture cooker for ClassID< CSwfTexture >"));
		return false;
	}

	// Create textures first to make sure no errors before modifying the SWF resource
	// in order to avoid potentially corrupting things if cooking fails
	for ( Uint32 i = 0; i < data.m_textureInfos.Size(); ++i )
	{
		const CSwfResource::TextureInfo& texInfo = data.m_textureInfos[ i ];

		CSwfTexture::FactoryInfo facTexInfo;
		facTexInfo.m_linkageName = texInfo.m_linkageName;
		facTexInfo.m_parent = swfResource;
		CSwfTexture* swfTexture = CSwfTexture::Create( facTexInfo );
		if ( ! swfTexture )
		{
			ERR_ENGINE(TXT("CSwfTexture::Create failed for linkageName %ls !"), facTexInfo.m_linkageName.AsChar() );
			return false;
		}

		if ( ! swfTexture->InitFromCompressedMip( texInfo.m_mipMap, texInfo.m_textureGroupName, texInfo.m_textureRawFormat ) )
		{
			ERR_ENGINE(TXT("swfTexture->InitFromCompressedMip() failed!"));
			swfTexture->Discard();
			return false;
		}

		ICooker::CookingOptions options( cookingPlatform );
		options.m_resource = swfTexture;
		if ( !textureCooker->DoCook( ICookerFramework::Default(cookingPlatform), options ) ) // tempshit
		{
			ERR_ENGINE( TXT("Cooking of SWF texture in SWF '%ls' failed"), swfResource->GetFriendlyName().AsChar() );
			swfTexture->Discard();
			return false;
		}
		swfTexture->SetFlag( OF_WasCooked );

		textures.PushBack( swfTexture );
	}
	
	swfResource->m_dataBuffer.MoveHandle( data.m_dataBuffer );
	swfResource->m_textureMap.Clear();

	for ( Uint32 i = 0; i < swfResource->m_textures.Size(); ++i )
	{
		swfResource->m_textures[ i ]->Discard();
	}

	swfResource->m_textures = Move( textures );

	return true;
}

#endif // ! NO_RESOURCE_IMPORT