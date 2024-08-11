/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "rttiType.h"
#include "variant.h"
#include "class.h"
#include "object.h"
#include "resource.h"
#include "depot.h"
#include "filePath.h"

RED_DEFINE_RTTI_NAME( CName );
RED_DEFINE_RTTI_NAME( CVariant );
RED_DEFINE_RTTI_NAME( String );
RED_DEFINE_RTTI_NAME( StringAnsi );
RED_DEFINE_RTTI_NAME( CGUID );

IMPLEMENT_FUNDAMENTAL_RTTI_TYPE( Bool );
IMPLEMENT_FUNDAMENTAL_RTTI_TYPE( Uint8 );
IMPLEMENT_FUNDAMENTAL_RTTI_TYPE( Int8 );
IMPLEMENT_FUNDAMENTAL_RTTI_TYPE( Uint16 );
IMPLEMENT_FUNDAMENTAL_RTTI_TYPE( Int16 );
IMPLEMENT_FUNDAMENTAL_RTTI_TYPE( Uint32 );
IMPLEMENT_FUNDAMENTAL_RTTI_TYPE( Int32 );
IMPLEMENT_FUNDAMENTAL_RTTI_TYPE( Uint64 );

IMPLEMENT_FUNDAMENTAL_RTTI_TYPE( Float );
IMPLEMENT_FUNDAMENTAL_RTTI_TYPE( Double );

IMPLEMENT_RTTI_TYPE( CSimpleRTTITypeString );
IMPLEMENT_RTTI_TYPE( CSimpleRTTITypeStringAnsi );
IMPLEMENT_RTTI_TYPE( CSimpleRTTITypeCGUID );
IMPLEMENT_RTTI_TYPE( CSimpleRTTITypeCName );
IMPLEMENT_RTTI_TYPE( CSimpleRTTITypeVariant );

CRTTIPointerType::CRTTIPointerType( IRTTIType *pointedType /*= NULL*/ )
	: m_pointedType( pointedType )
	, m_name( FormatName( pointedType ) )
{
}

CName CRTTIPointerType::FormatName( IRTTIType* pointedType )
{
	// Beware: this is not arbitrary and should exactly match naming rules used in FindType
	Char typeName[ RED_NAME_MAX_LENGTH ]; // what about really long names ?
	Red::System::SNPrintF( typeName, ARRAY_COUNT(typeName), TXT("ptr:%ls"), pointedType->GetName().AsChar() );
	return CName( typeName );
}

void CRTTIPointerType::Construct( void *object ) const
{	
	*( Uint8** )object = NULL;
}

void CRTTIPointerType::Destruct( void *object ) const
{
	// no deletes or sth - memory allocation and object construction/destruction
	// should be managed by external code
	*( Uint8** )object = NULL;
}

Bool CRTTIPointerType::Compare( const void* data1, const void* data2, Uint32 ) const
{
	// Easy case
	void* ptr1 = *(void**) data1;
	void* ptr2 = *(void**) data2;
	if ( ptr1 == ptr2 )
	{
		return true;
	}

	// Get the objects, assumes CObject handle...
	if ( m_pointedType->GetType() == RT_Class )
	{
		CClass* pointedClass = (CClass*)m_pointedType;
		if ( pointedClass->IsObject() )
		{
			CObject* object1 = ( CObject* ) ptr1;
			CObject* object2 = ( CObject* ) ptr2;
			if ( object1 && object2 && ( object1->GetClass() == object2->GetClass() ) )
			{
				// Compare only if inlined
				if ( object1->IsInlined() && object2->IsInlined() )
				{
					return object1->GetClass()->DeepCompare( object1, object2 );
				}
			}
		}
	}

	// Not equal
	return false;
}

void CRTTIPointerType::Copy( void* dest, const void* src ) const
{
	*( Uint8** ) dest = *( Uint8* const * ) src;
}

void CRTTIPointerType::Clean( void* data ) const
{
	*( Uint8** ) data = NULL;
}

Bool CRTTIPointerType::Serialize( IFile& file, void* data ) const
{
	if ( m_pointedType->GetType() == RT_Class )
	{
		CClass* pointedClass = static_cast< CClass* >( m_pointedType );
		void*& pointerRef = *(void**) data;
		file.SerializePointer( pointedClass, pointerRef );

		return true;
	}

	ERR_CORE( TXT("Unable to save pointer no a non class type '%ls'"), m_pointedType->GetName().AsChar() );
	return false;
}

Bool CRTTIPointerType::ToString( const void* object, String& valueString ) const
{
	// Always show the NULL pointer
	void* ptr = * ( ISerializable** ) object;
	if ( NULL == ptr )
	{
		valueString = TXT("NULL");
		return true;
	}

	// Special handling for objects
	if ( m_pointedType->GetType() == RT_Class )
	{
		CClass* pointedClass = static_cast< CClass* >( m_pointedType );
		if ( pointedClass->IsObject() )
		{
			CObject* obj = (CObject*) ptr;

			if ( obj->GetClass()->IsA< CResource >() )
			{
				CResource* res = Cast< CResource >( obj );
				if ( res->GetFile() )
				{
					valueString = res->GetDepotPath();
					return true;
				}
			}
			else if ( obj->GetClass() )
			{
				valueString = obj->GetFriendlyName();
				return true;
			}
		}	
	}

	// Default to class name
	valueString = TXT( "[" ) + m_pointedType->GetName().AsString() + TXT( "]" );
	return true;
}

Bool CRTTIPointerType::FromString( void* object, const String& valueString ) const
{
	if ( ( m_pointedType->GetType() == RT_Class ) && ((CClass*)m_pointedType)->IsObject() )
	{
		CClass *pointedClass = (CClass*)m_pointedType;
		if ( valueString.EqualsNC( TXT("NULL") ) )
		{
			*( CObject** ) object = NULL;
			return true;
		}
		else
		{
			CObject* obj = GDepot->LoadResource( valueString );
			if ( obj && obj->IsA( pointedClass ))
			{
				*( CObject** ) object = obj;
				return true;
			}
		}

		return false;
	}

	return false;
}

Bool CRTTIPointerType::DebugValidate( const void* data ) const
{
	if ( ( m_pointedType->GetType() == RT_Class ) && ((CClass*)m_pointedType)->IsObject() )
	{
		CClass *pointedClass = (CClass*)m_pointedType;
		if ( pointedClass )
		{
			CObject* object = *( CObject** ) data;
			return CObject::IsValidObject( object );
		}
	}

	// Valid
	return true;
}

