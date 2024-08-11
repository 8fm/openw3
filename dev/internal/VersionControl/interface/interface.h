/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __VERCON_INTERFACE_H__
#define __VERCON_INTERFACE_H__

#include "export.h"
#include "statusFlags.h"
#include "fileList.h"

namespace VersionControl
{
	/// Simple source control integration
	class Interface
	{
	public:
		Interface() {}
		virtual ~Interface() {}

		// Try to initialise source control system
		// The program parameter is used to identify the program connecting to version control 
		virtual bool Initialize( const char* program ) { return false; }

		virtual void Shutdown() {}

		virtual unsigned int GetCredentialsCount() const { return 0; }
		virtual const char** GetCredentials() const { return nullptr; }
		virtual bool SetCredential( const char* key, const char* value ) { return false; }
		virtual const char* GetCredential( const char* key ) const { return nullptr; }

		virtual bool Add( Filelist* list ) { return false; }
		virtual bool Add( const char* file ) { return false; }
		virtual bool Add( const char** files, unsigned int numFiles ) { return false; }

		virtual bool Checkout( Filelist* list ) { return false; }
		virtual bool Checkout( const char* file ) { return false; }
		virtual bool Checkout( const char** files, unsigned int numFiles ) { return false; }

		virtual bool Revert( Filelist* list ) { return false; }
		virtual bool Revert( const char* file ) { return false; }
		virtual bool Revert( const char** files, unsigned int numFiles ) { return false; }

		virtual bool Submit( Filelist* list, const char* description ) { return false; }
		virtual bool Submit( const char* file, const char* description ) { return false; }
		virtual bool Submit( const char** files, unsigned int numFiles, const char* description ) { return false; }

		virtual bool Delete( Filelist* list ) { return false; }
		virtual bool Delete( const char* file ) { return false; }
		virtual bool Delete( const char** files, unsigned int numFiles ) { return false; }

		virtual bool Sync( Filelist* list ) { return false; }
		virtual bool Sync( const char* file ) { return false; }
		virtual bool Sync( const char** files, unsigned int numFiles ) { return false; }

		virtual bool GetDeletedFiles( Filelist* list, const char* directory ) { return false; }

		virtual FileStatus GetStatus( const char* file ) { return FileStatus(); }

		virtual const char* GetLastError() { return nullptr; }

		virtual Filelist* CreateFileList() { return nullptr; }
		virtual void DestroyFileList( Filelist* ptr ) {}

		// Get status of file list
	// 	void GetFileStatus( const vector< SolutionFile* >& files );
	// 
	// 	// Displays the revision history for the specified set of packages.
	// 	void ShowHistory( const wxString& fileName );
	// 
	// 	// Checks a file out of the SCC depot.
	// 	void CheckOut( const wxString& fileName );
	// 
	// 	// Reverts a file checked out of the SCC depot.
	// 	void UncheckOut( const wxString& fileName );
	// 
	// 	// Checks file in into the depot
	// 	void CheckIn( const wxString& fileName );
	// 
	// 	// Adds a new file to the depot, file must exist
	// 	void Add( const wxString& fileName );
	// 
	// 	// Removes a file from the depot.
	// 	void Remove( const wxString& fileName );
	// 
	// 	// Performs diff
	// 	void Diff( const wxString& fileName );
	};
}

typedef VersionControl::Interface* (*CreateInterfaceFunc)();
typedef void (*DestroyInterfaceFunc)( VersionControl::Interface* );

RED_VC_DLL_C VersionControl::Interface* CreateInterface();
RED_VC_DLL_C void DestroyInterface( VersionControl::Interface* );

#endif // __VERCON_INTERFACE_H__
