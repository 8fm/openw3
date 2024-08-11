/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "fileVersionList.h"

#ifdef RED_LOGGING_ENABLED
#	define FMT_LOG_MSG( fmt, ... ) String::Printf( ( fmt ), ## __VA_ARGS__ )
#else
#	define FMT_LOG_MSG( fmt, ... ) String::EMPTY
#endif

class IFileLatentLoadingToken;
class IRTTIType;
class IFileDirectMemoryAccess;

struct Vector3;
struct Vector2;

struct Vector;
struct Color;
struct EulerAngles;
struct Matrix;
struct Plane;
class DeferredDataBuffer;
class CNamesRemapper;
class IFileSkipBlockCache;

namespace Red {	namespace Core { namespace ResourceManagement {
	class CResourceId;
} } }

namespace Red { namespace Core { namespace Bundle { 
	typedef Uint32 FileID;
} } }


class BaseSafeHandle;

// File flags
enum EFileFlags
{
	FF_Writer				= FLAG( 0 ),	//!< Stream is writing	
	FF_Reader				= FLAG( 1 ),	//!< Stream is reading
	FF_GarbageCollector		= FLAG( 2 ),	//!< This is not a file but a GC sweeper
	FF_FileBased			= FLAG( 3 ),	//!< Stream is file based
	FF_MemoryBased			= FLAG( 4 ),	//!< Stream is memory based
	FF_Compressed			= FLAG( 5 ),	//!< Stream is compressed
	FF_Buffered				= FLAG( 6 ),	//!< Reading/Writing is buffered
	FF_Encrypted			= FLAG( 7 ),	//!< Stream is encrypted
	FF_ByteSwap				= FLAG( 8 ),	//!< Convert endianess of simple types
	FF_Cooker				= FLAG( 9 ),	//!< This is cooker serialize
	FF_Mapper				= FLAG( 10 ),	//!< Mapper
	FF_ResourceResave		= FLAG( 11 ),	//!< Resource is being resaved to the same file it was opened from
	FF_XML					= FLAG( 13 ),	//!< XML file
	FF_HashNames			= FLAG( 14 ),	//!< File contains names saved as hashes
	FF_SavedGame			= FLAG( 15 ),	//!< File is saved game data
	FF_BundleData			= FLAG( 16 ),	//!< File is read from a bundle
	FF_ResourceCollector	= FLAG( 17 ),	//!< Actually this is a resource collector
	FF_Async				= FLAG( 18 ),	//!< File supports anychronous API
	FF_ErrorOccured			= FLAG( 19 ),	//!< File is in error state
	FF_NoBuffering			= FLAG( 20 ),	//!< File should not be additionaly buffered
	FF_FilterScriptProps	= FLAG( 22 ),	//!< Saving will filter out non-editable scripted properties (so we don't save liks to BS)
	FF_KeepCNameList		= FLAG( 23 ),	//!< CName strings list will be kept with this file, no need to save strings next to hashes
	FF_NoInlineSkipBlocks	= FLAG( 24 ),	//!< FileSkipableBlock data is not stored inline, instead it is cached
	FF_ScriptCollector		= FLAG( 25 )	//!< This is not a file but but the script collector.
};

extern Bool GCNameAsNumberSerialization;

/************************************************************************/
/* File based serialization interface									*/
/************************************************************************/
class IFile
{
public:
	Uint32		m_flags;		// File flags
	Uint32		m_version;		// Protocol/file version

public:	
	IFile( Uint32 flags ) : m_flags( flags ), m_version( VER_CURRENT ) {};
	virtual ~IFile() {};

	// Serialize data buffer of given size
	virtual void Serialize( void* buffer, size_t size )=0;

	// Get position in file stream
	virtual Uint64 GetOffset() const=0;

	// Get size of the file stream
	virtual Uint64 GetSize() const=0;

	// Seek to file position
	virtual void Seek( Int64 offset )=0;

public:
	// Query XML writer interface
	virtual class CXMLWriter* QueryXMLWriter() { return NULL; };

	// Query XML reader interface
	virtual class CXMLReader* QueryXMLReader() { return NULL; };

	// Query the dependency tracker interface (for advanced dependency tracking during cooking)
	virtual class IDependencyTracker* QueryDepndencyTracker() const { return NULL; }

	// Query speech collector.
	virtual class CSpeechCollector* QuerySpeechCollector() const { return NULL; }