CClass* CRTTIPointerType::GetPointedType() const
{
	return static_cast< CClass* >( m_pointedType );
}

CPointer CRTTIPointerType::GetPointer( const void* data ) const
{
	// Only to classes
	if ( !m_pointedType || m_pointedType->GetType() != RT_Class )
	{
		return CPointer();
	}

	// An ISerializable object requires special handling since it can have a derived type
	CClass* pointedClass = static_cast< CClass* >( m_pointedType );
	if ( pointedClass->IsSerializable() )
	{
		// Get pointed data
		ISerializable* obj = NULL;
		Copy( &obj, data );

		// Create pointer from serializable ( it has virtual getClass() )
		return CPointer( obj );
	}

	// In default case just use the pointed type as the pointer's class
	void* obj = NULL;
	Copy( &obj, data );

	// Raw pointer
	return CPointer( obj, pointedClass );
}

void CRTTIPointerType::SetPointer( void* data, const CPointer & ptr ) const
{
	// Only to classes
	if ( m_pointedType && m_pointedType->GetType() != RT_Class )
	{
		// Check type match
		CClass* pointedClass = static_cast< CClass* >( m_pointedType );
		if ( !ptr.IsNull() && !ptr.GetClass()->IsA( pointedClass ) )
		{
			ERR_CORE( TXT("Trying to assign object of type '%ls' to pointer '%ls'"), ptr.GetClass()->GetName().AsChar(), pointedClass->GetName().AsChar() );
			return;
		}

		// Save data
		* ( void** ) data = (void*) ptr.GetPointer();
	}
}


//////////////////////////////////////////////////////////////////////////

CRTTIArrayType::CRTTIArrayType( IRTTIType *innerType, EMemoryClass memoryClass, EMemoryPoolLabel memoryPool )
	: m_innerType( innerType )
	, m_memoryClass( memoryClass )
	, m_memoryPool( memoryPool )
	, m_name( FormatName( innerType, memoryClass, memoryPool ) )
{
}

CName CRTTIArrayType::FormatName( IRTTIType* innerType, EMemoryClass memoryClass /*= MC_DynArray*/, EMemoryPoolLabel memoryPool /*= MemoryPool_Default*/ )
{
	// We need to extract the inner type name
	const Char* innerTypeName = innerType->GetName().AsChar();

	// Format and cache the propper type name
	// Beware: this is not arbitrary and should exactly match naming rules used in FindType
	Char typeName[ RED_NAME_MAX_LENGTH ]; // what about really long names ?
	Red::System::SNPrintF( typeName, ARRAY_COUNT(typeName), TXT("array:%d,%d,%ls"), memoryClass, memoryPool, innerTypeName );

	// Finally cache the generated type as CName
	return CName( typeName );
}

void CRTTIArrayType::Construct( void *object ) const
{
	const Uint32 innerTypeSize = m_innerType->GetSize();
	const Red::System::AnsiChar* innerTypeNamePtr = "unknown";
#ifdef USE_ARRAY_METRICS
	innerTypeNamePtr = m_innerType->GetName().AsAnsiChar();
#endif
	new (object) CBaseArray( );
}

void CRTTIArrayType::Destruct( void *object ) const
{
	// destroy array fields
	if ( m_innerType->NeedsCleaning() )
	{
		const Uint32 count = GetArraySize( object );
		for ( Uint32 i = 0; i < count; ++i )
		{
			void* itemPtr = GetArrayElement( object, i );
			m_innerType->Destruct( itemPtr );
		}
	}

	// clear array's memory
	CBaseArray& ar = *( CBaseArray*)object;
	ar.Clear( m_memoryClass );
	ar.~CBaseArray();
}

Uint32 CRTTIArrayType::GetArraySize( const void* arrayData ) const
{	
	if ( arrayData )
	{
		const CBaseArray& ar = *(const CBaseArray*)arrayData;
		return ar.Size();
	}

	return 0;
}

void* CRTTIArrayType::GetArrayElement( void* arrayData, Uint32 index ) const
{
	if ( arrayData )
	{
		const CBaseArray& ar = *( const CBaseArray*)arrayData;
		if ( index < ar.Size() )
		{
			const Uint32 itemSize = m_innerType->GetSize();
			return OffsetPtr( (void*)ar.Data(), index * itemSize );
		}
	}

	return NULL;
}

const void* CRTTIArrayType::GetArrayElement( const void* arrayData, Uint32 index ) const
{
	if ( arrayData )
	{
		const CBaseArray& ar = *( const CBaseArray*)arrayData;
		if ( index < ar.Size() )
		{
			const Uint32 itemSize = m_innerType->GetSize();
			return OffsetPtr( (void*)ar.Data(), index * itemSize );
		}
	}

	return NULL;
}

Int32 CRTTIArrayType::AddArrayElement( void* arrayData, Uint32 count/*=1*/ ) const
{
	ASSERT( count >= 1 );

	if ( arrayData )
	{
		CBaseArray& ar = *( CBaseArray*)arrayData;
		const Uint32 itemIndex = ar.Size();
		const Uint32 arraySize = itemIndex + count;

		ar.Resize( arraySize, m_innerType->GetSize(), m_innerType->GetAlignment(), m_memoryClass );

		// Clean memory before construct
		Red::System::MemoryZero( GetArrayElement( arrayData, itemIndex ), count * m_innerType->GetSize() );

		for( Uint32 i=0; i<count; ++i )
		{
			m_innerType->Construct( GetArrayElement( arrayData, itemIndex + i ) );
		}

		return itemIndex;
	}

	return -1;
}

Bool CRTTIArrayType::DeleteArrayElement( void* arrayData, Int32 itemIndex ) const
{	
	if ( arrayData )
	{
		CBaseArray& ar = *( CBaseArray*)arrayData;
		if ( itemIndex >= 0 && itemIndex < (Int32)ar.Size() )
		{
			// Shift elements
			for ( Uint32 i=itemIndex+1; i<ar.Size(); i++ )
			{
				const void* srcItem = GetArrayElement( arrayData, i );
				void* destItem = GetArrayElement( arrayData, i-1 );
				m_innerType->Copy( destItem, srcItem );
			}			

			// destroy last one (previous are destroyed properly using "copy" operation
			// which in case of objects is implemented with = operator
			m_innerType->Destruct( GetArrayElement( arrayData, ar.Size()-1 ) );

			// Resize array
			ar.ShrinkBuffer( 1 );
			return true;
		}
	}

	return false;
}

