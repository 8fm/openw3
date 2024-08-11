/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/core/cooker.h"
#include "../../common/core/dependencyMapper.h"
#include "../../common/core/importer.h"
#include "../../common/engine/swfResource.h"
#include "../../common/engine/swfTexture.h"

#include "../importer/GFxExporter.h"

class CFlashCooker : public ICooker
{
	DECLARE_RTTI_SIMPLE_CLASS( CFlashCooker );

public:
	CFlashCooker();
	~CFlashCooker();
	virtual Bool DoCook( class ICookerFramework& cooker, const CookingOptions& options ) override;

private:
	String m_tmpDir;
};

BEGIN_CLASS_RTTI( CFlashCooker );
	PARENT_CLASS( ICooker );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CFlashCooker );

static Bool RunProcess( const String& exeName, const String& cmdLine );

CFlashCooker::CFlashCooker()
{
	m_resourceClass = ClassID< CSwfResource >();

	// Add supported platforms
	m_platforms.PushBack( PLATFORM_PC );
#ifndef WCC_LITE
	m_platforms.PushBack( PLATFORM_XboxOne );
	m_platforms.PushBack( PLATFORM_PS4 );
#endif

	Char buf[MAX_PATH];
	::GetTempPath(MAX_PATH, buf);
	m_tmpDir = String::Printf(TXT("%lsgfxcook{%ls}\\"), buf, ToString( Red::System::GUID::Create() ).AsChar() );
}

CFlashCooker::~CFlashCooker()
{
	// clean up temp dir recursively
}

Bool CFlashCooker::DoCook( class ICookerFramework& cooker, const CookingOptions& options )
{
	return true;

#if 0

	// Trying to cook resource that was already cooked
	if ( options.m_resource->HasFlag( OF_WasCooked ) )
	{
		WARN_WCC( TXT("Resource '%ls' was already cooked. Not cooking."), options.m_resource->GetFriendlyName().AsChar() );
		return true;
	}

	CSwfResource* swfResource = SafeCast< CSwfResource >( options.m_resource );
	if ( ! swfResource )
	{
		return false;
	}

	LatentDataBuffer& sourceSwf = swfResource->GetSourceSwf();
	if ( ! sourceSwf.Load() )
	{
		ERR_WCC( TXT("Could not load source SWF for '%ls'"), swfResource->GetDepotPath().AsChar() );
		return false;
	}

	if ( ! CSwfResource::VerifySwf( sourceSwf ) )
	{
		ERR_WCC( TXT("SWF file '%ls' is corrupt or empty"), swfResource->GetDepotPath().AsChar() );
		return false;
	}

	// The GFxExport or the nvidia texture tool apparently can't handle certain paths like with GUIDs and braces, so just doing this for extraction
	String swfExtractDir = GFileManager->GetDataDirectory() + TXT("tmp\\flashDump\\");

	CFilePath sourceSwfFilePath( swfResource->GetImportFile() );
	const String tmpFileSwf = swfExtractDir + sourceSwfFilePath.GetFileNameWithExt();
	IFile* swfWriter = GFileManager->CreateFileWriter( tmpFileSwf, FOF_AbsolutePath | FOF_Buffered );
	if ( ! swfWriter )
	{
		ERR_WCC( TXT("Could not open file '%ls' for writing"), tmpFileSwf.AsChar() );
		return false;
	}

	swfWriter->Serialize( sourceSwf.GetData(), sourceSwf.GetSize() );
	delete swfWriter;
	swfWriter = nullptr;

	// hacky flag to make the cooking faster when possible
	const Bool extraFastCooking = (Red::StringSearch( SGetCommandLine(), TXT("-extrafast") ) != nullptr);

	CGFxExporter gfxExporter;
	CGFxExporter::SExportOptions exportOptions;
	exportOptions.m_gfxExportExePath = TXT("tools\\GFx4\\gfxexport_mult4fix.exe");
	exportOptions.m_dumpDirectoryPath = m_tmpDir;
	exportOptions.m_sourceFilePath = tmpFileSwf;
	exportOptions.m_linkageFilePath = swfResource->GetImportFile();
	exportOptions.m_imageQuality = extraFastCooking ? CGFxExporter::ImageQuality_Fast : CGFxExporter::ImageQuality_Production;
	exportOptions.m_doFullExports = false;
	exportOptions.m_doSwfCompression = false; // use package compression

	CGFxExporter::SExportInfo exportInfo;

	if ( ! gfxExporter.DoExport( exportOptions, exportInfo ) )
	{
		ERR_WCC( TXT("Failed to process GFx file for '%ls'"), swfResource->GetDepotPath().AsChar() );
		GFileManager->DeleteFile( tmpFileSwf );
		return false;
	}

	GFileManager->DeleteFile( tmpFileSwf );

	CSwfResource::CookInfo cookInfo;
	cookInfo.m_dataBuffer.MoveHandle( exportInfo.m_strippedSwf );
	cookInfo.m_textureInfos = Move( exportInfo.m_textureInfos );
	if ( ! CSwfResource::SetCookedData( swfResource, cookInfo, options.m_platform ) )
	{
		ERR_WCC( TXT("Failed to set cooked data for file '%ls'"), swfResource->GetDepotPath().AsChar() );
		return false;
	}

	swfResource->CleanupSourceData();

	if ( ! CSwfResource::VerifySwf( swfResource->GetDataBuffer() ) )
	{
		ERR_WCC( TXT("SWF corrupted by cook or empty?") );
		return false;
	}

	return true;

#endif // #if 0
}
