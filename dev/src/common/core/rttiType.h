/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "pointer.h"
#include "rttiSystem.h"
#include "names.h"
#include "sortedarray.h"
#include "stringConversion.h"
#include "staticarray.h"
#include "typeName.h"

class IRTTIBaseObject
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RTTI );

public:
	virtual ~IRTTIBaseObject() {};
};

class IRTTIType : public IRTTIBaseObject
{
public:
	virtual ~IRTTIType() {}

	virtual const CName&	GetName() const = 0;
	virtual Uint32			GetSize() const = 0;
	virtual Uint32			GetAlignment() const = 0;
	virtual ERTTITypeType	GetType() const = 0;

	virtual Bool		IsArrayType() const { return false; }; // to save us the == RT_Array || == RT_StaticArray || == RT_NativeArray tests
	virtual Bool		IsPointerType() const { return false; }; // to save us the == RT_Pointer || == RT_Handle tests
	
	virtual void			Construct( void *object ) const = 0;
	virtual void			Destruct( void *object ) const = 0;
	virtual Bool 			Compare( const void* data1, const void* data2, Uint32 flags ) const=0;
	virtual void 			Copy( void* dest, const void* src ) const=0;
	virtual void 			Clean( void* data ) const=0;
	virtual Bool 			Serialize( IFile& file, void* data ) const=0;
	virtual Bool 			ToString( const void* object, String& valueString ) const=0;
	virtual Bool 			FromString( void* object, const String& valueString ) const=0;	
	virtual Bool			DebugValidate( const void* ) const { return true; }
	virtual Bool 			NeedsCleaning() { return false; }
	virtual Bool 			NeedsGC()=0;
	virtual void			ResizeBuffer( CBaseArray & buffer, Uint32 size ) final {}
};

template < typename T >
struct TTypeAlignment
{
	enum
	{
		Alignment = __alignof( T )
	};
};


template < class T, Uint32 Alignment = TTypeAlignment< T >::Alignment >
class TSimpleRTTIType : public IRTTIType
{
public:
	virtual Uint32 GetSize() const
	{
		return sizeof(T);
	}

	virtual Uint32 GetAlignment() const
	{
		return Alignment;
	}

	virtual void Construct( void* object ) const
	{
		new (object) T();		
	}

	virtual void Destruct( void* /*object*/ ) const
	{
		// empty
	}

	virtual Bool Compare( const void* data1, const void* data2, Uint32 /*flags*/ ) const
	{
		return *( const T* ) data1 == *( const T*) data2;
	}

	virtual void Copy( void* dest, const void* src ) const
	{
		*( T* ) dest = *( const T* ) src;		
	}

	virtual void Clean( void* data ) const
	{		
		Destruct( data );
		Construct( data );	
	}

	virtual Bool Serialize( IFile& file, void* data ) const
	{
		file << (*(T*) data);
		return true;
	}

	virtual Bool ToString( const void* data, String& valueString ) const
	{
		valueString = ::ToString<T>( *( const T*) data );
		return true;
	}

	virtual Bool FromString( void* data, const String& valueString ) const
	{
		return ::FromString<T>( valueString, *(T*) data );
	}

	virtual Bool DebugValidate( const void* /*data*/ ) const
	{
		return true;
	}

	virtual Bool NeedsCleaning()
	{		
		return !TPlainType< T >::Value;
	}

	virtual Bool NeedsGC()
	{
		return false;
	}
};

// a special abstract wrapper for pointer types
class IRTTIPointerTypeBase : public IRTTIType
{
public:
	//! Generic pointer
	virtual Bool IsPointerType() const { return true; }

	//! Get type of the pointed data (right now always a CClass, but not always a CObject class)
	virtual CClass* GetPointedType() const = 0;

	//! Get a universal pointer to the pointed data
	virtual CPointer GetPointer( const void* data ) const = 0;

