/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "scriptable.h"

// Forward declarations
class CClass;
class CMemoryStats;
class CScriptStackFrame;
class CStateMachine;

#define INVALID_OBJECT_INDEX		0xFFFFFFFF

// Objects will be default-aligned to 16 byte boundaries.
// This is to ensure any SSE types are aligned properly
#define DEFAULT_OBJECT_ALIGNMENT	16

// Object class
#define DECLARE_RTTI_OBJECT_CLASS( className )																						\
	friend class className##ClassBuilder;																							\
	private: static CClass* sm_classDesc; public:																					\
	void* operator new( size_t size ) { return GetStaticClass()->CreateObject( (const Uint32)size, false ); }						\
	void* operator new( size_t, void* mem ) { return mem; }																			\
	void operator delete( void *, void * ){ RED_FATAL_ASSERT(0, "This should never happen, we don't use exceptions!"); }            \
	void operator delete( void* /*mem*/, size_t /*size*/ ) { RED_FATAL( "Do not use delete on CObjects!" ); }						\
	enum MemoryContextInfo { MemoryClass = CurrentMemoryClass };																	\
	typedef CurrentMemoryPool MemoryPool;																							\
	friend class TTypedClass<className, (EMemoryClass)className::MemoryClass, className::MemoryPool>;							\
	static CClass* GetStaticClass() { return sm_classDesc; }

// Abstract class factory
#define DECLARE_ENGINE_ABSTRACT_CLASS( _class, _baseclass )													\
	DECLARE_RTTI_OBJECT_CLASS( _class )																		\
	public: typedef _baseclass TBaseClass;																	\
	private: _class& operator=( const _class& ) { return* this; }											\
	public: CObject* ToObject() { return static_cast< CObject* >( this ); }									\
	public: friend IFile &operator<<( IFile &ar, _class *&c ) { return ar << *(CObject **)&c; }

// Abstract class factory with custom memory class
#define DECLARE_ENGINE_ABSTRACT_CLASS_WITH_ALLOCATOR( _class, _baseclass, _memoryClass )					\
	public: using _baseclass::CurrentMemoryPool;															\
	public: enum ClassMemoryContext { CurrentMemoryClass = _memoryClass  };									\
	DECLARE_RTTI_OBJECT_CLASS( _class )																		\
	public: typedef _baseclass TBaseClass;																	\
	private: _class& operator=( const _class& ) { return* this; }											\
	public: CObject* ToObject() { return static_cast< CObject* >( this ); }									\
	public: friend IFile &operator<<( IFile &ar, _class *&c ) { return ar << *(CObject **)&c; }

// Class factory
#define DECLARE_ENGINE_CLASS( _class, _baseclass, _unusedZero )												\
	public: using _baseclass::CurrentMemoryClass; using _baseclass::CurrentMemoryPool;						\
	DECLARE_RTTI_OBJECT_CLASS( _class )																		\
	public: typedef _baseclass TBaseClass;																	\
	private: _class& operator=( const _class& ) { return* this; }											\
	public: CObject* ToObject() { return static_cast< CObject* >( this ); }									\
	public: friend IFile &operator<<( IFile&ar, _class *&c ) { return ar << *(CObject **)&c; }

// Class factory with optional allocator
#define DECLARE_ENGINE_CLASS_WITH_ALLOCATOR( _class, _baseclass, _memoryClass )								\
	public: using _baseclass::CurrentMemoryPool;															\
	public: enum ClassMemoryContext { CurrentMemoryClass = _memoryClass  };									\
	DECLARE_RTTI_OBJECT_CLASS( _class )																		\
	public: typedef _baseclass TBaseClass;																	\
	private: _class& operator=( const _class& ) { return* this; }											\
	public: CObject* ToObject() { return static_cast< CObject* >( this ); }									\
	public: friend IFile &operator<<( IFile&ar, _class *&c ) { return ar << *(CObject **)&c; }

// Implement class factory
#define IMPLEMENT_ENGINE_CLASS( _class ) \
	CClass* touchClass##_class() { return _class::GetStaticClass(); }	\
	IMPLEMENT_RTTI_CLASS( _class )

// Implement class factory
#define IMPLEMENT_ENGINE_CLASS_NAMESPACE( _class, _namespace ) \
	CClass* touchClass##_class() { return _namespace::_class::GetStaticClass(); }	\
	IMPLEMENT_RTTI_CLASS( _class )

// Helper macros for old stuff
#define NO_DEFAULT_CONSTRUCTOR( _class )	\
	protected: _class() {};

