/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "hashmap.h"
#include "property.h"

/// Forward declaration
struct deja_type;
class CName;
class CVariant;
class CObject;
class CDefaultValue;

/// Class info flags
enum EClassFlags
{
	CF_Abstract							= FLAG( 0 ),		//!< Class is abstract, no instance of it can be created
	CF_Native							= FLAG( 1 ),		//!< Class is defined in C++
	CF_Scripted							= FLAG( 2 ),		//!< Class has definition in script
	CF_Exported							= FLAG( 3 ),		//!< Class definition has been exported to C++ code
	CF_State							= FLAG( 4 ),		//!< Class is a state class
	CF_NoDefaultObjectSerialization		= FLAG( 5 ),		//!< Don't compare properties to default object on serialize
	CF_AlwaysTransient					= FLAG( 7 ),		//!< NEVER save or load objects of this class to ANY storage
	CF_EditorOnly						= FLAG( 8 ),		//!< Class and all derived classes should be used in editor only
	CF_UndefinedFunctions				= FLAG( 9 ),		//!< This class has one or more undefined functions
	CF_StateMachine						= FLAG( 10 ),		//!< Class is allowed to have states (set by scripts checked by script compiler only)
};

/// Casting from type of one class to the type of other class
template< class _SrcType, class _DestType >
RED_INLINE void* ClassCast( void* obj )
{
	return static_cast< _DestType* >( static_cast< _SrcType* >( obj ) );
}

/// Custom class serializer function type
typedef Bool (*TCustomClassSerializer)( IFile& file, void* data );

/// Class definition
class CClass : public IRTTIType
{
public:
	struct PropInfo
	{
		const IRTTIType*		m_type;
		Uint32					m_offset;

#ifndef RED_FINAL_BUILD
		CProperty*				m_prop;
		const AnsiChar*			m_name;
#endif
	};

	typedef TDynArray< CProperty*, MC_RTTI >			TPropertyList;	
	typedef TDynArray< PropInfo, MC_RTTI >				TDestroyPropList;
	typedef TDynArray< PropInfo, MC_RTTI >				TCreatePropList;
	typedef TDynArray< CFunction*, MC_RTTI >			TFunctionList;
	typedef TDynArray< CDefaultValue*, MC_RTTI >		TDefaultValuesList;
	typedef TDynArray< CClass*, MC_RTTI >				TStateClassList;
	typedef THashMap< CName, const CFunction*, DefaultHashFunc< CName >, DefaultEqualFunc< CName >, MC_RTTI > TFunctionMap;

	//! Invalid class index
	static const Int32 INVALID_CLASS_INDEX = -1;

	//! Get class index, -1 for non object classes
	RED_INLINE Int32 GetClassIndex() const { return m_classIndex; }

	//! Get class index of our last derived class
	RED_INLINE Int32 GetLastChildClassIndex() const { return m_lastChildClassIndex; }

	//! Get the n-th base class
	RED_INLINE CClass* GetBaseClass() const { return m_baseClass.m_parentClass ; }

	//! Do we have base class ?
	RED_INLINE Bool HasBaseClass() const { return (m_baseClass.m_parentClass != nullptr); }

	//! UpCast to base class for n-th class
	RED_INLINE void* UpCastUnsafe( void* ptr ) const { return m_baseClass.m_castToParentFunction( ptr ); }

	//! DonwCast from n-th base class
	RED_INLINE void* DownCastUnsafe( void* ptr ) const { return m_baseClass.m_castFromParentFunction( ptr ); }

	//! Is this class abstract
	RED_INLINE Bool IsAbstract() const { return ( m_flags & CF_Abstract ) != 0; }

	//! Is this class scripted ?
	RED_INLINE Bool IsScripted() const { return ( m_flags & CF_Scripted ) != 0; }

	//! Is this a native class ( C++ ) ?
	RED_INLINE Bool IsNative() const { return ( m_flags & CF_Native ) != 0; }

