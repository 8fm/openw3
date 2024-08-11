/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fileGrid.h"
#include "../../common/core/diskfile.h"

CEdFileGridEntryInfo::CEdFileGridEntryInfo( CDiskFile* templateFile )
	: m_file( templateFile )
{
	m_caption = templateFile->GetFileName().AsChar();
	if ( m_caption.EndsWith( TXT(".w2ent") ) )
	{
		m_caption = m_caption.LeftString( m_caption.GetLength() - 6 );
	}
}

CWXThumbnailImage* CEdFileGridEntryInfo::GetThumbnail()
{
	TDynArray<CThumbnail*> thumbs = m_file->GetThumbnails();
	if ( thumbs.Size() < 1 )
	{
		m_file->LoadThumbnail();
		thumbs = m_file->GetThumbnails();
	}
	if ( thumbs.Size() < 1 )
	{
		m_file->UpdateThumbnail();
		thumbs = m_file->GetThumbnails();

		// Save to keep the thumbnail
		if ( !m_file->IsLocal() && !m_file->IsCheckedOut() )
		{
			if ( m_file->SilentCheckOut() )
			{
				m_file->Save();
			}
		}
		else
		{
			m_file->Save();
		}
	}
	if ( thumbs.Size() )
	{
		return (CWXThumbnailImage*)(thumbs[0]->GetImage());
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////

CEdFileGrid::CEdFileGrid()
	: CEdIconGrid()
{
}

CEdFileGrid::CEdFileGrid( wxWindow* parent, Int32 style )
	: CEdIconGrid( parent, style )
{
}

CEdFileGridEntryInfo* CEdFileGrid::AddFile( CDiskFile* file )
{
	TDynArray<CDiskFile*> singleFile( 1 );
	singleFile[0] = file;
	return AddFiles( singleFile )[0];
}

TDynArray<CEdFileGridEntryInfo*> CEdFileGrid::AddFiles( const TDynArray<CDiskFile*>& files )
{
	TDynArray<CEdIconGridEntryInfo*> infos( files.Size() );
	TDynArray<CEdFileGridEntryInfo*> infosForEntities( files.Size() );
	for ( Uint32 i=0; i<infos.Size(); ++i )
	{
		infos[i] = infosForEntities[i] = new CEdFileGridEntryInfo( files[i] );
	}
	AddEntries( infos );
	return infosForEntities;
}

void CEdFileGrid::SetSelectedFile( CDiskFile* file )
{
	if ( file )
	{
		Int32 index = FindFile( file );
		if ( index != -1 )
		{
			SetSelected( index );
		}
	}
	else
	{
		SetSelected( -1 );
	}
}

CDiskFile* CEdFileGrid::GetSelectedFile() const
{
	Int32 index = GetSelected();
	if ( index != -1 )
	{
		CEdFileGridEntryInfo* info = dynamic_cast<CEdFileGridEntryInfo*>( GetEntryInfo( index ) );
		if ( info )
		{
			return info->GetFile();
		}
	}
	return NULL;
}

Int32 CEdFileGrid::FindFile( CDiskFile* file ) const
{
	for ( Uint32 i=0; i<m_entries.Size(); ++i )
	{
		CEdFileGridEntryInfo* info = dynamic_cast<CEdFileGridEntryInfo*>( GetEntryInfo( i ) );
		if ( info && info->GetFile() == file )
		{
			return i;
		}
	}
	return NULL;
}
