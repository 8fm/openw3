/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

//#define TRACK_OUT_OF_RANGE_BEHAVIOR_GRAPH_VALUES

class CBehaviorGraph;

class CBaseBehaviorVariable : public CObject
{
	DECLARE_ENGINE_CLASS( CBaseBehaviorVariable, CObject, 0 );

public:
	CName	m_name;
	Uint32	m_varIndex;	// Index on the containing variable list

	CBaseBehaviorVariable( const CName name = CName::NONE )
		: m_name( name )
		, m_varIndex( 0xFFFFFFFF )
	{}

	RED_FORCE_INLINE const CName& GetName() const { return m_name; }
	RED_FORCE_INLINE Uint32 GetVarIndex() const { return m_varIndex; }

#ifndef NO_EDITOR_GRAPH_SUPPORT
	void OnPropertyPostChange( IProperty* property ) override;
#endif
};

BEGIN_CLASS_RTTI( CBaseBehaviorVariable );
PARENT_CLASS( CObject );
PROPERTY_EDIT( m_name, TXT("Variable name") ) ;
PROPERTY( m_varIndex );
END_CLASS_RTTI();

class CBaseBehaviorVariablesList
{
protected:
	CBehaviorGraph*	m_behaviorGraph;	// parent graph
	Uint32			m_varIndexGenerator;

public:
	CBaseBehaviorVariablesList()
		: m_behaviorGraph( nullptr )
		, m_varIndexGenerator( 0 )
	{}

	void SetBehaviorGraph( CBehaviorGraph* graph ) { m_behaviorGraph = graph; }
	Uint32 GetIndexBasedSize() const { return m_varIndexGenerator; }
};

///////////////////////////////

class CBehaviorVariable : public CBaseBehaviorVariable
{
	DECLARE_ENGINE_CLASS( CBehaviorVariable, CBaseBehaviorVariable, 0 );

public:
	Float	m_value;
	Float	m_defaultValue;
	Float	m_minValue;
	Float	m_maxValue;
	Bool	m_isModifiableByEffect;
	Bool	m_shouldBeSyncedBetweenGraphs;

public:
	CBehaviorVariable( const CName name = CName::NONE,
		Float defaultValue = 0.0f,
		Float minValue = 0.0f,
		Float maxValue = 1.0f )
		: CBaseBehaviorVariable( name )
		, m_value( defaultValue ) 		
		, m_defaultValue( defaultValue )
		, m_minValue( minValue )
		, m_maxValue( maxValue )
		, m_isModifiableByEffect( false )
		, m_shouldBeSyncedBetweenGraphs( true )
	{
	}

	RED_INLINE void Reset() 
	{
		m_value = m_defaultValue;
	}

	RED_INLINE Float Reset() const
	{
		return m_defaultValue;
	}

	RED_INLINE Float GetValue() const 
	{ 
		return m_value; 
	}

	RED_INLINE Float GetDefaultValue() const 
	{ 
		return m_defaultValue; 
	}

	RED_INLINE void SetValue( Float value )
	{
#ifdef TRACK_OUT_OF_RANGE_BEHAVIOR_GRAPH_VALUES
		ASSERT( ! Red::Math::NumericalUtils::IsNan( value ) );
#ifndef NO_EDITOR_GRAPH_SUPPORT
		RED_WARNING( value >= m_minValue && value <= m_maxValue, "Behavior graph variable \"%s\" in \"%s\" value %.3f exceeded range %.3f - %.3f", m_name.AsChar(), GetParentBehaviorGraphDepotPath(), value, m_minValue, m_maxValue );
#endif
#endif
		m_value = Clamp( value, m_minValue, m_maxValue );		
	}

	RED_INLINE Float SetValue( Float value ) const
	{
#ifdef TRACK_OUT_OF_RANGE_BEHAVIOR_GRAPH_VALUES
		ASSERT( !Red::Math::NumericalUtils::IsNan( value ) );
#ifndef NO_EDITOR_GRAPH_SUPPORT
		RED_WARNING( value >= m_minValue && value <= m_maxValue, "Behavior graph variable \"%s\" in \"%s\" value %.3f exceeded range %.3f - %.3f", m_name.AsChar(), GetParentBehaviorGraphDepotPath(), value, m_minValue, m_maxValue );
#endif
#endif
		return Clamp( value, m_minValue, m_maxValue );
	}

#ifndef NO_EDITOR_GRAPH_SUPPORT
	const Char* GetParentBehaviorGraphDepotPath() const;
#endif

