
#include "build.h"
#include "itemsThumbnailGenerator.h"

#include "wxThumbnailImageLoader.h"
#include "../../common/core/depot.h"
#include "../../common/core/xmlFileReader.h"
#include "../../common/core/thumbnail.h"

namespace
{
	int GetEncoderClsid( const WCHAR* format, CLSID* pClsid )
	{
		UINT num = 0;          // number of image encoders
		UINT size = 0;         // size of the image encoder array in bytes

		Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

		Gdiplus::GetImageEncodersSize( &num, &size );
		if ( size == 0 )
			return -1;  // Failure

		pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(::malloc(size));
		if ( pImageCodecInfo == NULL )
			return -1;  // Failure

		Gdiplus::GetImageEncoders( num, size, pImageCodecInfo );

		for ( UINT j = 0; j < num; ++j )
		{
			if ( ::wcscmp( pImageCodecInfo[j].MimeType, format ) == 0 )
			{
				*pClsid = pImageCodecInfo[j].Clsid;
				::free(pImageCodecInfo);
				return j;  // Success
			}    
		}

		::free( pImageCodecInfo );
		return -1;  // Failure
	}

	Gdiplus::Bitmap* ResizeClone( Gdiplus::Bitmap *bmp, INT width, INT height )
	{
		UINT o_height = bmp->GetHeight();
		UINT o_width = bmp->GetWidth();
		INT n_width = width;
		INT n_height = height;
		double ratio = (double)o_width / (double)o_height;
		if (o_width > o_height) 
			// Resize down by width
			n_height = (double)n_width / ratio;
		else 
			n_width = n_height * ratio;
		Gdiplus::Bitmap* newBitmap = new Gdiplus::Bitmap( n_width, n_height, bmp->GetPixelFormat() );
		Gdiplus::Graphics graphics(newBitmap);
		graphics.DrawImage( bmp, 0, 0, n_width, n_height );
		return newBitmap;
	}

}

void CEdItemsThumbnailGenerator::ChooseOutputDir( wxWindow* parent, String& outputDirPath )
{
	wxDirDialog dirDialog( parent, wxT("Choose output directory"), GDepot->GetRootDataPath().AsChar(), wxDD_NEW_DIR_BUTTON );
	if ( dirDialog.ShowModal() == wxID_OK )
	{
		outputDirPath = dirDialog.GetPath().c_str().AsWChar();
		outputDirPath += TXT("\\");

		// converting path to depot path
		String localDepotPath;
		if ( GDepot->ConvertToLocalPath( outputDirPath, localDepotPath ) == false )
		{
			WARN_EDITOR( TXT("Could not convert '%s' to local depot path."), outputDirPath.AsChar() );
		}

		CDirectory* directory = GDepot->CreatePath( localDepotPath.AsChar() );
	}
}

void CEdItemsThumbnailGenerator::OutputToPNG( TDynArray< CThumbnail* >& thumbnails, const Uint32 width, const Uint32 height, const String& outputPath )
{
	const CWXThumbnailImage* img = dynamic_cast< const CWXThumbnailImage* >( thumbnails[0]->GetImage() );
	if ( img == nullptr )
	{
		WARN_EDITOR( TXT("Not supported thumbnail format") );
		return;
	}

	Gdiplus::Bitmap* bmp = img->GetBitmap();

	::CLSID pngClsid;
	if ( GetEncoderClsid( L"image/png", &pngClsid ) < 0 )
	{
		WARN_EDITOR( TXT("Cannot get png decoder") );
		return;
	}

	Red::TScopedPtr<Gdiplus::Bitmap> resizedBmp( ResizeClone( bmp, width, height ) );
	Gdiplus::Status status = resizedBmp->Save( outputPath.AsChar(), &pngClsid, NULL );
	if ( status != Gdiplus::Ok )
	{
		WARN_EDITOR( TXT("Error saving icon") );
	}
}

CEdItemsThumbnailGenerator::CEdItemsThumbnailGenerator( wxWindow* parent, CContextMenuDir* dir )
	: m_parent( parent )
	, m_dir( dir )
{
}