	//! Is this class a state in state machine ?
	RED_INLINE Bool IsState() const { return ( m_flags & CF_State ) != 0; }

	//! Is this class exported to script code.
	RED_INLINE Bool IsExported() const { return ( m_flags & CF_Exported ) != 0; }

	//! Is this class always transient ?
	RED_INLINE Bool IsAlwaysTransient() const { return ( m_flags & CF_AlwaysTransient ) != 0; }

	//! Do we have to compare properties to default object on serialize
	RED_INLINE Bool IsDefaultObjectSerialization() const { return ( m_flags & CF_NoDefaultObjectSerialization ) == 0; }	

	//! Is this class editor only
	RED_INLINE Bool IsEditorOnly() const { return ( m_flags & CF_EditorOnly ) != 0; }	

	//! Is this a CObject class (a lot faster than IsA< CObject >() )
	RED_INLINE Bool IsObject() const { return m_isObject; }

	//! Is this an ISerializable class (a lot faster than IsA< ISerializable >() )
	RED_INLINE Bool IsSerializable() const { return m_isSerializable; }

	//! Is this an IScriptable class (a lot faster than IsA< ISerializable >() )
	RED_INLINE Bool IsScriptable() const { return m_isScriptable; }

	//! Is this a state machine class
	RED_INLINE Bool IsStateMachine() const { return ( m_flags & CF_StateMachine ) != 0; }

	//! Is this class incomplete?
	RED_INLINE Bool HasUndefinedScriptFunctions() const { return ( m_flags & CF_UndefinedFunctions ) != 0; }	

	//! Get size of script data for this class
	RED_INLINE Uint32 GetScriptDataSize() const { return m_scriptSize; }

	//! Get state machine class
	RED_INLINE CClass* GetStateMachineClass() const { return m_stateMachineClass; }

	//! Get state classes
	RED_INLINE const TStateClassList& GetStateClasses() const { return m_stateClasses; }

	//! Get the name of the state this class represents
	RED_INLINE const CName GetStateName() const { return m_stateName; }

	//! Get number of alive objects from this class (not thread safe)
	RED_INLINE const Uint32 GetNumAllocatedObjects() const { return m_numAllocatedObjects; }

	//! Get class user data pointer
	RED_INLINE void* GetUserData() const { return m_userData; }

	//! Set class user data
	RED_INLINE void SetUserData( void* userData ) { m_userData = userData; }

public:	
	CClass( const CName& name, Uint32 size, Uint32 flags );
	~CClass();
	
	virtual TMemSize		GetInternalMemSize( const void* ) const { return 0; }

	void					ReuseScriptStub( Uint32 flags );

	Bool					IsBasedOn( const CClass *parentClass ) const;

	void					SetCustomSerializer( TCustomClassSerializer serializer );

	template< class _CurrentType, class _ParentType >
	void					AddParentClass();

	void					AddParentClass( CClass* baseClass );

	void					AddProperty( CProperty *property );

	void					AddFunction( CFunction* function );

	void					AddPropertyBinding( const CName propertyName, const CName binding );

	void					GetProperties( TDynArray< CProperty* > &properties ) const;

	template < class _F >
	void					IterateProperties( _F& f ) const;

	const TPropertyList&	GetCachedProperties( ) const;

	const TDestroyPropList&	GetCachedScriptPropertiesToDestroy( Bool& outIsValid ) const;

	const TFunctionList&	GetLocalFunctions( ) const { return m_localFunctions; }

	const TPropertyList&	GetLocalProperties() const { return m_localProperties; }

	void					RecalculateCachedProperties( const Bool force = false ) const;

	void					RecalculateCachedScriptPropertiesToDestroy();

	void					RecalculateFunctions( );

	Bool					RecalculateGCCachedProperties( ) const;

	void					ClearScriptData();

	void					ClearProperties();

	virtual void			CreateDefaultObject();