Bool CRTTIArrayType::DeleteArrayElementFast( void* arrayData, Int32 itemIndex ) const
{	
	if ( arrayData )
	{
		CBaseArray& ar = *( CBaseArray*)arrayData;
		if ( itemIndex >= 0 && itemIndex < (Int32)ar.Size() )
		{
			Int32 last = ( Int32 )( ar.Size() - 1 );

			if( itemIndex < last )
			{
				// Shift single element
				const void* srcItem = GetArrayElement( arrayData, last );
				void* destItem = GetArrayElement( arrayData, itemIndex );
				m_innerType->Copy( destItem, srcItem );
			}
			
			// destroy last one
			m_innerType->Destruct( GetArrayElement( arrayData, last ) );

			// Resize array
			// @todo MS: this doesn't have to creat a frame and call if, what about the ifdef there (?)
			ar.ShrinkBuffer( 1 );
			return true;
		}
	}

	return false;
}

Bool CRTTIArrayType::InsertArrayElementAt( void* arrayData, Int32 itemIndex ) const
{	
	if ( arrayData )
	{
		CBaseArray& ar = *( CBaseArray*)arrayData;
		if ( itemIndex >= 0 && itemIndex <= (Int32)ar.Size() )
		{
			// Add one element
			const Uint32 arraySize = ar.Size() + 1;

			ar.Resize( arraySize, m_innerType->GetSize(), m_innerType->GetAlignment(), m_memoryClass );

			// Clean memory before construct
			Red::System::MemoryZero( GetArrayElement( arrayData, ar.Size()-1 ), m_innerType->GetSize() );

			// initialize it
			m_innerType->Construct( GetArrayElement( arrayData, ar.Size()-1 ) );

			// Shift elements
			for ( Uint32 i = ar.Size() - 1; i > (Uint32)itemIndex; i-- )
			{
				const void* srcItem = GetArrayElement( arrayData, i - 1 );
				void* destItem = GetArrayElement( arrayData, i );
				m_innerType->Copy( destItem, srcItem );
			}			

			// Clear item
			m_innerType->Clean( GetArrayElement( arrayData, itemIndex ) );

			// Done
			return true;
		}
	}

	// Not deleted
	return false;
}

Bool CRTTIArrayType::Compare( const void* data1, const void* data2, Uint32 flags ) const
{
	// Get array size
	Uint32 size1 = GetArraySize( data1 );
	Uint32 size2 = GetArraySize( data2 );

	if ( size1 != size2 )
	{	
		return false;
	}

	for ( Uint32 i=0; i<size1; i++ )
	{
		const void* item1 = GetArrayElement( data1, i );
		const void* item2 = GetArrayElement( data2, i );
		if ( !m_innerType->Compare( item1, item2, flags ) )
		{
			return false;
		}
	}

	return true;
}

Bool CRTTIArrayType::DebugValidate( const void* data ) const
{
	// Validate array elements
	Uint32 size = GetArraySize( data );
	for ( Uint32 i=0; i<size; i++ )
	{
		const void* item = GetArrayElement( data, i );
		if ( !m_innerType->DebugValidate( item ) )
		{
			return false;
		}
	}

	// Valid
	return true;
}

void CRTTIArrayType::Copy( void* dest, const void* src ) const
{	
	CBaseArray& ar = *static_cast< CBaseArray* >( dest );

	// we need to call desctructors for the types that need cleaning
	if ( m_innerType->NeedsCleaning() )
	{
		Uint32 count = GetArraySize( dest );
		for ( Uint32 i = 0; i < count; i++ )
		{
			void* item = GetArrayElement( dest, i );
			m_innerType->Destruct( item );
		}
	}
	ar.ClearNoReallocate();

	const Uint32 count = GetArraySize( src );
	if ( count )
	{
		// Allocate memory
		ar.Resize( count, m_innerType->GetSize(), m_innerType->GetAlignment(), m_memoryClass );
		Red::System::MemorySet( ar.Data(), 0, m_innerType->GetSize() * count );

		// Destination items were destructed, so we need to construct them now
		for ( Uint32 i=0; i<count; i++ )
		{
			const void* srcItem = GetArrayElement( src, i );
			void* destItem = GetArrayElement( dest, i );
			m_innerType->Construct( destItem );				
			m_innerType->Copy( destItem, srcItem );
		}
	}
}

void CRTTIArrayType::Clean( void* data ) const
{
	ASSERT( data );

	// Clean elements
	Uint32 size = GetArraySize( data );
	for ( Uint32 i=0; i<size; i++ )
	{
		void* item = GetArrayElement( data, i );
		m_innerType->Clean( item );
	}

	// Clear the array
	CBaseArray& ar = *(CBaseArray*)data;
	ar.Clear( m_memoryClass );
}

Bool CRTTIArrayType::Serialize( IFile& file, void* data ) const
{
	ASSERT( m_innerType );

	// IMPORTANT: the static and dynamic array are what's called "binary comaptible"
	// It means that they are able to load each other's serialized data without problems.
	// Modify this code very carefully.

	// GC only serialization
	if ( file.IsGarbageCollector() )
	{
		if ( m_innerType->NeedsGC() )
		{
			const Uint32 count = GetArraySize( data );
			for ( Uint32 i=0; i<count; i++ )
			{
				void* itemData = GetArrayElement( data, i );
				m_innerType->Serialize( file, itemData );
			}
		}

		return true;
	}

	if ( file.IsWriter() )
	{
		// Write array count
		Uint32 count = GetArraySize( data );
		file << count;

		// Save elements
		for ( Uint32 i=0; i<count; i++ )
		{
			// Save property
			void* itemData = GetArrayElement( data, i );
			if ( !m_innerType->Serialize( file, itemData ) )
			{
				return false;
			}
		}
	}
	else if ( file.IsReader() )
	{
		// Read array count
		Uint32 count = 0;
		file << count;

		// Old serialization code
		if ( file.GetVersion() < VER_SERIALIZATION_DATA_CLEANUP )
		{
			CName notUsed1;
			Uint16 notUsed2;
			file << notUsed1;
			file << notUsed2;
		}

		// Setup array
		Clean( data );

		// Load items
		if ( count )
		{
			// Allocate memory
			CBaseArray& ar = *(CBaseArray*)data;
			const Uint32 elementSize = m_innerType->GetSize();
			ar.Resize( count, elementSize, m_innerType->GetAlignment(), m_memoryClass );

			if( m_innerType->GetType() == RT_Fundamental )
			{
				file.Serialize( ar.Data(), count * elementSize );
			}
			else
			{
				for( Uint32 i=0; i<count; ++i )
				{
					Red::System::MemoryZero( GetArrayElement( data, i ), m_innerType->GetSize() );
					m_innerType->Construct( GetArrayElement( data, i ) );
				}

				// Load elements
				for ( Uint32 i=0; i<count; i++ )
				{
					// Load property
					void* itemData = GetArrayElement( data, i );
					if ( !m_innerType->Serialize( file, itemData ) )
					{
						return false;
					}
				}
			}
		}
	}

	// Serialized
	return true;
}

