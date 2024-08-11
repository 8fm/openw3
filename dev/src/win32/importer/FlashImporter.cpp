/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "../../common/redSystem/guid.h"
#include "../../common/redSystem/utility.h"

#include "../../common/core/feedback.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/core/depot.h"
#include "../../common/core/importer.h"

#include "GFxExporter.h"

class CFlashImporter : public IImporter
{
	DECLARE_ENGINE_CLASS( CFlashImporter, IImporter, 0 );

private:
	String m_tmpDir;

public:
	CFlashImporter();

	virtual CResource* DoImport( const ImportOptions& options );
};

BEGIN_CLASS_RTTI( CFlashImporter )
	PARENT_CLASS( IImporter )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CFlashImporter );

CFlashImporter::CFlashImporter()
{
	// Importer
	m_resourceClass = ClassID< CSwfResource >();
	m_formats.PushBack( CFileFormat( TXT("swf"), TXT("Flash SWF File") ) );

	Char buf[MAX_PATH];
	::GetTempPath(MAX_PATH, buf);
	m_tmpDir = String::Printf(TXT("%lsgfximport{%ls}\\"), buf, ToString( Red::System::GUID::Create() ).AsChar() );

	// Load config
	LoadObjectConfig( TXT("User") );
}

#define ERR_MSG( fmt, ... )\
do\
{\
	ERR_IMPORTER( fmt, ## __VA_ARGS__ );\
	if ( showFeedback )\
	{\
		GFeedback->ShowError( fmt, ## __VA_ARGS__ );\
	}\
}while(0)

CResource* CFlashImporter::DoImport( const ImportOptions& options )
{
	// Save
	SaveObjectConfig( TXT("User") );

	Bool showFeedback = ! options.m_params || ! static_cast< CSwfResource::ImportParams* >( options.m_params )->m_noFeedback;

	CGFxExporter gfxExporter;
	CGFxExporter::SExportOptions exportOptions;
	exportOptions.m_gfxExportExePath = TXT("tools\\GFx4\\gfxexport_mult4fix.exe");
	exportOptions.m_dumpDirectoryPath = m_tmpDir;
	exportOptions.m_sourceFilePath = options.m_sourceFilePath;
	exportOptions.m_linkageFilePath = options.m_sourceFilePath;
	exportOptions.m_imageQuality = CGFxExporter::ImageQuality_Fast;
	exportOptions.m_doFullExports = true;
	exportOptions.m_doSwfCompression = true;

	CGFxExporter::SExportInfo exportInfo;

	if ( ! gfxExporter.DoExport( exportOptions, exportInfo ) )
	{
		ERR_MSG( TXT("Failed to process GFx file for '%ls'"), options.m_sourceFilePath.AsChar() );
		return nullptr;
	}

	CSwfResource::FactoryInfo info;
	info.m_dataBuffer.MoveHandle( exportInfo.m_strippedSwf );
	info.m_linkageName = Move( exportInfo.m_linkageName );
	info.m_textureInfos = Move( exportInfo.m_textureInfos );
	info.m_fonts = Move( exportInfo.m_fonts );
	info.m_header = exportInfo.m_header;
	info.m_imageImportOptions = exportInfo.m_imageExportString;
	
	IFile* sourceSwfReader = GFileManager->CreateFileReader( options.m_sourceFilePath, FOF_AbsolutePath | FOF_Buffered );
	if ( ! sourceSwfReader )
	{
		ERR_MSG( TXT("Failed to open '%ls' for reading"), options.m_sourceFilePath.AsChar() );
		return nullptr;
	}

	info.m_sourceSwf.Allocate( static_cast< DataBuffer::TSize >( sourceSwfReader->GetSize() ) );
	sourceSwfReader->Serialize( info.m_sourceSwf.GetData(), info.m_sourceSwf.GetSize() );
	delete sourceSwfReader;
	sourceSwfReader = nullptr;

	CSwfResource* swfResource = nullptr;
	info.m_parent = options.m_parentObject;
	info.m_reuse = Cast< CSwfResource >( options.m_existingResource );

	swfResource = CSwfResource::Create( info );
	if ( ! swfResource )
	{
		ERR_MSG( TXT("Failed to create SWF resource for '%ls'"), options.m_sourceFilePath.AsChar() );
		return nullptr;
	}

	if ( ! CSwfResource::VerifySwf( swfResource->GetDataBuffer() ) )
	{
		ERR_MSG( TXT("Imported SWF is corrupt or empty '%ls'"), options.m_sourceFilePath.AsChar() );
		swfResource->Discard();
		return nullptr;
	}

	return swfResource;
}
