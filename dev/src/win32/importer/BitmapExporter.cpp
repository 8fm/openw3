/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/core/exporter.h"
#include "../../common/engine/bitmapTexture.h"

/// Bitmap texture Exporter based on DirectXTex functionality
class CBitmapExporter : public IExporter
{
	DECLARE_ENGINE_CLASS( CBitmapExporter, IExporter, 0 );

public:
	CBitmapExporter();
	virtual Bool DoExport( const ExportOptions& options );

	static GpuApi::eTextureSaveFormat FileExtensionToSaveFormat( const String& extension );
};

DEFINE_SIMPLE_RTTI_CLASS(CBitmapExporter,IExporter);
IMPLEMENT_ENGINE_CLASS(CBitmapExporter);

CBitmapExporter::CBitmapExporter()
{
	// Supported class
	m_resourceClass = ClassID< CBitmapTexture >();

	// Supported formats
	m_formats.PushBack( CFileFormat( TXT("dds"), TXT("DirectDraw Surface") ) );
	m_formats.PushBack( CFileFormat( TXT("bmp"), TXT("Windows Bitmap") ) );
	m_formats.PushBack( CFileFormat( TXT("jpg"), TXT("Joint Photographics Experts Group") ) );
	m_formats.PushBack( CFileFormat( TXT("tga"), TXT("Truevision Targa") ) );
	m_formats.PushBack( CFileFormat( TXT("png"), TXT("Portable Network Graphics") ) );
}

GpuApi::eTextureSaveFormat CBitmapExporter::FileExtensionToSaveFormat( const String& extension )
{
	struct 
	{
		GpuApi::eTextureSaveFormat	m_format;
		Char						m_extension[4];
	} Mapper[] = 
	{
		{ GpuApi::SAVE_FORMAT_DDS,	TXT("dds"), },
		{ GpuApi::SAVE_FORMAT_BMP,	TXT("bmp"), },
		{ GpuApi::SAVE_FORMAT_JPG,	TXT("jpg"), },
		{ GpuApi::SAVE_FORMAT_TGA,	TXT("tga"), },
		{ GpuApi::SAVE_FORMAT_PNG,	TXT("png"), },
	};

	for ( Int32 i = 0; i < ARRAY_COUNT( Mapper ); i ++ )
	{
		if ( !Red::System::StringCompare( Mapper[i].m_extension, extension.AsChar() ) )
		{
			return Mapper[i].m_format;
		}
	}

	return GpuApi::SAVE_FORMAT_PNG;
}

bool CBitmapExporter::DoExport( const ExportOptions& options )
{
	ASSERT( options.m_resource->IsA( ClassID<CBitmapTexture>() ));

	// Get bitmap texture resource
	CBitmapTexture* bitmapTexture = Cast< CBitmapTexture >( options.m_resource );
	if ( !bitmapTexture->GetMips().Size() )
	{
		return false;
	}

	// Get mipmap
	const CBitmapTexture::MipMap& baseMip = bitmapTexture->GetMips()[0];
	if ( !const_cast< CBitmapTexture::MipMap& >( baseMip ).m_data.Load() )
	{
		return false;
	}

	GpuApi::eTextureFormat format = bitmapTexture->GetPlatformSpecificCompression();
	GpuApi::eTextureSaveFormat saveFormat = CBitmapExporter::FileExtensionToSaveFormat( options.m_saveFileFormat.GetExtension() );

	GpuApi::TextureDataDesc textureToSave;
	Uint8* pData = (Uint8*)baseMip.m_data.GetData();
	textureToSave.data = &pData;
	textureToSave.slicePitch = baseMip.m_data.GetSize();
	textureToSave.width = baseMip.m_width;
	textureToSave.height = baseMip.m_height;
	textureToSave.format = format;
	textureToSave.rowPitch = baseMip.m_pitch;

	return GpuApi::SaveTextureToFile( textureToSave, options.m_saveFilePath.AsChar(), saveFormat );

	//return GpuApi::SaveTexture( (Uint8*)baseMip.m_data.GetData(), baseMip.m_data.GetSize(), baseMip.m_width, baseMip.m_height, format, baseMip.m_pitch, options.m_saveFilePath.AsChar(), saveFormat );
}
