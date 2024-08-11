///////////////////////////////////////////////////////////////////////////////////////////
// Save game system is very basic, block + stream based solution that hopefully will meet
// our needs in that regard. Save file is binary file with very basic hierarchical block structure.
// All data in the save file should be read IN ORDER, exactly the way it was written ( stream based ).
// Each save file consists of some number of TOP LEVEL blocks like. FactsDB, Quests, Community, etc
// that can be accessed any time during the loading phase ( block nature ).
///////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "class.h"
#include "memory.h"
#include "namesRegistry.h"

///////////////////////////////////////////////////////////////////////////////////////////
// quick config section

// remove massive logging 
#define NO_SAVE_VERBOSITY

// disable save import
#if !defined( RED_PLATFORM_WIN32 ) && !defined( RED_PLATFORM_WIN64 )
#	define NO_SAVE_IMPORT
#endif

// TLS optimization
#define USE_TLS_FOR_SAVES

///////////////////////////////////////////////////////////////////////////////////////////

class CGameStorageReader;
class CGameStorageWriter;

/// Game data storage - for storing save stream in memory
/// Storages can be layer used to merge final stream
class IGameDataStorage
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_GameSave, MC_Gameplay );

public:
	virtual ~IGameDataStorage() {};

	//! Get data size
	virtual Uint32 GetSize() const=0;

	//! Get stream data
	virtual const void* GetData() const=0;

	//! Create data reader
	virtual ISaveFile* CreateReader() const=0;

	//! Create data writer
	virtual ISaveFile* CreateWriter() = 0;

	//! Reserve memory for storage
	virtual void Reserve( Uint32 bytes ) = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////

/// Game saver - single stream writer
class IGameSaver
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_GameSave, MC_Gameplay );

public:
	virtual ~IGameSaver() {};

	//! Start named block
	virtual void BeginBlock( CName name )=0;

	//! End named block
	virtual void EndBlock( CName name )=0;

	//! Write raw ansi buffer as string
	virtual void WriteRawAnsiValue( CName name, const AnsiChar* data, Uint32 size )=0;

	//! Write typed value
	virtual void WriteValue( CName name, IRTTIType* type, const void* data )=0;

	//! Write property
	virtual void WriteProperty( void* object, CProperty* prop )=0;

	//! Paste storage stream
	virtual void AddStorageStream( IGameDataStorage* storageStream )=0;

	//! Finalize the save data
	virtual void Finalize()=0;

	//! Flush all buffers
	virtual void Close()=0;

	//! Get data buffer
	virtual const void* GetData() const=0;

	//! Get buffer size
	virtual Uint32 GetDataSize() const=0;

	//! Get buffer allocation size
	virtual Uint32 GetDataCapacity() const=0;

	//! A helper tool for saving properties of an object
	template< typename T >
	void SaveObject( T* object );

	//! A helper tool for hierarchical gathering of savable properties from a class
	void GetSavableProperties( CClass* theClass, TDynArray< CProperty* >& outProperties ) const
	{
		if ( theClass->HasBaseClass() )
		{
			GetSavableProperties( theClass->GetBaseClass(), outProperties );
		}

		const auto& props = theClass->GetLocalProperties();

		for ( Uint32 i=0; i<props.Size(); i++ )
		{
			CProperty* prop = props[i];
			if ( prop->IsSaved() )
			{
				outProperties.PushBack( prop );
			}
		}
	}

	//! A helper tool for hierarchical gathering of savable properties from a class, that doesn't allocate the memory on the heap
	Uint32 GetSavableProperties( CClass* theClass, CProperty** outProperties, Uint32 maxNumProperties ) const
	{
		Uint32 numProperties( 0 );
		if ( theClass->HasBaseClass() )
		{
			numProperties = GetSavableProperties( theClass->GetBaseClass(), outProperties, maxNumProperties );
		}

		const auto& props = theClass->GetLocalProperties(); 
		for ( Uint32 i = 0; i < props.Size() && numProperties < maxNumProperties; ++i )
		{
			CProperty* prop = props[i];
			if ( prop->IsSaved() )
			{
				outProperties[ numProperties ] = prop;
				++numProperties;
			}
		}

		return numProperties;
	}

	//! A helper tool for just getting the number of savable properties from a class
	static Uint32 CountSavableProperties( CClass* theClass )
	{
		Uint32 numProperties( 0 );
		if ( theClass->HasBaseClass() )
		{
			numProperties = CountSavableProperties( theClass->GetBaseClass() );
		}

		const auto& props = theClass->GetLocalProperties(); 
		for ( Uint32 i = 0; i < props.Size(); ++i )
		{
			CProperty* prop = props[i];
			if ( prop->IsSaved() )
			{
				++numProperties;
			}
		}

		return numProperties;
	}

	//! A helper tool for checking if a class does have any saveable properties
	static Bool TestForSavableProperties( CClass* theClass )
	{
		const auto& props = theClass->GetLocalProperties(); 
		for ( Uint32 i = 0; i < props.Size(); ++i )
		{
			CProperty* prop = props[i];
			if ( prop->IsSaved() )
			{
				return true;
			}
		}

		if ( theClass->HasBaseClass() )
		{
			return TestForSavableProperties( theClass->GetBaseClass() );
		}

		return false;
	}