#define BEGIN_FLAGS( name ) \
	enum name	\
	{			\
	name##_MINVALUE = 0,


#define BEGIN_FLAGS_INHERIT( name, base_name ) \
	enum name	\
	{			\
	name##_MINVALUE = base_name##_MAXVALUE,

#define END_FLAGS( name, topvalue )												\
	name##_MAXVALUE = topvalue + name##_MINVALUE								\
	};																			\
	typedef char ERR_Assert##__LINE__ [ ( name##_MAXVALUE <= 32 ) ? 1 : -1]; //compile assert

#define FLAGS( name, x )  ( 1 << ( x +  name##_MINVALUE) )

// Object flags
BEGIN_FLAGS( EObjectFlags )
	OF_Finalized		= FLAGS( EObjectFlags, 1 ),		// Object had its OnFinalize method called
	OF_Root				= FLAGS( EObjectFlags, 2 ),		// Object belongs to the root set 
	OF_Inlined			= FLAGS( EObjectFlags, 3 ),		// Object created via inlined object property
	OF_Scripted			= FLAGS( EObjectFlags, 4 ),		// Object has scripts attached
	OF_Discarded		= FLAGS( EObjectFlags, 5 ),		// Object has been discarded
	OF_Transient		= FLAGS( EObjectFlags, 6 ),		// Object is not saved to disk
	OF_Referenced		= FLAGS( EObjectFlags, 7 ),		// Template component property (component is part of included template)
	OF_Highlighted		= FLAGS( EObjectFlags, 8 ),		// Highlighted (editor only)
	OF_DefaultObject	= FLAGS( EObjectFlags, 9 ),		// Default object of class
	OF_ScriptCreated	= FLAGS( EObjectFlags, 10 ),	// This object was created by script's new
	OF_HasHandle		= FLAGS( EObjectFlags, 11 ),	// Object has some THandle pointing to it
	OF_Unused			= FLAGS( EObjectFlags, 12 ),
	OF_WasCooked		= FLAGS( EObjectFlags, 13 ),	// This object was cooked
	OF_UserFlag			= FLAGS( EObjectFlags, 14 ),	// User flag from some crap
	OF_SerializationMask = OF_Inlined | OF_WasCooked, // Only the flags in this mask will be saved into the export table
END_FLAGS( EObjectFlags, 15 )

template< class T >
RED_INLINE Bool ISerializable::IsA() const
{
	return GetClass()->IsA( ClassID< T >() );
}

template< class T >
RED_INLINE Bool ISerializable::IsExactlyA() const
{
	return GetClass() == ClassID< T >();
}

template< class T >
RED_INLINE Bool IScriptable::IsA() const
{
	return GetLocalClass()->IsA( ClassID< T >() );
}

template< class T >
RED_INLINE Bool IScriptable::IsExactlyA() const
{
	return GetLocalClass() == ClassID< T >();
}

/// Property setter for some types
template< typename BaseClass, typename T >
class TObjectPropertySetterRef : public IPropertySetter
{
public:
	//! The function pointer type for the setter function
	typedef void ( BaseClass::*TSetterFunction )( const T& value );

protected:
	TSetterFunction		m_function;		//!< Setter function itself

public:
	//! Constructor
	RED_INLINE TObjectPropertySetterRef( TSetterFunction func, const T* ptr )
		: m_function( func )
	{
		RED_UNUSED( ptr );
		ASSERT( m_function );
	}

	//! Set the value of property
	virtual void SetValue( void* context, IRTTIType* type, const void* valueData ) const
	{
		ASSERT( valueData );
		ASSERT( type );
		ASSERT( type->GetSize() == sizeof(T) );
		ASSERT( type->GetName() == ::GetTypeName<T>() );

		// Get the object context
		BaseClass* object = ( BaseClass* ) context;
		ASSERT( object );

		// Set value
		const T* typedValueData = ( const T* ) valueData;
		(object->*m_function)( *typedValueData );
	}
};

/// Property setter for some types
template< typename BaseClass, typename T >
class TObjectPropertySetter : public IPropertySetter
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	//! The function pointer type for the setter function
	typedef void ( BaseClass::*TSetterFunction )( T value );

protected:
	TSetterFunction		m_function;		//!< Setter function itself

public:
	//! Constructor
	RED_INLINE TObjectPropertySetter( TSetterFunction func, const T* )
		: m_function( func )
	{
		ASSERT( m_function );
	}

	//! Set the value of property
	virtual void SetValue( void* context, IRTTIType* type, const void* valueData ) const
	{
		ASSERT( valueData );
		ASSERT( type );
		ASSERT( type->GetSize() == sizeof(T) );
		ASSERT( type->GetName() == ::GetTypeName<T>() );

		RED_UNUSED( type );

		// Get the object context
		BaseClass* object = ( BaseClass* ) context;
		ASSERT( object );

		// Set value
		const T* typedValueData = ( const T* ) valueData;
		(object->*m_function)( *typedValueData );
	}
};

