/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "exporters/storySceneExporter.h"
#include "exporters/localizationExporter.h"

class CLipsyncDataSceneExporter : public AbstractLocalizationExporter, public CStorySceneExporter
{
public:
	CLipsyncDataSceneExporter();

private:
	Bool m_wasLocalizationManagerIgnoringPackResources;
	Bool m_generateOnlyText;
	
	String	m_editorLocale;

	String	m_voiceoverDirectory;
	String	m_lipsyncDirectory;
	String	m_textDirectory;

	TDynArray< String >	m_addedLipsyncsPaths;

public:
	virtual void BeginBatchExport();
	virtual void EndBatchExport();
	virtual void ExportResource( CResource* resource );
	virtual Bool CanExportResource( CResource* resource );

	virtual Bool DoesExportResources() { return true; }
	virtual void ExportCustom( const CVariant& val ) {}

	virtual Bool IsBatchedExport() { return true; }

	virtual void ExportBatchEntry ( const String& entry );

	virtual Bool IsBatchEntryValid( const String& entry ) const override;
	virtual Bool ValidateBatchEntry( const String& entry) override;
	virtual void PushBatchEntryError( const String& entry, const String& errorMsg ) override;

protected:
	virtual void DoBeginExport() {}
	virtual void DoEndExport() {}
	virtual void DoExportEmptyLine() {}
	virtual void DoExportStorySceneChoice( const CStorySceneChoice* sectionChoice, CStoryScene* storyScene ) {}
	virtual void DoExportStorySceneComment( const CStorySceneComment* sceneComment, CStoryScene* storyScene, Bool isLastElement = false ) {}
	virtual void DoExportSceneSectionHeader( const CStorySceneSection* section, CStoryScene* storyScene ) {}
	virtual void DoExportStorySceneLine( const CStorySceneLine* sceneLine, CStoryScene* storyScene, const CStorySceneSection* section, Bool isLastLine = false );

private:
	Bool ShouldRegenerateLipsync( const String& lipsyncPath, const String& voiceoverPath ) const;

protected:
	virtual Bool	CanExportFile( CDiskFile* file ) const { return file != NULL && file->GetFileName().EndsWith( TXT( ".w2scene" ) ); }
	virtual String	GetResourceExtension() const { return TXT( ".w2scene" ); }
};