	//! Set new pointer to data
	virtual void SetPointer( void* data, const CPointer & ptr ) const = 0;
};

// class for handling pointer types
class CRTTIPointerType : public IRTTIPointerTypeBase
{
	IRTTIType*		m_pointedType;
	CName			m_name;			// cached in constructor

public:
	CRTTIPointerType( IRTTIType *pointedType = NULL );

	static CName FormatName( IRTTIType* pointedType );

	virtual const CName&	GetName() const { return m_name; }
	virtual Uint32			GetSize() const { return sizeof( void* ); }
	virtual Uint32			GetAlignment() const { return TTypeAlignment< void* >::Alignment; }
	virtual ERTTITypeType	GetType() const { return RT_Pointer; }

	virtual void 			Construct( void *object ) const;
	virtual void 			Destruct( void *object ) const;
	virtual Bool 			Compare( const void* data1, const void* data2, Uint32 flags ) const;
	virtual void 			Copy( void* dest, const void* src ) const;
	virtual void 			Clean( void* data ) const;
	virtual Bool 			Serialize( IFile& file, void* data ) const;
	virtual Bool 			ToString( const void* data, String& valueString ) const;
	virtual Bool 			FromString( void* data, const String& valueString ) const;
	virtual Bool			DebugValidate( const void* data ) const;

	void*					GetPointed( void *pointerData ) const { return *((void**)pointerData); }
	const void*				GetPointed( const void *pointerData ) const { return *((void**)pointerData); }

	virtual Bool			NeedsGC() { return true; }

	// IRTTIPointerType interface
	virtual CClass*	GetPointedType() const;
	virtual CPointer GetPointer( const void* data ) const;
	virtual void SetPointer( void* data, const CPointer & ptr ) const;
};

// Base class for array types, just to get the inner type
// The interface functions have different names that the actual implemnetation 
// just to save few cycles in cases when we know the exact type (manual devirtualization)
class IRTTIBaseArrayType : public IRTTIType
{
public:
	//! Generic array type
	virtual Bool IsArrayType() const { return true; }

	//! Get type of array element
	virtual IRTTIType* ArrayGetInnerType() const  = 0;

	//! Is this array type resizable ?
	virtual Bool ArrayIsResizable() const = 0;

	//! Get number of elements in the array
	virtual Uint32 ArrayGetArraySize( const void* arrayData ) const = 0;

	//! Get pointer to array inner element (writable)
	virtual void* ArrayGetArrayElement( void* arrayData, Uint32 index ) const = 0;

	//! Get pointer to array inner element (read-only)
	virtual const void* ArrayGetArrayElement( const void* arrayData, Uint32 index ) const = 0;

	//! Add element to array, returs array index of the added element
	virtual Int32 ArrayAddElement( void* arrayData, Uint32 count=1 ) const = 0;

	//! Remove element from array at given index
	virtual Bool ArrayDeleteElement( void* arrayData, Int32 itemIndex ) const = 0;

	//! Insert element into the array at given index
	virtual Bool ArrayInsertElement( void* arrayData, Int32 itemIndex ) const = 0;
};

// dynamic array type basically reimplements everyting that TDynArray template offers
// but using its inner type rather then template parameter
// we cannot create ArrayType as template as it would require registering all possible types of arrays
// in rtti which is rather impossible task (think of int************************** ;-)
class CRTTIArrayType : public IRTTIBaseArrayType
{
	IRTTIType*			m_innerType;	// element type
	EMemoryClass		m_memoryClass;	// memory class the array should allocate memory from
	EMemoryPoolLabel	m_memoryPool;	// memory pool the array should allocate memory from
		
	CName				m_name;			// cached full type name (in form of array<[type],[memoryclass],[memorypool]>)

public:
	CRTTIArrayType( IRTTIType *innerType, EMemoryClass memoryClass, EMemoryPoolLabel memoryPool );

