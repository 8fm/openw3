#include "build.h"
#include "importTextureSourceData.h"

#include "../../common/core/depot.h"
#include "../../common/core/garbageCollector.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/renderer.h"
#include "../../common/engine/bitmapTexture.h"

BEGIN_EVENT_TABLE( CEdImportTextureSourceData, wxFrame )
	EVT_BUTTON( XRCID("CheckOutAndReimport"), CEdImportTextureSourceData::OnStartReimport )
	EVT_BUTTON( XRCID("DeleteNotCooked"), CEdImportTextureSourceData::OnClearDepot )
	EVT_BUTTON( XRCID("DeleteNotCookedMeshes"), CEdImportTextureSourceData::OnClearDepotMeshes )
	EVT_BUTTON( XRCID("Refresh"), CEdImportTextureSourceData::OnRefresh )
	EVT_HTML_LINK_CLICKED( XRCID("TextureSourceDataLog"), CEdImportTextureSourceData::OnLinkClicked )
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////////


CEdImportTextureSourceData::CEdImportTextureSourceData( wxWindow* parent )
{
	// Load designed frame from resource
	wxXmlResource::Get()->LoadFrame( this, parent, TXT("TextureSourceDataImportTool") );

	// Get the log window
	m_stats = XRCCTRL( *this, "TextureSourceDataLog", wxHtmlWindow );

	RefreshLog();

	// Show the dialog
	Show();
	Layout();
}

CEdImportTextureSourceData::~CEdImportTextureSourceData()
{

}


void CEdImportTextureSourceData::OnLinkClicked( wxHtmlLinkEvent& event )
{
	// Get the link
	const wxHtmlLinkInfo& linkInfo = event.GetLinkInfo();
	wxString href = linkInfo.GetHref();

	if ( href.StartsWith( wxT("fix:") ) )
	{
		wxString resourcePath = href.AfterFirst( ':' );

		CBitmapTexture* object = LoadResource< CBitmapTexture >( String( resourcePath.wc_str() ) );

		if ( !object )
		{
			return;
		}
		
		// Find importer
		// Enumerate importable formats
		TDynArray< CFileFormat > formats;
		IImporter::EnumImportFormats( object->GetClass(), formats );

		// Decompose to file path
		String name;
		if ( object->GetFile() )
		{
			name = object->GetFile()->GetFileName();
		}
		else 
		{
			name = object->GetImportFile();
		}

		String defaultDir = TXT("Z:\\W2_Assets\\");
		String defaultFile = name;
		String wildCard = String::Printf( TXT("Bitmap files|*.%s"), formats[0].GetExtension().AsChar() );
		String importPath;

		for ( Uint32 i=1; i<formats.Size(); i++ )
		{
			wildCard += String::Printf( TXT(";*.%s")
				, formats[i].GetExtension().AsChar() );
		}

		wxFileDialog loadFileDialog( this, TXT("Import file"), defaultDir.AsChar(), defaultFile.AsChar(), wildCard.AsChar(), wxFD_OPEN );
		if ( loadFileDialog.ShowModal() == wxID_OK )
		{
			importPath = String( loadFileDialog.GetPath().wc_str() );
		}
		else 
		{
			return;
		}

		if ( !object->MarkModified() )
		{
			return;
		}

		object->SetImportFile( importPath );

		object->Save();

		//RefreshLog();

		return;
	}

	// Select asset
	String depotPath = href.wc_str();
	SEvents::GetInstance().DispatchEvent( CNAME( SelectAsset ), CreateEventData( depotPath ) );
}

namespace ImportTextureSourceDataHelper
{
	// Status of texture we are trying to determine
	enum ECheckTextureSourceDataStatus
	{
		ETS_HasSourceData,
		ETS_NotFileBasedResource,
		ETS_NoCompression,
		ETS_Locked,
		ETS_NoImportFileAtAll,
		ETS_ImportFileDoesntExist,
		ETS_HasImportFile,
	};

	ECheckTextureSourceDataStatus CheckTextureStatus( CBitmapTexture* texture )
	{
		if ( texture->GetSourceData() != NULL )
		{
			return ETS_HasSourceData;
		}

		if ( !texture->GetFile() )
		{
			return ETS_NotFileBasedResource;
		}

		if ( texture->GetCompression() == TCM_None )
		{
			String filename = texture->GetFile()->GetDepotPath();
			filename.ToLower();

			if ( filename.BeginsWith( TXT( "engine" ) ) )
			{
				return ETS_NoCompression;
			}
		}

		if ( texture->GetImportFile().Empty() )
		{
			return ETS_NoImportFileAtAll;
		}

		if ( GFileManager->GetFileSize( texture->GetImportFile() ) < 1 )
		{
			return ETS_ImportFileDoesntExist;
		}

		if ( texture->GetFile()->GetOpenStatus() == OS_OpenedBySomeoneElse )
		{
			return ETS_Locked;
		}

		return ETS_HasImportFile;
	}

