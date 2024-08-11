/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "textureArrayGrid.h"
#include "../../common/core/thumbnail.h"
#include "../../common/core/diskFile.h"
#include "../../common/engine/bitmapTexture.h"
#include "../../common/engine/textureArray.h"

CEdTextureArrayGridEntryInfo::CEdTextureArrayGridEntryInfo(  CWXThumbnailImage* thumbnail, const String& caption )
	: m_thumbnail( thumbnail )
	, m_caption( caption )
{
}

String CEdTextureArrayGridEntryInfo::GetCaption() const
{
	return m_caption;
}

CWXThumbnailImage* CEdTextureArrayGridEntryInfo::GetThumbnail()
{
	return m_thumbnail;
}

CEdTextureArrayGrid::CEdTextureArrayGrid( wxWindow* parent, Int32 style )
	: CEdIconGrid( parent, style )
	, CDropTarget( this )
	, m_textureArray( NULL )
	, m_allowEdit( false )
	, m_autoSave( false )
{
}

void CEdTextureArrayGrid::UpdateEntriesFromTextureArray()
{
	Clear();
	if ( !m_textureArray )
	{
		return;
	}

	TDynArray<CBitmapTexture*> bitmaps;
	m_textureArray->GetTextures( bitmaps );

	TDynArray<CEdIconGridEntryInfo*> infos;

	for ( Uint32 i=0; i<bitmaps.Size(); ++i )
	{
		CDiskFile* file = bitmaps[i]->GetFile();
		if ( file )
		{
			TDynArray<CThumbnail*> thumbnails = file->GetThumbnails();
			if ( thumbnails.Empty() )
			{
				file->UpdateThumbnail();
				thumbnails = file->GetThumbnails();
			}

			if ( !thumbnails.Empty() )
			{
				CWXThumbnailImage* thumbnail = (CWXThumbnailImage*)thumbnails[0]->GetImage();
				String caption = file->GetFileName();
				if ( caption.EndsWith( TXT(".xbm") ) ) caption = caption.LeftString( caption.GetLength() - 4 );
				infos.PushBack( new CEdTextureArrayGridEntryInfo( thumbnail, caption ) );
			}
		}
	}

	AddEntries( infos );

	Refresh( false );
}

void CEdTextureArrayGrid::AttemptAutoSave()
{
	if ( m_autoSave )
	{
		m_textureArray->SetDirty( true );
		m_textureArray->Save();
	}
}

void CEdTextureArrayGrid::SetTextureArray( CTextureArray* textureArray )
{
	if ( textureArray != m_textureArray )
	{
		m_textureArray = textureArray;
		SetThumbnailSize( 94 );
		SetThumbnailZoom( 1 );
		UpdateEntriesFromTextureArray();
	}
}

void CEdTextureArrayGrid::SetAllowEdit( bool allowEdit /* = true */, bool autoSave /* = false */ )
{
	m_allowEdit = allowEdit;
	m_autoSave = autoSave;
}

Bool CEdTextureArrayGrid::OnDropResources( wxCoord x, wxCoord y, TDynArray<CResource*>& resources )
{
	// Fail if we dont allow editing
	if ( !m_allowEdit )
	{
		return false;
	}

	// Check if all resources are proper textures
	for ( Uint32 i=0; i<resources.Size(); ++i )
	{
		if ( !resources[i]->IsA<CBitmapTexture>() )
		{
			wxMessageBox( wxT("You can only drop textures in the texture array grid. No other resouces are supported."), wxT("Only textures can be dropped here"), wxOK|wxCENTRE|wxICON_ERROR, this );
			return false;
		}

		CBitmapTexture* tex = static_cast<CBitmapTexture*>( resources[i] );
		wxString texName = tex->GetDepotPath().AsChar();
		
		// Check if the texture already exists
		if ( m_textureArray->Contains( tex ) )
		{
			if ( wxMessageBox( wxT("Texture '") + texName + wxT("' is already present in this array. Are you sure you want to add another instance?"), wxT("Duplicate texture"), wxCENTRE|wxYES_NO|wxICON_QUESTION ) != wxYES )
			{
				return false;
			}
		}

		String error;
		if ( !CTextureArray::IsTextureValidForArray( tex, m_textureArray, error ) )
		{
			wxMessageBox( error.AsChar(), wxT("Error"), wxICON_ERROR|wxCENTRE|wxOK );
			return false;
		}
	}

	// Check if we can actually modify the texture array
	if ( !m_textureArray->MarkModified() )
	{
		return false;
	}

	// Check if the user wants to replace a texture
	Int32 entryIndex = GetEntryAt( x, y );
	CBitmapTexture* replaceTexture = NULL;
	if ( resources.Size() == 1 && entryIndex != -1 )
	{
		TDynArray<CBitmapTexture*> textures;
		m_textureArray->GetTextures( textures );
		replaceTexture = textures[entryIndex];
		if ( wxMessageBox( wxT("Do you want to replace texture '") + wxString( replaceTexture->GetDepotPath().AsChar() ) + wxT("' with '") + wxString( static_cast<CBitmapTexture*>( resources[0] )->GetDepotPath().AsChar() ) + wxT("'?"), wxT("Replace texture"), wxICON_QUESTION|wxCENTRE|wxYES_NO ) != wxYES )
		{
			replaceTexture = NULL;
		}
	}

	// Add/replace the dropped texture resources to the texture array
	if ( replaceTexture )
	{
		m_textureArray->RemoveTextureAt( entryIndex );
		m_textureArray->InsertTexture( static_cast<CBitmapTexture*>( resources[0] ), entryIndex );
	}
	else
	{
		for ( Uint32 i=0; i<resources.Size(); ++i )
		{
			m_textureArray->AddTexture( static_cast<CBitmapTexture*>( resources[i] ) );
		}
	}

	// Dirtify, save, etc
	AttemptAutoSave();

	// Update
	UpdateEntriesFromTextureArray();
	m_textureArray->CreateRenderResource();

	// Notify hook
	CEdTextureArrayGridHook* hook = dynamic_cast<CEdTextureArrayGridHook*>( GetHook() );
	if ( hook )
	{
		hook->OnTextureArrayGridModified( this );
	}

	return true;
}

wxDragResult CEdTextureArrayGrid::OnDragOver( wxCoord x, wxCoord y, wxDragResult def )
{
	return m_allowEdit ? wxDragCopy : wxDragNone;
}
