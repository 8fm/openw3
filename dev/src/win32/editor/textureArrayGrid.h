/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "iconGrid.h"

class CEdTextureArrayGrid;

class CEdTextureArrayGridHook : public IEdIconGridHook
{
public:
	virtual void OnTextureArrayGridModified( class CEdTextureArrayGrid* grid ){}
};

class CEdTextureArrayGridEntryInfo : public CEdIconGridEntryInfo
{
	CWXThumbnailImage*				m_thumbnail;
	String							m_caption;

public:
	CEdTextureArrayGridEntryInfo( CWXThumbnailImage* thumbnail, const String& caption );

	virtual String GetCaption() const;
	virtual CWXThumbnailImage* GetThumbnail();
};

class CEdTextureArrayGrid : public CEdIconGrid, public CDropTarget
{
	CTextureArray*					m_textureArray;
	bool							m_allowEdit;
	bool							m_autoSave;

	void AttemptAutoSave();

public:
	CEdTextureArrayGrid( wxWindow* parent, Int32 style );

	void SetTextureArray( CTextureArray* textureArray );
	RED_INLINE CTextureArray* GetTextureArray() const { return m_textureArray; }
	void UpdateEntriesFromTextureArray();

	void SetAllowEdit( bool allowEdit = true, bool autoSave = false );
	RED_INLINE bool GetAllowEdit() const { return m_allowEdit; }
	RED_INLINE bool GetAutoSave() const { return m_autoSave; }

	virtual Bool OnDropResources( wxCoord x, wxCoord y, TDynArray<CResource*>& resources );
	virtual wxDragResult OnDragOver( wxCoord x, wxCoord y, wxDragResult def );
};