	virtual void			DestroyDefaultObject();
	
	void					MarkAsScripted();

	void					MarkAsIncomplete();

	void					MarkAsExported();

	void					MarkAsStateMachine();

	void					SetAutoStateName( const CName autoStateName );

	const CName				GetAutoStateName() const;

	void					SetAlignment( const Uint32 alignment );

	virtual Bool 			NeedsCleaning();

	void					AddState( CName stateName, CClass* stateClass );

	CClass*					FindStateClass( CName name ) const;

	CProperty*				FindProperty( const CName &name ) const;

	CName					FindPropertyBinding( const CName name ) const;

	CName					FindLocalPropertyBinding( const CName name ) const;

	const CFunction*		FindFunction( const CName& name ) const;

	const CFunction*		FindFunctionNonCached( const CName& name ) const;

	const Bool				HasStateClasses() const;

	void					CacheClassFlags() const;

	template< class _Type > Bool IsA() const;

	Bool					IsA( const CClass *otherClass ) const;
	Bool					IsA_OldTest( const CClass *otherClass ) const;

	template< class _Type > Bool DynamicIsA() const; // always do a class hierarchy walk - should be used only in script compilation/RTTI class tree building

	Bool					DynamicIsA( const CClass *otherClass ) const; // always do a class hierarchy walk - should be used only in script compilation/RTTI class tree building

	// cast up the hierarchy - you have object 'obj', of class 'this' and want to cast it to class 'destClass'
	void*					CastTo( const CClass *destClass, void *obj ) const;

	// cast down the hierarchy - you have object 'obj', of class 'this' (you can always get it, GetClass is virtual when necessary)
	// but you point it with pointer to 'srcClass'
	void*					CastFrom( const CClass *srcClass, void *obj ) const;

	// cast up the hierarchy - you have object 'obj', of class 'this' and want to cast it to class 'destClass'
	template< class _DestType > _DestType*	CastTo( void *obj ) const;

	// allocate memory buffer for object, sets the m_class field for scripted objects, can optionally call the contructor (via Construct)
	void*					CreateObject( Uint32 sizeCheck, const Bool callConstruct = true, void* inMemory = nullptr ) const;

	// free object memory buffer, calls the destructor (via Destruct)
	void					DestroyObject( void *mem, const Bool callDestructor = true, const Bool freeMemory = true ) const;

	// create object - allocated memory (via native CreateObject) and calls constructor
	template< class _Type > _Type* CreateObject()  const;

	// delete object - calls destructor and frees memory (via DestroyObjectMemory)
	template< class _Type > void DestroyObject( _Type *obj, const Bool callDestructor = true ) const;

	// delete object - dynamically typed (gets the runtime class from object first)
	template< class _Type > void DestroyObjectRuntime( _Type *obj, const Bool callDestructor = true ) const;

	template< class _Type > _Type* GetDefaultObject() { return static_cast<_Type*>( CastTo( _Type::GetStaticClass(), GetDefaultObjectImp() ) ); }

	void					ApplyDefaultValues( void* object, Bool createInlinedObjects, Bool applyBaseClassDefaults ) const;

	void					ApplyDefaultValues( IScriptable* owner, void* object, Bool createInlinedObjects, Bool applyBaseClassDefaults ) const;

	void					AddDefaultValue( CDefaultValue* defaultValue );

	Bool					DeepCompare( const void* data1, const void* data2 ) const;

	// get memory class we are allocating memory for this object
	virtual EMemoryClass	GetMemoryClass() const = 0;

	// get memory class we are allocating memory for this object
	virtual EMemoryPoolLabel GetMemoryPool() const = 0;

public:
	virtual const CName&	GetName() const { return m_name; }
	
	virtual Uint32			GetAlignment() const { return m_alignment; }

	virtual Uint32			GetSize() const { return m_size; }

	virtual ERTTITypeType	GetType() const { return RT_Class; }