	static CName FormatName( IRTTIType* innerType, EMemoryClass memoryClass, EMemoryPoolLabel memoryPool );

	virtual const CName&	GetName() const { return m_name; }
	virtual Uint32			GetSize() const { return sizeof( TDynArray<Int32> ); } // we assume that every array variation uses the same amount of memory
	virtual Uint32			GetAlignment() const { return TTypeAlignment< TDynArray<Int32> >::Alignment; } // no alignment restrictions
	virtual ERTTITypeType	GetType() const { return RT_Array; }

	virtual void 			Construct( void *object ) const;
	virtual void 			Destruct( void *object ) const;
	virtual Bool 			Compare( const void* data1, const void* data2, Uint32 flags ) const;
	virtual void 			Copy( void* dest, const void* src ) const;
	virtual void 			Clean( void* data ) const;
	virtual Bool 			Serialize( IFile& file, void* data ) const;
	virtual Bool 			ToString( const void* data, String& valueString ) const;
	virtual Bool 			FromString( void* data, const String& valueString ) const;
	virtual Bool			DebugValidate( const void* data ) const;
	virtual Bool 			NeedsCleaning();
	virtual Bool			NeedsGC();

public:
	// Array data manipulation interface
	// It is essential that the memory pointed by arrayData contains MATCHING array
	// By matching I mean the following:
	//  - array element type
	//  - memory class
	//  - memory pool
	Uint32					GetArraySize( const void* arrayData ) const;
	void*					GetArrayElement( void* arrayData, Uint32 index ) const;
	const void*				GetArrayElement( const void* arrayData, Uint32 index ) const;
	Int32					AddArrayElement( void* arrayData, Uint32 count=1 ) const;

	/// delete is copying the whole array, preserving order of elements
	Bool					DeleteArrayElement( void* arrayData, Int32 itemIndex ) const;

	/// delete fast is replacing the last element with a deleted element, not preserving order of elements
	Bool					DeleteArrayElementFast( void* arrayData, Int32 itemIndex ) const;

	Bool					InsertArrayElementAt( void* arrayData, Int32 itemIndex ) const;

public:
	// Get the type of array element, can be anything (array of arrays is allowed, etc)
	IRTTIType*			GetInnerType() const { return m_innerType; }

	// Get the memory class  used by array
	EMemoryClass		GetMemoryClass() const { return m_memoryClass; } 

	// Get the memory pool used by array
	EMemoryPoolLabel	GetMemoryPool() const { return m_memoryPool; } 

public:
	// IRTTIBaseArrayType interface implementation
	virtual Bool ArrayIsResizable() const { return true; }
	virtual IRTTIType* ArrayGetInnerType() const { return GetInnerType(); }
	virtual Uint32 ArrayGetArraySize( const void* arrayData ) const { return GetArraySize( arrayData ); }
	virtual void* ArrayGetArrayElement( void* arrayData, Uint32 index ) const { return GetArrayElement( arrayData, index ); }
	virtual const void* ArrayGetArrayElement( const void* arrayData, Uint32 index ) const { return GetArrayElement( arrayData, index ); }
	virtual Int32 ArrayAddElement( void* arrayData, Uint32 count=1 ) const { return AddArrayElement( arrayData, count ); }
	virtual Bool ArrayDeleteElement( void* arrayData, Int32 itemIndex ) const { return DeleteArrayElement( arrayData, itemIndex ); }
	virtual Bool ArrayInsertElement( void* arrayData, Int32 itemIndex ) const { return InsertArrayElementAt( arrayData, itemIndex ); }
};

// static array type for <type>[N] kind of stuff
class CRTTINativeArrayType : public IRTTIBaseArrayType
{
	IRTTIType*			m_innerType;	// element type
	Uint32				m_elementCount;	// size of the array (number of elements) (it's part of the type definition)
	CName				m_name;			// cached full type name (in form of array<[type],[memoryclass],[memorypool]>)

public:
	CRTTINativeArrayType( IRTTIType *innerType, const Uint32 elementCount );