Bool CRTTIArrayType::ToString( const void* data, String& valueString ) const
{
	Uint32 count = GetArraySize( data );
	if ( !count )
	{
		valueString = String::Printf( TXT("[Empty Array]") );
	}
	else
	{
		valueString = String::Printf( TXT("[Array of %i element%ls]"), count, count > 1 ? TXT("s") : TXT("") );
	}
	return true;
}

Bool CRTTIArrayType::FromString( void*, const String& ) const
{
	return false;
}

Bool CRTTIArrayType::NeedsCleaning()
{
	return true;
}

Bool CRTTIArrayType::NeedsGC()
{
	return m_innerType ? m_innerType->NeedsGC() : false;
}

//////////////////////////////////////////////////////////////////////////

CRTTINativeArrayType::CRTTINativeArrayType( IRTTIType *innerType, const Uint32 elementCount )
	: m_elementCount( elementCount )
	, m_innerType( innerType )
	, m_name( FormatName( innerType, elementCount ) )
{
	ASSERT( m_elementCount >= 1 );
	ASSERT( innerType != NULL );
}

CName CRTTINativeArrayType::FormatName( IRTTIType* innerType, const Uint32 elementCount )
{
	ASSERT( elementCount >= 1 );
	ASSERT( innerType != NULL );

	// Format and cache the propper type name
	// Beware: this is not arbitrary and should exactly match naming rules used in FindType
	Char typeName[ RED_NAME_MAX_LENGTH ]; // what about really long names ?
	Red::System::SNPrintF( typeName, ARRAY_COUNT(typeName), TXT("[%d]%ls"), elementCount, innerType->GetName().AsChar() );
	return CName( typeName );
}

Uint32	CRTTINativeArrayType::GetSize() const
{
	return m_innerType->GetSize() * m_elementCount;
}

Uint32 CRTTINativeArrayType::GetAlignment() const
{
	return m_innerType->GetAlignment();
}

void CRTTINativeArrayType::Construct( void *object ) const
{
	for ( Uint32 i=0; i<m_elementCount; ++i )
	{
		void* elementPtr = GetArrayElement( object, i );
		m_innerType->Construct( elementPtr );
	}
}

void CRTTINativeArrayType::Destruct( void *object ) const
{
	for ( Uint32 i=0; i<m_elementCount; ++i )
	{
		void* elementPtr = GetArrayElement( object, i );
		m_innerType->Destruct( elementPtr );
	}
}

Bool CRTTINativeArrayType::Compare( const void* data1, const void* data2, Uint32 flags ) const
{
	for ( Uint32 i=0; i<m_elementCount; ++i )
	{
		const void* elementPtr1 = GetArrayElement( data1, i );
		const void* elementPtr2 = GetArrayElement( data2, i );
		if ( !m_innerType->Compare( elementPtr1, elementPtr2, flags ) )
		{
			return false;
		}
	}

	return true;
}

void CRTTINativeArrayType::Copy( void* dest, const void* src ) const
{
	for ( Uint32 i=0; i<m_elementCount; ++i )
	{
		void* elementPtrDest = GetArrayElement( dest, i );
		const void* elementPtrSrc = GetArrayElement( src, i );
		m_innerType->Copy( elementPtrDest, elementPtrSrc );
	}
}

void CRTTINativeArrayType::Clean( void* ) const
{
	// does not work on static arrays
}

Bool CRTTINativeArrayType::Serialize( IFile& file, void* data ) const
{
	ASSERT( m_innerType );

	// IMPORTANT: the static and dynamic array are what's called "binary comaptible"
	// It means that they are able to load each other's serialized data without problems.
	// Modify this code very carefully.

	// GC only serialization
	if ( file.IsGarbageCollector() )
	{
		if ( m_innerType->NeedsGC() )
		{
			for ( Uint32 i=0; i<m_elementCount; i++ )
			{
				// Save property
				void* itemData = GetArrayElement( data, i );
				m_innerType->Serialize( file, itemData );
			}
		}

		return true;
	}

	if ( file.IsWriter() )
	{
		// Write array count
		Uint32 count = m_elementCount;
		file << count;

		// Save elements
		for ( Uint32 i=0; i<count; i++ )
		{
			// Save property
			void* itemData = GetArrayElement( data, i );
			if ( !m_innerType->Serialize( file, itemData ) )
			{
				return false;
			}
		}
	}
	else if ( file.IsReader() )
	{
		// Read array count
		Uint32 count = 0;
		file << count;

		// Old serialization code
		if ( file.GetVersion() < VER_SERIALIZATION_DATA_CLEANUP )
		{
			CName notUsed1;
			Uint16 notUsed2;
			file << notUsed1;
			file << notUsed2;
		}

		// Cleanup the array buffer, this can cause potential memory leaks but it seems 
		// we cannot trust the original content enough to call destructors here 
		Red::System::MemoryZero( data, GetSize() );

		// Construct all array elements
		for ( Uint32 i=0; i<m_elementCount; i++ )
		{
			void* itemData = GetArrayElement( data, i );
			m_innerType->Construct( itemData );
		}

		// Create temporary buffer for extra elements
		void* tempMemory = NULL;
		Bool tempMemoryAllocated = false;
		if ( count > m_elementCount )
		{
			const Uint32 innerTypeSize = m_innerType->GetSize();
			if ( innerTypeSize <= 256 )
			{
				tempMemory = alloca( innerTypeSize );
			}
			else
			{
				tempMemory = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_RTTI, innerTypeSize );
				tempMemoryAllocated = true;
			}

			Red::System::MemoryZero( tempMemory, innerTypeSize );
		}

		// Read all saved elements
		for ( Uint32 i=0; i<count; i++ )
		{
			// Load only elements within the size of the array, discard rest
			if ( i < m_elementCount )
			{
				void* itemData = GetArrayElement( data, i );
				if ( !m_innerType->Serialize( file, itemData ) )
				{
					return false;
				}
			}
			else
			{
				// We have stored more elements that we can handle in the static array, we need to discard those elements SAFELY and without any memory leaks
				m_innerType->Construct( tempMemory );
				if ( !m_innerType->Serialize( file, tempMemory ) )
				{
					m_innerType->Destruct( tempMemory );
					return false;
				}
				m_innerType->Destruct( tempMemory );
			}
		}

		// Release allocated memory
		if ( tempMemoryAllocated )
		{
			RED_MEMORY_FREE( MemoryPool_Default, MC_RTTI, tempMemory );
		}
	}

	// Serialized
	return true;
}