Bool CEdItemsThumbnailGenerator::Execute()
{
	if ( m_dir->GetDirs().Empty() )
	{
		return false;
	}

	TOptional<String> configRes = ReadConfigFile();
	if ( configRes.IsInitialized() )
	{
		GFeedback->ShowError( configRes.Get().AsChar() );
		return false;
	}

	CDirectory* dir = m_dir->GetDirs()[0];
	String path;
	dir->GetAbsolutePath( path );

	wxFileDialog fd( m_parent, wxT("Pick up items definition XML"), path.AsChar(), wxEmptyString, wxT("*.xml"), wxFD_OPEN );
	if ( fd.ShowModal() == wxID_OK )
	{
		String path = fd.GetPath();
		THashSet< String > warnings;

		TOptional< String > itemsRes;
		GFeedback->BeginTask( TXT("Generating"), false );
		{
			itemsRes = ReadItemsFile( path, warnings );
		}
		GFeedback->EndTask();

		if ( itemsRes.IsInitialized() )
		{
			GFeedback->ShowError( itemsRes.Get().AsChar() );
			return false;
		}

		if ( !warnings.Empty() )
		{
			String warningsStr;
			for ( const String& warning : warnings )
			{
				warningsStr += warning + TXT("\n");
			}
			GFeedback->ShowWarn( warningsStr.AsChar() );
		}

		return true;
	}

	return false;
}

TOptional<String> CEdItemsThumbnailGenerator::ReadConfigFile()
{
	m_itemInfos.Clear();

	const String xmlPath = GFileManager->GetBaseDirectory() + TXT("config/item_icon_generation.xml");
	IFile* reader = GFileManager->CreateFileReader( xmlPath, FOF_AbsolutePath );
	if ( !reader )
	{
		return false;
	}

	CXMLFileReader xmlReader( reader );
	if ( xmlReader.BeginNode( TXT("item_icon_generation") ) )
	{
		while ( xmlReader.BeginNode( TXT("item_desc") ) )
		{
			String category;
			if ( !xmlReader.Attribute( TXT("category"), category ) ) return TXT("'category' attribute not found");

			ItemInfo itemInfo;
			itemInfo.backgroundColor = Vector::ZEROS;

			while ( xmlReader.BeginNextNode() )
			{
				String nodeName;
				xmlReader.GetNodeName( nodeName );
				if ( nodeName == TXT("icon_desc") )
				{
					String w, h;
					ItemInfo::IconDef iconDef;
					if ( !xmlReader.AttributeT<Int32>( TXT("width"), iconDef.width ) ) return TXT("'width' attribute not found or wrong format");
					if ( !xmlReader.AttributeT<Int32>( TXT("height"), iconDef.height ) ) return TXT("'height' attribute not found or wrong format");
					itemInfo.iconDefs.PushBack( iconDef );
				}
				else if ( nodeName == TXT("environment") )
				{
					if ( !xmlReader.AttributeT<String>( TXT("name"), itemInfo.environment ) ) return TXT("'name' attribute not found or wrong format");
					CDiskFile* envFile = GDepot->FindFile( itemInfo.environment );
					if ( !envFile )
					{
						GFeedback->ShowWarn( TXT("Couldn't open env file '%s'"), itemInfo.environment.AsChar() );
					}
				}
				else if ( nodeName == TXT("camera") )
				{
					if ( !xmlReader.AttributeT<Float>( TXT("angle"), itemInfo.cameraAngle ) ) return TXT("'cameraAngle' attribute not found or wrong format");
					if ( !xmlReader.AttributeT<Float>( TXT("pitch"), itemInfo.cameraPitch ) ) return TXT("'cameraPitch' attribute not found or wrong format");
					if ( !xmlReader.AttributeT<Float>( TXT("fov"), itemInfo.cameraFov ) ) return TXT("'cameraFov' attribute not found or wrong format");
				}
				else if ( nodeName == TXT("light") )
				{
					if ( !xmlReader.AttributeT<Float>( TXT("angle"), itemInfo.lightAngle ) ) return TXT("'angle' attribute not found or wrong format");
				}
				else if ( nodeName == TXT("entity_search_dir") )
				{
					String entitySearchDir;
					if ( !xmlReader.AttributeT<String>( TXT("path"), entitySearchDir ) ) return TXT("'path' attribute not found or wrong format");
					itemInfo.entitySearchDirs.PushBack( entitySearchDir );
				}
				else if ( nodeName == TXT("background_color") )
				{
					if ( !xmlReader.AttributeT<Float>( TXT("r"), itemInfo.backgroundColor.X ) ) return TXT("'r' attribute not found or wrong format");
					if ( !xmlReader.AttributeT<Float>( TXT("g"), itemInfo.backgroundColor.Y ) ) return TXT("'g' attribute not found or wrong format");
					if ( !xmlReader.AttributeT<Float>( TXT("b"), itemInfo.backgroundColor.Z ) ) return TXT("'b' attribute not found or wrong format");
					if ( !xmlReader.AttributeT<Float>( TXT("a"), itemInfo.backgroundColor.W ) ) return TXT("'a' attribute not found or wrong format");
				}
				xmlReader.EndNode();
			}

			m_itemInfos.Insert( category, itemInfo );
			xmlReader.EndNode(); // item_desc
		}
		xmlReader.EndNode();
	}

	return TOptional<String>();
}