	static CName FormatName( IRTTIType* innerType, const Uint32 elementCount );

	virtual const CName&	GetName() const { return m_name; }
	virtual Uint32			GetSize() const; // this returns byte size of the whole type, do not confuse this with GetElementCount() !!!!
	virtual Uint32			GetAlignment() const;
	virtual ERTTITypeType	GetType() const { return RT_NativeArray; }

	virtual void 			Construct( void *object ) const;
	virtual void 			Destruct( void *object ) const;
	virtual Bool 			Compare( const void* data1, const void* data2, Uint32 flags ) const;
	virtual void 			Copy( void* dest, const void* src ) const;
	virtual void 			Clean( void* data ) const;
	virtual Bool 			Serialize( IFile& file, void* data ) const;
	virtual Bool 			ToString( const void* data, String& valueString ) const;
	virtual Bool 			FromString( void* data, const String& valueString ) const;
	virtual Bool			DebugValidate( const void* data ) const;
	virtual Bool 			NeedsCleaning();
	virtual Bool			NeedsGC();

public:
	// Array data manipulation interface
	// It is essential that the memory pointed by arrayData contains MATCHING array
	// By matching I mean the following:
	//  - array element type
	//  - memory class
	//  - memory pool
	Uint32					GetArraySize( const void* arrayData ) const;
	void*					GetArrayElement( void* arrayData, Uint32 index ) const;
	const void*				GetArrayElement( const void* arrayData, Uint32 index ) const;

public:
	// Get the type of array element, can be anything (array of arrays is allowed, etc)
	IRTTIType*			GetInnerType() const { return m_innerType; }

	// Get number of array elements defined in the array type
	Uint32		GetElementCount() const { return m_elementCount; } 

public:
	// IRTTIBaseArrayType interface
	virtual Bool ArrayIsResizable() const { return false; }
	virtual IRTTIType* ArrayGetInnerType() const { return GetInnerType(); }
	virtual Uint32 ArrayGetArraySize( const void* arrayData ) const { return GetArraySize( arrayData ); }
	virtual void* ArrayGetArrayElement( void* arrayData, Uint32 index ) const { return GetArrayElement( arrayData, index ); }
	virtual const void* ArrayGetArrayElement( const void* arrayData, Uint32 index ) const { return GetArrayElement( arrayData, index ); }
	virtual Int32 ArrayAddElement( void* arrayData, Uint32 count=1 ) const { RED_UNUSED( arrayData ); RED_UNUSED( count ); return -1; }
	virtual Bool ArrayDeleteElement( void* arrayData, Int32 itemIndex ) const { RED_UNUSED( arrayData ); RED_UNUSED( itemIndex ); return false; }
	virtual Bool ArrayInsertElement( void* arrayData, Int32 itemIndex ) const { RED_UNUSED( arrayData ); RED_UNUSED( itemIndex ); return false; }
};

// RTTI wrapper for TStaticArray template
class CRTTIStaticArrayType : public IRTTIBaseArrayType
{
	IRTTIType*			m_innerType;	// element type
	Uint32				m_maxSize;		// static array capacity
		
	CName				m_name;			// cached full type name (in form of array<[type],[memoryclass],[memorypool]>)

public:
	CRTTIStaticArrayType( IRTTIType *innerType, const Uint32 maxSize );

	static CName FormatName( IRTTIType* innerType, const Uint32 maxSize );

	virtual const CName&	GetName() const { return m_name; }
	virtual Uint32			GetSize() const;
	virtual Uint32			GetAlignment() const;
	virtual ERTTITypeType	GetType() const { return RT_StaticArray; }