Bool CRTTINativeArrayType::ToString( const void* data, String& valueString ) const
{
	Uint32 count = GetArraySize( data );
	valueString = String::Printf( TXT("[Static array of %i element%ls]"), count, count > 1 ? TXT("s") : TXT("") );
	return true;
}

Bool CRTTINativeArrayType::FromString( void* , const String& ) const
{
	return false;
}

Bool CRTTINativeArrayType::DebugValidate( const void* data ) const
{
	// Validate array elements
	Uint32 size = GetArraySize( data );
	for ( Uint32 i=0; i<size; i++ )
	{
		const void* item = GetArrayElement( data, i );
		if ( !m_innerType->DebugValidate( item ) )
		{
			return false;
		}
	}

	// Valid
	return true;
}

Bool CRTTINativeArrayType::NeedsCleaning()
{
	return m_innerType->NeedsCleaning();
}

Bool CRTTINativeArrayType::NeedsGC()
{
	return m_innerType->NeedsGC();
}

Uint32 CRTTINativeArrayType::GetArraySize( const void* ) const
{
	// this does not depend on the data
	return m_elementCount;
}

void* CRTTINativeArrayType::GetArrayElement( void* arrayData, Uint32 index ) const
{
	ASSERT( index < m_elementCount );
	if ( index < m_elementCount )
	{
		return OffsetPtr( arrayData, index * m_innerType->GetSize() );
	}
	else
	{
		// this will cause access violation in the calling code, always better than overwrite some random memory
		return NULL;
	}
}

const void* CRTTINativeArrayType::GetArrayElement( const void* arrayData, Uint32 index ) const
{
	ASSERT( index < m_elementCount );
	if ( index < m_elementCount )
	{
		return OffsetPtr( arrayData, index * m_innerType->GetSize() );
	}
	else
	{
		// this will cause access violation in the calling code, always better than overwrite some random memory
		return NULL;
	}
}

//////////////////////////////////////////////////////////////////////////

CRTTIStaticArrayType::CRTTIStaticArrayType( IRTTIType *innerType, const Uint32 maxSize )
	: m_innerType( innerType )
	, m_maxSize( maxSize )
	, m_name( FormatName( innerType, maxSize ) )
{
	ASSERT( m_maxSize >= 1 );
	ASSERT( m_innerType != NULL );
}

CName CRTTIStaticArrayType::FormatName( IRTTIType* innerType, const Uint32 maxSize )
{
	// We need to extract the inner type name
	const Char* innerTypeName = innerType->GetName().AsChar();

	// Format and cache the propper type name
	// Beware: this is not arbitrary and should exactly match naming rules used in FindType
	Char typeName[ RED_NAME_MAX_LENGTH ]; // what about really long names ?
	Red::System::SNPrintF( typeName, ARRAY_COUNT(typeName), TXT("static:%d,%ls"), maxSize, innerTypeName );

	// Finally cache the generated type as CName
	return CName( typeName );
}

Uint32 CRTTIStaticArrayType::GetSize() const
{
	// very very nasty code
	const Uint32 dataSize = IBaseStaticArray::CalcTypeSize( m_innerType->GetSize(), m_maxSize );
	return dataSize;
}

Uint32 CRTTIStaticArrayType::GetAlignment() const
{
	return 16;
}

void CRTTIStaticArrayType::Construct( void *object ) const
{
	IBaseStaticArray* staticArrayData = static_cast< IBaseStaticArray* >( object );
	staticArrayData->Clear();
}

void CRTTIStaticArrayType::Destruct( void *object ) const
{
	Clean( object );
}

Bool CRTTIStaticArrayType::Compare( const void* data1, const void* data2, Uint32 flags ) const
{
	const Uint32 size1 = ArrayGetArraySize( data1 );
	const Uint32 size2 = ArrayGetArraySize( data2 );
	if ( size1 != size2 )
	{
		return false;
	}

	for ( Uint32 i=0; i<size1; ++i )
	{
		const void* elemData1 = ArrayGetArrayElement( data1, i );
		const void* elemData2 = ArrayGetArrayElement( data2, i );

		if ( !m_innerType->Compare( elemData1, elemData2, flags ) )
		{
			return false;
		}
	}
	
	return true;
}

void CRTTIStaticArrayType::Copy( void* dest, const void* src ) const
{
	Clean( dest );

	const Uint32 count = ArrayGetArraySize( src );
	if ( count )
	{
		// Allocate memory
		IBaseStaticArray& ar = *(IBaseStaticArray*)dest;
		ar.Grow( count );

		// Copy elements
		for ( Uint32 i=0; i<count; i++ )
		{
			const void* srcItem = ArrayGetArrayElement( src, i );
			void* destItem = ArrayGetArrayElement( dest, i );
			Red::System::MemorySet( destItem, 0, m_innerType->GetSize() * count );
			m_innerType->Construct( destItem );
			m_innerType->Copy( destItem, srcItem );
		}
	}
}

void CRTTIStaticArrayType::Clean( void* data ) const
{
	const Uint32 size = ArrayGetArraySize( data );
	for ( Uint32 i=0; i<size; ++i )
	{
		void* elemData = ArrayGetArrayElement( data, i );
		m_innerType->Destruct( elemData );
	}
	
	IBaseStaticArray* staticArrayData = static_cast< IBaseStaticArray* >( data );
	staticArrayData->Clear();
}

