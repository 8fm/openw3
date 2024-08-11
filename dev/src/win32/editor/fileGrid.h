/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "iconGrid.h"

class CEdFileGridEntryInfo : public CEdIconGridEntryInfo
{
	String							m_caption;
	CDiskFile*						m_file;

public:
	CEdFileGridEntryInfo( CDiskFile* file );

	RED_INLINE CDiskFile* GetFile() const { return m_file; }

	virtual String GetCaption() const { return m_caption; };
	virtual CWXThumbnailImage* GetThumbnail();
};

class CEdFileGrid : public CEdIconGrid
{
public:
	CEdFileGrid();
	CEdFileGrid( wxWindow* parent, Int32 style );

	CEdFileGridEntryInfo* AddFile( CDiskFile* file );
	TDynArray<CEdFileGridEntryInfo*> AddFiles( const TDynArray<CDiskFile*>& files );
	void RemoveFile( CDiskFile* file );

	void SetSelectedFile( CDiskFile* file );
	CDiskFile* GetSelectedFile() const;

	Int32 FindFile( CDiskFile* file ) const;
};
