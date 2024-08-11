/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/gpuApiUtils/gpuApiMemory.h"
#include "../../common/core/importer.h"
#include "../../common/engine/bitmapTexture.h"

GpuApi::eTextureImportFormat FileExtensionToFormat( const String& extension )
{
	struct 
	{
		GpuApi::eTextureImportFormat	m_format;
		Char							m_extension[4];
	} Mapper[] = 
	{
		{ GpuApi::TIF_DDS,	TXT("dds"), },
		{ GpuApi::TIF_TGA,	TXT("tga"), },
	};

	for ( Int32 i = 0; i < ARRAY_COUNT( Mapper ); i ++ )
	{
		if ( !Red::System::StringCompare( Mapper[i].m_extension, extension.AsChar() ) )
		{
			return Mapper[i].m_format;
		}
	}

	return GpuApi::TIF_WIC;
}

/// Bitmap texture importer based on DirectXTex functionality
class CBitmapImporter : public IImporter
{
	DECLARE_ENGINE_CLASS( CBitmapImporter, IImporter, 0 );

public:
	CName	m_textureGroup;
	Bool	m_preserveArtistData;

public:
	CBitmapImporter();

	virtual CResource* DoImport( const ImportOptions& options );

	virtual bool PrepareForImport( const String& filePath, ImportOptions& options ) override;
};

BEGIN_CLASS_RTTI( CBitmapImporter )
	PARENT_CLASS(IImporter)
	PROPERTY_CUSTOM_EDIT( m_textureGroup, TXT("Texture group to use"), TXT("TextureGroupList") );
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS(CBitmapImporter);

CBitmapImporter::CBitmapImporter()
{
	// Supported class
	m_resourceClass = ClassID< CBitmapTexture >();
	m_preserveArtistData = false;
	m_textureGroup = CNAME( WorldDiffuse );

	// Supported formats
	m_formats.PushBack( CFileFormat( TXT("dds"), TXT("DirectDraw Surface") ) );
	m_formats.PushBack( CFileFormat( TXT("bmp"), TXT("Windows Bitmap") ) );
	m_formats.PushBack( CFileFormat( TXT("jpg"), TXT("Joint Photographics Experts Group") ) );
	m_formats.PushBack( CFileFormat( TXT("tga"), TXT("Truevision Targa") ) );
	m_formats.PushBack( CFileFormat( TXT("png"), TXT("Portable Network Graphics") ) );
}

CResource* CBitmapImporter::DoImport( const ImportOptions& options )
{
#ifndef NO_TEXTURE_IMPORT
	void* srcData = NULL;
	Uint32 rowPitch, depthPitch;
	GpuApi::TextureDesc desc;

	size_t lastDotIndex;
	if ( !options.m_sourceFilePath.FindCharacter( TXT('.'), lastDotIndex, true ) )
	{
		HALT( "Error parsing resource filename.")
		return nullptr;
	}
	ASSERT( lastDotIndex < options.m_sourceFilePath.Size(), TXT(" No separator for file extension found, invalid filename ") );
	GpuApi::eTextureImportFormat format = FileExtensionToFormat( options.m_sourceFilePath.MidString( lastDotIndex + 1, options.m_sourceFilePath.Size() - lastDotIndex ) );
	if ( !GpuApi::ImportTexture( options.m_sourceFilePath.AsChar(), format, srcData, rowPitch, depthPitch, desc ) )
	{
		HALT("Texture import failed" );
		return nullptr;
	}

	CName textureGroupName = m_textureGroup;
	Uint32 pcBias = 1;
	Uint32 xboneBias = 1;
	Uint32 ps4Bias = 1;

	// Copy or reuse reuse parameters
	CBitmapTexture* resource = Cast< CBitmapTexture >( options.m_existingResource );
	if ( resource )
	{
		if ( !resource->MarkModified() )
		{
			return resource;
		}

		textureGroupName = resource->GetTextureGroupName();
		pcBias    = resource->GetPCDownscaleBias();
		xboneBias = resource->GetXBoneDownscaleBias();
		ps4Bias   = resource->GetPS4DownscaleBias();
	}
	else
	{
		resource = CreateObject< CBitmapTexture >( options.m_parentObject );
	}

	Uint32 pixelSize;
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
	
	CSourceTexture* sourceTexture = CreateObject< CSourceTexture >( resource );
	sourceTexture->Init( desc.width, desc.height, rawFormat );

	ASSERT( srcData );
	// Allocate mipmap data
	CSourceTexture::CopyBufferPitched( sourceTexture->GetBufferAccessPointer(), desc.width * pixelSize, srcData, rowPitch, desc.width * pixelSize, desc.height );

	// Fill source data and mips
	resource->InitFromSourceData( sourceTexture, textureGroupName );

	resource->SetPCDownscaleBias( pcBias );
	resource->SetXBoneDownscaleBias( xboneBias );
	resource->SetPS4DownscaleBias( ps4Bias );

	if ( pcBias != 1 )
	{
		resource->DropMipmaps( pcBias - 1 );
	}

	// free data that was used to initialize the texture
	GpuApi::FreeTextureData( srcData );

	srcData = NULL;

	return resource;
#else
	return nullptr;
#endif
}

Bool CBitmapImporter::PrepareForImport( const String& filePath, ImportOptions& options )
{
	RED_UNUSED(options);
	if ( m_textureGroup != CNAME( WorldDiffuse ) )
	{
		return true;
	}

	m_textureGroup = CNAME( WorldDiffuse );

	if ( filePath.EndsWith( TXT("_n") ) )
	{
		m_textureGroup = CNAME( NormalmapGloss );
	}

	if ( filePath.EndsWith( TXT("_s") ) )
	{
		m_textureGroup = CNAME( WorldSpecular );
	}

	return true;
}