Bool CRTTIStaticArrayType::Serialize( IFile& file, void* data ) const
{
	ASSERT( m_innerType );

	// IMPORTANT: the static and dynamic array are what's called "binary comaptible"
	// It means that they are able to load each other's serialized data without problems.
	// Modify this code very carefully.

	// GC only serialization
	if ( file.IsGarbageCollector() )
	{
		if ( m_innerType->NeedsGC() )
		{
			const Uint32 count = ArrayGetArraySize( data );
			for ( Uint32 i=0; i<count; i++ )
			{
				void* itemData = ArrayGetArrayElement( data, i );
				m_innerType->Serialize( file, itemData );
			}
		}

		return true;
	}

	if ( file.IsWriter() )
	{
		// Write array count
		Uint32 count = ArrayGetArraySize( data );
		file << count;

		// Save elements
		for ( Uint32 i=0; i<count; i++ )
		{
			// Save property
			void* itemData = ArrayGetArrayElement( data, i );
			if ( !m_innerType->Serialize( file, itemData ) )
			{
				return false;
			}
		}
	}
	else if ( file.IsReader() )
	{
		// Read array count
		Uint32 count = 0;
		file << count;

		// Old serialization code
		if ( file.GetVersion() < VER_SERIALIZATION_DATA_CLEANUP )
		{
			CName notUsed1;
			Uint16 notUsed2;
			file << notUsed1;
			file << notUsed2;
		}

		// Setup array
		Clean( data );

		// Load items
		if ( count )
		{
			const Uint32 maxElements = Max< Uint32 >( count, m_maxSize );

			// Allocate memory
			IBaseStaticArray& ar = *(IBaseStaticArray*)data;
			ar.Grow( maxElements );

			// Create elements
			for( Uint32 i=0; i<maxElements; ++i )
			{
				void* element = ArrayGetArrayElement( data, i );
				Red::System::MemoryZero( element, m_innerType->GetSize() );
				m_innerType->Construct( element );
			}

			// Create temporary buffer for extra elements
			void* tempMemory = NULL;
			Bool tempMemoryAllocated = false;
			if ( count > m_maxSize )
			{
				const Uint32 innerTypeSize = m_innerType->GetSize();
				if ( innerTypeSize <= 256 )
				{
					tempMemory = alloca( innerTypeSize );
				}
				else
				{
					tempMemory = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_RTTI, innerTypeSize );
					tempMemoryAllocated = true;
				}

				Red::System::MemoryZero( tempMemory, innerTypeSize );
			}

			// Load elements
			for ( Uint32 i=0; i<count; i++ )
			{
				if ( i < m_maxSize )
				{
					// Load property
					void* itemData = ArrayGetArrayElement( data, i );
					if ( !m_innerType->Serialize( file, itemData ) )
					{
						return false;
					}
				}
				else
				{
					// We have stored more elements that we can handle in the static array, we need to discard those elements SAFELY and without any memory leaks
					m_innerType->Construct( tempMemory );
					if ( !m_innerType->Serialize( file, tempMemory ) )
					{
						m_innerType->Destruct( tempMemory );
						return false;
					}
					m_innerType->Destruct( tempMemory );
				}
			}

			// Release allocated memory
			if ( tempMemoryAllocated )
			{
				RED_MEMORY_FREE( MemoryPool_Default, MC_RTTI, tempMemory );
			}
		}
	}

	// Serialized
	return true;
}

Bool CRTTIStaticArrayType::ToString( const void* data, String& valueString ) const
{
	const Uint32 count = ArrayGetArraySize( data );
	if ( !count )
	{
		valueString = String::Printf( TXT("[Empty Static Array]") );
	}
	else
	{
		valueString = String::Printf( TXT("[Static Array with %i element%ls]"), count, count > 1 ? TXT("s") : TXT("") );
	}
	return true;
}

Bool CRTTIStaticArrayType::FromString( void* data, const String& valueString ) const
{
	RED_UNUSED( data );
	RED_UNUSED( valueString );
	return true;
}

Bool CRTTIStaticArrayType::DebugValidate( const void* data ) const
{
	const Uint32 size = ArrayGetArraySize( data );
	for ( Uint32 i=0; i<size; ++i )
	{
		const void* elemData = ArrayGetArrayElement( data, i );
		if ( !m_innerType->DebugValidate( elemData ) )
		{
			return false;
		}
	}

	return true;
}

Bool CRTTIStaticArrayType::NeedsCleaning()
{
	return m_innerType->NeedsCleaning();
}

Bool CRTTIStaticArrayType::NeedsGC()
{
	return m_innerType->NeedsGC();
}

Uint32 CRTTIStaticArrayType::ArrayGetArraySize( const void* arrayData ) const
{
	const IBaseStaticArray* staticArrayData = static_cast< const IBaseStaticArray* >( arrayData );
	return staticArrayData->GetSize();
}

void* CRTTIStaticArrayType::ArrayGetArrayElement( void* arrayData, Uint32 index ) const
{
	IBaseStaticArray* staticArrayData = static_cast< IBaseStaticArray* >( arrayData );
	const Uint32 innerTypeSize = m_innerType->GetSize();
	return staticArrayData->GetElement( index, innerTypeSize );
}

const void* CRTTIStaticArrayType::ArrayGetArrayElement( const void* arrayData, Uint32 index ) const
{
	const IBaseStaticArray* staticArrayData = static_cast< const IBaseStaticArray* >( arrayData );
	const Uint32 innerTypeSize = m_innerType->GetSize();
	return staticArrayData->GetElement( index, innerTypeSize );
}

Int32 CRTTIStaticArrayType::ArrayAddElement( void* arrayData, Uint32 count/*=1*/ ) const
{
	ASSERT( count >= 1 );

	if ( arrayData )
	{
		IBaseStaticArray* staticArrayData = static_cast< IBaseStaticArray* >( arrayData );
		const Uint32 itemIndex = staticArrayData->GetSize();
		if ( itemIndex + count <= m_maxSize )
		{
			const Uint32 innerTypeSize = m_innerType->GetSize();

			staticArrayData->Grow( count );
			Red::System::MemoryZero( staticArrayData->GetElement( innerTypeSize, itemIndex ), count * innerTypeSize );

			for ( Uint32 i=0; i<count; ++i )
			{
				void* elementData = staticArrayData->GetElement( innerTypeSize, i + itemIndex );
				m_innerType->Construct( elementData );
			}

			return itemIndex;
		}
	}

	return -1;
}

Bool CRTTIStaticArrayType::ArrayDeleteElement( void* arrayData, Int32 itemIndex ) const
{
	if ( arrayData )
	{
		IBaseStaticArray* staticArrayData = static_cast< IBaseStaticArray* >( arrayData );
		const Uint32 size = staticArrayData->GetSize();

		if ( itemIndex >= 0 && itemIndex < (Int32)size )
		{
			const Uint32 innerTypeSize = m_innerType->GetSize();

			// Shift elements
			for ( Uint32 i=itemIndex+1; i<size; i++ )
			{
				const void* srcItem = staticArrayData->GetElement( innerTypeSize, i );
				void* destItem = staticArrayData->GetElement( innerTypeSize, i-1 );
				m_innerType->Copy( destItem, srcItem );
			}

			// destroy last one (previous are destroyed properly using "copy" operation
			// which in case of objects is implemented with = operator
			void* lastElement = staticArrayData->GetElement( innerTypeSize, size-1 );
			m_innerType->Destruct( lastElement );

			// Resize array
			staticArrayData->Remove( 1 );
			return true;
		}
	}

	return false;

}

