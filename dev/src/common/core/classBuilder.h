/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "class.h"
#include "typedClass.h"
#include "propertyBuilder.h"
#include "numericLimits.h"
#include "typeName.h"

//RED_MESSAGE("FIXME>>>>>>>>>>>>>>>>>>>>>>>>>>>>>")
RED_DISABLE_WARNING_CLANG( "-Winvalid-offsetof" )
#define INTERNAL_OFFSETOF( Type, Member ) offsetof( Type, Member )

// Because WinRT projects would convert L"m_abc" + 2 into a Platform::String^
#define INTERNAL_PROPERTY_TXT( x ) (&(MACRO_TXT( x )[2]))

class CProperty;
class IClassBuilder
{
public:	
	IClassBuilder()
	{
		SRTTI::GetInstance().RegisterClassBuilder( this );
	}

	virtual ~IClassBuilder()
	{
	}

	virtual CClass* InitClass() = 0;

	virtual void DeinitClass() = 0;
};

enum ClassExtraction
{
	ClassExtratorType = 0,
};

template< Uint32 M >
struct TClassExtractor
{
};

template<>
struct TClassExtractor<0>
{
	template< class T >
	RED_INLINE static CClass* ExtractClass( const T* object )
	{
		RED_UNUSED( object );
		return T::GetStaticClass();
	}
};

namespace Red { namespace UnitTest { class Attorney; } } // ctremblay this is for mocking CObject when doing unit test.

/* RTTI struct declaration, default memory class and pool (old behaviour) */
#define DECLARE_RTTI_STRUCT( className )																																			\
														friend class className##ClassBuilder;																						\
													private:																														\
														static CClass* sm_classDesc;																								\
													public:																															\
														enum MemoryContextInfo { MemoryClass = CurrentMemoryClass };																\
														typedef CurrentMemoryPool MemoryPool;																						\
														friend class TTypedClass<className, (EMemoryClass)className::MemoryClass, className::MemoryPool>;							\
														friend class TTypedClassNoCopy<className, (EMemoryClass)className::MemoryClass, className::MemoryPool>;						\
														friend IFile &operator<<( IFile &ar, className*& c ) { ar.SerializePointer( GetStaticClass(), (void*&)c ); return ar; }		\
														void* operator new( size_t size ) { return GetStaticClass()->CreateObject( (const Uint32)size, false ); }					\
														void* operator new( size_t, void* mem )  { return mem; }																	\
														void operator delete( void *, void * ){ RED_FATAL_ASSERT(0, "This should never happen, we don't use exceptions!"); }        \
														void operator delete( void* mem ) { GetStaticClass()->DestroyObject( (className*) mem, false ); }							\
														static CClass* GetStaticClass() { return sm_classDesc; }																	\
														CClass* GetNativeClass() { return sm_classDesc; }																			\
														CClass* GetClass() const { return TClassExtractor<0>::ExtractClass( this ); }

/* It is the same as STRUCT, but allows inheritance, default memory class and pool */
#define DECLARE_RTTI_SIMPLE_CLASS( className )																																		\
														friend class className##ClassBuilder; friend class Red::UnitTest::Attorney;																								\
													private:																														\
														static CClass* sm_classDesc;																								\
													public:																															\
														enum MemoryContextInfo { MemoryClass = CurrentMemoryClass };																\
														typedef CurrentMemoryPool MemoryPool;																						\
														friend class TTypedClass<className, (EMemoryClass)className::MemoryClass, className::MemoryPool>;							\
														friend class TTypedClassNoCopy<className, (EMemoryClass)className::MemoryClass, className::MemoryPool>;						\
														void* operator new( size_t size ) { return GetStaticClass()->CreateObject( (const Uint32)size, false ); }					\
														void* operator new( size_t, void* mem ) { return mem; }																		\
														void operator delete( void *, void * ){ RED_FATAL_ASSERT(0, "This should never happen, we don't use exceptions!"); }        \
														void operator delete( void* mem ) { TClassExtractor<ClassExtratorType>::ExtractClass( (className*)mem )->DestroyObjectRuntime< className >( (className*)mem, false ); }		\
														friend IFile &operator<<( IFile &ar, className*& c ) { ar.SerializePointer( GetStaticClass(), (void*&)c ); return ar; }		\
														static CClass* GetStaticClass() { return sm_classDesc; }																	\
														virtual CClass* GetNativeClass() const { return sm_classDesc; }																\
														virtual CClass* GetClass() const { return TClassExtractor<ClassExtratorType>::ExtractClass( this ); }

/* It is the same as STRUCT, but allows inheritance, default memory class and pool */
#define DECLARE_RTTI_SIMPLE_CLASS_WITH_ALLOCATOR( className, _memoryClass )					\
		public: enum ClassMemoryContext { CurrentMemoryClass = _memoryClass  };				\
		DECLARE_RTTI_SIMPLE_CLASS( className );