	wxString GetStatusText( ECheckTextureSourceDataStatus status )
	{
		switch ( status )
		{
		case ETS_NoImportFileAtAll:
			return wxT( "No import file in resource" );
		case ETS_NoCompression:
			return wxT( "No compression, engine resource" );
		case ETS_ImportFileDoesntExist:
			return wxT( "Import file doesn't exist" );
		case ETS_Locked:
			return wxT( "Locked in source control" );
		case ETS_HasImportFile:
			return wxT( "Ok, should be able to import" );
		default: 
			return wxT( "Unknown" );
		}
	}

	// Status of reimporting texture
	enum EReimportTextureStatus
	{
		ERS_UnableToCheckout,
		ERS_UnableToMarkModified,
		ERS_ErrorReimporting,
		ERS_ReimportedButNoSourceData,	// reimported, but source data didn't save - WTF?
		ERS_Ok,
	};

	wxString GetReimportStatusText( EReimportTextureStatus status )
	{
		switch ( status )
		{
		case ERS_UnableToCheckout:
			return wxT( "Unable to checkout" );
		case ERS_UnableToMarkModified:
			return wxT( "Unable to mark resource as modified" );
		case ERS_ErrorReimporting:
			return wxT( "Some error during reimporting" );
		case ERS_ReimportedButNoSourceData:
			return wxT( "Reimported but it didn't help - wtf? Ask programmers." );
		case ERS_Ok:
			return wxT( "Succeeded" );
		default: 
			return wxT( "Unknown" );
		}
	}

	Bool Reimport( CBitmapTexture* bitmapTexture )
	{
		CFilePath path( bitmapTexture->GetImportFile() );

		// Get suitable importer for this resource
		IImporter* importer = IImporter::FindImporter( bitmapTexture->GetClass(), path.GetExtension() );
		if ( !importer )
		{
			WARN_EDITOR( TXT("No valid importer for extension '%s'"), path.GetExtension().AsChar() );
			return false;
		}

		// Define import data, make sure to specify existing resource
		IImporter::ImportOptions options;
		options.m_existingResource = bitmapTexture;
		options.m_parentObject = bitmapTexture->GetParent();
		options.m_sourceFilePath = bitmapTexture->GetImportFile();

		// Do the (re)import !
		CResource* imported = importer->DoImport( options );
		if ( !imported )
		{
			// Report error
			WARN_EDITOR( TXT("Unable to reimport '%s' from '%s'"), bitmapTexture->GetClass()->GetName().AsString().AsChar(), bitmapTexture->GetImportFile().AsChar() );
			return false;
		}

		// Old resource and imported resource should point to the same location
		ASSERT( imported == bitmapTexture );

		// Set import file 
		imported->SetImportFile( options.m_sourceFilePath );

		// Save this resource for the first time, this will also create thumbnail
		if ( !imported->Save() )
		{
			WARN_EDITOR( TXT("Unable to save reimported '%s'"), path.GetFileName().AsChar() );
			return false;
		}

		// Well, we have imported something
		return true;
	}
};