	virtual void 			Construct( void *object ) const;
	virtual void 			Destruct( void *object ) const;
	virtual Bool 			Compare( const void* data1, const void* data2, Uint32 flags ) const;
	virtual void 			Copy( void* dest, const void* src ) const;
	virtual void 			Clean( void* data ) const;
	virtual Bool 			Serialize( IFile& file, void* data ) const;
	virtual Bool 			ToString( const void* data, String& valueString ) const;
	virtual Bool 			FromString( void* data, const String& valueString ) const;
	virtual Bool			DebugValidate( const void* data ) const;
	virtual Bool 			NeedsCleaning();
	virtual Bool			NeedsGC();

public:
	// IRTTIBaseArrayType interface implementation
	virtual Bool ArrayIsResizable() const { return true; }
	virtual IRTTIType* ArrayGetInnerType() const { return m_innerType; }
	virtual Uint32 ArrayGetArraySize( const void* arrayData ) const;
	virtual void* ArrayGetArrayElement( void* arrayData, Uint32 index ) const;
	virtual const void* ArrayGetArrayElement( const void* arrayData, Uint32 index ) const;
	virtual Int32 ArrayAddElement( void* arrayData, Uint32 count=1 ) const;
	virtual Bool ArrayDeleteElement( void* arrayData, Int32 itemIndex ) const;
	virtual Bool ArrayInsertElement( void* arrayData, Int32 itemIndex ) const;
};

// class for handle types ( BaseHandle and such )
class CRTTIHandleType : public IRTTIPointerTypeBase
{
	IRTTIType*		m_pointedType;
	CName			m_name;

public:
	CRTTIHandleType( IRTTIType *pointedType = NULL );

	static CName FormatName( IRTTIType* pointedType );

	virtual const CName&	GetName() const;
	virtual Uint32			GetSize() const;	
	virtual Uint32			GetAlignment() const; // no alignment restrictions
	virtual ERTTITypeType	GetType() const;

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
	virtual Bool 			NeedsCleaning() { return true; }

	// IRTTIPointerType interface
	virtual CClass*	GetPointedType() const;
	virtual CPointer GetPointer( const void* data ) const;
	virtual void SetPointer( void* data, const CPointer & ptr ) const;
};

// array type name generator
template < typename T, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_INLINE const CName& TDynArray<T, memoryClass, memoryPool>::GetTypeName()
{
	static CName typeName;

	if ( !typeName )
	{
		Char typeNameStr[ RED_NAME_MAX_LENGTH ];

		const Char* innerTypeName = TTypeName<T>::GetTypeName().AsChar();
		Red::System::SNPrintF( typeNameStr, ARRAY_COUNT(typeNameStr), TXT("array:%d,%d,%ls"), (Uint32) memoryClass, /*(Uint32)memoryPool*/ 0, innerTypeName );

		typeName = CName( typeNameStr );
	}

	return typeName;
};

// static array type name generator
template < typename T, size_t MaxSize >
RED_INLINE const CName& TStaticArray<T, MaxSize>::GetTypeName()
{
	static CName typeName;

	if ( !typeName )
	{
		Char typeNameStr[ RED_NAME_MAX_LENGTH ];

		const Char* innerTypeName = TTypeName<T>::GetTypeName().AsChar();
		Red::System::SNPrintF( typeNameStr, ARRAY_COUNT(typeNameStr), TXT("static:%d,%ls"), MaxSize, innerTypeName );

		typeName = CName( typeNameStr );
	}

	return typeName;
};

// hash map type name generator
template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_INLINE const CName& THashMap< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::GetTypeName()
{
	static CName typeName;

	if ( !typeName )
	{
		Char typeNameStr[ RED_NAME_MAX_LENGTH ];

		const Char* keyTypeName = TTypeName<K>::GetTypeName().AsChar();
		const Char* valueTypeName = TTypeName<V>::GetTypeName().AsChar();
		Red::System::SNPrintF( typeNameStr, ARRAY_COUNT(typeNameStr), TXT("hashmap:%d,%ls->%ls"), (Uint32) memoryClass, keyTypeName, valueTypeName );

		typeName = CName( typeNameStr );
	}

	return typeName;
};