	virtual void			Construct( void *mem, Bool isDefaultObject ) const = 0;

	virtual void			Construct( void *mem ) const = 0;

	virtual void			Destruct( void *mem ) const = 0;

	virtual Bool 			Compare( const void* data1, const void* data2, Uint32 flags ) const = 0;

	virtual void 			Copy( void* dest, const void* src ) const = 0;
	
	virtual void 			Clean( void* data ) const = 0;

	virtual Bool 			Serialize( IFile& file, void* data ) const;

	Bool 					DefaultSerialize( IFile& file, void* data ) const;

	virtual Bool 			ToString( const void* object, String& valueString ) const;

	virtual Bool 			FromString( void* object, const String& valueString ) const;	

	virtual void*			GetDefaultObjectImp() const = 0;

	virtual void			RecalculateClassDataSize();

	virtual Uint32			CalcScriptClassAlignment() const;

	virtual Bool			ValidateLayout() const;

	virtual Bool			NeedsGC();

	const TPropertyList&	GetCachedGCProperties( ) const;

	// During serialization, if we read in a property that does not exist in the current class (either different name
	// or the same name as an existing property, but different type and no way to convert), this will be called, allowing
	// a class to translate the property. It is called after all known properties have been serialized. The order in which
	// unknown properties are processed depends on their ordering in the file, so should not be relied on.
	// Return true if the property was handled, false otherwise.
	virtual Bool			OnReadUnknownProperty( void*, const CName&, const CVariant& ) const { return false; }

public:
	//! Serialize a differential object data - requires a valid object template (or NULL).
	//! Serialization can be done agains _ANY_ template now as long as it can be (kind of) restored back.
	//! By default the serialization is and should be done agains the class'es default object data (which is a constructor's default values)
	//! Name of the method was changed to reflect the slight change in the functinality.
	//! Note that the owner is now a ISerializable interface, not a CObject
	Bool SerializeDiff( class ISerializable* owner, IFile& file, void* data, const void* defaultData, CClass* defaultDataClass ) const;

	//! Serialize a GC only properties (way faster than full serialization, requires precached list)
	Bool SerializeGC( IFile& file, void* data ) const;

private:

	// class serialization flags
	enum ESerializationFlag
	{
		eSerializationFlag_CookedStream   = FLAG( 2 ),   // cooked serialization stream data
	};

	// handle serialization flags saving/loading
	void HandleSerializationFlags( IFile& file, Uint8& flags ) const;

	// serialization functions for normal stuff
	Bool WritePropertyList( class ISerializable* owner, IFile& file, void* data, const void* defaultData, CClass* defaultDataClass ) const;
	Bool ReadPropertyList( class ISerializable* owner, IFile& file, void* data ) const;

	//! Save/Load object content using serialization stream
	Bool WriteDataStream( class ISerializable* owner, IFile& file, const void* data, const void* defaultData, CClass* defaultDataClass ) const;
	Bool ReadDataStream( class ISerializable* owner, IFile& file, void* data ) const;

protected:
	typedef void* (*tCastFunction)( void * );
	typedef Red::Threads::AtomicOps::TAtomic32 ThreadSafeCounter;

	// allocate memory from memory class and pool related to this class, implemented in native classes only
	virtual void* AllocateRelatedMemory( Uint32 size, Uint32 alignment ) const = 0;

	// free memory from memory class and pool related to this class, implemented in native classes only
	virtual void FreeRelatedMemory( void* mem ) const = 0;
	
	// Allocate / Free typed class.
	virtual void* AllocateClass() const = 0;
	virtual void FreeClass( void * mem ) const = 0;

	struct SParentClassDesc
	{		
		CClass*			m_parentClass;
		tCastFunction	m_castToParentFunction;
		tCastFunction	m_castFromParentFunction;

		SParentClassDesc()
			: m_parentClass( NULL )
			, m_castToParentFunction( NULL )
			, m_castFromParentFunction( NULL )
		{
		}
	};

