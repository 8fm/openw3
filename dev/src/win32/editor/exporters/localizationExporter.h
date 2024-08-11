/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

struct BatchExportGroup
{
	String							m_groupName;
	TDynArray< String >				m_groupEntries;
	TDynArray< BatchExportGroup >	m_subGroups;
	Uint32							m_order;

	BatchExportGroup()
		: m_order( static_cast< Int32 >( -1 ) )
	{
	}

	void Clear()
	{
		m_groupName.Clear();
		m_groupEntries.Clear();
		/*for ( Uint32 i = 0; i < m_subGroups.Size(); ++i )
		{
			m_subGroups[ i ].Clear();
		}*/
		m_subGroups.Clear();
		m_order = static_cast< Int32 >( -1 );
	}

	~BatchExportGroup()
	{
		Clear();
	}

	static bool Compare( const BatchExportGroup& p1, const BatchExportGroup& p2 )
	{
		return Red::System::StringCompareNoCase( p1.m_groupName.AsChar(), p2.m_groupName.AsChar() ) < 0;
	}
};

class IBatchExporter
{
public:
	virtual ~IBatchExporter(){}
	virtual void BeginBatchExport() = 0;
	virtual void EndBatchExport() = 0;
	virtual void ExportBatchEntry ( const String& entry ) = 0;

	/*
	Checks whether batch entry is valid.

	\return True - batch entry is valid, false - otherwise.
	*/
	virtual Bool IsBatchEntryValid( const String& entry ) const = 0;

	/*
	Makes batch entry valid.

	\return True - batch entry validated successfully, false - couldn't validate batch entry.
	*/
	virtual Bool ValidateBatchEntry( const String& entry ) = 0;

	/*
	Pushes error message associated with batch entry.

	It's ok to push many error messages for the same entry.
	*/
	virtual void PushBatchEntryError( const String& entry, const String& errorMsg ) = 0;

	virtual BatchExportGroup*	GetRootExportGroup() { return NULL; }
};

class AbstractLocalizationExporter : public IBatchExporter
{
protected:
	String	m_sourceLanguage;
	TDynArray< String >	m_targetLanguages;
	Uint32	m_numberOfTargetLanguages;

	Uint32	m_minResourcePathDepth;
	Uint32	m_maxResourcePathDepth;
	
	TDynArray< TDynArray< String > > m_exportData;
	TDynArray< TDynArray< String > > m_errorData;

	String	m_savePath;
	String  m_errorsPath;

	BatchExportGroup	m_rootExportGroup;

public:
	void SetLanguages( const String& sourceLanguage, const String& targetLanguage = String::EMPTY );
	void SetNumberOfTargetLanguages( Uint32 numberOfTargetLanguages );
	void SetTargetLanguages( const TDynArray< String >& targetLanguages );
	void SetSavePath( const String& savePath ) { m_savePath = savePath; }
	void SetErrorsPath( const String& errorsPath ) { m_errorsPath = errorsPath; }
	void SetMinResourcePathDepth( Uint32 depth ) { m_minResourcePathDepth = depth; }
	void SetMaxResourcePathDepth( Uint32 depth ) { m_maxResourcePathDepth = depth; }
	
	virtual BatchExportGroup*	GetRootExportGroup() { return &m_rootExportGroup; }

	virtual Bool ExportToDirectory() { return false; }


	virtual void ShowSetupDialog( wxWindow* parent ) { RED_UNUSED( parent ); }

protected:
	String GetSourceString( const LocalizedString& content );
	String GetTargetString( const LocalizedString& content, Uint32 targetLanguage = 0 );
	virtual String GetResourceName( CResource* resource );

	String ConvertToCSV();
	String ConvertErrorsToCSV();

	Bool DumpErrorsFile();
	void PrepareErrorFileHeaders();

	void ApplyBrackets( String& cellData );

protected:
	void AddDirectoryToBatchGroup( CDirectory* directory, BatchExportGroup& batchGroup );
	
	virtual void FillRootExportGroup( BatchExportGroup& exportGroup );
	
	virtual Bool	CanExportFile( CDiskFile* file ) const { return false; }
	virtual String	GetResourceExtension() const = 0;
	

};