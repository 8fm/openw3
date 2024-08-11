#pragma once

/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "behTreeEvent.h"
#include "aiParameters.h"
#include "moveSteeringBehavior.h"
#include "formation.h"

class IBehTreeNodeDefinition;
class CBehTreeSpawnContext;
class IBehTreeNodeInstance;
class CAITree;

//#define DEBUG_EVENT_LISTENERS_BINDING

////////////////////////////////////////////////////////////////////////
// Value objects - some value that might be overwritten by
// variables in context.
// RTTI declared in subclasses.
////////////////////////////////////////////////////////////////////////

class CBehTreeValBase
{
public:
	CName		m_varName;

	CBehTreeValBase( CName varName = CName::NONE )
		: m_varName( varName )											{}

	RED_INLINE void			SetName( CName name )						{ m_varName = name; }
	RED_INLINE const CName&	GetName() const								{ return m_varName; }
	RED_INLINE Bool			HasVariableSet() const						{ return !m_varName.Empty(); }

	Bool operator<( const CBehTreeValBase& v ) const					{ return m_varName < v.m_varName; }
};


class IBehTreeValueEnum : public IScriptable
{
protected:
	CName		m_varName;
public:
	typedef Int32 VarType;

	DECLARE_RTTI_SIMPLE_CLASS( IBehTreeValueEnum );

	IBehTreeValueEnum()													{ EnableReferenceCounting( true ); }

	RED_INLINE void			SetName( CName name )						{ m_varName = name; }
	RED_INLINE const CName&	GetName() const								{ return m_varName; }
	RED_INLINE Bool			HasVariableSet() const						{ return !m_varName.Empty(); }
	
	void				SetVal( Int32 val );
	Int32				GetVal( const CBehTreeSpawnContext& context ) const;
	void				GetValRef( const CBehTreeSpawnContext& context, Int32& val ) const { val = GetVal( context ); }
	Int32				GetValue() const;
	const IRTTIType*	GetEnumType() const;
};

BEGIN_NODEFAULT_CLASS_RTTI( IBehTreeValueEnum );
	PARENT_CLASS( IScriptable )
	PROPERTY_EDIT( m_varName, TXT("Variable") );
END_CLASS_RTTI();

template < class T >
class TBehTreeValue : public CBehTreeValBase
{
protected:
	RED_INLINE T TGetVal( const CBehTreeSpawnContext& context ) const;
	RED_INLINE Bool TGetValRef( const CBehTreeSpawnContext& context, T& val ) const;

public:
	typedef T		VarType;

	T				m_value;

	TBehTreeValue()														{}
	TBehTreeValue(const T& val)
		: m_value( val )												{}
	TBehTreeValue(CName name, const T& val)
		: CBehTreeValBase( name )
		, m_value( val )												{}

	const T& GetValue() const											{ return m_value; }
};




typedef TDynArray< THandle< CAITree > > TBehTreeValList;
class CBehTreeValList : public TBehTreeValue< TBehTreeValList >
{
	DECLARE_RTTI_STRUCT( CBehTreeValList );

public:
	CBehTreeValList() {}
	CBehTreeValList( TBehTreeValList val )
		: TBehTreeValue( val )										{}
	CBehTreeValList( CName name, TBehTreeValList val )
		: TBehTreeValue( name, val )								{}
	Bool GetValRef( const CBehTreeSpawnContext& context, TBehTreeValList& val ) const;

};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeValList );
	PROPERTY_EDIT( m_varName, TXT("Variable") );
	PROPERTY_EDIT( m_value, TXT("Value") );
END_CLASS_RTTI();

class CBehTreeValFloat : public TBehTreeValue< Float >
{
	DECLARE_RTTI_STRUCT( CBehTreeValFloat );
public:
	CBehTreeValFloat( Float val = 0.f )
		: TBehTreeValue( val )										{}
	CBehTreeValFloat( CName name, Float val = 0.f )
		: TBehTreeValue( name, val )								{}
	Float GetVal( const CBehTreeSpawnContext& context ) const;
	void GetValRef( const CBehTreeSpawnContext& context, Float& val ) const	{ val = GetVal( context ); }
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeValFloat );
	PROPERTY_EDIT( m_varName, TXT("Variable") );
	PROPERTY_EDIT( m_value, TXT("Value") );
