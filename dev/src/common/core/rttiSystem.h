/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "hashmap.h"
#include "staticarray.h"
#include "names.h"

class IRTTIRegistrator;
class IRTTIType;
class CClass;
class CEnum;
class CBitField;
class CProperty;
class FuncPerfData;
class IClassBuilder;
class CFunction;

// Name of the RTTI layout file
#define RTTI_LAYOUT_FILE			TXT("rttiLayout.xml")

enum ERTTITypeType
{	
	RT_Simple,
	RT_Enum,
	RT_Class,
	RT_Array,				// TDynArray
	RT_StaticArray,			// TStaticArray
	RT_NativeArray,			// T[N]
	RT_Pointer,
	RT_Handle,
	RT_SoftHandle,
	RT_BitField,
	RT_Void,
	RT_Fundamental
};

class CRTTISystem
{
public:
	CRTTISystem();
	~CRTTISystem();

	//----
	void Init();
	void Deinit();
	void CreateDefaultObjects();
	void DestroyDefaultObjects();
	void ClearScriptData( const void* scriptablesToClean );
	void RestoreScriptData( const void* scriptablesToRestore );
	void LoadClassRemapTable();
	void RecacheClassProperties();
	void ReindexClasses();

	//----
	void RegisterType( IRTTIType *type );

	void UnregisterType( IRTTIType *type );
	
	void RegisterGlobalFunction( CFunction* function );

	//----
	void RegisterProperty( CProperty* prop );

	void UnregisterProperty( CProperty* prop );

	//----
	void BeginFunctionProfiling();

	void EndFunctionProfiling( TDynArray< FuncPerfData* >& data );

	//----

	IRTTIType* FindType( const CName& name );

	IRTTIType* FindType( const CName& name, ERTTITypeType type );

	CClass* FindClass( const CName& name );

	CEnum* FindEnum( const CName& name );

	CBitField* FindBitField( const CName& name );

	CFunction* FindGlobalFunction( const CName& name );

	IRTTIType* FindSimpleType( const CName& name );

	IRTTIType* FindFundamentalType( const CName & name );

	CProperty* FindProperty( const Uint64 propertyHash );

	CEnum* FindEnumValue( const String& name, Int32& value );

	void EnumFunctions( TDynArray< CFunction* >& functions );

    void EnumClasses( CClass* baseClass, TDynArray< CClass* > &classes, Bool classFilter( CClass * ) = NULL, Bool allowAbstract = false );

	void EnumDerivedClasses( CClass* baseClass, TDynArray< CClass* > &classes );

	void EnumEnums( TDynArray< CEnum* > &enums );

	bool DumpRTTILayout( const Char* absoluteFileName );

	bool CheckRTTILayout( const Char* absoluteFileName );

	Bool ValidateLayout() const;

	//------

	//! Dump to log
	static void DumpToLogFun( CClass * c, Int32 level );

	//! Dump class hierarchy under given baseClass
	void DumpClassHierarchy( CClass* baseClass, void dumpFunction( CClass *, Int32 ) = DumpToLogFun, Int32 level = 0 );

	//! Dump (to file) a captured state of all RTTI objects we know about, all memory accessible under RTTI system is dumped.
	//! If file name is not specified it's autoformated to be the executable name and current timestamp
	void DumpRTTIMemory( const Char* customFileName );

	//! Create RTTI type for scripted class
	CClass* CreateScriptedClass( CName className, Uint32 flags );

	//! Create RTTI type for scripted enum
	CEnum* CreateScriptedEnum( CName enumName, Uint32 enumSize );

	//! Check if we can cast from one type to another ( no conversion )
	Bool CanCast( const CName& sourceType, const CName& destType ) const;

	//! Check if we can cast from one type to another ( no conversion )
	Bool CanCast( const IRTTIType* sourceType, const IRTTIType* destType ) const;

	//! Cast pointer from one pointer type to another
	void* CastPointer( const CName& destTypeName, const CName& srcTypeName, void* ptr );

	// Get the indexed list of classes, classes are sorted by their "depth"
	typedef TStaticArray< CClass*, 8192 >		TStaticClassList;
	RED_INLINE const TStaticClassList& GetIndexedClasses() { return m_indexedClasses; }

private:
	void RegisterClassBuilder( IClassBuilder *builder );
	void UnregisterClassBuilder( IClassBuilder *builder );

	// Fallback method that handles naming conversion for old type names (@Int, #CEntity, etc)
	IRTTIType* HandleOldWrappedTypes( const CName& typeName );

	typedef TDynArray< IClassBuilder*, MC_RTTI >		TClassBuilders;
	typedef THashMap< CName, IRTTIType*, DefaultHashFunc< CName >, DefaultEqualFunc< CName >, MC_RTTI >		TTypes;
	typedef THashMap< CName, CFunction*, DefaultHashFunc< CName >, DefaultEqualFunc< CName >, MC_RTTI >		TFunctions;
	typedef THashMap< Uint64, CProperty*, DefaultHashFunc< Uint64 >, DefaultEqualFunc< Uint64 >, MC_RTTI >	TProperties;
	typedef THashMap< CName, CName, DefaultHashFunc< CName >, DefaultEqualFunc< CName >, MC_RTTI >			TRemapTable;
	typedef TDynArray< CClass*, MC_RTTI >																	TStubScriptClasses;
	typedef TDynArray< CEnum*, MC_RTTI >																	TStubEnumClasses;

	TClassBuilders				m_classBuilders;
	TTypes						m_types;
	TFunctions					m_globalFunctions;
	TProperties					m_properties;
	TRemapTable					m_classRemapTable;
	TStubScriptClasses			m_stubScriptClasses;
	TStubEnumClasses			m_stubScriptEnums;
	TStaticClassList			m_indexedClasses;

	friend class IClassBuilder;
	friend class CRTTISerializer;
};

typedef TSingleton< CRTTISystem > SRTTI;

#define TYPENAME(_type) TTypeName<_type>::GetTypeName()
