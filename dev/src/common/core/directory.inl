/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __DIRECTORY_INL__
#define __DIRECTORY_INL__

RED_INLINE void CDirectory::NeedChildren() const
{
	if ( !m_populated )
	{
		const_cast< CDirectory* >( this )->Repopulate( false );
	}
}

//! Get file objects
RED_INLINE const TFiles& CDirectory::GetFiles() const
{
	const_cast<CDirectory*>(this)->NeedChildren();
	return m_files;
}

//! Get file objects
RED_INLINE const TDirs& CDirectory::GetDirectories() const
{
	const_cast<CDirectory*>(this)->NeedChildren();
	return m_directories;
}

//! Inform whether contains any checked out files
RED_INLINE Bool CDirectory::IsCheckedOut() const
{
	const_cast<CDirectory*>(this)->NeedChildren();
	return m_checkedOut > 0;
}

//! Get parent directory
RED_INLINE CDirectory* CDirectory::GetParent() const
{
	return m_parent;
}

//! Is this directory populated ?
RED_INLINE const Bool CDirectory::IsPopulated() const
{
	return m_populated;
}

//! Get absolute directory path
RED_INLINE String CDirectory::GetAbsolutePath() const
{
	String str;
	GetAbsolutePath( str );
	return str;
}

//! Get depot directory path
RED_INLINE String CDirectory::GetDepotPath() const
{
	String str;
	GetDepotPath( str );
	return str;
};

RED_INLINE const String& CDirectory::GetName() const
{
	return m_name;
}

#endif // __DIRECTORY_INL__