Bool CRTTIStaticArrayType::ArrayInsertElement( void* arrayData, Int32 itemIndex ) const
{
	if ( arrayData )
	{
		IBaseStaticArray* staticArrayData = static_cast< IBaseStaticArray* >( arrayData );
		const Uint32 size = staticArrayData->GetSize();

		if ( itemIndex >= 0 && itemIndex <= (Int32)size && size < m_maxSize )
		{
			// Add one element
			staticArrayData->Grow( 1 );

			// initialize it
			const Uint32 innerTypeSize = m_innerType->GetSize();
			
			// Clean memory before construct
			void* item = staticArrayData->GetElement( innerTypeSize, staticArrayData->GetSize() - 1 );
			Red::System::MemoryZero( item, innerTypeSize );
			m_innerType->Construct( item );

			// Shift elements
			for ( Uint32 i = staticArrayData->GetSize()-1; i > (Uint32)itemIndex; i-- )
			{
				const void* srcItem = staticArrayData->GetElement( innerTypeSize, i - 1 );
				void* destItem = staticArrayData->GetElement( innerTypeSize, i );
				m_innerType->Copy( destItem, srcItem );
			}			

			// Clear item
			m_innerType->Clean( staticArrayData->GetElement( innerTypeSize, itemIndex ) );
			return true;
		}
	}

	// Not inserted
	return false;
}

//////////////////////////////////////////////////////////////////////////

CRTTIHandleType::CRTTIHandleType( IRTTIType *pointedType /*= NULL*/ )
	: m_pointedType( static_cast< CClass* >( pointedType ) )
	, m_name( FormatName( pointedType ) )
{
}

CName CRTTIHandleType::FormatName( IRTTIType* pointedType )
{
	ASSERT( pointedType && pointedType->GetType() == RT_Class );

	// Format and cache the propper type name
	// Beware: this is not arbitrary and should exactly match naming rules used in FindType
	Char typeName[ RED_NAME_MAX_LENGTH ]; // what about really long names ?
	Red::System::SNPrintF( typeName, ARRAY_COUNT(typeName), TXT("handle:%ls"), pointedType->GetName().AsChar() );
	return CName( typeName );
}

void CRTTIHandleType::Construct( void *object ) const
{
	new ( object ) BaseSafeHandle();
}

void CRTTIHandleType::Destruct( void *object ) const
{
	BaseSafeHandle* handle = ( BaseSafeHandle* ) object;
	handle->~BaseSafeHandle();
}

Bool CRTTIHandleType::Compare( const void* data1, const void* data2, Uint32 ) const
{
	// Easy compare
	const BaseSafeHandle& handle1 = * ( const BaseSafeHandle* )( data1 );
	const BaseSafeHandle& handle2 = * ( const BaseSafeHandle* )( data2 );
	if ( handle1.Get() == handle2.Get() )
	{
		return true;
	}

	// Get the objects, assumes CObject handle...
	if ( m_pointedType->GetType() == RT_Class )
	{
		const CClass* pointedClass = static_cast< const CClass* > (m_pointedType );
		if ( pointedClass->IsObject() )
		{
			const CObject* object1 = static_cast< const CObject* >( handle1.Get() );
			const CObject* object2 = static_cast< const CObject* >( handle2.Get() );
			if ( object1 && object2 && ( object1->GetClass() == object2->GetClass() ) )
			{
				// Compare only if inlined
				if ( object1->IsInlined() && object2->IsInlined() )
				{
					return object1->GetClass()->DeepCompare( object1, object2 );
				}
			}
		}
	}

	// Not equal
	return false;
}

void CRTTIHandleType::Copy( void* dest, const void* src ) const
{
	BaseSafeHandle& handleDest = * ( BaseSafeHandle* )( dest );
	const BaseSafeHandle& handleSrc = * ( const BaseSafeHandle* )( src );
	handleDest = handleSrc;
}

void CRTTIHandleType::Clean( void* data ) const
{
	BaseSafeHandle& handle = * ( BaseSafeHandle* )( data );
	handle.Clear();
}

Bool CRTTIHandleType::Serialize( IFile& file, void* data ) const
{
	ASSERT( m_pointedType->GetType() == RT_Class );
	BaseSafeHandle& handle = * ( BaseSafeHandle* )( data );

	const CClass* pointedClass = static_cast< const CClass* >( m_pointedType );
	if ( pointedClass->IsSerializable() )
	{
		if ( file.IsWriter() )
		{
#ifndef RED_FINAL_BUILD
			CHandleSerializationMarker marker;
#endif
			void* ptr = handle.Get();
			file.SerializePointer( pointedClass, ptr );
		}
		else if ( file.IsReader() )
		{
			void* ptr = handle.Get();
			file.SerializePointer( pointedClass, ptr );

			if ( ptr )
			{
				THandle< IReferencable > objectHandle( static_cast< IReferencable* >( ptr ) );
				handle = (BaseSafeHandle&)objectHandle;
			}
			else
			{
				handle.Clear();
			}
		}
	}

	return true;
}

Bool CRTTIHandleType::ToString( const void* data, String& valueString ) const
{
	const BaseSafeHandle& handle = *( const BaseSafeHandle* )( data );

	// Easy cases
	if ( !handle.Get() )
	{
		valueString = TXT("NULL");
		return true;
	}

	// Get the object
	const IReferencable* ptr = static_cast< const IReferencable* >( handle.Get() );
	if ( ptr && ptr->GetClass()->IsA< CResource >() )
	{
		const CResource* res = static_cast< const CResource* >( ptr );
		if ( res->GetFile() )
		{
			valueString = res->GetDepotPath();
			return true;
		}
	}

	// Use friendly name
	valueString = TXT( "[" ) + m_pointedType->GetName().AsString() + TXT( "]" );
	return true;
}

