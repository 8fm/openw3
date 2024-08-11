/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Clipboard data with entities
class CClipboardData : public wxDataObject
{
protected:
	wxDataFormat			m_format;		// Data format
	TDynArray< Uint8 >		m_data;			// Data buffer
	Bool					m_isCopy;

public:
	CClipboardData*			m_next;

public:
	// Get clipboard data buffer
	RED_INLINE TDynArray< Uint8 >& GetData() { return m_data; }

	// Get clipboard data format
	RED_INLINE wxDataFormat GetDataFormat() const { return m_format; }

	RED_INLINE Bool IsCopy() const { return m_isCopy; }

public:
	CClipboardData( const String& formatName );
	CClipboardData( const String& formatName, const TDynArray< Uint8 >& data, Bool isCopy = true );
	~CClipboardData();

	// wxDataObject
	virtual void GetAllFormats( wxDataFormat *formats, Direction dir ) const;
	virtual bool GetDataHere( const wxDataFormat &format, void* buf ) const;
	virtual size_t GetDataSize( const wxDataFormat &format) const;
	virtual size_t GetFormatCount( Direction dir ) const;
	virtual wxDataFormat GetPreferredFormat( Direction dir ) const;
	virtual bool SetData(const wxDataFormat &format, size_t len, const void* buf );
};