template< class _Type >
RED_INLINE IRTTIType* GetTypeObject()
{
	IRTTIType* rttiType = NULL;
	if ( NULL == rttiType )
	{
		rttiType = SRTTI::GetInstance().FindType( TTypeName<_Type>::GetTypeName() );
	}
	return rttiType;
}

template< class _Type >
RED_INLINE Bool IsArrayType()
{
	return TTypeName<_Type>::IsArray();
}

// TODO: this is not very safe
template< class _Type >
RED_INLINE Bool IsPointerType()
{
	CName typeName = TTypeName<_Type>::GetTypeName();
	return typeName.AsChar()[0] == TXT('*');
}

///////////////////////////////////////////////////////////////////////////
// helper for registering types in RTTI

template< class _Type >
class TRTTITypeRegistrator
{
	_Type* m_registeredType;

public:
	TRTTITypeRegistrator()
		: m_registeredType( NULL )
	{
		m_registeredType = new _Type();
		SRTTI::GetInstance().RegisterType( m_registeredType );
	}

	~TRTTITypeRegistrator()
	{
		delete m_registeredType;
		m_registeredType = NULL;
	}
};


RED_DECLARE_RTTI_NAME( CName );
RED_DECLARE_RTTI_NAME( CVariant );
RED_DECLARE_RTTI_NAME( String );
RED_DECLARE_RTTI_NAME( StringAnsi );
RED_DECLARE_RTTI_NAME( CGUID );

#define IMPLEMENT_RTTI_TYPE(type)			RED_DEFINE_RTTI_NAME( type ) \
											TRTTITypeRegistrator<type> g_simpleType##type##Registrator;

//////////////////////////////////////////////////////////////////////////
// simple types

#define DEFINE_SIMPLE_RTTI_TYPE(type)		RED_DECLARE_RTTI_NAME( type )													\
											class CSimpleRTTIType##type : public TSimpleRTTIType<type, sizeof(type)>		\
											{																				\
											public:																			\
												virtual const CName& GetName() const { return RED_NAME( type ); }			\
												virtual ERTTITypeType GetType() const { return RT_Simple; }					\
											} ;																				\
																															\
											template<>																		\
											struct TTypeName<type>															\
											{																				\
												static const CName& GetTypeName() { return RED_NAME( type ); }				\
												static Bool IsArray() { return false; }										\
											};


#define IMPLEMENT_SIMPLE_RTTI_TYPE(type)	RED_DEFINE_RTTI_NAME( type ); TRTTITypeRegistrator< CSimpleRTTIType##type > g_simpleType##type##Registrator;


#define DEFINE_FUNDAMENTAL_RTTI_TYPE(type)		RED_DECLARE_RTTI_NAME( type )													\
class CFundamentalRTTIType##type : public TSimpleRTTIType<type, sizeof(type)>		\
{																				\
public:																			\
	virtual const CName& GetName() const { return RED_NAME( type ); }			\
	virtual ERTTITypeType GetType() const { return RT_Fundamental; }					\
} ;																				\
	\
	template<>																		\
struct TTypeName<type>															\
{																				\
	static const CName& GetTypeName() { return RED_NAME( type ); }				\
	static Bool IsArray() { return false; }										\
};


#define IMPLEMENT_FUNDAMENTAL_RTTI_TYPE(type)	RED_DEFINE_RTTI_NAME( type ); TRTTITypeRegistrator< CFundamentalRTTIType##type > g_fundamentalType##type##Registrator;