Bool CRTTIHandleType::FromString( void* data, const String& valueString ) const
{
	if ( ( m_pointedType->GetType() == RT_Class ) && ((CClass*)m_pointedType)->IsObject() )
	{
		const CClass *pointedClass = static_cast< const CClass* >( m_pointedType );
		if ( valueString.EqualsNC( TXT("NULL") ) )
		{
			BaseSafeHandle& handle = * ( BaseSafeHandle* )( data );
			handle.Clear();
			return true;
		}
		else
		{
			CObject* obj = GDepot->LoadResource( valueString );
			if ( obj && obj->IsA( pointedClass ) )
			{
				BaseSafeHandle& handle = * ( BaseSafeHandle* )( data );
				THandle< CObject > objectHandle( obj );
				handle = (BaseSafeHandle&) objectHandle;
				return true;
			}
		}

		return false;
	}

	return false;
}

Bool CRTTIHandleType::DebugValidate( const void* ) const
{
	return true;
}

void* CRTTIHandleType::GetPointed( void *pointerData ) const
{
	BaseSafeHandle& handle = * ( BaseSafeHandle* )( pointerData );
	return handle.Get();
}

CClass* CRTTIHandleType::GetPointedType() const
{
	return static_cast< CClass* >( m_pointedType );
}

CPointer CRTTIHandleType::GetPointer( const void* data ) const
{
	const BaseSafeHandle& handle = * ( const BaseSafeHandle* )( data );
	return CPointer( handle.Get(), GetPointedType() );
}

void CRTTIHandleType::SetPointer( void* data, const CPointer & ptr ) const
{
	BaseSafeHandle& handle = * ( BaseSafeHandle* )( data );
	handle.Set( ptr.GetObjectPtr() );
}

const CName& CRTTIHandleType::GetName() const 
{ 
	return m_name; 
} 

Uint32 CRTTIHandleType::GetSize() const 
{ 
	return sizeof( BaseSafeHandle ); 
}

Uint32 CRTTIHandleType::GetAlignment() const 
{ 
	return TTypeAlignment< BaseSafeHandle >::Alignment; 
} 

ERTTITypeType CRTTIHandleType::GetType() const 
{ 
	return RT_Handle; 
}

//////////////////////////////////////////////////////////////////////////

CRTTISoftHandleType::CRTTISoftHandleType( IRTTIType *pointedType )
	: m_pointedType( pointedType )
	, m_name( FormatName( pointedType ) )
{
}

CName CRTTISoftHandleType::FormatName( IRTTIType* pointedType )
{
	ASSERT( pointedType && pointedType->GetType() == RT_Class );

	// Format and cache the propper type name
	// Beware: this is not arbitrary and should exactly match naming rules used in FindType
	Char typeName[ RED_NAME_MAX_LENGTH ]; // what about really long names ?
	Red::System::SNPrintF( typeName, ARRAY_COUNT(typeName), TXT("soft:%ls"), pointedType->GetName().AsChar() );
	return CName( typeName );
}

void CRTTISoftHandleType::Construct( void *object ) const
{
	new ( object ) BaseSoftHandle();
}

void CRTTISoftHandleType::Destruct( void *object ) const
{
	BaseSoftHandle* handle = ( BaseSoftHandle* ) object;
	handle->~BaseSoftHandle();
}

Bool CRTTISoftHandleType::Compare( const void* data1, const void* data2, Uint32 ) const
{
	// Easy compare
	const BaseSoftHandle& handle1 = * ( const BaseSoftHandle* )( data1 );
	const BaseSoftHandle& handle2 = * ( const BaseSoftHandle* )( data2 );
	if ( handle1 == handle2 )
	{
		return true;
	}

	// Not equal
	return false;
}

void CRTTISoftHandleType::Copy( void* dest, const void* src ) const
{
	BaseSoftHandle& handleDest = * ( BaseSoftHandle* )( dest );
	const BaseSoftHandle& handleSrc = * ( const BaseSoftHandle* )( src );
	handleDest = handleSrc;
}

void CRTTISoftHandleType::Clean( void* data ) const
{
	BaseSoftHandle& handle = * ( BaseSoftHandle* )( data );
	handle.Clear();
}

Bool CRTTISoftHandleType::Serialize( IFile& file, void* data ) const
{
	ASSERT( m_pointedType->GetType() == RT_Class );
	BaseSoftHandle& handle = * ( BaseSoftHandle* )( data );
	handle.Serialize( file );
	return true;
}

Bool CRTTISoftHandleType::ToString( const void* data, String& valueString ) const
{
	const BaseSoftHandle& handle = *( const BaseSoftHandle* )( data );
	valueString = handle.GetPath();
	if ( valueString.Empty() == true )
	{
		valueString = TXT("NULL");
	}
	return true;
}

Bool CRTTISoftHandleType::FromString( void* data, const String& valueString ) const
{
	if ( valueString.EqualsNC( TXT("NULL") ) )
	{
		// Clear the handle
		BaseSoftHandle& handle = * ( BaseSoftHandle* )( data );
		handle.Clear();
		return true;
	}
	else
	{
		ASSERT( m_pointedType->GetType() == RT_Class );
		CClass* pointedClass = static_cast< CClass* >( m_pointedType );
		if ( pointedClass->IsBasedOn( ClassID< CResource >() ) )
		{
			// Get extension
			CResource* baseRes = pointedClass->GetDefaultObject< CResource >();
			const Char* extension = baseRes->GetExtension();
			if ( extension && extension[0] )
			{
				// Check extension
				CFilePath filePath( valueString );
				if ( filePath.GetExtension().EqualsNC( extension ) )
				{
					// Initialize from path only, no loading
					BaseSoftHandle newHandle( valueString );
					* ( BaseSoftHandle* )( data ) = (BaseSoftHandle&) newHandle;
					return true;
				}
			}
		}
	}

	// Not set
	return false;
}

Bool CRTTISoftHandleType::DebugValidate( const void* ) const
{
	return true;
}

void* CRTTISoftHandleType::GetPointed( void* pointerData ) const
{
	BaseSoftHandle& handle = * ( BaseSoftHandle* )( pointerData );
	return handle.Get();
}

CClass* CRTTISoftHandleType::GetPointedType() const
{
	return static_cast< CClass* >( m_pointedType );
}

CPointer CRTTISoftHandleType::GetPointer( const void* data ) const
{
	const BaseSoftHandle& handle = *(const BaseSoftHandle*)( data );
	return CPointer( handle.Get() ); // it's a CResource
}

void CRTTISoftHandleType::SetPointer( void* data, const CPointer & ptr ) const
{
	BaseSoftHandle& handle = * (BaseSoftHandle*)( data );	
	handle = BaseSoftHandle( Cast< CResource >( ptr.GetObjectPtr() ) );
}
