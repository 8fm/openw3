/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/core/versionControl.h"

class ISourceControl;
class ClientApi;
class CWatcher;
class IVersionControlEditorInterface;

struct StatFields
{
	String action;
	String headAction;
	Int32 headRev;
	Int32 haveRev;
	Int32 headChange;
	TDynArray< String > otherOpen;
};

struct ChangelistDescription
{
	struct FieldDescribtion
	{
		String	m_file;
		String	m_action;
		String	m_type;
		String	m_revision;
	};

	String	m_user;
	String	m_client;
	String	m_time;
	String	m_desc;

	TDynArray< FieldDescribtion > m_fields;
};

struct ChangeDescription
{
	String	m_change;
	String	m_time;
	String	m_user;
	String	m_client;
	String	m_status;
	String	m_desc;
};

/// Source control version based on P4
class CSourceControlP4 : public ISourceControl
{
private:
	class CVersionControlThread : public Red::Threads::CThread
	{
		CSourceControlP4				*m_control;

	public :
		Red::Threads::CAtomic<Bool>		m_active;

		//! Try to initialize on thread
		CVersionControlThread(CSourceControlP4 *control)
			: Red::Threads::CThread( "Version Control Thread" )
			, m_control( control )
		{}

		void ThreadFunc()
		{
			// Try to initialize
			while ( m_active.GetValue() && !m_control->Init( true ) )
			{
				Sleep( 1000 );
			}
			
			// We are good to go !
			if ( m_active.GetValue() )
			{
				m_control->SetOperational( true );
			}
		}
	};

private:
	// bind path
	String m_depotPath;

	// P4 client API
	ClientApi* m_client;

	// connection thread
	CVersionControlThread* m_thread;	

	// flags
	Bool m_tagged;
	Bool m_open;

	// Has the user been warned that the connection failed?
	Bool m_warned;

	// true if version control system is operational
	volatile Bool m_operational;
	
	// protection for m_operational
	Red::Threads::CMutex m_mutex;

	// editor interface
	IVersionControlEditorInterface *m_interface;

	Bool automaticChangelists;

protected:
	// Initiates connection with the server
	Bool Init( Bool tagged );

	// Closes connection with the server
	void Close();

	// Sets tagged connection protocol with the server
	void SetTagged();

	Bool Run( const String &command, CWatcher &watcher, Bool tagged );
	Bool Run( const TDynArray< String > &arguments, const String &command, CWatcher &watcher, Bool tagged);

	Bool ToDepot( const String &path, String &location );
	Bool ToLocal( const String &path, String &location );

	Bool TestConnection();

	Int32 FStat(const String &file, struct StatFields &stats);

	Bool IsOperational();
	void SetOperational( Bool value );

	// performs actual check out and does nothing beside that
	Int32 DoCheckOut( const String &path, const SChangelist& changelist, Bool exclusive );
	
public:
	CSourceControlP4( IVersionControlEditorInterface *iface, const SSourceControlSettings& settings );
	~CSourceControlP4();

	virtual Bool IsSourceControlDisabled() const override;

	// Sets information about user, client, host, etc
	virtual void SetSettings( const SSourceControlSettings& settings );

	virtual Bool AutomaticChangelistsEnabled() const;

	Bool CreateChangelist( const String& name, SChangelist& changelist );
	// Warning: the changelist with the number must already exist, otherwise use CreateChangelist above
	Bool CreateChangelistWithNumber( const Uint32 number, SChangelist& changelist );
	Bool IsDefaultChangelist( const SChangelist& changelist ) const;

	// method description in base class header
	Bool GetStatus(CDiskFile &resource, TDynArray<String> *users = 0);
	EOpenStatus GetOpenStatus(const String &file);

	// check out file or file list
	Bool EnsureCheckedOut( const TDynArray< CDiskFile* >& fileList, Bool exclusive = true ) override;		// NOTICE: check out will return false only if some files cannot be checked out (it will return true even if some file is local). There is inconsistency between CheckOut on file, but its for function usability reasons (we don't want to iterate files separately and filter out already checked out).
	Bool CheckOut( CDiskFile &resource, Bool exclusive = true ) override;									// NOTICE: check out file will return false (and notify user) if file wasn't checked out. This behavior is inconsistant with above function.
	Bool CheckOut( const String &absoluteFilePath, const SChangelist &changeList = SChangelist::DEFAULT, Bool exclusive = true ) override;
	Bool SilentCheckOut( CDiskFile &resource, Bool exclusive = true ) override;
	
	Bool Submit( CDiskFile &resource );
	Bool Submit( CDiskFile &resource, const String& description );
	Bool Submit( CDirectory &directory );
	Bool Submit( CDirectory &directory, const String& description );
	Bool Submit( TDynArray< CDiskFile *> &files);
	Bool Submit( TDynArray< CDiskFile *> &files, const String& description );
	Bool Submit( TDynArray< CDiskFile * > &files, TSet< CDiskFile * > &chosen);
	Bool Submit( TDynArray< CDiskFile * > &files, TSet< CDiskFile * > &chosen, const String& description );
	Bool Submit( const SChangelist& changelist );
	
	Bool Revert( CDiskFile &resource, Bool silent=false );
	Bool Revert( TDynArray< CDiskFile * > &files );
	Bool RevertAbsolutePath( const String& absFileName );
	Bool Revert( const SChangelist& changelist, Bool unchangedOnly = false );
	
	Bool Delete( CDiskFile &resource, Bool confirm = true);
	Bool Delete( TDynArray< CDiskFile * > &files, Bool confirm = true );

	Bool Rename( CDiskFile &resource, const String &newAbsolutePath );

	Bool Save( CDiskFile &resource );
	Bool Edit( CDiskFile &resource );
	
	Bool GetLatest( CDiskFile &resource );
	Bool GetLatest( CDirectory &resource, Bool force = false ) override;
	// This one is so that we can check perforce in general on a path to a file and not only CFile or CDirectory
	Bool GetLatest( const String &absoluteFilePath, Bool force = false ) override;

	Bool SoftSync( CDiskFile &resource );

	Bool Opened( TDynArray< CDiskFile * > &files );
	Bool GetListOfFiles( const String& folderPath, TDynArray < String > &files );

	Bool Add( CDiskFile & resource, const SChangelist& changelist = SChangelist::DEFAULT );

	Bool Lock( CDiskFile & resource);

	Bool FileLog( CDiskFile &file, TDynArray< THashMap< String, String > > &history );

	Bool FileLastEditedBy( CDiskFile &file, String& p4user );
	Bool FileLastEditedBy( const String& absoluteFilePath, String& p4user );

	Bool ChangelistLog( Uint32 number, ChangelistDescription& out );

	Bool GetAttribute( const String &absFileName, const String &name, String &value );

	Bool GetChangelists( String path, Uint32 fromChangelist, Uint32 toChangelist, TDynArray< ChangeDescription > &out );	

	Bool GetLastChangelist( Uint32& number );

	Bool GetLastLocalChangelistNumber( Uint32& number );
	// This one is so that we can check perforce in general on a path to a file and not only CFile or CDirectory
	Bool DoesFileExist( const String &absoluteFilePath );
	Bool DoesFolderExist( const String &absolutePath );

	String GetUser() const;
	String GetWorkspace() const;

	// makes sure that path has correct upper/lower case data
	static String GetProperPath( const String& caseAgnosticPath );
};