public:
	//! Write value
	template< typename T >
	void WriteValue( const CName& name, const T& val )
	{
		#ifdef USE_TLS_FOR_SAVES
			RED_TLS static IRTTIType* type( nullptr );
			if ( nullptr == type ) // workaround of stupid msvc unablility to initialize thread_local dynamically
			{
				type = SRTTI::GetInstance().FindType( ::GetTypeName< T > () );
			}
		#else
			IRTTIType* type = SRTTI::GetInstance().FindType( ::GetTypeName< T > () );
		#endif

		ASSERT( type, TXT( "Trying to write unsupported type to save game" ) );
		WriteValue( name, type, &val );
	}
};

///////////////////////////////////////////////////////////////////////////////////////////

/// Game state loader
class IGameLoader
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_GameSave, MC_Gameplay );

public:
	virtual ~IGameLoader() {};

	//! Get game build version
	virtual Uint32 GetGameVersion() const=0;

	//! Get save binary version
	virtual Uint32 GetSaveVersion() const=0;

	//! Begin reading save block, will fail if given block was not found in stream
	virtual void BeginBlock( CName blockName )=0;

	//! End reading save block, will fail if block name does not match
	virtual void EndBlock( CName blockName )=0;

	//! Read named value from save, will fail if name or type does not match
	virtual void ReadValue( CName name, IRTTIType* type, const void* data, CObject* defaultParent )=0;

	//! Read property if possible
	virtual void ReadProperty( void* object, CClass* theClass, CObject* defaultParent )=0;

	//! Extract data storage block
	virtual IGameDataStorage* ExtractDataStorage()=0;

	//! Skip data storage block
	virtual void SkipDataStorage()=0;
	
	//! A helper tool for restoring properties of an object
	template< typename T >
	T* RestoreObject();

public:
	//! Read value
	template< typename T >
	void ReadValue( const CName& name, T& val )
	{
		#ifdef USE_TLS_FOR_SAVES
			RED_TLS static IRTTIType* type( nullptr );
			if ( nullptr == type ) // workaround of stupid msvc unablility to initialize thread_local dynamically
			{
				type = SRTTI::GetInstance().FindType( ::GetTypeName< T > () );
			}
		#else
			IRTTIType* type = SRTTI::GetInstance().FindType( ::GetTypeName< T > () );
		#endif

		ASSERT( type, TXT( "Trying to read unsupported type from save game" ) );
		ReadValue( name, type, &val, NULL );
	}

	//! Read and return value
	template< typename T >
	T ReadValue( const CName& name, const T& def = T(0) )
	{
		T val = def;
		ReadValue( name, val );
		return val;
	}
};

///////////////////////////////////////////////////////////////////////////////////////////