	struct SAutoBindingInfo
	{
		CName		m_propertyName;
		CName		m_bindingInformation;
	};

	typedef TDynArray< SAutoBindingInfo, MC_RTTI >		TPropertyAutoBinding;

	Int16							m_classIndex;							//!< Local runtime class index, used instead of class map
	Int16							m_lastChildClassIndex;					//!< Index of the last child class (the same as class index if there are no child classes)
	SParentClassDesc				m_baseClass;							//!< Base class + casting functions
	mutable ThreadSafeCounter		m_numAllocatedObjects;					//!< Number of live object created from this class

	CName							m_name;									//!< Name of the class
	TPropertyList					m_localProperties;						//!< Class local properties
	TPropertyAutoBinding			m_localAutoBinding;						//!< Class local auto binding information
	TFunctionList					m_localFunctions;						//!< Native & script functions defined in this class
	TDefaultValuesList				m_defaultValues;						//!< Default values defined at this class
	Uint32							m_size;									//!< C++ Size of class
	Uint32							m_scriptSize;							//!< Script Size of class ( 0 for native classes )
	Uint32							m_flags;								//!< Class flags	
	Uint32							m_alignment;							//!< Native (c++) alignment of the class itself
	mutable TFunctionMap			m_allFunctionsMap;						//!< Cached map of all functions visible inside the class
	TStateClassList					m_stateClasses;							//!< Classes of states associated with state machine represented by this class
	CClass*							m_stateMachineClass;					//!< State machine class, not null for states
	CName							m_stateName;							//!< Name of state this class represents
	TCustomClassSerializer			m_customSerializer;						//!< Optional class-specific serializer
	CName							m_autoStateName;						//!< For state machines: the initial state state machine is spawned in
	
	void*							m_userData;								//!< Generic user data pointer (used by some systems)

	mutable TPropertyList			m_cachedProperties;						//!< Cached properties of class
	mutable TPropertyList			m_propertiesToDestroy;					//!< Explicit list of properties we need to call destroy on
	mutable TPropertyList			m_cachedGCProperties;					//!< Cached properties of class that needs GC cleanup
	TDestroyPropList				m_cachedScriptPropertiesToDestory;		//!< Cached script properties that should be destroyed with the class

	mutable Uint8					m_arePropertiesCached:1;				//!< Is property cache (only of current class) up to date
	mutable Uint8					m_needsCleaningWasChecked:1;			//!< Was the cleaning flag checked for this class
	mutable Uint8					m_areGCPropertiesCached:1;				//!< Is GC property cache up to date
	mutable Uint8					m_areScriptPropertiesToDestroyCached:1;	//!< Is the list of script properties to destroy cached (only of current class) up to date
	mutable Uint8					m_isObject:1;							//!< This is a CObject class (cached for performance reasons)
	mutable Uint8					m_isSerializable:1;						//!< This is an ISerializable class (cached for performance reasons)
	mutable Uint8					m_isScriptable:1;						//!< This is an IScriptable class (cached for performance reasons)

	friend class CRTTISystem;
	friend class CRTTISerializer;
	friend class CObjectHandleSystem;
	friend class CDiscardList;
	friend class CScriptedClass;
	friend class IScriptable;
	friend class IDependencyLoaderMemoryAllocator;
};

template< class _Type >
RED_INLINE Bool CClass::IsA() const
{
	const CClass *testedClass = _Type::GetStaticClass();
	return IsA( testedClass );
}

template< class _Type >
RED_INLINE Bool CClass::DynamicIsA() const
{
	const CClass *testedClass = _Type::GetStaticClass();
	return DynamicIsA( testedClass );
}

// Function to support 'IsA' feature for non-CObject classes
template< class _DestType, class _RTTIType >
RED_INLINE Bool IsType( const _RTTIType* check )
{
	if ( check == NULL )
	{
		return false;
	}
	else
	{
		return check->GetClass()->template IsA< _DestType >();
	}
}

