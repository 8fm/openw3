/**
* Copyright c 2011 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "exporters/localizationExporter.h"

class CQuestGraph;

class CSceneUsageExporter : public AbstractLocalizationExporter
{
protected:
	TSet< String > m_usedScenes;
	TDynArray< String > m_directories;
	TSet< CQuestGraph* > m_exploredGraphs;

public:
	CSceneUsageExporter ();

	virtual void BeginBatchExport();
	virtual void EndBatchExport();

	virtual void ExportBatchEntry ( const String& entry );
	//virtual void FillRootExportGroup( BatchExportGroup& exportGroup );

	virtual Bool IsBatchEntryValid( const String& entry ) const override;
	virtual Bool ValidateBatchEntry( const String& entry) override;
	virtual void PushBatchEntryError( const String& entry, const String& errorMsg ) override;

protected:
	virtual Bool	CanExportFile( CDiskFile* file ) const;
	virtual String	GetResourceExtension() const;

	virtual void FillRootExportGroup( BatchExportGroup& exportGroup );

	void CollectScenesPaths( CDirectory* directory, TDynArray<String>& usedScenes, TDynArray< String >& unusedScenes );
	void ExportSingleColumnLine( const String& text );
	void ExportPath( const String& path );

	void ScanDirectoryForScene( CDirectory* directory, Uint32 level = 0 );
	void ScanEntityForScenes( CEntityTemplate* entityTemplate );
	void ScanQuestForScene( CQuestPhase* quest );
	void ScanCommunityForScene( CCommunity* community );
};