END_CLASS_RTTI();


class CBehTreeValBool : public TBehTreeValue< Bool >
{
	DECLARE_RTTI_STRUCT( CBehTreeValBool );
public:
	CBehTreeValBool()
		: TBehTreeValue( false )										{}
	CBehTreeValBool( Bool b )
		: TBehTreeValue( b )											{}
	CBehTreeValBool( CName name, Bool b = false )
		: TBehTreeValue( name, b )										{}
	Bool GetVal( const CBehTreeSpawnContext& context ) const;
	void GetValRef( const CBehTreeSpawnContext& context, Bool& val ) const	{ val = GetVal( context ); }
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeValBool );
	PROPERTY_EDIT( m_varName, TXT("Variable") );
	PROPERTY_EDIT( m_value, TXT("Value") );
END_CLASS_RTTI();

class CBehTreeValInt : public TBehTreeValue< Int32 >
{
	DECLARE_RTTI_STRUCT( CBehTreeValInt );
public:
	CBehTreeValInt( Int32 val = 0 )
		: TBehTreeValue( val )											{}
	CBehTreeValInt( CName name, Int32 val = 0 )
		: TBehTreeValue( name, val )									{}
	Int32 GetVal( const CBehTreeSpawnContext& context ) const;
	void GetValRef( const CBehTreeSpawnContext& context, Int32& val ) const	{ val = GetVal( context ); }
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeValInt );
	PROPERTY_EDIT( m_varName, TXT("Variable") );
	PROPERTY_EDIT( m_value, TXT("Value") );
END_CLASS_RTTI();


class CBehTreeValVector : public TBehTreeValue< Vector >
{
	DECLARE_RTTI_STRUCT( CBehTreeValVector );
public:
	CBehTreeValVector()
		: TBehTreeValue()												{ m_value = Vector::ZEROS; }
	Bool GetValRef( const CBehTreeSpawnContext& context, Vector& outVal ) const;
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeValVector );
	PROPERTY_EDIT( m_varName, TXT("Variable") );
	PROPERTY_EDIT( m_value, TXT("Value") );
END_CLASS_RTTI();

class CBehTreeValTemplate : public TBehTreeValue< IBehTreeNodeDefinition* >
{
	DECLARE_RTTI_STRUCT( CBehTreeValTemplate );
public:
	CBehTreeValTemplate()
		: TBehTreeValue( NULL )											{}
	CBehTreeValTemplate( CName name )
		: TBehTreeValue( name, NULL )									{}

	IBehTreeNodeDefinition* GetVal( const CBehTreeSpawnContext& context ) const;
	void GetValRef( const CBehTreeSpawnContext& context, IBehTreeNodeDefinition*& val ) const	{ val = GetVal( context ); }
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeValTemplate );
	PROPERTY_EDIT( m_varName, TXT("Variable") );
	PROPERTY_CUSTOM_EDIT( m_value, TXT("Value"), TXT("TreeEditorButton") );
END_CLASS_RTTI();

class CBehTreeValFile : public TBehTreeValue< THandle< CBehTree > >
{
	DECLARE_RTTI_STRUCT( CBehTreeValFile );
public:
	CBehTreeValFile()
		: TBehTreeValue( NULL )											{}
	CBehTreeValFile( CName name )
		: TBehTreeValue( name, NULL )									{}
	CBehTreeValFile( CName name, CBehTree* val )
		: TBehTreeValue( name, val )									{}

	CBehTree* GetVal( const CBehTreeSpawnContext& context ) const;
	void GetValRef( const CBehTreeSpawnContext& context, CBehTree*& val ) const	{ val = GetVal( context ); }
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeValFile);
	PROPERTY_EDIT( m_varName, TXT("Variable") );
	PROPERTY_CUSTOM_EDIT( m_value, TXT("Value"), TXT("Beh tree file") );