// Factory template for creating property setter
template< typename BaseClass >
class PropertySetterFactory
{
public:
	template< class T >
	static IPropertySetter* CreateSetter( typename TObjectPropertySetter< BaseClass, T >::TSetterFunction func, const T* ptr )
	{
		return new TObjectPropertySetter< BaseClass, T >( func, ptr );
	}

	template< class T >
	static IPropertySetter* CreateSetterRef( typename TObjectPropertySetterRef< BaseClass, T >::TSetterFunction func, const T* ptr )
	{
		return new TObjectPropertySetterRef< BaseClass, T >( func, ptr );
	}
};

/************************************************/
/* Base object with RTTI and properties			*/
/************************************************/
class CObject : public IScriptable
{
public:  
	enum ClassMemoryContext { CurrentMemoryClass = MC_AllObjects };
	typedef MemoryPoolType< MemoryPool_CObjects > CurrentMemoryPool;
	DECLARE_RTTI_OBJECT_CLASS( CObject );

	friend class CObjectGC_Legacy;
	
private:
	CObject*				m_parent;						//!< Parent object in object hierarchy
	CObject*				m_previousChild;
	CObject*				m_nextChild;					//!< Link to next child
	CObject*				m_children;						//!< Child objects
	Uint32					m_index;						//!< Object index

protected:
	Uint32					m_objectFlags;					//!< Object flags

public:
	CObject();
	virtual ~CObject(); 

	// Get root for this object ( the outermost parent )
	RED_INLINE CObject* GetRoot() const;

	// Get parent object
	RED_FORCE_INLINE CObject* GetParent() const { return m_parent; }

	// Get object flags
	RED_INLINE Uint32 GetFlags() const { return m_objectFlags; }

	// Checks if object is under given parent, returns true if parent == this
	RED_INLINE Bool IsContained( CObject* parentObject ) const;

	// Checks if object has given flag set
	RED_INLINE Bool HasFlag( Uint32 flag ) const { return ( m_objectFlags & flag ) != 0; }

	// Set object flag
	RED_INLINE void SetFlag( Uint32 flag ) { m_objectFlags |= flag; }

	// Clear object flag
	RED_INLINE void ClearFlag( Uint32 flag ) { m_objectFlags &= ~flag; }

	// Is this object transient
	RED_INLINE Bool IsTransient() const { return HasFlag( OF_Transient ); }

	// Is this object inlined
	RED_INLINE Bool IsInlined() const { return HasFlag( OF_Inlined ); }

	// Is this object cooked
	RED_INLINE Bool IsCooked() const { return HasFlag( OF_WasCooked ); }

	// Fast test for being an CObject
	RED_INLINE const Bool IsObject() const override RED_FINAL { return true; }

	// Get object index (implemented only for CObjects), 0 is invalid index
	RED_INLINE const Uint32 GetObjectIndex() const override RED_FINAL { return m_index; }

	// Is object in root set
	RED_INLINE Bool IsInRootSet() const { return HasFlag( OF_Root ); }

	// Find parent of specified class moving up in object hierarchy
	template< class T > RED_INLINE T* FindParent() const;

public:
	// Add object to root set
	RED_MOCKABLE void AddToRootSet();

	// Remove object from root set
	RED_MOCKABLE void RemoveFromRootSet();

	// Set object parent, use with care
	void SetParent( CObject* object );

	// Get all children
	void GetChildren( TDynArray< CObject* >& children ) const;

	// Get object class index ( for debug shit mostly )
	Int32 GetClassIndex() const;

	// Mark resource we are in as modified
	virtual Bool MarkModified();

	// Check if object can by modified
	virtual Bool CanModify();

	// Check if object can be created manually in the editor
	virtual Bool IsManualCreationAllowed() const { return true; }

	// Discard this object, all children and all attached memory, will call OnFinalize(preorder) and destructor(postorder) on the object tree
	void Discard();

	// DO NOT USE. GC ONLY
	void DiscardNoLock(); 

