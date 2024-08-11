/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdFileDialog
{
private:
	OPENFILENAME				m_settings;
	CFileFormat					m_fileFormat;
	TDynArray< CFileFormat >	m_formats;
	TDynArray< String >			m_files;		//!< Full file paths
	String						m_iniTag;

	String						m_directory;	//!< Common directory for all files
	TDynArray< String >			m_filenames;	//!< Filenames, without directory

public:
	CEdFileDialog();
	~CEdFileDialog();

public:
	void ClearFormats();
	void AddFormat( const String &extension, const String &description );
	void AddFormat( const CFileFormat& format );
	void AddFormats( const TDynArray<CFileFormat>& formats );
	void SetMultiselection( Bool multiselection );
	void SetDirectory( const String &directory );
	void SetIniTag( const String& tag );

	const TDynArray< String >&	GetFiles() const;
	const String&				GetFile() const;
	const String&				GetFileDirectory() const;
	const TDynArray< String >&	GetFileNames() const;
	const String&				GetFileName() const;
	const CFileFormat&			GetFileFormat() const { return m_fileFormat; }

	Bool DoOpen( HWND owner, Bool getDirFromIni=false );
	Bool DoSave( HWND owner, const Char* defaultFileName=NULL, Bool getDirFromIni=false );

private:
	void UpdateFilters( Bool all );

	void ExtractDirectoryAndFilenames();
};