	Bool ShouldBeSyncedBetweenGraphs() const { return m_shouldBeSyncedBetweenGraphs; }

	void Set( const CBehaviorVariable* rhs )
	{
		m_name = rhs->m_name;
		m_value = rhs->m_value;
		m_defaultValue = rhs->m_defaultValue;
		m_minValue = rhs->m_minValue;
		m_maxValue = rhs->m_maxValue;
		m_isModifiableByEffect = rhs->m_isModifiableByEffect;
		m_varIndex = rhs->m_varIndex;
	}
};

BEGIN_CLASS_RTTI( CBehaviorVariable );
PARENT_CLASS( CBaseBehaviorVariable );
PROPERTY_EDIT( m_value, TXT("Variable value") );
PROPERTY_EDIT( m_defaultValue, TXT("Variable default value") );
PROPERTY_EDIT( m_minValue, TXT("Variable minimum value") );
PROPERTY_EDIT( m_maxValue, TXT("Variable maximum value") );
PROPERTY_EDIT( m_isModifiableByEffect, TXT("Modifiable by effect") );
PROPERTY_EDIT( m_shouldBeSyncedBetweenGraphs, TXT("Unmark if this variable should be used only by this behavior graph") );
END_CLASS_RTTI();

////////////////////////////////////////////

class CBehaviorVariablesList : public CBaseBehaviorVariablesList
{
	typedef THashMap< CName, CBehaviorVariable* > TContainerType;

	//! variables list
	TContainerType	m_variables;

	TDynArray<CBehaviorVariable*> m_oldVariables; // TODO: Remove once all instances of this get resaved with VER_BEHAVIOR_VARS_BY_NAME or later

public:
	void AddVariable( const CName name,
		Float value = 0.0f,
		Float minValue = 0.0f,
		Float maxValue = 1.0f );
	void RemoveVariable( const CName name );

	CBehaviorVariable* GetVariable( const CName name ) const;
	RED_FORCE_INLINE Bool HasVariable( const CName name ) const { return m_variables.KeyExist( name ); }

	Uint32 GetNumVariables() const;

	void ReserveAdditionalVariables(Uint32 num);

	void Reset();

	void GetVariables( TDynArray< CBehaviorVariable* >& variables ) const;
	const TContainerType& GetVariables() const { return m_variables; }
	TContainerType& GetVariables() { return m_variables; }

	Uint32 GetSize() const;

	Bool DoesContain( CBehaviorVariable* variable ) const;

	CBehaviorVariablesList& operator=( const CBehaviorVariablesList& rhs );

	friend IFile& operator<<( IFile &file, CBehaviorVariablesList &list );
	void OnPostLoad();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	void OnVariableNameChanged();
#endif

};

////////////////////////////////////////////

enum EVectorVariableType
{
	VVT_Position,
	VVT_Rotation
};

BEGIN_ENUM_RTTI( EVectorVariableType );
ENUM_OPTION( VVT_Position );
ENUM_OPTION( VVT_Rotation );
END_ENUM_RTTI();

enum EVariableSpace
{
	VS_Model,
	VS_Global
};

BEGIN_ENUM_RTTI( EVariableSpace );
ENUM_OPTION( VS_Model );
ENUM_OPTION( VS_Global );
END_ENUM_RTTI();

