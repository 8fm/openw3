/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "exporters/localizationExporter.h"
#include "journalTree.h"

struct SJournalTreeItemBEG;

class CJournalLocalizationExporter : public AbstractLocalizationExporter, public CJournalTree
{
public:
	CJournalLocalizationExporter();
	~CJournalLocalizationExporter();

	virtual void	BeginBatchExport();
	virtual void	EndBatchExport();
	virtual void	ExportBatchEntry( const String& entry );
	virtual String	GetResourceExtension() const;

	virtual Bool IsBatchEntryValid( const String& entry ) const override;
	virtual Bool ValidateBatchEntry( const String& entry) override;
	virtual void PushBatchEntryError( const String& entry, const String& errorMsg ) override;

	const THashMap< String, Int32 >& GetJournalStrings() { return m_journalStrings; }

private:
	virtual SJournalItemHandle* AddItemAppend( SJournalItemHandle* parentItem, CJournalBase* entry, CJournalResource* journalResource, CDirectory* sectionDirectory );
	virtual void MaximumNumberOfChildEntries( SJournalItemHandle* parentItem, Uint32 number );
	virtual void Sort( SJournalItemHandle* parentHandle );

private:
	virtual void FillRootExportGroup( BatchExportGroup& exportGroup );

	void FillJournalStrings( THashMap< String, Int32 >& journalStrings, BatchExportGroup& exportGroup, Uint32& order );

	THashMap< CGUID, CJournalBase* > m_journalEntries;
	THashMap< String, Int32 >        m_journalStrings;
};
