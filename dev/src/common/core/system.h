#pragma once
/**
* Copyright © 2007 CDProjekt Red, Inc. All Rights Reserved.
*/
/************************************************************************/
/* Per platform types definitions                                       */
/************************************************************************/
#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
# include "osTypesWin32.h"
#elif defined( RED_PLATFORM_ORBIS )
# include "osTypesOrbis.h"
#endif

/************************************************************************/
/* Global flags															*/
/************************************************************************/

// True if we are running under editor
extern Bool GIsEditor;

// True if we are running cooker
extern Bool GIsCooker;

// Set to destination platform for cooking (***ab> troche slabe bo juz jest CookingContext zawierajacy platforme, ale metody OnSerialize nie maja do niego dostepu)
extern Uint32 GCookingPlatform;

// True if we are closing
extern Bool GIsClosing;

// True if we are inside an editor game ( PIE )
extern Bool GIsEditorGame;

// True if we are running game.exe
extern Bool GIsGame;

/************************************************************************/
/* Globals                                                              */
/************************************************************************/
extern class CSystemIO		GSystemIO;

// Forward declaration of Char(Wide char) TString. This should allow us to maintain compatibility.
template < class T > class TString;
template < class Char > class TString;
typedef TString< Char > String;
typedef TString< AnsiChar > StringAnsi;

/************************************************************************/
/* System I/O                                                           */
/************************************************************************/
class CSystemIO
{
public:
	Bool CopyFile(const Char* existingFileName, const Char* newFileName, Bool failIfExists) const;

	Bool CreateDirectory(const Char* pathName) const;

	Bool DeleteFile(const Char* fileName) const;

	Bool IsFileReadOnly(const Char* fileName) const;

	Bool FileExist(const Char* fileName) const;

	Bool SetFileReadOnly(const Char* fileName, Bool readOnlyFlag );

	Bool MoveFile(const Char* existingFileName, const Char* newFileName) const;

	Bool RemoveDirectory(const Char* pathName) const;

	Uint64 GetFileSize( const Char* pathName ) const;

	Bool CreatePath( const Char* pathName ) const;

private:
	friend class CFileManager;

	static Red::System::DateTime GetFileTime( const Char* pathName );
};

/************************************************************************/
/* System File                                                          */
/************************************************************************/

#define MAX_FILE_NAME 256
class CSystemFile
{
	FileObject			m_file;
	Char				m_fileName[MAX_FILE_NAME+1];
	Uint64				m_lastPointer;
	Int64				m_currentPointer;

	static CSystemFile* m_first;
	CSystemFile*		m_next;

public:
	class CTime
	{
		friend class CSystemFile;

		FileTimeObject m_fileTime;
	public:
		CTime();
		CTime(Uint64 time);

		Uint64 ToUint64() const;
		Bool operator>(const CTime& fileTime) const;

		String ToString();
		void   Extract( Int32& year, Int32& month, Int32& day, Int32& hour, Int32& minute, Int32& second );
	};

public:
	CSystemFile();
	CSystemFile( const CSystemFile& other );

	virtual ~CSystemFile();

	operator Bool() const;

	Bool CreateWriter( const Char* fileName, Bool append = false );

	Bool CreateReader( const Char* fileName );

	Bool Close();

	static CSystemFile* FindAlreadyOpened( const Char* filePath );

	Bool FlushBuffers() const;

	Uint64 GetSize() const;

	CTime GetCreationTime() const;

	CTime GetLastAccessTime() const;

	CTime GetLastWriteTime() const;

	size_t Read(void* buf, size_t bytesToRead);

	uintptr_t Write(const void* buf, size_t bytesToWrite);

	Bool SetEndOfFile();

	Bool SetPointerBegin(Int64 distanceToMove);

	Bool GetPointerCurrent(Uint64& currentPointer) const;

	const Char *GetFileName() const { return m_fileName; }

	const Uint32 GetFileHandle() const { return (Uint32)m_file; }
};

/************************************************************************/
/* System FindFile                                                      */
/************************************************************************/
class CSystemFindFile
{
	FindFileObject	m_findFile;
public:
	CSystemFindFile(const Char* findName);

	~CSystemFindFile();

	operator Bool() const;

	void operator ++();

	const Char* GetFileName();

	Bool IsDirectory() const;

	Uint32 GetSize();
};