class CBehaviorVectorVariable : public CBaseBehaviorVariable
{
	DECLARE_ENGINE_CLASS( CBehaviorVectorVariable, CBaseBehaviorVariable, 0 );

public:
	Vector	m_value;
	Vector	m_defaultValue;
	Vector	m_minValue;
	Vector	m_maxValue;	
	EVariableSpace		m_space;
	EVectorVariableType m_type;
	Bool	m_shouldBeSyncedBetweenGraphs;

public:
	CBehaviorVectorVariable( CName name = CName::NONE,
		const Vector& defaultValue = Vector::ZEROS,
		const Vector& minValue = Vector(-FLT_MAX, -FLT_MAX, -FLT_MAX),
		const Vector& maxValue = Vector(FLT_MAX, FLT_MAX, FLT_MAX) )
		: CBaseBehaviorVariable( name )
		, m_value( defaultValue ) 		
		, m_defaultValue( defaultValue )
		, m_minValue( minValue )
		, m_maxValue( maxValue )
		, m_type( VVT_Position )
		, m_space( VS_Model )
		, m_shouldBeSyncedBetweenGraphs( true )
	{
	}

	RED_INLINE void Reset() 
	{
		m_value = m_defaultValue;

		// Get value in global space is slow. We need it only in behavior editor so reset can set VS_Local.
		m_space = VS_Model;
	}

	RED_INLINE Vector Reset() const
	{
		return m_defaultValue;
	}

	Vector GetValue() const;

	Vector GetVectorValue() const;

	Bool ShouldBeSyncedBetweenGraphs() const { return m_shouldBeSyncedBetweenGraphs; }

	RED_INLINE EVectorVariableType GetType() const
	{
		return m_type;
	}

	RED_INLINE Vector GetDefaultValue() const 
	{ 
		return m_defaultValue; 
	}

	RED_INLINE void SetValue( const Vector& value )
	{
		ASSERT( !Red::Math::NumericalUtils::IsNan( value.X ) );
		ASSERT( !Red::Math::NumericalUtils::IsNan( value.Y ) );
		ASSERT( !Red::Math::NumericalUtils::IsNan( value.Z ) );
		ASSERT( !Red::Math::NumericalUtils::IsNan( value.W ) );

		RED_WARNING( value.X >= m_minValue.X && value.X <= m_maxValue.X, "Behavior graph vector variable component X \"%s\" value %.3f exceeded range %.3f - %.3f", m_name.AsChar(), value.X, m_minValue.X, m_maxValue.X );
		RED_WARNING( value.Y >= m_minValue.Y && value.Y <= m_maxValue.Y, "Behavior graph vector variable component Y \"%s\" value %.3f exceeded range %.3f - %.3f", m_name.AsChar(), value.Y, m_minValue.Y, m_maxValue.Y );
		RED_WARNING( value.Z >= m_minValue.Z && value.Z <= m_maxValue.Z, "Behavior graph vector variable component Z \"%s\" value %.3f exceeded range %.3f - %.3f", m_name.AsChar(), value.Z, m_minValue.Z, m_maxValue.Z );
		RED_WARNING( value.W >= m_minValue.W && value.W <= m_maxValue.W, "Behavior graph vector variable component W \"%s\" value %.3f exceeded range %.3f - %.3f", m_name.AsChar(), value.W, m_minValue.W, m_maxValue.W );
		
		if ( value.IsOk() )
		{
			m_value.X = Clamp( value.X, m_minValue.X, m_maxValue.X );
			m_value.Y = Clamp( value.Y, m_minValue.Y, m_maxValue.Y );
			m_value.Z = Clamp( value.Z, m_minValue.Z, m_maxValue.Z );
			m_value.W = Clamp( value.W, m_minValue.W, m_maxValue.W );		
		}
		else
		{
			m_value = m_defaultValue;
		}
	}