	// Get the direct memory access for the file
	virtual class IFileDirectMemoryAccess* QueryDirectMemoryAccess() { return NULL; }

	// get direct serialization table access
	virtual const class CFileDirectSerializationTables* QuerySerializationTables() const { return nullptr; }

	// Create latent loading token for current file position
	virtual IFileLatentLoadingToken* CreateLatentLoadingToken( Uint64 /*currentOffset*/ ) { return NULL; }

	// Query a the skip-block cache interface
	virtual IFileSkipBlockCache* QuerySkippableBlockCache() { return nullptr; }

public:
	// Get file flags
	RED_INLINE Uint32 GetFlags() const { return m_flags; }

	// Get file version
	RED_INLINE Uint32 GetVersion() const { return m_version; }

	// Is this a writer ?
	RED_INLINE Bool IsWriter() const { return ( m_flags & FF_Writer ) != 0; }

	// Is this a reader
	RED_INLINE Bool IsReader() const { return ( m_flags & FF_Reader ) != 0; }

	// Is this a file based interface
	RED_INLINE Bool IsFileBased() const { return ( m_flags & FF_FileBased) != 0; }

	// Is this a memory based interface
	RED_INLINE Bool IsMemoryBased() const { return ( m_flags & FF_MemoryBased) != 0; }

	// Is this a compressed file ?
	RED_INLINE Bool IsCompressed() const { return ( m_flags & FF_Compressed) != 0; }

	// Is this a garbage collector serialized ?
	RED_INLINE Bool IsGarbageCollector() const { return ( m_flags & FF_GarbageCollector) != 0; }

	// Is this a cooker serialized ?
	RED_INLINE Bool IsCooker() const { return ( m_flags & FF_Cooker) != 0; }

	// Is endianess swapping enabled ?
	RED_INLINE Bool IsByteSwapping() const { return ( m_flags & FF_ByteSwap ) != 0; }

	// Is this a mapper ?
	RED_INLINE Bool IsMapper() const { return ( m_flags & FF_Mapper ) != 0; }

	// Is this a buffered file?
	RED_INLINE Bool IsBuffered() const { return ( m_flags & FF_Buffered) != 0; }

	// Is this a resource resave ?
	RED_INLINE Bool IsResourceResave() const { return ( m_flags & FF_ResourceResave ) != 0; }

	// Is this a XML file ?
	RED_INLINE Bool IsXML() const { return ( m_flags & FF_XML ) != 0; }

	// File contains names saved as hashes
	RED_INLINE Bool IsHashNames() const { return ( m_flags & FF_HashNames ) != 0; }

	// File is a saved game 
	RED_INLINE Bool IsSavedGame() const { return ( m_flags & FF_SavedGame ) != 0; }

	// File is a resource collector
	RED_INLINE Bool IsResourceCollector() const { return ( m_flags & FF_ResourceCollector ) != 0; }

	// Is this file in error state ?
	RED_INLINE Bool HasErrors() const { return ( m_flags & FF_ErrorOccured ) != 0; }

	// Are we filtering scripted properties ?
	RED_INLINE Bool IsFilteringScriptedProperties() const { return ( m_flags & FF_FilterScriptProps ) != 0; }

	// Are we reporting used cnames ?
	RED_INLINE Bool IsKeepingCNameList() const { return ( m_flags & FF_KeepCNameList ) != 0; }

	// Do we allow skip-block data to be stored inline
	RED_INLINE Bool IsSkipBlockDataInline() const { return (m_flags & FF_NoInlineSkipBlocks) == 0; }

	RED_INLINE Bool IsScriptCollector() const { return ( m_flags & FF_ScriptCollector ) != 0; }

public:
	// Enable/Disable byteswap
	RED_INLINE void SetByteSwapping( Bool swapBytes ) 
	{
		if ( swapBytes )
		{
			m_flags |= FF_ByteSwap;
		}
		else
		{
			m_flags &= ~FF_ByteSwap;
		}
	}

	// Enable/Disable byteswap
	RED_INLINE void SetCooker( Bool isCooker )
	{
		if ( isCooker )
		{
			m_flags |= FF_Cooker;
		}
		else
		{
			m_flags &= ~FF_Cooker;
		}
	}

	RED_INLINE void SetHashNames( Bool isHashNames )
	{
		if ( isHashNames )
		{
			m_flags |= FF_HashNames;
		}
		else
		{
			m_flags &= ~FF_HashNames;
		}
	}