TOptional<String> CEdItemsThumbnailGenerator::ReadItemsFile( const String& path, THashSet< String >& warnings )
{
	String outputDirPath;
	ChooseOutputDir( m_parent, outputDirPath );

	IFile* reader = GFileManager->CreateFileReader( path, FOF_AbsolutePath );

	if ( !reader )
	{
		return TXT("File not found");
	}

	CXMLFileReader xmlReader( reader );
	if ( xmlReader.BeginNode( TXT("redxml") ) )
	{
		if ( xmlReader.BeginNode( TXT("definitions") ) )
		{
			if ( xmlReader.BeginNode( TXT("items") ) )
			{
				while( xmlReader.BeginNode( TXT("item") ) )
				{
					String category, entityTemplate, originalIconPath;
					if ( !xmlReader.AttributeT<String>( TXT("category"), category ) ) return TXT("'category' attribute not found or wrong format");
					if ( !xmlReader.AttributeT<String>( TXT("equip_template"), entityTemplate ) )
					{
						if ( !xmlReader.AttributeT<String>( TXT("hold_template"), entityTemplate ) )
						{
							String name;
							xmlReader.AttributeT<String>( TXT("name"), name ); 
							LOG_EDITOR( TXT("Item %s is missing 'equip_template/hold_template' attribute, skipping."),  );
							xmlReader.EndNode(); // item
							continue;
						}
					}
					if ( !xmlReader.AttributeT<String>( TXT("icon_path"), originalIconPath ) )
					{
						originalIconPath = entityTemplate;
					}
					GenerateThumbnail( outputDirPath, category, entityTemplate, originalIconPath, warnings );
					xmlReader.EndNode(); // item
				}
				xmlReader.EndNode(); // items
			}
			xmlReader.EndNode(); // definitions
		}
		xmlReader.EndNode(); // redxml
	}

	return TOptional<String>();
}

void CEdItemsThumbnailGenerator::GenerateThumbnail( 
	const String& outputPath, const String& category, const String& entityTemplate, const String& originalIconPath, THashSet< String >& warnings 
	)
{
	ItemInfo itemInfo;
	if ( !m_itemInfos.Find( category, itemInfo ) )
	{
		warnings.Insert( TXT("Unknown category '") + category + TXT("'") );
		return;
	}

	if ( itemInfo.iconDefs.Empty() || itemInfo.entitySearchDirs.Empty() )
	{
		warnings.Insert( TXT("Bad definition for category '") + category + TXT("'") );
		return;
	}

	CDiskFile* entTemplate = nullptr;
	for ( const String& searchPath : itemInfo.entitySearchDirs )
	{
		if ( CDirectory* dir = GDepot->FindPath( searchPath ) )
		{
			TDynArray< CDiskFile* > foundTemplates;
			dir->Search( entityTemplate + TXT(".w2ent"), foundTemplates );
			if ( !foundTemplates.Empty() )
			{
				entTemplate = foundTemplates[0];
				break;
			}
		}
	}

	if ( entTemplate == nullptr )
	{
		warnings.Insert( TXT("Cannot find item entity template '") + entityTemplate + TXT("'") );
		return;
	}

	SThumbnailSettings settings;
	settings.m_cameraRotation = EulerAngles( 0.f, itemInfo.cameraPitch, itemInfo.cameraAngle );
	settings.m_cameraFov = itemInfo.cameraFov;
	settings.m_lightPosition = itemInfo.lightAngle;
	settings.m_customSettings = TCS_CameraRotation|TCS_CameraFov|TCS_LightPosition;
	settings.m_environmentPath = itemInfo.environment;
	settings.m_flags |= TF_SetBackgroundColor;
	settings.m_backgroundColor = itemInfo.backgroundColor;
	settings.m_item = true;

	TDynArray< CThumbnail* > thumbnails;
	if ( !entTemplate->CreateThumbnail( thumbnails, &settings ) || thumbnails.Empty() )
	{
		warnings.Insert( TXT("Cannot generate thumbnail") );
		return;
	}

	for ( ItemInfo::IconDef& iconDef : itemInfo.iconDefs )
	{
		String outFileNameBase = CFilePath( originalIconPath ).GetFileName();
		size_t underscoreIdx = 0;
		if ( outFileNameBase.FindCharacter( L'_', underscoreIdx, true ) )
		{
			outFileNameBase = outFileNameBase.LeftString( underscoreIdx );
		}
		String outFileName = String::Printf( TXT("%s_%ix%i.png"), outFileNameBase.AsChar(), iconDef.width, iconDef.height );
		String outFilePath = outputPath + TXT("\\") + outFileName;

		OutputToPNG( thumbnails, iconDef.width, iconDef.height, outFilePath );
	}
}