/* It is the same as STRUCT, but allows inheritance, default memory class and pool */
#define DECLARE_RTTI_STRUCT_WITH_ALLOCATOR( className, _memoryClass )						\
		public: enum ClassMemoryContext { CurrentMemoryClass = _memoryClass  };				\
		DECLARE_RTTI_STRUCT( className );

/* It is the same as STRUCT, but allows inheritance, default memory class and custom pool */
#define DECLARE_RTTI_SIMPLE_CLASS_WITH_POOL( className, _memoryPool, _memoryClass )			\
		public: enum ClassMemoryContext { CurrentMemoryClass = _memoryClass };				\
		typedef MemoryPoolType< _memoryPool > CurrentMemoryPool;							\
		DECLARE_RTTI_SIMPLE_CLASS( className );

/* It is the same as STRUCT, but allows inheritance, default memory class and custom pool */
#define DECLARE_RTTI_STRUCT_WITH_POOL( className, _memoryPool, _memoryClass )				\
		public: enum ClassMemoryContext { CurrentMemoryClass = _memoryClass };				\
		typedef MemoryPoolType< _memoryPool > CurrentMemoryPool;							\
		DECLARE_RTTI_STRUCT( className );

#define ALIGN_CLASS( alignment )														\
{																						\
	m_registeredClass->SetAlignment( alignment );										\
}

