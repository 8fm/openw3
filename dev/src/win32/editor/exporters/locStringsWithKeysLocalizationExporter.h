/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "localizationExporter.h"
#include "../localizedStringsWithKeysManager.h"

class CLocStringsWithKeysLocalizationExporter : public AbstractLocalizationExporter
{
public:
	CLocStringsWithKeysLocalizationExporter();

	virtual void BeginBatchExport();
	virtual void EndBatchExport();
	virtual void ExportResource( CResource* resource ) {}
	virtual Bool CanExportResource( CResource* resource ) { return false; }
	virtual Bool DoesExportResources() { return false; }
	virtual void ExportCustom( const CVariant& val );

	virtual void ExportBatchEntry ( const String& entry );
	virtual void FillRootExportGroup( BatchExportGroup& exportGroup );

	virtual Bool IsBatchEntryValid( const String& entry ) const override;
	virtual Bool ValidateBatchEntry( const String& entry) override;
	virtual void PushBatchEntryError( const String& entry, const String& errorMsg ) override;

private:
	CLocalizedStringsWithKeys m_locStrMan;

protected:
	virtual String	GetResourceExtension() const { return String::EMPTY; }
};