	RED_INLINE Vector SetValue( const Vector& value ) const
	{
		ASSERT( !Red::Math::NumericalUtils::IsNan( value.X ) );
		ASSERT( !Red::Math::NumericalUtils::IsNan( value.Y ) );
		ASSERT( !Red::Math::NumericalUtils::IsNan( value.Z ) );
		ASSERT( !Red::Math::NumericalUtils::IsNan( value.W ) );

		RED_WARNING( value.X >= m_minValue.X && value.X <= m_maxValue.X, "Behavior graph vector variable component X \"%s\" value %.3f exceeded range %.3f - %.3f", m_name.AsChar(), value.X, m_minValue.X, m_maxValue.X );
		RED_WARNING( value.Y >= m_minValue.Y && value.Y <= m_maxValue.Y, "Behavior graph vector variable component Y \"%s\" value %.3f exceeded range %.3f - %.3f", m_name.AsChar(), value.Y, m_minValue.Y, m_maxValue.Y );
		RED_WARNING( value.Z >= m_minValue.Z && value.Z <= m_maxValue.Z, "Behavior graph vector variable component Z \"%s\" value %.3f exceeded range %.3f - %.3f", m_name.AsChar(), value.Z, m_minValue.Z, m_maxValue.Z );
		RED_WARNING( value.W >= m_minValue.W && value.W <= m_maxValue.W, "Behavior graph vector variable component W \"%s\" value %.3f exceeded range %.3f - %.3f", m_name.AsChar(), value.W, m_minValue.W, m_maxValue.W );
		
		Vector temp;

		if ( value.IsOk() )
		{
			temp.X = Clamp( value.X, m_minValue.X, m_maxValue.X );
			temp.Y = Clamp( value.Y, m_minValue.Y, m_maxValue.Y );
			temp.Z = Clamp( value.Z, m_minValue.Z, m_maxValue.Z );
			temp.W = Clamp( value.W, m_minValue.W, m_maxValue.W );			
		}
		else
		{
			temp = m_defaultValue;
		}

		return temp;
	}

	void Set( const CBehaviorVectorVariable* rhs )
	{
		m_name = rhs->m_name;
		m_value = rhs->m_value;
		m_defaultValue = rhs->m_defaultValue;
		m_minValue = rhs->m_minValue;
		m_maxValue = rhs->m_maxValue;
		m_type = rhs->m_type;
		m_space = rhs->m_space;
		m_varIndex = rhs->m_varIndex;
	}
};

BEGIN_CLASS_RTTI( CBehaviorVectorVariable );
PARENT_CLASS( CBaseBehaviorVariable );
PROPERTY_EDIT( m_value, TXT("Variable value") );
PROPERTY_EDIT( m_defaultValue, TXT("Variable default value") );
PROPERTY_EDIT( m_minValue, TXT("Variable minimum value") );
PROPERTY_EDIT( m_maxValue, TXT("Variable maximum value") );
PROPERTY_EDIT( m_space, TXT("") );
PROPERTY_EDIT( m_type, TXT("Type") );
PROPERTY_EDIT( m_shouldBeSyncedBetweenGraphs, TXT("Unmark if this variable should be used only by this behavior graph") );
END_CLASS_RTTI();

////////////////////////////////////////////////////////

class CBehaviorVectorVariablesList : public CBaseBehaviorVariablesList
{
	typedef THashMap< CName, CBehaviorVectorVariable* > TContainerType;

	//! variables list
	TContainerType	m_variables;
	
	TDynArray<CBehaviorVectorVariable*> m_oldVariables; // TODO: Remove once all instances of this get resaved with VER_BEHAVIOR_VARS_BY_NAME or later

public:
	void AddVariable( const CName name,
		const Vector& defaultValue = Vector::ZEROS,
		const Vector& minValue = Vector(-FLT_MAX, -FLT_MAX, -FLT_MAX),
		const Vector& maxValue = Vector(FLT_MAX, FLT_MAX, FLT_MAX) );

	void RemoveVariable( const CName name );
	CBehaviorVectorVariable* GetVariable( const CName name ) const;
	RED_FORCE_INLINE Bool HasVariable( const CName name ) const { return m_variables.KeyExist( name ); }

	Uint32 GetNumVariables() const;

	void ReserveAdditionalVariables(Uint32 num);

	void GetVariables( TDynArray< CBehaviorVectorVariable* >& variables ) const;
	const TContainerType& GetVariables() const { return m_variables; }
	TContainerType& GetVariables() { return m_variables; }

	void Reset();

	Uint32 GetSize() const;

	Bool DoesContain( CBehaviorVectorVariable* variable ) const;

	CBehaviorVectorVariablesList& operator=( const CBehaviorVectorVariablesList& rhs );

	friend IFile& operator<<( IFile &file, CBehaviorVectorVariablesList &list );
	void OnPostLoad();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	void OnVariableNameChanged();
#endif
};
