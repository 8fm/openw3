/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "localizationExporter.h"
#include "storySceneExporter.h"

enum ELocalizationExportAdditionalColumns
{
	LEAC_None			= FLAG( 0 ),
	LEAC_Oneliners		= FLAG( 1 ),
	LEAC_CSDescription	= FLAG( 2 ),
	LEAC_OnlyCutscenes	= FLAG( 3 ),
	LEAC_OnlyActorList	= FLAG( 4 )
};

class CStorySceneLocalizationExporter : public CStorySceneExporter, public AbstractLocalizationExporter
{
private:
	Bool	m_isBatch;

	Bool	m_markVoicetags;
	Bool	m_markOneliners;
	Bool	m_cutsceneDescriptions;
	Int32		m_options;

	TDynArray< String > m_importantVoicetagsStrings;
	THashMap< String, String >	m_linkSourceSections;

public:
	CStorySceneLocalizationExporter( Int32 additionalColumnsFlags = LEAC_None );

	virtual void BeginBatchExport();
	virtual void EndBatchExport();
	virtual void ExportResource( CResource* resource );
	virtual Bool CanExportResource( CResource* resource );

	virtual void ExportBatchEntry ( const String& entry );

	virtual Bool IsBatchEntryValid( const String& entry ) const override;
	virtual Bool ValidateBatchEntry( const String& entry) override;
	virtual void PushBatchEntryError( const String& entry, const String& errorMsg ) override;

public:
	virtual Bool IsBatchedExport();
	virtual void ExportScene( CStoryScene* scene );
	virtual void ExportSceneSection( CStoryScene* storyScene, const  CStorySceneSection* section );

	RED_INLINE void SetMarkVoicetags( Bool mark ) { m_markVoicetags = mark; }

protected:
	virtual void DoBeginExport();
	virtual void DoEndExport();
	virtual void DoExportEmptyLine();
	virtual void DoExportStorySceneChoice( const CStorySceneChoice* sectionChoice, CStoryScene* storyScene );
	virtual void DoExportStorySceneComment( const CStorySceneComment* sceneComment, CStoryScene* storyScene, Bool isLastElement = false );
	virtual void DoExportSceneSectionHeader( const CStorySceneSection* section, CStoryScene* storyScene );
	virtual void DoExportStorySceneLine( const CStorySceneLine* sceneLine, CStoryScene* storyScene, const CStorySceneSection* section, Bool isLastLine = false );

	virtual String GetResourceName( CResource* resource );

protected:
	virtual String	GetSceneName( CStoryScene* scene );

	void GetNextSections( const CStorySceneLinkElement* link, TDynArray< String >& nextSectionNames );
	void PushLanguageLineData( TDynArray< String >& lineData, const String& comment, const String& voicetag, const String& directorsComment, const String& line, const String& choice, const String& nextSectionName );
	void DoExportAdditionalLink( CStoryScene* storyScene, const String& nextSectionName );

	Bool CanExportScene( CStoryScene* scene );

protected:
	virtual Bool	CanExportFile( CDiskFile* file ) const { return file != NULL && file->GetFileName().EndsWith( TXT( ".w2scene" ) ); }
	virtual String	GetResourceExtension() const { return TXT( ".w2scene" ); }

protected:
	Bool IsSceneValid( const CStoryScene* scene ) const;
	Bool ValidateScene( CStoryScene* scene ) const;

private:
	Uint32 PerformExtraSceneChecks( const CStoryScene* scene, TDynArray< String >& outInfoRows ) const;
};