#define BEGIN_CLASS_RTTI_EX( className, flags )																				\
class className##ClassBuilder : public IClassBuilder																		\
{																															\
	CClass		*m_registeredClass;																							\
public:																														\
	className##ClassBuilder()																								\
	: m_registeredClass( NULL )																								\
	{																														\
		m_registeredClass = new TTypedClass																					\
			<className, (EMemoryClass)className::MemoryClass, className::MemoryPool>										\
			( CName( TXT(#className) ), sizeof( className ), flags );														\
		className::sm_classDesc = m_registeredClass;																		\
		SRTTI::GetInstance().RegisterType( m_registeredClass );																\
	}																														\
																															\
	~className##ClassBuilder()																								\
	{																														\
		delete m_registeredClass;																							\
		m_registeredClass = NULL;																							\
		className::sm_classDesc = NULL;																						\
	}																														\
																															\
	virtual void DeinitClass()																								\
	{																														\
		m_registeredClass->DestroyDefaultObject();																			\
		m_registeredClass->ClearProperties();																				\
		SRTTI::GetInstance().UnregisterType( m_registeredClass );															\
	}																														\
																															\
	virtual CClass* InitClass()																								\
	{																														\
		typedef className tCurrClassType;																					\
		ALIGN_CLASS( __alignof( className ) )																				

#define BEGIN_CLASS_RTTI( className	)	BEGIN_CLASS_RTTI_EX( className, 0 )

#define BEGIN_NODEFAULT_CLASS_RTTI( className )														\
class className##ClassBuilder : public IClassBuilder												\
{																									\
	CClass		*m_registeredClass;																	\
public:																								\
	className##ClassBuilder()																		\
		:  m_registeredClass( NULL )																\
	{																								\
		m_registeredClass = new TTypedClass															\
		<className, (EMemoryClass)className::MemoryClass, className::MemoryPool>					\
		(																							\
			CName( TXT(#className ) ),																\
			sizeof( className ),																	\
			CF_NoDefaultObjectSerialization															\
		);																							\
		className::sm_classDesc = m_registeredClass;												\
		SRTTI::GetInstance().RegisterType( m_registeredClass );										\
	}																								\
																									\
	~className##ClassBuilder()																		\
	{																								\
		delete m_registeredClass;																	\
		m_registeredClass = NULL;																	\
		className::sm_classDesc = NULL;																\
	}																								\
																									\
	virtual void DeinitClass()																		\
	{																								\
		m_registeredClass->DestroyDefaultObject();													\
		m_registeredClass->ClearProperties();														\
		SRTTI::GetInstance().UnregisterType( m_registeredClass );									\
	}																								\
																									\
	virtual CClass* InitClass()																		\
	{																								\
		typedef className tCurrClassType;															\
		ALIGN_CLASS( __alignof( className ) )														\


#define BEGIN_ABSTRACT_CLASS_RTTI( className )															\
class className##ClassBuilder : public IClassBuilder													\
{																										\
	CClass		*m_registeredClass;																		\
public:																									\
	className##ClassBuilder()																			\
		:  m_registeredClass( NULL )																	\
	{																									\
		m_registeredClass = new CAbstractClass(  CName( TXT(#className) ), sizeof( className ), 0 );	\
		className::sm_classDesc = m_registeredClass;													\
		SRTTI::GetInstance().RegisterType( m_registeredClass );											\
	}																									\
																										\
	~className##ClassBuilder()																			\
	{																									\
		delete m_registeredClass;																		\
		m_registeredClass = NULL;																		\
		className::sm_classDesc = NULL;																	\
	}																									\
																										\
	virtual void DeinitClass()																			\
	{																									\
		m_registeredClass->DestroyDefaultObject();														\
		m_registeredClass->ClearProperties();															\
		SRTTI::GetInstance().UnregisterType( m_registeredClass );										\
	}																									\
																										\
	virtual CClass* InitClass()																			\
	{																									\
		typedef className tCurrClassType;																


#define BEGIN_NOCOPY_CLASS_RTTI( className )																		\
class className##ClassBuilder : public IClassBuilder																\
{																													\
	CClass *m_registeredClass;																						\
public:																												\
	className##ClassBuilder()																						\
		:  m_registeredClass( NULL )																				\
	{																												\
		m_registeredClass = new TTypedClassNoCopy																	\
			<className, (EMemoryClass)className::MemoryClass, className::MemoryPool>								\
			( CName( TXT(#className) ), sizeof( className ), 0 );													\
		className::sm_classDesc = m_registeredClass;																\
		SRTTI::GetInstance().RegisterType( m_registeredClass );														\
	}																												\
																													\
	~className##ClassBuilder()																						\
	{																												\
		delete m_registeredClass;																					\
		m_registeredClass = NULL;																					\
		className::sm_classDesc = NULL;																				\
	}																												\
																													\
	virtual void DeinitClass()																						\
	{																												\
		m_registeredClass->DestroyDefaultObject();																	\
		m_registeredClass->ClearProperties();																		\
		SRTTI::GetInstance().UnregisterType( m_registeredClass );													\
	}																												\
																													\
	virtual CClass* InitClass()																						\
	{																												\
		typedef className tCurrClassType;																			\
		ALIGN_CLASS( __alignof( className ) )


#define CUSTOM_SERIALIZER( serializer )													\
{																						\
	m_registeredClass->SetCustomSerializer( serializer );								\
}

#define PARENT_CLASS( className )														\
{																						\
	m_registeredClass->AddParentClass< tCurrClassType, className >();					\
}

#define ADD_PROPERTY( field )															\
	if ( currProperty )																	\
	{																					\
		m_registeredClass->AddProperty( currProperty );									\
	}																					\
	else																				\
	{																					\
	}																					\
}

#define PROPERTY( field )											\
{																	\
	CProperty* currProperty = RedPropertyBuilder::CreateProperty	\
	(																\
		m_registeredClass,											\
		INTERNAL_OFFSETOF( tCurrClassType, field ),					\
		CName( INTERNAL_PROPERTY_TXT( #field ) ),					\
		GetTypeName( static_cast< tCurrClassType* >( 0 )->field ),	\
		String::EMPTY,												\
		0,															\
		-NumericLimits<Float>::Max(),								\
		NumericLimits<Float>::Max(),								\
		String::EMPTY,												\
		false														\
	);																\
	ADD_PROPERTY( field )

#define PROPERTY_BITFIELD( field, type )							\
{																	\
	CProperty* currProperty = RedPropertyBuilder::CreateProperty	\
	(																\
		m_registeredClass,											\
		INTERNAL_OFFSETOF( tCurrClassType, field ),					\
		CName( INTERNAL_PROPERTY_TXT( #field ) ),					\
		GetTypeName< type >(),										\
		String::EMPTY,												\
		0,															\
		-NumericLimits<Float>::Max(),								\
		NumericLimits<Float>::Max(),								\
		String::EMPTY,												\
		false														\
	);																\
	ADD_PROPERTY( field )

#define PROPERTY_NAME( field, name )								\
{																	\
	CProperty* currProperty = RedPropertyBuilder::CreateProperty	\
	(																\
		m_registeredClass,											\
		INTERNAL_OFFSETOF( tCurrClassType, field ),					\
		CName( name ),												\
		GetTypeName( static_cast< tCurrClassType* >( 0 )->field ),	\
		String::EMPTY,												\
		0,															\
		-NumericLimits<Float>::Max(),								\
		NumericLimits<Float>::Max(),								\
		String::EMPTY,												\
		false														\
	);																\
	ADD_PROPERTY( field )

//////////////////////////////////////////////////////////////////////////
//
#define INTERNAL_PROPERTY_EDIT( field, hint )						\
{																	\
	CProperty* currProperty = RedPropertyBuilder::CreateProperty	\
	(																\
		m_registeredClass,											\
		INTERNAL_OFFSETOF( tCurrClassType, field ),					\
		CName( INTERNAL_PROPERTY_TXT( #field ) ),					\
		GetTypeName( static_cast< tCurrClassType* >( 0 )->field ),	\
		hint,														\
		PF_Editable,												\
		-NumericLimits<Float>::Max(),								\
		NumericLimits<Float>::Max(),								\
		String::EMPTY,												\
		false														\
	);																\
	ADD_PROPERTY( field )

#define INTERNAL_PROPERTY_EDIT_ARRAY( field, hint )					\
{																	\
	CProperty* currProperty = RedPropertyBuilder::CreateProperty	\
	(																\
		m_registeredClass,											\
		INTERNAL_OFFSETOF( tCurrClassType, field ),					\
		CName( INTERNAL_PROPERTY_TXT( #field ) ),					\
		GetTypeName( static_cast< tCurrClassType* >( 0 )->field ),	\
		hint,														\
		PF_Editable,												\
		-NumericLimits<Float>::Max(),								\
		NumericLimits<Float>::Max(),								\
		String::EMPTY,												\
		true														\
	);																\
	ADD_PROPERTY( field )

#define INTERNAL_PROPERTY_EDIT_NAME( field, name, hint )			\
{																	\
	CProperty* currProperty = RedPropertyBuilder::CreateProperty	\
	(																\
		m_registeredClass,											\
		INTERNAL_OFFSETOF( tCurrClassType, field ),					\
		CName( name ),												\
		GetTypeName( static_cast< tCurrClassType* >( 0 )->field ),	\
		hint,														\
		PF_Editable,												\
		-NumericLimits<Float>::Max(),								\
		NumericLimits<Float>::Max(),								\
		String::EMPTY,												\
		false														\
	);																\
	ADD_PROPERTY( field )

#define INTERNAL_PROPERTY_EDIT_IN( str, field, hint )					\
{																		\
	CProperty* currProperty = RedPropertyBuilder::CreateProperty		\
	(																	\
		m_registeredClass,												\
		INTERNAL_OFFSETOF( tCurrClassType, str.field ),					\
		CName( INTERNAL_PROPERTY_TXT( #field ) ),						\
		GetTypeName( static_cast< tCurrClassType* >( 0 )->str.field ),	\
		hint,															\
		PF_Editable,													\
		-NumericLimits<Float>::Max(),									\
		NumericLimits<Float>::Max(),									\
		String::EMPTY,													\
		false															\
	);																	\
	ADD_PROPERTY( field )

#define INTERNAL_PROPERTY_CUSTOM_EDIT( field, hint, customEditor )	\
{																	\
	CProperty* currProperty = RedPropertyBuilder::CreateProperty	\
	(																\
		m_registeredClass,											\
		INTERNAL_OFFSETOF( tCurrClassType, field ),					\
		CName( INTERNAL_PROPERTY_TXT( #field ) ),					\
		GetTypeName( static_cast< tCurrClassType* >( 0 )->field ),	\
		hint,														\
		PF_Editable,												\
		-NumericLimits<Float>::Max(),								\
		NumericLimits<Float>::Max(),								\
		customEditor,												\
		false														\
	);																\
	ADD_PROPERTY( field )

#define INTERNAL_PROPERTY_CUSTOM_EDIT_RANGE( field, hint, customEditor, rangeMin, rangeMax )	\
{																								\
	CProperty* currProperty = RedPropertyBuilder::CreateRangedProperty							\
	(																							\
	m_registeredClass,																			\
	INTERNAL_OFFSETOF( tCurrClassType, field ),													\
	CName( INTERNAL_PROPERTY_TXT( #field ) ),													\
	GetTypeName( static_cast< tCurrClassType* >( 0 )->field ),									\
	hint,																						\
	PF_Editable,																				\
	&static_cast< tCurrClassType* >( 0 )->field,												\
	rangeMin,																					\
	rangeMax,																					\
	customEditor,																				\
	false																						\
	);																							\
	ADD_PROPERTY( field )

#define INTERNAL_PROPERTY_BITFIELD_EDIT( field, type, hint )		\
{																	\
	CProperty* currProperty = RedPropertyBuilder::CreateProperty	\
	(																\
		m_registeredClass,											\
		INTERNAL_OFFSETOF( tCurrClassType, field ),					\
		CName( INTERNAL_PROPERTY_TXT( #field ) ),					\
		::GetTypeName< type >(),									\
		hint,														\
		PF_Editable,												\
		-NumericLimits<Float>::Max(),								\
		NumericLimits<Float>::Max(),								\
		String::EMPTY,												\
		false														\
	);																\
	ADD_PROPERTY( field )

#define INTERNAL_PROPERTY_BITFIELD_RO( field, type, hint )			\
{																	\
	CProperty* currProperty = RedPropertyBuilder::CreateProperty	\
	(																\
		m_registeredClass,											\
		INTERNAL_OFFSETOF( tCurrClassType, field ),					\
		CName( INTERNAL_PROPERTY_TXT( #field ) ),					\
		::GetTypeName< type >(),									\
		hint,														\
		PF_Editable | PF_ReadOnly,									\
		-NumericLimits<Float>::Max(),								\
		NumericLimits<Float>::Max(),								\
		String::EMPTY,												\
		false														\
	);																\
	ADD_PROPERTY( field )

#define INTERNAL_PROPERTY_CUSTOM_EDIT_NAME( field, name, hint, customEditor )	\
{																				\
	CProperty* currProperty = RedPropertyBuilder::CreateProperty				\
	(																			\
		m_registeredClass,														\
		INTERNAL_OFFSETOF( tCurrClassType, field ),								\
		CName( name ),															\
		GetTypeName( static_cast< tCurrClassType* >( 0 )->field ),				\
		hint,																	\
		PF_Editable,															\
		-NumericLimits<Float>::Max(),											\
		NumericLimits<Float>::Max(),											\
		customEditor,															\
		false																	\
	);																			\
	ADD_PROPERTY( field )


#define INTERNAL_PROPERTY_CUSTOM_EDIT_ARRAY( field, hint, customEditor )	\
{																			\
	CProperty* currProperty = RedPropertyBuilder::CreateProperty			\
	(																		\
		m_registeredClass,													\
		INTERNAL_OFFSETOF( tCurrClassType, field ),							\
		CName( INTERNAL_PROPERTY_TXT( #field ) ),							\
		GetTypeName( static_cast< tCurrClassType* >( 0 )->field ),			\
		hint,																\
		PF_Editable,														\
		-NumericLimits<Float>::Max(),										\
		NumericLimits<Float>::Max(),										\
		customEditor,														\
		true																\
	);																		\
	ADD_PROPERTY( field )

#define INTERNAL_PROPERTY_CUSTOM_EDIT_ARRAY_NAME( field, name, hint, customEditor )	\
{																					\
	CProperty* currProperty = RedPropertyBuilder::CreateProperty					\
	(																				\
		m_registeredClass,															\
		INTERNAL_OFFSETOF( tCurrClassType, field ),									\
		CName( name ),																\
		GetTypeName( static_cast< tCurrClassType* >( 0 )->field ),					\
		hint,																		\
		PF_Editable,																\
		-NumericLimits<Float>::Max(),												\
		NumericLimits<Float>::Max(),												\
		customEditor,																\
		true																		\
	);																				\
	ADD_PROPERTY( field )

#define INTERNAL_PROPERTY_EDIT_RANGE( field, hint, rangeMin, rangeMax )		\
{																			\
	CProperty* currProperty = RedPropertyBuilder::CreateRangedProperty		\
	(																		\
		m_registeredClass,													\
		INTERNAL_OFFSETOF( tCurrClassType, field ),							\
		CName( INTERNAL_PROPERTY_TXT( #field ) ),							\
		GetTypeName( static_cast< tCurrClassType* >( 0 )->field ),			\
		hint,																\
		PF_Editable,														\
		&static_cast< tCurrClassType* >( 0 )->field,						\
		rangeMin,															\
		rangeMax,															\
		String::EMPTY,														\
		false																\
	);																		\
	ADD_PROPERTY( field )

#define INTERNAL_PROPERTY_EDIT_RANGE_NAME( field, hint, rangeMin, rangeMax, name )		\
		{																			\
		CProperty* currProperty = RedPropertyBuilder::CreateRangedProperty		\
		(																		\
		m_registeredClass,													\
		INTERNAL_OFFSETOF( tCurrClassType, field ),							\
		CName( name ),							\
		GetTypeName( static_cast< tCurrClassType* >( 0 )->field ),			\
		hint,																\
		PF_Editable,														\
		&static_cast< tCurrClassType* >( 0 )->field,						\
		rangeMin,															\
		rangeMax,															\
		String::EMPTY,														\
		false																\
		);																		\
		ADD_PROPERTY( field )

#define INTERNAL_PROPERTY_RO( field, hint )									\
{																			\
	CProperty* currProperty = RedPropertyBuilder::CreateProperty			\
	(																		\
		m_registeredClass,													\
		INTERNAL_OFFSETOF( tCurrClassType, field ),							\
		CName( INTERNAL_PROPERTY_TXT( #field ) ),							\
		GetTypeName( static_cast< tCurrClassType* >( 0 )->field ),			\
		hint,																\
		PF_Editable | PF_ReadOnly,											\
		-NumericLimits<Float>::Max(),										\
		NumericLimits<Float>::Max(),										\
		String::EMPTY,														\
		false																\
	);																		\
	ADD_PROPERTY( field )

#define INTERNAL_PROPERTY_INLINED( field, hint )							\
{																			\
	CProperty* currProperty = RedPropertyBuilder::CreateProperty			\
	(																		\
		m_registeredClass,													\
		INTERNAL_OFFSETOF( tCurrClassType, field ),							\
		CName( INTERNAL_PROPERTY_TXT( #field ) ),							\
		GetTypeName( static_cast< tCurrClassType* >( 0 )->field ),			\
		hint,																\
		PF_Editable | PF_Inlined,											\
		-NumericLimits<Float>::Max(),										\
		NumericLimits<Float>::Max(),										\
		String::EMPTY,														\
		false																\
	);																		\
	ADD_PROPERTY( field )

#define INTERNAL_PROPERTY_INLINED_NAME( field, name, hint )					\
{																			\
	CProperty* currProperty = RedPropertyBuilder::CreateProperty			\
	(																		\
		m_registeredClass,													\
		INTERNAL_OFFSETOF( tCurrClassType, field ),							\
		CName( name ),														\
		GetTypeName( static_cast< tCurrClassType* >( 0 )->field ),			\
		hint,																\
		PF_Editable | PF_Inlined,											\
		-NumericLimits<Float>::Max(),										\
		NumericLimits<Float>::Max(),										\
		String::EMPTY,														\
		false																\
	);																		\
	ADD_PROPERTY( field )


#define INTERNAL_PROPERTY_INLINED_RO( field, hint )							\
{																			\
	CProperty* currProperty = RedPropertyBuilder::CreateProperty			\
	(																		\
		m_registeredClass,													\
		INTERNAL_OFFSETOF( tCurrClassType, field ),							\
		CName( INTERNAL_PROPERTY_TXT( #field ) ),							\
		GetTypeName( static_cast< tCurrClassType* >( 0 )->field ),			\
		hint,																\
		PF_Inlined | PF_Editable | PF_ReadOnly ,							\
		-NumericLimits<Float>::Max(),										\
		NumericLimits<Float>::Max(),										\
		String::EMPTY,														\
		false																\
	);																		\
	ADD_PROPERTY( field )


#ifndef NO_EDITOR_PROPERTY_SUPPORT

#	define PROPERTY_EDIT( field, hint )													INTERNAL_PROPERTY_EDIT( field, hint )
#	define PROPERTY_EDIT_ARRAY( field, hint )											INTERNAL_PROPERTY_EDIT_ARRAY( field, hint )
#	define PROPERTY_EDIT_NAME( field, name, hint )										INTERNAL_PROPERTY_EDIT_NAME( field, name, hint )
#	define PROPERTY_EDIT_IN( str, field, hint )											INTERNAL_PROPERTY_EDIT_IN( str, field, hint )
#	define PROPERTY_CUSTOM_EDIT( field, hint, customEditor )							INTERNAL_PROPERTY_CUSTOM_EDIT( field, hint, customEditor )
#	define PROPERTY_CUSTOM_EDIT_RANGE( field, hint, customEditor, rangeMin, rangeMax )	INTERNAL_PROPERTY_CUSTOM_EDIT_RANGE( field, hint, customEditor, rangeMin, rangeMax )
#	define PROPERTY_BITFIELD_EDIT( field, type, hint )									INTERNAL_PROPERTY_BITFIELD_EDIT( field, type, hint )
#	define PROPERTY_BITFIELD_RO( field, type, hint )									INTERNAL_PROPERTY_BITFIELD_RO( field, type, hint )
#	define PROPERTY_CUSTOM_EDIT_NAME( field, name, hint, customEditor )					INTERNAL_PROPERTY_CUSTOM_EDIT_NAME( field, name, hint, customEditor )
#	define PROPERTY_CUSTOM_EDIT_ARRAY( field, hint, customEditor )						INTERNAL_PROPERTY_CUSTOM_EDIT_ARRAY( field, hint, customEditor )
#	define PROPERTY_CUSTOM_EDIT_ARRAY_NAME( field, name, hint, customEditor )			INTERNAL_PROPERTY_CUSTOM_EDIT_ARRAY_NAME( field, name, hint, customEditor )
#	define PROPERTY_EDIT_RANGE( field, hint, rangeMin, rangeMax )						INTERNAL_PROPERTY_EDIT_RANGE( field, hint, rangeMin, rangeMax )
#	define PROPERTY_EDIT_RANGE_NAME( field, name, hint, rangeMin, rangeMax  )			INTERNAL_PROPERTY_EDIT_RANGE_NAME( field, hint, rangeMin, rangeMax, name )
#	define PROPERTY_RO( field, hint )													INTERNAL_PROPERTY_RO( field, hint )
#	define PROPERTY_INLINED( field, hint )												INTERNAL_PROPERTY_INLINED( field, hint )
#	define PROPERTY_INLINED_NAME( field, name, hint )									INTERNAL_PROPERTY_INLINED_NAME( field, name, hint )
#	define PROPERTY_INLINED_RO( field, hint )											INTERNAL_PROPERTY_INLINED_RO( field, hint )

#else

#	define PROPERTY_EDIT( field, hint )													INTERNAL_PROPERTY_EDIT( field, String::EMPTY )
#	define PROPERTY_EDIT_ARRAY( field, hint )											INTERNAL_PROPERTY_EDIT_ARRAY( field, hint )
#	define PROPERTY_EDIT_NAME( field, name, hint )										INTERNAL_PROPERTY_EDIT_NAME( field, name, String::EMPTY )
#	define PROPERTY_EDIT_IN( str, field, hint )											INTERNAL_PROPERTY_EDIT_IN( str, field, String::EMPTY )
#	define PROPERTY_CUSTOM_EDIT( field, hint, customEditor )							INTERNAL_PROPERTY_CUSTOM_EDIT( field, String::EMPTY, String::EMPTY )
#	define PROPERTY_CUSTOM_EDIT_RANGE( field, hint, customEditor, rangeMin, rangeMax )	INTERNAL_PROPERTY_CUSTOM_EDIT_RANGE( field, String::EMPTY, String::EMPTY, rangeMin, rangeMax )
#	define PROPERTY_BITFIELD_EDIT( field, type, hint )									INTERNAL_PROPERTY_BITFIELD_EDIT( field, type, String::EMPTY )
#	define PROPERTY_BITFIELD_RO( field, type, hint )									INTERNAL_PROPERTY_BITFIELD_RO( field, type, String::EMPTY )
#	define PROPERTY_CUSTOM_EDIT_NAME( field, name, hint, customEditor )					INTERNAL_PROPERTY_CUSTOM_EDIT_NAME( field, name, String::EMPTY, String::EMPTY )
#	define PROPERTY_CUSTOM_EDIT_ARRAY( field, hint, customEditor )						INTERNAL_PROPERTY_CUSTOM_EDIT_ARRAY( field, String::EMPTY, String::EMPTY )
#	define PROPERTY_CUSTOM_EDIT_ARRAY_NAME( field, name, hint, customEditor )			INTERNAL_PROPERTY_CUSTOM_EDIT_ARRAY_NAME( field, name, String::EMPTY, String::EMPTY )
#	define PROPERTY_EDIT_RANGE( field, hint, rangeMin, rangeMax )						INTERNAL_PROPERTY_EDIT_RANGE( field, String::EMPTY, rangeMin, rangeMax )
#	define PROPERTY_EDIT_RANGE_NAME( field, name, hint, rangeMin, rangeMax  )			INTERNAL_PROPERTY_EDIT_RANGE_NAME( field, hint, rangeMin, rangeMax, name )
#	define PROPERTY_RO( field, hint )													INTERNAL_PROPERTY_RO( field, String::EMPTY )
#	define PROPERTY_INLINED( field, hint )												INTERNAL_PROPERTY_INLINED( field, String::EMPTY )
#	define PROPERTY_INLINED_NAME( field, name, hint )									INTERNAL_PROPERTY_INLINED_NAME( field, name, String::EMPTY )
#	define PROPERTY_INLINED_RO( field, hint )											INTERNAL_PROPERTY_INLINED_RO( field, String::EMPTY )

#endif


#define END_CLASS_RTTI()							\
		return m_registeredClass;					\
	}												\
};

#define PROPERTY_SETTER( _prop, _setter )					\
CProperty::InstallPropertySetter							\
(															\
	m_registeredClass,										\
	INTERNAL_PROPERTY_TXT( #_prop ),						\
	PropertySetterFactory< tCurrClassType >::CreateSetter	\
	(														\
		&tCurrClassType::_setter,							\
		&static_cast< tCurrClassType* >( 0 )->_prop			\
	)														\
);

#define PROPERTY_SETTER_REF( _prop, _setter )					\
CProperty::InstallPropertySetter								\
(																\
	m_registeredClass,											\
	INTERNAL_PROPERTY_TXT( #_prop ),							\
	PropertySetterFactory< tCurrClassType >::CreateSetterRef	\
	(															\
		&tCurrClassType::_setter,								\
		&static_cast< tCurrClassType* >( 0 )->_prop				\
	)															\
);

#define NO_PROPERTY_SERIALIZATION( _prop )	\
	CProperty::ChangePropertyFlag( m_registeredClass, CName( INTERNAL_PROPERTY_TXT(#_prop) ), 0, PF_NotSerialized );

#define NO_PROPERTY_COOKING( _prop )	\
	CProperty::ChangePropertyFlag( m_registeredClass, CName( INTERNAL_PROPERTY_TXT(#_prop) ), 0, PF_NotCooked );

#define PROPERTY_CONFIG( _prop )	\
	CProperty::ChangePropertyFlag( m_registeredClass, CName( INTERNAL_PROPERTY_TXT(#_prop) ), 0, PF_Config );

#define PROPERTY_SAVED( _prop )	\
	PROPERTY( _prop )			\
	CProperty::ChangePropertyFlag( m_registeredClass, CName( INTERNAL_PROPERTY_TXT(#_prop) ), 0, PF_Saved );

#define PROPERTY_BITFIELD_SAVED( _prop, _type )	\
	PROPERTY_BITFIELD( _prop, _type  )			\
	CProperty::ChangePropertyFlag( m_registeredClass, CName( INTERNAL_PROPERTY_TXT(#_prop) ), 0, PF_Saved );

#define PROPERTY_NOSERIALIZE( _prop )	\
	PROPERTY( _prop )					\
	CProperty::ChangePropertyFlag( m_registeredClass, CName( INTERNAL_PROPERTY_TXT(#_prop) ), 0, PF_NotSerialized );

#define PROPERTY_RO_SAVED( _prop, _edit )	\
	PROPERTY_RO( _prop, _edit )			\
	CProperty::ChangePropertyFlag( m_registeredClass, CName( INTERNAL_PROPERTY_TXT(#_prop) ), 0, PF_Saved );

#define PROPERTY_EDIT_SAVED( _prop, _edit )	\
	PROPERTY_EDIT( _prop, _edit )			\
	CProperty::ChangePropertyFlag( m_registeredClass, CName( INTERNAL_PROPERTY_TXT(#_prop) ), 0, PF_Saved );

#define PROPERTY_INLINED_SAVED( _prop, _hint )	\
	PROPERTY_INLINED( _prop, _hint )			\
	CProperty::ChangePropertyFlag( m_registeredClass, CName( INTERNAL_PROPERTY_TXT(#_prop) ), 0, PF_Saved );

#define PROPERTY_EDIT_NOSERIALIZE( _prop, _edit )	\
	PROPERTY_EDIT( _prop, _edit )					\
	CProperty::ChangePropertyFlag( m_registeredClass, CName( INTERNAL_PROPERTY_TXT(#_prop) ), 0, PF_NotSerialized );

#define PROPERTY_EDIT_SAVED_NOSERIALIZE( _prop, _edit )	\
	PROPERTY_EDIT_SAVED( _prop, _edit )			\
	CProperty::ChangePropertyFlag( m_registeredClass, CName( INTERNAL_PROPERTY_TXT(#_prop) ), 0, PF_NotSerialized );

#define PROPERTY_INLINED_NOSERIALIZE( _prop, _edit )	\
	PROPERTY_INLINED( _prop, _edit )					\
	CProperty::ChangePropertyFlag( m_registeredClass, CName( INTERNAL_PROPERTY_TXT(#_prop) ), 0, PF_NotSerialized );

#define PROPERTY_NOT_COOKED( _prop )	\
	PROPERTY( _prop )					\
	CProperty::ChangePropertyFlag( m_registeredClass, CName( INTERNAL_PROPERTY_TXT(#_prop) ), 0, PF_NotCooked );

#define PROPERTY_EDIT_NOT_COOKED( _prop, _edit )	\
	PROPERTY_EDIT( _prop, _edit )					\
	CProperty::ChangePropertyFlag( m_registeredClass, CName( INTERNAL_PROPERTY_TXT(#_prop) ), 0, PF_NotCooked );

#define PROPERTY_RO_NOT_COOKED( _prop, _edit )	\
	PROPERTY_RO( _prop, _edit )					\
	CProperty::ChangePropertyFlag( m_registeredClass, CName( INTERNAL_PROPERTY_TXT(#_prop) ), 0, PF_NotCooked );

#define PROPERTY_CUSTOM_EDIT_NOT_COOKED( _prop, _edit , _editor)	\
	PROPERTY_CUSTOM_EDIT( _prop, _edit, _editor)					\
	CProperty::ChangePropertyFlag( m_registeredClass, CName( INTERNAL_PROPERTY_TXT(#_prop) ), 0, PF_NotCooked );

#define PROPERTY_EDIT_NAME_NOT_COOKED( _prop, _name, _edit )	\
	PROPERTY_EDIT_NAME( _prop, _name, _edit )					\
	CProperty::ChangePropertyFlag( m_registeredClass, CName( _name ), 0, PF_NotCooked );

#define PROPERTY_INLINED_NOT_COOKED( _prop, _edit )																									\
	PROPERTY_INLINED( _prop, _edit )																												\
	CProperty::ChangePropertyFlag( m_registeredClass, CName( INTERNAL_PROPERTY_TXT(#_prop) ), 0, PF_NotCooked );


#define DEFINE_SIMPLE_RTTI_CLASS( _className, _parentName )		BEGIN_CLASS_RTTI( _className );														\
																	PARENT_CLASS( _parentName );													\
																END_CLASS_RTTI();

#define DEFINE_SIMPLE_RTTI_CLASS_EX( _className, _parentName, _flags )		BEGIN_CLASS_RTTI_EX( _className, _flags );								\
																			PARENT_CLASS( _parentName );											\
																			END_CLASS_RTTI();


#define DEFINE_SIMPLE_ABSTRACT_RTTI_CLASS( _className, _parentName )		BEGIN_ABSTRACT_CLASS_RTTI( _className );								\
																				PARENT_CLASS( _parentName );										\
																			END_CLASS_RTTI();


#define IMPLEMENT_RTTI_CLASS(className) static className##ClassBuilder UNIQUE_NAME( classRegistrator ) ;													\
										CClass* className::sm_classDesc = NULL;