END_CLASS_RTTI();

class CBehTreeValString : public TBehTreeValue< String >
{
	DECLARE_RTTI_STRUCT( CBehTreeValString );
public:
	CBehTreeValString()
		: TBehTreeValue( String::EMPTY )										{}
	CBehTreeValString(String string)
		: TBehTreeValue( string )										{}
	Bool GetValRef( const CBehTreeSpawnContext& context, String& str ) const;
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeValString );
	PROPERTY_EDIT( m_varName, TXT("Variable") );
	PROPERTY_EDIT( m_value, TXT("Value") );
END_CLASS_RTTI();

class CBehTreeValCName : public TBehTreeValue< CName >
{
	DECLARE_RTTI_STRUCT( CBehTreeValCName );
public:
	CBehTreeValCName( CName val = CName::NONE )
		: TBehTreeValue( val )											{}
	CBehTreeValCName( CName name, CName val )
		: TBehTreeValue( name, val )									{}
	CName GetVal( const CBehTreeSpawnContext& context ) const;
	void GetValRef( const CBehTreeSpawnContext& context, CName& val ) const	{ val = GetVal( context ); }
};
BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeValCName );
	PROPERTY_EDIT( m_varName, TXT("Variable") );
	PROPERTY_EDIT( m_value, TXT("Value") );
END_CLASS_RTTI();

class CBehTreeValRes : public TBehTreeValue< THandle< CAITree > >
{
	DECLARE_RTTI_STRUCT( CBehTreeValRes );
public:
	CBehTreeValRes()
		: TBehTreeValue( nullptr )										{}
	CBehTreeValRes( CName name, CAITree* val )
		: TBehTreeValue( name, val )									{}
	CAITree* GetVal( const CBehTreeSpawnContext& context ) const;
	void GetValRef( const CBehTreeSpawnContext& context, THandle< CAITree >& val ) const	{ val = GetVal( context ); }
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeValRes );
	PROPERTY_EDIT( m_varName, TXT("Variable") );
	PROPERTY_INLINED( m_value, TXT("Value") );
END_CLASS_RTTI();

class CBehTreeValEntityHandle : public TBehTreeValue< EntityHandle >
{
	DECLARE_RTTI_STRUCT( CBehTreeValEntityHandle );
public:
	CBehTreeValEntityHandle()
		: TBehTreeValue()												{}
	CBehTreeValEntityHandle( CName name, EntityHandle& val )
		: TBehTreeValue( name, val )									{}
	Bool GetValRef( const CBehTreeSpawnContext& context, EntityHandle& val ) const;
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeValEntityHandle );
	PROPERTY_EDIT( m_varName, TXT("Variable") );
	PROPERTY_EDIT( m_value, TXT("Value") );
END_CLASS_RTTI();

class CBehTreeValSteeringGraph : public TBehTreeValue< THandle< CMoveSteeringBehavior > >
{
	DECLARE_RTTI_STRUCT( CBehTreeValSteeringGraph );
public:
	CBehTreeValSteeringGraph()
		: TBehTreeValue( NULL )											{}

	CMoveSteeringBehavior* GetVal( const CBehTreeSpawnContext& context ) const;
	void GetValRef( const CBehTreeSpawnContext& context, THandle< CMoveSteeringBehavior >& val ) const	{ val = GetVal( context ); }
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeValSteeringGraph );
	PROPERTY_EDIT( m_varName, TXT("Variable") );
	PROPERTY_EDIT( m_value, TXT("Steering graph default resource") );
END_CLASS_RTTI();

class CBehTreeValFormation : public TBehTreeValue< THandle< CFormation > >
{
	DECLARE_RTTI_STRUCT( CBehTreeValFormation );
public:
	CBehTreeValFormation()
		: TBehTreeValue( NULL )											{}

	CFormation* GetVal( const CBehTreeSpawnContext& context ) const;
	void GetValRef( const CBehTreeSpawnContext& context, THandle< CFormation >& val ) const	{ val = GetVal( context ); }
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeValFormation );
	PROPERTY_EDIT( m_varName, TXT("Variable") );
	PROPERTY_EDIT( m_value, TXT("Formation default resource") );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// Base class holding all predefined variables
