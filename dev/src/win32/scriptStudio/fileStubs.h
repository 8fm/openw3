/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/redThreads/redThreadsThread.h"

#include "../../common/scripts/Lexer/definition.h"
#include "../../common/scripts/Memory/allocatorProxy.h"

#include "lexer\context.h"
#include "solution\slnDeclarations.h"

class SolutionFile;
struct SSLexicalData;

//////////////////////////////////////////////////////////////////////////
// Stub types
//////////////////////////////////////////////////////////////////////////
enum EStubType
{
	STUB_File,
	STUB_Property,
	STUB_Function,
	STUB_Class,
	STUB_Struct,
	STUB_State,
	STUB_Enum,
	STUB_EnumOption,
};

//////////////////////////////////////////////////////////////////////////
// Flag list
//////////////////////////////////////////////////////////////////////////
struct SSFlagList
{
	vector< wstring >		m_flags;

	SSFlagList();
	SSFlagList( const wstring& flag );

	void Clear();
	bool Empty() const;
	bool Has( const wstring& str ) const;
	void Add( const wstring& flag );

	SSFlagList& operator=( const wchar_t* flag );
	SSFlagList& operator+( const wstring& flag );
	SSFlagList& operator+( const SSFlagList& flags );
};

//////////////////////////////////////////////////////////////////////////
// Basic Stub
//////////////////////////////////////////////////////////////////////////
struct SSBasicStub
{
	EStubType				m_type;
	SSFileContext			m_context;
	SSFlagList				m_flags;
	wstring					m_name;

	SSBasicStub( EStubType type, const SSFileContext& context, const wstring& name, const SSFlagList& flags );
	~SSBasicStub();
};

//////////////////////////////////////////////////////////////////////////
// Property Stub
//////////////////////////////////////////////////////////////////////////
struct SSPropertyStub : public SSBasicStub
{
	wstring					m_typeName;

	SSPropertyStub( const SSFileContext& context, const wstring& name, const SSFlagList& flags, const wstring& typeName );
	~SSPropertyStub();
};

//////////////////////////////////////////////////////////////////////////
// Function Stub
//////////////////////////////////////////////////////////////////////////
struct SSFunctionStub : public SSBasicStub
{
	wstring							m_className;
	wstring							m_retValueType;
	vector< SSPropertyStub* >		m_params;
	vector< SSPropertyStub* >		m_locals;
	int								m_lastLine;

	SSFunctionStub( const SSFileContext& context, const wstring& className, const wstring& name, const SSFlagList& flags );
	~SSFunctionStub();

	void AddParam( SSPropertyStub* param );
	void AddLocal( SSPropertyStub* local );
	bool IsLineInRange( int line ) const;

	SSPropertyStub* FindField( const wstring& name, bool searchInParent );
};

//////////////////////////////////////////////////////////////////////////
// Class/State/Structure Stub
//////////////////////////////////////////////////////////////////////////
struct SSClassStub : public SSBasicStub
{	
	vector< SSPropertyStub* >			m_fields;
	vector< SSFunctionStub* >			m_functions;
	wstring								m_extends;
	wstring								m_stateMachine;
	int									m_lastLine;

	SSClassStub( const SSFileContext& context, const wstring& name, const SSFlagList& flags ); // Structure
	SSClassStub( const SSFileContext& context, const wstring& name, const SSFlagList& flags, const wstring& extends ); // Class 
	SSClassStub( const SSFileContext& context, const wstring& name, const SSFlagList& flags, const wstring& extends, const wstring& stateMachine ); // State
	~SSClassStub();

	void AddField( SSPropertyStub* field );
	void AddFunction( SSFunctionStub* func );
	bool IsLineInRange( int line ) const;

	SSClassStub* FindParentClass();
	SSClassStub* FindStateMachineClass();

	SSFunctionStub* FindFunction( const wstring& name, bool searchInParent );
	SSPropertyStub* FindField( const wstring& name, bool searchInParent );
	SSClassStub* FindState( const wstring& name, bool searchInParent );
};

//////////////////////////////////////////////////////////////////////////
// Enum option Stub
//////////////////////////////////////////////////////////////////////////
struct SSEnumOptionStub : public SSBasicStub
{
	int							m_value;

	SSEnumOptionStub( const SSFileContext& contex, const wstring& name, int value );
	~SSEnumOptionStub();
};

//////////////////////////////////////////////////////////////////////////
// Enum Stub
//////////////////////////////////////////////////////////////////////////
struct SSEnumStub : public SSBasicStub
{
	vector< SSEnumOptionStub* >		m_options;

	SSEnumStub( const SSFileContext& contex, const wstring& name );
	~SSEnumStub();

	void AddOption( SSEnumOptionStub* option );
	SSEnumOptionStub* FindOption( const wstring& name );
};

//////////////////////////////////////////////////////////////////////////
// Script File Stub
//////////////////////////////////////////////////////////////////////////
struct SSFileStub : public SSBasicStub
{
	SSLexicalData*				m_lexData;

	vector< SSClassStub* >		m_classes;		//!< Top level classes
	vector< SSEnumStub* >		m_enums;		//!< Top level enums
	vector< SSFunctionStub* >	m_functions;	//!< Top level functions

	map< int, SSBasicStub* >	m_lineToStubMap;

	SSFileStub( const wstring& solutionPath );
	~SSFileStub();

