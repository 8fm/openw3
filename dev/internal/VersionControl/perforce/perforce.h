/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __VERCON_PERFORCE_H__
#define __VERCON_PERFORCE_H__

#include "interface/interfaceCommon.h"

#include "api.h"
#include "feedback/base.h"

namespace VersionControl
{
	/// Simple source control integration
	class Perforce : public InterfaceCommon
	{
	public:
		Perforce();
		virtual ~Perforce();

		// From Interface
	public:
		// Try to initialise source control system
		virtual bool Initialize( const char* program ) final;

		// Cleanup (Call once after initialise when you no longer need source control)
		virtual void Shutdown() final;
		
		virtual unsigned int GetCredentialsCount() const final;
		virtual const char** GetCredentials() const final;
		virtual bool SetCredential( const char* key, const char* value ) final;
		virtual const char* GetCredential( const char* key ) const final;

		virtual bool Add( const char** files, unsigned int numFiles ) final;
		virtual bool Checkout( const char** files, unsigned int numFiles ) final;
		virtual bool Revert( const char** files, unsigned int numFiles ) final;
		virtual bool Submit( const char** files, unsigned int numFiles, const char* description ) final;
		virtual bool Delete( const char** files, unsigned int numFiles ) final;
		virtual bool Sync( const char** files, unsigned int numFiles ) final;
		virtual bool GetDeletedFiles( Filelist* list, const char* directory ) final;

		virtual FileStatus GetStatus( const char* file ) final;

		virtual const char* GetLastError() final;

	private:
		bool GetFileProperty( const char* file, const char* propertyName, StrBuf& property );
		bool GetChangelistNumber( const char* file, StrBuf& changelist );
		bool GetDepotPath( const char* file, StrBuf& path );
		bool GetLocalPath( const char* file, StrBuf& path );

		bool GetLatestPendingChangelist( StrBuf& changelist );

		bool Run( const char* command, const char** parameters, unsigned int numParameters, Feedback::Base& feedback = Feedback::Base() );
		

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
		// 	

	private:
		ClientApi m_perforce;

		StrBuf m_errors;

		enum ECredentials
		{
			Credential_Username,
			Credential_Password,
			Credential_Workspace,
			Credential_Server,

			Credential_Max
		};

		static const char* CREDENTIALS[ Credential_Max ];
	};
}

#endif // __VERCON_PERFORCE_H__