////////////////////////////////////////////////////////////////////////
class CBehTreeSpawnContext
{
	DECLARE_RTTI_STRUCT( CBehTreeSpawnContext )
public:
	struct SEventHandlerInfo
	{
		SBehTreeEvenListeningData		m_event;
		IBehTreeNodeInstance*			m_instance;
	};
	typedef TDynArray< SEventHandlerInfo > tEventHandlersArray;
protected:
	
	Bool											m_isInDynamicBranch;
	TStaticArray< const IAIParameters*, 16 >		m_context;
	tEventHandlersArray								m_eventListeners;
public:
	CBehTreeSpawnContext( Bool isDynamicBranch = false )
		: m_isInDynamicBranch( isDynamicBranch )						{}
#ifdef DEBUG_EVENT_LISTENERS_BINDING
	~CBehTreeSpawnContext();
#endif

	// Context queries
	Uint32 GetStackDepth() const										{ return m_context.Size(); }

	// Context stack handling
	Bool Push( const IAIParameters* aiParams );
	void Pop( const IAIParameters* aiParams );
	void Pop( Uint32 i = 1 );

	Bool IsInDynamicBranch() const										{ return m_isInDynamicBranch; }
	Bool MarkAsDynamicBranch( Bool b )									{ Bool prev = m_isInDynamicBranch; m_isInDynamicBranch = b; return prev; }

	void AddEventListener( const SBehTreeEvenListeningData& e, IBehTreeNodeInstance* l );
	tEventHandlersArray& GetEventListeners()							{ return m_eventListeners; }

	// Max nested tree depth.
	// This should also prevent nested tree loops.
	Bool IsStackFull() const											{ return m_context.Size() < m_context.Capacity(); }

	template< class T >
	Bool HasVal( CName valName ) const
	{
		for ( Int32 i = GetStackDepth()-1; i >= 0; --i )
		{
			if ( m_context[ i ]->HasParameter( valName ) )
			{
				return true;
			}
		}

		return false;
	}

	// this interface is usable for basic types (ints, floats) as its more comfy to get them in nodes constructors by value
	template< class T >
	T GetVal( CName valName, const T& defaultVal ) const
	{
		static_assert( sizeof( T ) <= 8, "GetVal function is intended to be used by basic types. Large value types one should get by reference (GetValRef)." );

		T val = defaultVal;

		for ( Int32 i = GetStackDepth()-1; i >= 0; --i )
		{
			if ( m_context[ i ]->GetParameter( valName, val ) )
			{
				return val;
			}
		}

		return defaultVal;
	}

	// interface to use with complex data types
	template< class T >
	Bool GetValRef( CName valName, T& val ) const
	{
		for ( Int32 i = GetStackDepth()-1; i >= 0; --i )
		{
			if ( m_context[ i ]->GetParameter( valName, val ) )
			{
				return true;
			}
		}

		return false;
	}
	
	IAIParameters *const GetAiParametersByClassName( CName contextName ) const
	{
		for ( Int32 i = GetStackDepth()-1; i >= 0; --i )
		{
			const IAIParameters *const context = m_context[ i ];

			IAIParameters *const foundContext = context->GetAiParametersByClassName( contextName );
			if ( foundContext )
			{
				return foundContext;
			}
		}
		return nullptr;
	}
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeSpawnContext );
END_CLASS_RTTI( )

template < class T >
T TBehTreeValue< T >::TGetVal( const CBehTreeSpawnContext& context ) const
{
	if ( !m_varName.Empty() )
	{
		return context.GetVal< T >( m_varName, m_value );
	}

	return m_value;
}

template < class T >
Bool TBehTreeValue< T >::TGetValRef( const CBehTreeSpawnContext& context, T& val ) const
{
	if ( !m_varName.Empty() )
	{
		return context.GetValRef< T >( m_varName, val );
	}

	return false;
}

