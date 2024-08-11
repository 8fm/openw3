/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SCRIPT_STUDIO_EXPORTER_H__
#define __SCRIPT_STUDIO_EXPORTER_H__

class CSSExporter : public wxDirTraverser
{
public:
	CSSExporter( const wxString& source, const wxString& dest );
	virtual ~CSSExporter();

	bool IsDestinationEmpty() const;
	void Export();

protected:
	static wxString ConvertPath( const wxString& file, const wxString& source, const wxString& dest );
	static wxString ConvertPath( wxFileName& file, const wxString& source, const wxString& dest );

protected:
	virtual wxDirTraverseResult OnDir( const wxString& dir ) override;
	virtual wxDirTraverseResult OnFile( const wxString& file ) override;

protected:
	wxString m_source;
	wxString m_dest;
};

//////////////////////////////////////////////////////////////////////////
//
class CSSDiffExporter : public CSSExporter
{
public:
	CSSDiffExporter( const wxString& source, const wxString& comp, const wxString& dest );
	virtual ~CSSDiffExporter();

private:
	virtual wxDirTraverseResult OnDir( const wxString& dir ) override final;
	virtual wxDirTraverseResult OnFile( const wxString& file ) override final;

	bool ReadFile( const wchar_t* path, void** buffer, int& bufferSize, int& fileSize );

private:
	wxString m_comp;

	void* m_fileA;
	void* m_fileB;

	int m_bufferSizeA;
	int m_bufferSizeB;
};

#endif //__SCRIPT_STUDIO_EXPORTER_H__
