#include "build.h"
#include "versionControlIconsPainter.h"


CVersionControlIconsPainter::CVersionControlIconsPainter( CEdCanvas& canvas )
: m_canvas( canvas )
{
	m_checkOutIcon = m_canvas.ConvertToGDI( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_CHECKED_OUT") ) );
	m_deleteIcon = m_canvas.ConvertToGDI( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_MARKED_DELETE") ) );
	m_localIcon = m_canvas.ConvertToGDI( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_LOCAL") ) );
	m_addIcon = m_canvas.ConvertToGDI( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_MARKED_ADD") ) );
	m_checkInIcon = m_canvas.ConvertToGDI( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_CHECK_IN") ) );
	m_notSyncedIcon = m_canvas.ConvertToGDI( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_NOT_SYNCED")));
}


void CVersionControlIconsPainter::PaintVersionControlIcon( const CDiskFile* file, const wxRect &rect )
{
	if ( file == NULL )
	{
		return;
	}

	if ( file->IsCheckedOut() )
	{
		m_canvas.DrawImage(m_checkOutIcon, rect.GetTopLeft());
	}
	else if ( file->IsAdded() )
	{
		m_canvas.DrawImage(m_addIcon, rect.GetLeftTop());
	}
	else if ( file->IsLocal() )
	{
		m_canvas.DrawImage(m_localIcon, rect.GetTopLeft());
	}
	else if ( file->IsDeleted() )
	{
		m_canvas.DrawImage(m_deleteIcon, rect.GetTopLeft());
	}
	else if ( file->IsNotSynced() )
	{
		m_canvas.DrawImage(m_notSyncedIcon, rect.GetTopLeft());
	}
	else // !file->IsCheckedOut()
	{
		m_canvas.DrawImage(m_checkInIcon, rect.GetTopLeft());
	}
}