	// Clone this object
	CObject* Clone( CObject* newParent, Bool cloneWithTransientObjects = true, Bool cloneWithReferencedObjects = true ) const;

	// Get all resources used by this object and its dependents
	void CollectUsedResources( TDynArray< class CResource* >& resources, Bool loadSoftHandles = false  ) const;

	// Load object configuration using config file from class and category from class name
	Bool LoadObjectConfig( const Char* category, const Char* section = NULL );

	// Save object configuration
	Bool SaveObjectConfig( const Char* category, const Char* section = NULL ) const;

public:

#if !defined(NO_DEBUG_PAGES) && !defined(NO_LOG)
	// Dump list of objects
	static void DebugDumpList();

	// Dump per class list of objects
	static void DebugDumpClassList();

	// Dump list of object for root set
	static void DebugDumpRootsetList();

	// Dump list of default object
	static void DebugDumpDefaultObjectList();
#endif

public:
	// Remenesant of old CObject hierarchy: get the serialization parent of this object
	virtual ISerializable* GetSerializationParent() const override RED_FINAL;

	// When ISerializable is deserialized an a parent is known this method is called
	virtual void RestoreSerializationParent( ISerializable* parent ) override RED_FINAL;

	// Get template that is used to instance this object
	virtual CObject* GetTemplate() const;

	// Get template default instance
	virtual const CObject* GetTemplateInstance() const;

	// Recreate object
	virtual CObject* CreateTemplateInstance( CObject* parent ) const;

	//! Get base object data to compare against when saving this object, by default it usually uses the class default object
	virtual const void* GetBaseObjectData() const;

	//! Get base object class to compare against when saving this object, by default it usually uses object's class
	virtual CClass* GetBaseObjectClass() const;

public:
	// Create new object  
	static CObject *CreateObjectStatic( CClass *baseClass, CObject *parent, Uint16 flags, Bool callConstructor );

	// Replace references to given object, returns number of references replaced
	static Uint32 ReplaceReferences( CObject* object, CObject* replaceWith );

	// Check if given object pointer is valid
	static Bool IsValidObject( CObject* object );

	// Validate object structure
	static void ValidateObjectStructure();

	// Test is given object is reachable from root set
	static Bool TestObjectReachability( CObject* object, CObject* ignoredParent );

	// Print object dependency for class
	static void PrintDependencies( const CName& className );

	// Initialize object system
	static void InitializeSystem();

public:
	// Called after object's property is changed in the editor
	virtual void OnPropertyPostChange( IProperty* property );

	// Called when object is ready to be deleted by GC
	virtual void OnFinalize();

	// Debug stuff, verify object internal structure, called from time to time in debug builds
	virtual void OnDebugValidate();

	// Called just before a capture of this object is taken prior to script compilation
	virtual void OnScriptPreCaptureSnapshot();

	// Called just after a capture of this object is taken prior to script compilation
	virtual void OnScriptPostCaptureSnapshot();

	// Called when scripts have been successfully reloaded
	virtual void OnScriptReloaded();

	//! Called when all THandles to this IReferencable are gone
	virtual void OnAllHandlesReleased();

	//! Validate handle creation for this object - NOT USED IN FINAL
	virtual Bool OnValidateHandleCreation() const;

#ifndef NO_RESOURCE_COOKING
	// Cooker entry point - prepare resource to be used by game, this method is guaranteed to be called only ONCE
	virtual void OnCook( class ICookerFramework& cooker );
#endif

	// REMOVE AFTER RESAVE
	static RED_INLINE Uint16 Remap32BitFlagsTo16Bit( Uint32 flags );

private:
	//! Link to child list
	void LinkToChildList( CObject*& list );

	//! Unlink from child list
	void UnlinkFromChildList();
	void UnlinkFromChildListNoLock();

	//! Unlink object from object map
	void UnlinkFromObjectMap();
	void UnlinkFromObjectMapNoLock();

	//! Remove the editor references to this object
	void UnlinkEditorReferenced();

private:
	void funcGetParent( CScriptStackFrame& stack, void* result );
	void funcClone( CScriptStackFrame& stack, void* result );
	void funcIsIn( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CObject );
	PARENT_CLASS( IScriptable );
	NATIVE_FUNCTION( "GetParent", funcGetParent );
	NATIVE_FUNCTION( "Clone", funcClone );
	NATIVE_FUNCTION( "IsIn", funcIsIn );
END_CLASS_RTTI();

#include "object.inl"

// TEMPSHIT
#include "objectIterator.h"