void CEdImportTextureSourceData::RefreshLog()
{
	m_texturesInDepot.ClearFast();
	m_texturesInDepot.Reserve( 1024 );

	m_resourcesToReimport.ClearFast();

	String depotPath;
	GDepot->GetAbsolutePath( depotPath );
	GFileManager->FindFiles( depotPath, TXT("*.xbm"), m_texturesInDepot, true );

	for ( Uint32 i = 0; i < m_texturesInDepot.Size(); ++i )
	{
		String dp;
		GDepot->ConvertToLocalPath( m_texturesInDepot[i], dp );
		m_texturesInDepot[i] = dp;
	}

	GFileManager->FindFiles( depotPath, TXT("*.w2mesh"), m_meshesInDepot, true );

	for ( Uint32 i = 0; i < m_meshesInDepot.Size(); ++i )
	{
		String dp;
		GDepot->ConvertToLocalPath( m_meshesInDepot[i], dp );
		m_meshesInDepot[i] = dp;
	}

	GRender->ForceFakeDeviceLost();

	// Format source code
	wxString sourceCode;

	sourceCode += wxT( "<table><tr><td>Resource</td><td>Status</td><td>Import file</td></tr>" );

	Uint32 counter = 0;

	GFeedback->BeginTask( TXT("Enumerating all textures..."), false );

	for ( Uint32 i = 0; i < m_texturesInDepot.Size(); ++i )
	{
		if ( counter++ == 300 )
		{
			counter = 0;
			SGarbageCollector::GetInstance().CollectNow();
		}

		GFeedback->UpdateTaskProgress( i, m_texturesInDepot.Size() );

		CBitmapTexture* texture = LoadResource<CBitmapTexture>( m_texturesInDepot[i] );

		if ( texture )
		{
			ImportTextureSourceDataHelper::ECheckTextureSourceDataStatus objectStatus;
			objectStatus = ImportTextureSourceDataHelper::CheckTextureStatus( texture );

			if ( objectStatus != ImportTextureSourceDataHelper::ETS_NotFileBasedResource && objectStatus != ImportTextureSourceDataHelper::ETS_HasSourceData && objectStatus != ImportTextureSourceDataHelper::ETS_NoCompression )
			{

				wxString color = objectStatus == ImportTextureSourceDataHelper::ETS_HasImportFile ? wxT("#aaffaa") : wxT("#ffaaaa");

				wxString str;
				if ( objectStatus == ImportTextureSourceDataHelper::ETS_ImportFileDoesntExist )
				{
					str.Printf( wxT( "<tr bgcolor=%s><td><a href=\"%s\">%s</a></td><td>%s</td><td>%s<br /><a href=\"fix:%s\">Change it to other file</a></td></tr>" ), color.wc_str(), texture->GetFile()->GetDepotPath().AsChar(), texture->GetFile()->GetDepotPath().AsChar(), ImportTextureSourceDataHelper::GetStatusText( objectStatus ).wc_str(), texture->GetImportFile().AsChar(), texture->GetFile()->GetDepotPath().AsChar() );
				}
				else
				{
					str.Printf( wxT( "<tr bgcolor=%s><td><a href=\"%s\">%s</a></td><td>%s</td><td>%s</td></tr>" ), color.wc_str(), texture->GetFile()->GetDepotPath().AsChar(), texture->GetFile()->GetDepotPath().AsChar(), ImportTextureSourceDataHelper::GetStatusText( objectStatus ).wc_str(), texture->GetImportFile().AsChar() );
				}
				sourceCode += str;

				if ( objectStatus == ImportTextureSourceDataHelper::ETS_HasImportFile )
				{
					m_resourcesToReimport.PushBackUnique( texture->GetFile()->GetDepotPath() );
				}
			}

		}
	}

	sourceCode += wxT( "</table>" );

	GFeedback->EndTask();

	m_stats->SetPage( sourceCode );

	GRender->ForceFakeDeviceUnlost();
	GRender->ForceFakeDeviceReset();
}