DEFINE_FUNDAMENTAL_RTTI_TYPE( Bool );
DEFINE_FUNDAMENTAL_RTTI_TYPE( Uint8 );
DEFINE_FUNDAMENTAL_RTTI_TYPE( Int8 );
DEFINE_FUNDAMENTAL_RTTI_TYPE( Uint16 );
DEFINE_FUNDAMENTAL_RTTI_TYPE( Int16 );
DEFINE_FUNDAMENTAL_RTTI_TYPE( Uint32 );
DEFINE_FUNDAMENTAL_RTTI_TYPE( Int32 );
DEFINE_FUNDAMENTAL_RTTI_TYPE( Float );
DEFINE_FUNDAMENTAL_RTTI_TYPE( Double );
DEFINE_FUNDAMENTAL_RTTI_TYPE( Uint64 );
//DEFINE_FUNDAMENTAL_RTTI_TYPE( long );


//////////////////////////////////////////////////////////////////////////
// string type

class CSimpleRTTITypeString : public TSimpleRTTIType< String >
{
public:
	virtual void Destruct( void *object ) const
	{
		static_cast< String* >(object)->~String();
	}

	virtual const CName& GetName() const { return CNAME( String ); }

	virtual ERTTITypeType GetType() const { return RT_Simple; };

	virtual Bool FromString( void* data, const String& valueString ) const
	{
		*( String *) data = valueString;
		return true;
	}
};

template<>
struct TTypeName< String >
{													
	static const CName& GetTypeName() { return CNAME( String ); }	
};

//////////////////////////////////////////////////////////////////////////
// string ansi type

class CSimpleRTTITypeStringAnsi : public TSimpleRTTIType< StringAnsi >
{
public:
	virtual void Destruct( void *object ) const
	{
		static_cast< StringAnsi* >(object)->~StringAnsi();
	}

	virtual const CName& GetName() const { return CNAME( StringAnsi ); }

	virtual ERTTITypeType GetType() const { return RT_Simple; };

	virtual Bool FromString( void* data, const StringAnsi& valueString ) const
	{
		*( StringAnsi *) data = valueString;
		return true;
	}
	using TSimpleRTTIType< StringAnsi >::FromString;
};

template<>
struct TTypeName< StringAnsi >
{
	static const CName& GetTypeName() { return CNAME( StringAnsi ); }	
};

//////////////////////////////////////////////////////////////////////////
// guid type

class CSimpleRTTITypeCGUID : public TSimpleRTTIType< CGUID >
{
public:
	virtual const CName& GetName() const { return CNAME( CGUID ); }

	virtual ERTTITypeType GetType() const { return RT_Simple; };

	virtual Bool ToString( const void* data, String& valueString ) const
	{
		valueString.Resize( RED_GUID_STRING_BUFFER_SIZE );

		const Red::System::GUID* guid = static_cast< const Red::System::GUID* >( data );
		guid->ToString( valueString.TypedData(), valueString.Size() );
		return true;
	}

	virtual Bool FromString( void* data, const String& valueString ) const
	{
		Red::System::GUID* guid = static_cast< Red::System::GUID* >( data );
		return guid->FromString( valueString.TypedData() );
	}
};

template<>
struct TTypeName< CGUID >
{
	static const CName& GetTypeName() { return CNAME( CGUID ); }	
};

//////////////////////////////////////////////////////////////////////////
// CName type

class CSimpleRTTITypeCName : public TSimpleRTTIType< CName >
{
public:
	virtual const CName& GetName() const { return CNAME( CName ); }

	virtual ERTTITypeType GetType() const { return RT_Simple; }

	// names do not require cleaning (nothing in the destructor)
	virtual Bool NeedsCleaning() override { return false; }

	virtual Bool ToString( const void* data, String& valueString ) const
	{
		valueString = static_cast< const CName *>( data )->AsChar();
		return true;
	}

	virtual Bool FromString( void* data, const String& valueString ) const
	{
		*static_cast< CName* >( data ) = CName( valueString.AsChar() );
		return true;
	}
};

template<>
struct TTypeName< CName >
{						
	static const CName& GetTypeName() { return CNAME( CName ); }	
};