// Function to support 'IsExactlyA' feature for non-CObject classes
template< class _DestType, class _RTTIType >
RED_INLINE Bool IsExactlyType( const _RTTIType* check )
{
	if ( check == NULL )
	{
		return false;
	}
	else
	{
		return check->GetClass() == _DestType::GetStaticClass();
	}
}

template< class _Type >
_Type* CClass::CreateObject() const
{
	const Bool callConstructor = true; // we want to call constructor for a general call like this
	void *mem = CreateObject( sizeof(_Type), callConstructor );
	CClass *destTypeClass = _Type::GetStaticClass();
	return static_cast< _Type* >( CastTo( destTypeClass, mem ) );
}

template< class _Type >
void CClass::DestroyObject( _Type *obj, const Bool callDestructor ) const
{
	CClass *srcTypeClass = _Type::GetStaticClass();
	void* thisObject = CastFrom( srcTypeClass, obj );
	DestroyObject( thisObject, callDestructor ); 
}

template< class _Type >
void CClass::DestroyObjectRuntime( _Type *obj, const Bool callDestructor ) const
{
	if ( obj )
	{
		const CClass* srcTypeClass = obj->GetClass();
		void* thisObject = CastFrom( srcTypeClass, obj );
		DestroyObject( thisObject, callDestructor ); 
	}
}

template< class _CurrentType, class _ParentType >
void CClass::AddParentClass()
{
	CClass *parentClass = _ParentType::GetStaticClass();
	if ( !parentClass )
	{
		WARN_CORE( TXT( "Trying to set class parent class for %s - parent class is not registered in RTTI system!" ), GetName().AsString().AsChar() );
		return;
	}

	// Make sure number of base classer per class is hold
	ASSERT( m_baseClass.m_parentClass == nullptr );

	// Add new class definition
	m_baseClass.m_parentClass = parentClass;
	m_baseClass.m_castToParentFunction = &ClassCast< _CurrentType, _ParentType >;
	m_baseClass.m_castFromParentFunction = &ClassCast< _ParentType, _CurrentType >;
}

template < class _F >
RED_INLINE void CClass::IterateProperties( _F& f ) const
{
	if ( HasBaseClass() )
	{
		GetBaseClass()->IterateProperties( f );
	}

	for ( auto it = m_localProperties.Begin(), end = m_localProperties.End(); it != end; ++it )
	{
		f( *it );
	}
}

RED_INLINE Bool CClass::IsA( const CClass* testedClass ) const
{
	const Bool newTest = testedClass && (m_classIndex >= testedClass->m_classIndex && m_classIndex <= testedClass->m_lastChildClassIndex);

#if !defined( NO_EDITOR )

	const Bool oldTest = IsA_OldTest( testedClass );
	RED_ASSERT( newTest == oldTest,  TXT( "IsA test failed. Please tell Dex!!!!" ) );
	RED_UNUSED( newTest );
	return oldTest;

#else 

	RED_FATAL_ASSERT( newTest == IsA_OldTest( testedClass ),  "IsA test failed. Please tell Dex!!!!" );
	return newTest;

#endif

}

// Helper to get class id from type
template <class T>
RED_INLINE CClass* ClassID()
{
	return T::GetStaticClass();
}

// If the source class we ask to cast from is deriving from IScriptable, we bypass the virtual function call to call directly GetLocalClass.
template< Bool IsIScriptable >
struct RTTIClassSelector
{
	template< typename T >
	static CClass * Get( T * object ) { return object->GetLocalClass(); } 

	template< typename T >
	static const CClass * Get( const T * object ) { return object->GetLocalClass(); } 
};

template<>
struct RTTIClassSelector< false >
{
	template< typename T >
	static CClass * Get( T * object ) { return object->GetClass(); } 