void CEdImportTextureSourceData::OnStartReimport( wxCommandEvent& event )
{
	GFeedback->BeginTask( TXT("Reimporting textures"), false );

	GRender->ForceFakeDeviceLost();

	// Format source code
	wxString sourceCode;

	sourceCode += wxT( "<table><tr><td>Resource</td><td>Status</td></tr>" );

	Uint32 counter = 0;
	Uint32 counterGC = 0;

	for ( Uint32 i = 0; i < m_resourcesToReimport.Size(); ++i )
	{
		CBitmapTexture* texture = LoadResource<CBitmapTexture>( m_resourcesToReimport[i] );
		ASSERT( texture );

		if ( counterGC++ == 50 )
		{
			counterGC = 0;
			SGarbageCollector::GetInstance().CollectNow();
		}

		if ( texture )
		{
			ImportTextureSourceDataHelper::ECheckTextureSourceDataStatus objectStatus;
			objectStatus = ImportTextureSourceDataHelper::CheckTextureStatus( texture );

			if ( objectStatus == ImportTextureSourceDataHelper::ETS_HasImportFile )
			{
				if ( m_resourcesToReimport.Exist( texture->GetFile()->GetDepotPath() ) )
				{
					GFeedback->UpdateTaskProgress( counter++, m_resourcesToReimport.Size() );

					ImportTextureSourceDataHelper::EReimportTextureStatus reimportStatus = ImportTextureSourceDataHelper::ERS_Ok;

					if ( texture->GetFile()->SilentCheckOut() )
					{
						if ( texture->MarkModified() )
						{
							if ( ImportTextureSourceDataHelper::Reimport( texture ) )
							{
								if ( texture->GetSourceData() )
								{
									reimportStatus = ImportTextureSourceDataHelper::ERS_Ok;
								}
								else
								{
									reimportStatus = ImportTextureSourceDataHelper::ERS_ReimportedButNoSourceData;
								}
							}
							else
							{
								reimportStatus = ImportTextureSourceDataHelper::ERS_ErrorReimporting;
							}
						}
						else
						{
							reimportStatus = ImportTextureSourceDataHelper::ERS_UnableToMarkModified;
						}
					}
					else
					{
						reimportStatus = ImportTextureSourceDataHelper::ERS_UnableToCheckout;
					}

					wxString color = reimportStatus == ImportTextureSourceDataHelper::ERS_Ok ? wxT("#aaffaa") : wxT("#ffaaaa");

					wxString str;
					str.Printf( wxT( "<tr bgcolor=%s><td><a href=\"%s\">%s</a></td><td>%s</td></tr>" ), color.wc_str(), texture->GetFile()->GetDepotPath().AsChar(), texture->GetFile()->GetDepotPath().AsChar(), ImportTextureSourceDataHelper::GetReimportStatusText( reimportStatus ).wc_str() );
					sourceCode += str;
				}
			}

		}
	}
	

	sourceCode += wxT( "</table>" );

	m_resourcesToReimport.ClearFast();

	GFeedback->EndTask();

	GRender->ForceFakeDeviceUnlost();
	GRender->ForceFakeDeviceReset();

	m_stats->SetPage( sourceCode );
}

void CEdImportTextureSourceData::OnRefresh( wxCommandEvent& event )
{
	RefreshLog();
}

void CEdImportTextureSourceData::OnClearDepot( wxCommandEvent& event )
{
	String defaultDir;
	String defaultFile;
	String wildCard = TXT("List files|*.list");
	String filePath;

	m_texturesToDelete = m_texturesInDepot;

	wxFileDialog loadFileDialog( this, TXT("Import file"), defaultDir.AsChar(), defaultFile.AsChar(), wildCard.AsChar(), wxFD_OPEN );
	if ( loadFileDialog.ShowModal() == wxID_OK )
	{
		filePath = String( loadFileDialog.GetPath().wc_str() );
		Char buf[ 4096 ];

		FILE* fil = _wfopen( filePath.AsChar(), TXT( "r" ) );

		if ( !fil )
		{
			GFeedback->ShowError( TXT("Unable to open file") );
		}

		while ( fgetws( buf, 4096, fil ) )
		{
			String readPath = String::Printf( TXT("%s"), buf );
			readPath.Trim();

			m_texturesToDelete.Remove( readPath );
		}

		ASSERT( m_texturesToDelete.Size() != m_texturesInDepot.Size() );

		for ( Uint32 i = 0; i < m_texturesToDelete.Size(); ++i )
		{
			CDiskFile* file = GDepot->FindFile( m_texturesToDelete[i] );
			if ( file )
			{
				file->Delete( false, false );
			}
		}
	}

}


void CEdImportTextureSourceData::OnClearDepotMeshes( wxCommandEvent& event )
{
	String defaultDir;
	String defaultFile;
	String wildCard = TXT("List files|*.list");
	String filePath;

	m_meshesToDelete = m_meshesInDepot;

	wxFileDialog loadFileDialog( this, TXT("Import file"), defaultDir.AsChar(), defaultFile.AsChar(), wildCard.AsChar(), wxFD_OPEN );
	if ( loadFileDialog.ShowModal() == wxID_OK )
	{
		filePath = String( loadFileDialog.GetPath().wc_str() );
		Char buf[ 4096 ];

		FILE* fil = _wfopen( filePath.AsChar(), TXT( "r" ) );

		if ( !fil )
		{
			GFeedback->ShowError( TXT("Unable to open file") );
		}

		while ( fgetws( buf, 4096, fil ) )
		{
			String readPath = String::Printf( TXT("%s"), buf );
			readPath.Trim();

			m_meshesToDelete.Remove( readPath );
		}

		ASSERT( m_meshesToDelete.Size() != m_meshesInDepot.Size() );

		for ( Uint32 i = 0; i < m_meshesToDelete.Size(); ++i )
		{
			CDiskFile* file = GDepot->FindFile( m_meshesToDelete[i] );
			if ( file )
			{
				file->Delete( false, false );
			}
		}
	}

}