	void AddClass( SSClassStub* classStub );
	void AddEnum( SSEnumStub* enumStub );
	void AddFunction( SSFunctionStub* functionStub );
};

//////////////////////////////////////////////////////////////////////////
// Stub parsing thread
//////////////////////////////////////////////////////////////////////////
class SSStubParsingThread : public Red::Threads::CThread
{
protected:
	deque< SolutionFileWeakPtr >		m_filesToProcess;
	bool								m_requestExit;
	bool								m_paused;
	bool								m_shutdown;

	Red::Scripts::LexerDefinition		m_definition;
	Red::AllocatorProxy					m_allocProxy;

	Red::Threads::CMutex				m_idleLock;
	Red::Threads::CConditionVariable	m_idleCondition;

public:
	SSStubParsingThread();
	virtual ~SSStubParsingThread();

	// Schedule file to parse
	void Schedule( const SolutionFileWeakPtr& stub );

	// Remove file from queue
	void Unschedule( const SolutionFileWeakPtr& stub );

	void RequestExit();

	// Pause
	void Pause( bool flag );
	RED_INLINE bool HasShutdown() const { return m_shutdown; }

	// Return the number of file to parse
	unsigned int GetFilesCountToProcess() const;

protected:
	void ProcessFile( const SolutionFileWeakPtr& weakFile );

	virtual void ThreadFunc() override final;
};

//////////////////////////////////////////////////////////////////////////
// Mutex
//////////////////////////////////////////////////////////////////////////
class SSStubSystemReadLock
{
public:
	SSStubSystemReadLock();
	~SSStubSystemReadLock();
};

class SSStubSystemReadLockNonBlocking
{
public:
	SSStubSystemReadLockNonBlocking();
	~SSStubSystemReadLockNonBlocking();

	bool TryLock();

private:
	bool m_locked;
};

class SSStubSystemWriteLock
{
public:
	SSStubSystemWriteLock();
	~SSStubSystemWriteLock();
};

//////////////////////////////////////////////////////////////////////////
// Stub system
//////////////////////////////////////////////////////////////////////////
class SSStubSystem
{
	friend class SSStubSystemReadLock;
	friend class SSStubSystemWriteLock;
	friend class SSStubSystemReadLockNonBlocking;

private:
	map< wstring, SSClassStub* >		m_classes;
	map< wstring, SSFunctionStub* >		m_functions;
	map< wstring, SSEnumStub* >			m_enums;
	map< wstring, SSEnumStub* >			m_enumMembers;
	map< wstring, wstring >				m_globalKeywords;	//!< First = Script Keyword, Second = C++ Class
	SRWLOCK								m_rwLock;
	SSStubParsingThread*				m_parsingThread;

public:
	SSStubSystem();
	~SSStubSystem();

	// Start processing thread
	void StartThread();

	void StopThread();

	// Start processing
	void StartProcessing();

	// Pause system
	void Pause();

	// Unpause system
	void Unpause();

	// Unregister stub in system
	void Register( SSFileStub* fileStub );

	// Unregister stub from system
	void Unregister( SSFileStub* fileStub );

	// Schedule file to process
	void Schedule( const SolutionFilePtr& file );

	// Unregister file
	void Unschedule( const SolutionFilePtr& file );

	// Get the number of files to process by parsing thread
	unsigned int GetFilesCountToProcess() const;

public:
	void RegisterGlobalKeyword( const wstring& scriptKeyword, const wstring& cppClass );
	const map< wstring, wstring >& GetGlobalsMap() const { return m_globalKeywords; }

public:
	// Translate stub handle to existing stub
	SSBasicStub* GetStub( unsigned int index ) const;

	// Find function stub at line in given file
	bool FindFunctionAtLine( const wstring& file, int line, SSClassStub*& classStub, SSFunctionStub*& functionStub ) const;

	// Collect visible symbols at given context
	bool CollectVisibleSymbols( SSClassStub* classStub, SSFunctionStub* functionStub, vector< wstring >& expressionTokens, const wstring& initialMatch, bool force, vector< SSBasicStub* >& stubs ) const; 

	// Resolve context, find final type :)
	bool ResolveExpressionType( SSClassStub* classStub, SSFunctionStub* functionStub, const vector< wstring >& expressionTokens, wstring& outTypeName, SSFunctionStub*& outFunction ) const;

	// Resolve function we are calling
	SSFunctionStub* ResolveFunctionCall( SSClassStub* classStub, SSFunctionStub* functionStub, const vector< wstring >& expressionTokens ) const;

public:
	// Find class/struct by name
	SSClassStub* FindClass( const wstring& name ) const;

	// Find state in class
	SSClassStub* FindState( SSClassStub* classStub, const wstring& name ) const;

	// Find enum by name 
	SSEnumStub* FindEnum( const wstring& name ) const;

	// Find enum by member name 
	SSEnumStub* FindEnumByMember( const wstring& name ) const;

	// Find global function by name 
	SSFunctionStub* FindFunction( const wstring& name ) const;

public:
	// For find symbol dialog
	const map< wstring, SSClassStub* >& GetAllClasses() const { return m_classes; }
	const map< wstring, SSFunctionStub* >& GetAllFunctions() const { return m_functions; }
	const map< wstring, SSEnumStub* >& GetAllEnums() const { return m_enums; }
};

extern SSStubSystem GStubSystem;