/// Scoped writer block
class CGameSaverBlock : public Red::System::NonCopyable
{
protected:
	const CName&	m_blockName;
	IGameSaver*		m_saver;
	IGameLoader*	m_loader;

public:
	CGameSaverBlock( IGameSaver* saver, const CName& blockName )
		: m_blockName( blockName )
		, m_saver( saver )
		, m_loader( NULL )
	{
		m_saver->BeginBlock( m_blockName );
	}

	CGameSaverBlock( IGameLoader* loader, const CName& blockName )
		: m_blockName( blockName )
		, m_saver( NULL )
		, m_loader( loader )
	{
		m_loader->BeginBlock( m_blockName );
	}

	~CGameSaverBlock()
	{
		if ( m_saver )
		{
			m_saver->EndBlock( m_blockName );
		}

		if ( m_loader )
		{
			m_loader->EndBlock( m_blockName );
		}
	}
};

template< typename T >
RED_INLINE T* IGameLoader::RestoreObject()
{
	T* instance = NULL;

	CName typeName;
	if ( GetSaveVersion() < 39 ) /* SAVE_VERSION_GOT_RID_OF_HASHES */  // Aaarrrgh... for F@#%s sake! Have to move this crap out of the core...
	{
		Uint32 typeNameHash = ReadValue< Uint32 >( CNAME(type), 0 );
		typeName = CName::CreateFromHash( Red::CNameHash( typeNameHash ) );
	}
	else
	{
		typeName = ReadValue< CName >( CNAME( type ), CName::NONE );
	}

	// Create object
	CClass* cl = SRTTI::GetInstance().FindClass( typeName );	
	if( cl )
	{
		instance = cl->CreateObject< T >();
		ASSERT( instance );
	}
	else
	{
		WARN_CORE( TXT("IGameLoader::RestoreObject class %s not found"), typeName.AsString().AsChar() );
	}

	// restore the properties
	if ( instance )
	{
		const Uint32 numProperties = ReadValue< Uint32 >( CNAME(numProperties) );
		for ( Uint32 i = 0; i < numProperties; ++i )
		{
			ReadProperty( instance, cl, instance );
		}
	}

	return instance;
}

//! A helper tool for saving properties of an object
template< typename T >
RED_INLINE void IGameSaver::SaveObject( T* object )
{
	CClass* type = object->GetClass();
	CName name = type->GetName();
	WriteValue( CNAME( type ), name );

	// save properties
	{
		// Get properties that can be saved
		const Uint32 maxNumProperties( 128 );
		CProperty* propertiesToSave[ maxNumProperties ];
		Uint32 numProperties = GetSavableProperties( object->GetClass(), propertiesToSave, maxNumProperties );
		if ( numProperties >= maxNumProperties )
		{
			HALT( "IGameSaver::SaveObject() reached maxNumProperties (=%ld) limit! Please increase this limit or reduce number of saveable properties in class %ls.", maxNumProperties, object->GetClass()->GetName().AsChar() ); 
		}

		// Save count
		WriteValue( CNAME( numProperties ), numProperties );

		// Save properties
		for ( Uint32 i = 0; i < numProperties; ++i )
		{
			WriteProperty( object, propertiesToSave[ i ] );
		}
	}
}

// utils for simple save profiling
#ifdef RED_LOGGING_ENABLED
#	define TIMER_BLOCK( varName ) Double varName; { Red::System::ScopedStopClock ___clk( varName );
#	ifdef RED_PLATFORM_ORBIS
#		define END_TIMER_BLOCK( varName ) } RED_LOG( TimerBlock, TXT("%s() took %.2lf msec (%ls) - file %s, line %ld"), __FUNCTION__, varName * 1000.0, TXT(#varName), __FILE__, __LINE__ );
#	else
#		define END_TIMER_BLOCK( varName ) } RED_LOG( TimerBlock, TXT("%ls() took %.2lf msec (%ls) - file %ls, line %ld"), __FUNCTIONW__, varName * 1000.0, TXT(#varName), __FILEW__, __LINE__ );
#	endif
#else
#	define TIMER_BLOCK( varName ) 
#	define END_TIMER_BLOCK( varName ) 
#endif