	RED_INLINE void SetSavedGame( Bool isSavedGame )
	{
		if ( isSavedGame )
		{
			m_flags |= FF_SavedGame;
		}
		else
		{
			m_flags &= ~FF_SavedGame;
		}
	}

	RED_INLINE void SetKeepCNameList( Bool shouldReportNames )
	{
		if ( shouldReportNames )
		{
			m_flags |= FF_KeepCNameList;
		}
		else
		{
			m_flags &= ~FF_KeepCNameList;
		}
	}

	// Set file error flags
	RED_INLINE void SetError()
	{
		m_flags |= FF_ErrorOccured;
	}

	// Clear file error flag
	RED_INLINE void ClearError()
	{
		m_flags &= ~FF_ErrorOccured;
	}

	// Serialize simple data type, implements byte swapping if needed
	RED_INLINE void SerializeSimpleType( void* buffer, Uint32 size )
	{
		#ifndef RED_ENDIAN_SWAP_SUPPORT_DEPRECATED

			// No byte swapping on consoles
			Serialize( buffer, size );

		#else

			// If we need a byteswap serialize data buffer in reverse order
			if ( IsByteSwapping() )
			{
				// TODO: optimize
				for ( Uint32 i=0; i<size; i++ )
				{
					Serialize( (Uint8*)buffer + ( (size-1) - i ), 1 );
				}
			}
			else
			{
				// No byte swapping required
				Serialize( buffer, size );
			}
		#endif
	}

	// Pointer serialization helper
	virtual void SerializePointer( const class CClass* pointerClass, void*& pointer );

	// Name serialization helper
	virtual void SerializeName( class CName& name );

	// Name serialization helper
	void SerializeNameAsHash( class CName& name );

	// Soft handle serialization helper
	virtual void SerializeSoftHandle( class BaseSoftHandle& softHandle );

	// Serialize reference to RTTI type
	virtual void SerializeTypeReference( class IRTTIType*& type );

	// Serialize reference to the property
	virtual void SerializePropertyReference( const class CProperty*& prop );

	// Serialize the deferred data buffer
	virtual void SerializeDeferredDataBuffer( DeferredDataBuffer & buffer );

	// Copy file content to other file
	virtual void CopyToFile( IFile &dstFile, Uint64 offset, Uint64 size );

public:
	// Async interface - 

public:
	// Indexed name serialization
	// DO NOT VIRTUALIZE THE OPERATOR, use the SerializeName function instead
	RED_INLINE virtual IFile &operator<<( class CName &name ) RED_FINAL
	{
		SerializeName( name );
		return *this;
	}

	// Soft dependency serialization
	// DO NOT VIRTUALIZE THE OPERATOR, use the SerializeSoftHandle function instead
	RED_INLINE virtual IFile& operator<<( class BaseSoftHandle& softHandle ) RED_FINAL
	{
		SerializeSoftHandle( softHandle );
		return *this;
	}

	// RTTI Type
	// DO NOT VIRTUALIZE THE OPERATOR, use the SerializeTypeReference function instead
	RED_INLINE virtual IFile& operator<<( IRTTIType*& type ) RED_FINAL
	{
		SerializeTypeReference( type );
		return *this;
	}

	/// RTTI property
	// DO NOT VIRTUALIZE THE OPERATOR, use the SerializePropertyReference function instead
	RED_INLINE virtual IFile& operator<<( const CProperty*& prop ) RED_FINAL
	{
		SerializePropertyReference( prop );
		return *this;
	}