	template< typename T >
	static const CClass * Get( const T * object ) { return object->GetClass(); } 
};

// Non-const version
template< class _DestType, class _SrcType >
RED_FORCE_INLINE _DestType* Cast( _SrcType *srcObj )
{
	if( srcObj )
	{
		typedef RTTIClassSelector< std::is_base_of< IScriptable, _SrcType >::value > ClassSelector;
		return ClassSelector::Get( srcObj )->IsA( ClassID< _DestType >() ) ? static_cast< _DestType* >( srcObj ) : nullptr;
	}

	return nullptr;	
}

// Const version
template< class _DestType, class _SrcType >
RED_FORCE_INLINE const _DestType* Cast( const _SrcType *srcObj )
{
	if( srcObj )
	{
		typedef RTTIClassSelector< std::is_base_of< IScriptable, _SrcType >::value > ClassSelector; 
		return ClassSelector::Get( srcObj )->IsA( ClassID< _DestType >() ) ? static_cast< const _DestType* >( srcObj ) : nullptr;
	}

	return nullptr;	
}

template< class _DestType > 
RED_INLINE _DestType* CClass::CastTo( void *obj ) const
{
	CClass *destTypeClass = _DestType::GetStaticClass();
	return static_cast<_DestType*>( CastTo( destTypeClass, obj ) );
}

template< class _DestType >
RED_INLINE _DestType* Cast( const CClass* srcClass, void *srcObject )
{
	CClass *destTypeClass = _DestType::GetStaticClass();	
	return static_cast<_DestType*>( srcClass->CastTo( destTypeClass, srcObject ) );
}

template< class _DestType, class _SrcType >
RED_INLINE _DestType* SafeCast( _SrcType *srcObj )
{
	ASSERT( srcObj && srcObj->template IsA< _DestType >() );
	return Cast<_DestType>( srcObj );
}

// abstract class - asserted implementations of all operations
class CAbstractClass : public CClass
{
public:
	CAbstractClass( const CName& name, Uint32 size, Uint32 flags );

	virtual void* AllocateRelatedMemory( Uint32, Uint32 ) const override final;

	virtual void FreeRelatedMemory( void* ) const override final;

	virtual void * AllocateClass() const override final;

	virtual void FreeClass( void * ) const override final;

	virtual EMemoryClass GetMemoryClass() const;

	virtual EMemoryPoolLabel GetMemoryPool() const;

	virtual void Construct( void*, Bool ) const;

	virtual void Construct( void* ) const;

	virtual void Destruct( void* ) const;

	virtual Bool Compare( const void*, const void*, Uint32 ) const;

	virtual void Copy( void*, const void* ) const;

	virtual void Clean( void* ) const;

	virtual void* GetDefaultObjectImp() const;
};

// scripted class
class CScriptedClass : public CClass
{
private:
	mutable void*				m_defaultObject;

public:
	CScriptedClass( const CName& name, Uint32 flags );

	virtual EMemoryClass GetMemoryClass() const;

	virtual EMemoryPoolLabel GetMemoryPool() const;

	virtual void* AllocateRelatedMemory( Uint32 size, Uint32 alignment ) const override final;

	virtual void FreeRelatedMemory( void* mem ) const override final;

	virtual void* AllocateClass() const override final;

	virtual void FreeClass( void * mem ) const override final;

	virtual void Construct( void *mem, Bool isDefaultObject ) const;
	
	virtual void Construct( void *mem ) const;

	virtual void Destruct( void *mem ) const;

	virtual Bool Compare( const void* data1, const void* data2, Uint32 ) const;

	virtual void Copy( void* dest, const void* src ) const;

	virtual void Clean( void* data ) const;

	virtual void* GetDefaultObjectImp() const;

	virtual void DestroyDefaultObject();

	virtual void RecalculateClassDataSize();

private:
	void ConstructProperties( void* mem, Bool isDefaultObject ) const;
};


