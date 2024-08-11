/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __VERCON_FLAGS_H__
#define __VERCON_FLAGS_H__

namespace VersionControl
{
	/// Status of a file
	enum StatusFlag
	{
		VCSF_InDepot = 0,			// Managed by source control
		VCSF_Added,					// Freshly added to the depot
		VCSF_CheckedOut,			// Checked out by the logged in user
		VCSF_CheckedOutByAnother,	// Checked out by another user elsewhere
		VCSF_Deleted,				// Marked for removal/deletion from the depot
		VCSF_OutOfDate,				// Not sync'd with the latest revision

		VCSF_Max
	};

	class FileStatus
	{
	public:
		FileStatus() : m_flags( 0 ) {}
		~FileStatus() {}

		inline void SetFlag( StatusFlag flag ) { m_flags |= ( 1 << flag ); }
		inline void UnSetFlag( StatusFlag flag ) { m_flags &= ~( 1 << flag ); }
		inline bool HasFlag( StatusFlag flag ) const { return ( m_flags & ( 1 << flag ) ) != 0; }
		inline unsigned int operator&( const FileStatus& other ) const { return ( m_flags & other.m_flags ); }
		inline void operator=( unsigned int other ) { m_flags = other; }

	private:
		unsigned int m_flags;
	};
}

#endif //__VERCON_FLAGS_H__