	// Object pointer serialization (compatibility)
	// DO NOT VIRTUALIZE THE OPERATOR, use the SerializePointer function instead
	virtual IFile &operator<<( class CObject*& object ) RED_FINAL;

public:
	// Serialization operators for basic types
	friend IFile& operator<<( IFile& file, Bool& val )			{ file.SerializeSimpleType( &val, sizeof( val ) ); return file; }
	friend IFile& operator<<( IFile& file, Int8& val )			{ file.SerializeSimpleType( &val, sizeof( val ) ); return file; }
	friend IFile& operator<<( IFile& file, Uint8& val )			{ file.SerializeSimpleType( &val, sizeof( val ) ); return file; }
	friend IFile& operator<<( IFile& file, Int16& val )			{ file.SerializeSimpleType( &val, sizeof( val ) ); return file; }
	friend IFile& operator<<( IFile& file, Uint16& val )		{ file.SerializeSimpleType( &val, sizeof( val ) ); return file; }
	friend IFile& operator<<( IFile& file, Int32& val )			{ file.SerializeSimpleType( &val, sizeof( val ) ); return file; }
	friend IFile& operator<<( IFile& file, Uint32& val )		{ file.SerializeSimpleType( &val, sizeof( val ) ); return file; }
	friend IFile& operator<<( IFile& file, Int64& val )			{ file.SerializeSimpleType( &val, sizeof( val ) ); return file; }
	friend IFile& operator<<( IFile& file, Uint64& val )		{ file.SerializeSimpleType( &val, sizeof( val ) ); return file; }
	friend IFile& operator<<( IFile& file, Float& val )			{ file.SerializeSimpleType( &val, sizeof( val ) ); return file; }
	friend IFile& operator<<( IFile& file, Double& val )		{ file.SerializeSimpleType( &val, sizeof( val ) ); return file; }
	friend IFile& operator<<( IFile& file, Char& val )			{ file.SerializeSimpleType( &val, sizeof( val ) ); return file; }

	// Math types 
	friend IFile& operator<<( IFile& file, Vector& val );
	friend IFile& operator<<( IFile& file, Vector2& val );
	friend IFile& operator<<( IFile& file, Vector3& val );
	friend IFile& operator<<( IFile& file, Color& val );
	friend IFile& operator<<( IFile& file, EulerAngles& val );
	friend IFile& operator<<( IFile& file, Matrix& val );
	friend IFile& operator<<( IFile& file, Plane& val );
	friend IFile& operator<<( IFile& file, CGUID& val );
	friend IFile& operator<<( IFile& file, Red::Core::ResourceManagement::CResourceId& val );
	friend IFile& operator<<( IFile& file, DeferredDataBuffer& buffer );

public:
	// For debug purposes only
	virtual const Char *GetFileNameForDebug() const { return TXT(""); }

	void* operator new( size_t size );	
	void  operator delete( void* ptr );	
};

class IFileEx : public IFile
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	IFileEx( Uint32 flags ) : IFile( flags ) {};
	virtual ~IFileEx() {};

	// Save file to HDD
	virtual void Close()=0;

	// Get data buffer
	virtual const void* GetBuffer() const=0;

	// Get the buffer allocation size
	virtual size_t GetBufferCapacity() const=0;
};

class ISaveFile : public IFileEx
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

	friend class CGameSaveManager;
	friend class CGameStorageSaver;
	friend class CW2SaveImporter;

protected:
	Uint32			m_saveVersion;
	CNamesRemapper& m_remapper;

public:
	ISaveFile( Uint32 flags, Uint32 saveVersion, CNamesRemapper& remapper ) 
		: IFileEx( flags | FF_SavedGame )
		, m_saveVersion( saveVersion )
		, m_remapper( remapper ) 
	{}
	virtual ~ISaveFile() {}
	virtual void SerializeName( class CName& name );
};

/// Direct access to the file tables - use with care
class CFileDirectSerializationTables
{
public:
	// mapped names
	const CName*			m_mappedNames;
	Uint32					m_numMappedNames;

	// mapped types
	const IRTTIType**		m_mappedTypes;
	Uint32					m_numMappedTypes;

	// mapped properties in the file being deserialized
	const class CProperty**	m_mappedProperties;
	const Int32*			m_mappedPropertyOffsets;
	Uint32					m_numMappedProperties;

public:
	CFileDirectSerializationTables();
};

/// Memory based file access - allows you to READ/WRITE directly to the buffer
/// This is a very delicate stuff - use only if you know what you're doing
/// TODO: merge IFileEx into this
class IFileDirectMemoryAccess
{
public:
	virtual ~IFileDirectMemoryAccess() {};

	// get base of the file's memory buffer
	virtual Uint8* GetBufferBase() const = 0;

	// get size of the file's memory buffer
	virtual Uint32 GetBufferSize() const = 0;
};

class IFileSkipBlockCache
{
public:
	virtual ~IFileSkipBlockCache() { }
	virtual Uint32 QuerySkippableBlockOffset( Uint32 blockStartOffset ) = 0;
	virtual void RegisterSkippableBlockOffset( Uint32 blockStartOffset, Uint32 blockSkipOffset ) = 0;
};