/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "handleMap.h"
#include "class.h"

// Forward declaration
class CJobLoadResource;
typedef CJobLoadResource CSoftHandleLoadJob;

/// Base soft handle
class BaseSoftHandle
{
protected:
	mutable BaseSafeHandle			m_resource;		//!< Handle to loaded resource
	mutable CSoftHandleLoadJob*		m_asyncLoad;	//!< Token for async loading
	String							m_path;			//!< Path to resource

public:
	enum EAsyncLoadingResult
	{
		ALR_InProgress,
		ALR_Loaded,
		ALR_Failed
	};

public:
	RED_INLINE const String& GetPath() const { return m_path; }

	RED_INLINE const BaseSafeHandle& GetHandle() const { return m_resource; }

public:
	BaseSoftHandle();

	BaseSoftHandle( class CResource* resource );

	BaseSoftHandle( const String& path );

	BaseSoftHandle( const BaseSoftHandle& other );

	BaseSoftHandle( BaseSoftHandle&& other );

	~BaseSoftHandle();

public:
	//! Copy
	BaseSoftHandle& operator=( const BaseSoftHandle& other );

	//! Move
	BaseSoftHandle& operator=( BaseSoftHandle&& other );

public:
	//! Compare
	RED_INLINE Bool operator==( const BaseSoftHandle& other ) const
	{
		return m_path == other.m_path;
	}

	//! Not equal
	RED_INLINE Bool operator!=( const BaseSoftHandle& other ) const
	{
		return m_path != other.m_path;
	}

	//! Less
	RED_INLINE Bool operator<( const BaseSoftHandle& other ) const
	{
		return m_path < other.m_path;
	}

public:
	//! Get the object from the handle, sync
	THandle< CResource > Get() const;

	//! Try to get object, try to load in background. If passing a callback, do NOT call this repeatedly, you should not be polling manually!
	EAsyncLoadingResult GetAsync( Bool immediate = false ) const;

	//! Clear handle
	Bool Clear();

	//! Load binded resource
	Bool Load() const;

	//! Release resource reference
	Bool Release() const;

	//! Release resource reference
	Bool ReleaseJob() const;

	//! Serialize
	void Serialize( IFile& file );

	//! Is resource loading. Warn: Func is based on disk file so it's slow call
	Bool IsLoading() const;
};

/// Handle to resource that is not automatically loaded
template< class T >
class TSoftHandle : private BaseSoftHandle
{
public:
	RED_INLINE TSoftHandle()
		: BaseSoftHandle()
	{};

	RED_INLINE TSoftHandle( const TSoftHandle& other )
		: BaseSoftHandle( other )
	{};

	RED_INLINE TSoftHandle( TSoftHandle&& other )
		: BaseSoftHandle( Move( other ) )
	{};

	RED_INLINE TSoftHandle( const String& path )
		: BaseSoftHandle( path )
	{};

	RED_INLINE TSoftHandle( T* object )
		: BaseSoftHandle( object )
	{};

	RED_INLINE TSoftHandle& operator=( const TSoftHandle& other )
	{
		BaseSoftHandle::operator=( other );
		return *this;
	}

	RED_INLINE TSoftHandle& operator=( TSoftHandle&& other )
	{
		BaseSoftHandle::operator=( Move( other ) );
		return *this;
	}

	RED_INLINE TSoftHandle& operator=( T* object )
	{
		BaseSoftHandle other( object );
		BaseSoftHandle::operator=( other );
		return *this;
	}

	RED_INLINE Bool operator==( const TSoftHandle& other ) const
	{
		return BaseSoftHandle::operator ==( other );
	}

	RED_INLINE Bool operator!=( const TSoftHandle& other ) const
	{
		return BaseSoftHandle::operator !=( other );
	}

	RED_INLINE Bool operator<( const TSoftHandle& other ) const
	{
		return BaseSoftHandle::operator <( other );
	}

	//! Get the object from the handle right now (synchronous load)
	RED_INLINE THandle< T > Get() const { return Cast< T >( BaseSoftHandle::Get() ); }

	//! Get the object from the handle (asynchronously load if required). If passing a callback, do NOT call this repeatedly, you should not be polling manually!
	RED_INLINE EAsyncLoadingResult GetAsync( Bool immediate = false ) const { return BaseSoftHandle::GetAsync( immediate ); }

	//! Get the path
	RED_INLINE const String& GetPath() const { return BaseSoftHandle::GetPath(); }

	//! Unload resource from handle
	RED_INLINE Bool Release() const { return BaseSoftHandle::Release(); }

	//! Load resource synchronously
	RED_INLINE Bool Load() const { return BaseSoftHandle::Load(); }

    RED_INLINE Bool IsLoaded() const { return BaseSoftHandle::GetHandle().Get() != NULL; }

	RED_INLINE Bool IsEmpty() const { return BaseSoftHandle::GetPath().Empty(); }

	//! Is resource loading. Warn: Func is based on disk file so it's slow call
	RED_INLINE Bool IsLoading() const { return BaseSoftHandle::IsLoading(); }

	//! Expose Serialize interface for GC collection
	RED_INLINE void CollectForGC( IFile& file ) { ASSERT( file.IsGarbageCollector() ); BaseSoftHandle::Serialize( file ); }

	static const CName& GetTypeName()
	{
		static const CName name( InitName() );
		return name;
	}

private:
	static CName InitName()
	{
		Char typeName[ RED_NAME_MAX_LENGTH ];

		const Char* innerTypeName = TTypeName< T >::GetTypeName().AsChar();
		Red::System::SNPrintF( typeName, ARRAY_COUNT(typeName), TXT("soft:%ls"), innerTypeName );

		return CName( typeName );
	}
};

//////////////////////////////////////////////////////////////////////////
// class for soft handle types ( BaseSoftHandle and such )
class CRTTISoftHandleType : public IRTTIPointerTypeBase
{
	IRTTIType*		m_pointedType;
	CName			m_name;

public:
	CRTTISoftHandleType( IRTTIType *pointedType = NULL );

	static CName FormatName( IRTTIType* pointedType );

	virtual const CName&	GetName() const { return m_name; } // name is cached in constructor
	virtual Uint32			GetSize() const { return sizeof( BaseSoftHandle ); }
	virtual Uint32			GetAlignment() const { return TTypeAlignment< BaseSoftHandle >::Alignment; } 
	virtual ERTTITypeType	GetType() const { return RT_SoftHandle; }

	virtual void 			Construct( void *object ) const;
	virtual void 			Destruct( void *object ) const;
	virtual Bool 			Compare( const void* data1, const void* data2, Uint32 flags ) const;
	virtual void 			Copy( void* dest, const void* src ) const;
	virtual void 			Clean( void* data ) const;
	virtual Bool 			Serialize( IFile& file, void* data ) const;
	virtual Bool 			ToString( const void* data, String& valueString ) const;
	virtual Bool 			FromString( void* data, const String& valueString ) const;
	virtual Bool			DebugValidate( const void* data ) const;
	void*					GetPointed( void *pointerData ) const;

	virtual IRTTIType*		GetInternalType() const { return m_pointedType; }

	virtual Bool			NeedsGC() { return true; }

	//! IRTTIPointerTypeBase interface
	virtual CClass* GetPointedType() const;
	virtual CPointer GetPointer( const void* data ) const;
	virtual void SetPointer( void* data, const CPointer & ptr ) const;
};

