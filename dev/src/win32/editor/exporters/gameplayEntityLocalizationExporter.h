/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "localizationExporter.h"

class CGameplayEntityLocalizationExporter : public AbstractLocalizationExporter
{
public:
	CGameplayEntityLocalizationExporter();

	virtual void BeginBatchExport();
	virtual void EndBatchExport();
	virtual void ExportResource( CResource* resource );
	virtual Bool CanExportResource( CResource* resource );

	virtual void ExportBatchEntry ( const String& entry );

	virtual Bool IsBatchEntryValid( const String& entry ) const override;
	virtual Bool ValidateBatchEntry( const String& entry) override;
	virtual void PushBatchEntryError( const String& entry, const String& errorMsg ) override;

protected:
	virtual Bool	CanExportFile( CDiskFile* file ) const;
	virtual String	GetResourceExtension() const { return TXT( ".w2ent" ); }
};