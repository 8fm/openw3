#pragma once



enum EStatsType
{
	EST_Default,
	EST_BaseStats,
};

BEGIN_ENUM_RTTI(EStatsType);
	ENUM_OPTION( EST_Default );
	ENUM_OPTION( EST_BaseStats );
END_ENUM_RTTI();

struct SSingleStat 
{	
	DECLARE_RTTI_STRUCT( SSingleStat );
public:
	SSingleStat(){}
	SSingleStat( CName name, Float val ) 
		: m_name( name )
		, m_baseValue( val )
		, m_modifiedValue( val )
	{

	}
	SSingleStat( CName name, Float val, Float mValue ) 
		: m_name( name )
		, m_baseValue( val )
		, m_modifiedValue( mValue )
	{

	}
	CName	m_name;
	Float	m_baseValue;
	Float	m_modifiedValue;

public:	
	RED_INLINE void	Reset(){ m_modifiedValue = m_baseValue; }
	RED_INLINE Float	GetValue(){ return m_modifiedValue; }	
	RED_INLINE Float	GetBaseValue(){ return m_baseValue; }	
	RED_INLINE void	SetBaseValue( Float val ){ m_modifiedValue = m_baseValue = val; }
};

BEGIN_CLASS_RTTI( SSingleStat )
	PROPERTY_CUSTOM_EDIT( m_name	, TXT("Stat name"), TXT("StatisticsNamesEditor") );	//StatisticsNamesEditor,2daValueSelection	
	PROPERTY_EDIT( m_baseValue	, TXT("") );
END_CLASS_RTTI();

struct SStatsList
{
	DECLARE_RTTI_STRUCT( SStatsList );
public:
	SStatsList()
		: m_type( EST_Default ){};

	EStatsType					m_type;
	TDynArray< SSingleStat >	m_stats;

};

BEGIN_CLASS_RTTI( SStatsList )
	PROPERTY_EDIT( m_type	, TXT("Type of stat" )	);	
	PROPERTY_EDIT( m_stats	, TXT("List of stats")	);
END_CLASS_RTTI();

class CStatsContainerComponent : public CComponent /*, public I2dArrayPropertyOwner*/
{
	DECLARE_ENGINE_CLASS( CStatsContainerComponent, CComponent, 0 );
private:
	//TDynArray< SSingleStat >	m_stats;
	TDynArray< SStatsList >		m_statsLists;

	TDynArray< SSingleStat >* GetStats( EStatsType statType );

public:
	static const Int32 MAX_STAT_VALUE = 10;
	static const Int32 MIN_STAT_VALUE = 0;

	virtual void OnAttached( CWorld* world );

	Float GetStatValue( CName name );
	Float GetStatValue( CName name, EStatsType statType );
	Float GetStatBaseValue( CName name, EStatsType statType );
	void AddStatValue( CName name, Float valueToAdd, Bool addIfNotExists = false );
	void AddStatValue( CName name, Float valueToAdd, EStatsType statType, Bool addIfNotExists = false );
	Bool SetStatValue( CName name, Float valueToAdd );
	Bool SetStatValue( CName name, Float valueToAdd, EStatsType statType );
	void ResetStats();
	void ResetStats( EStatsType statType );

	/*void Get2dArrayPropertyAdditionalProperties( IProperty *property, S2daValueProperties &valueProperties ) override;*/

private:
	void funcGetStatValue		( CScriptStackFrame& stack, void* result );
	void funcGetStatBaseValue	( CScriptStackFrame& stack, void* result );
	//void funcGetStat			( CScriptStackFrame& stack, void* result );
	void funcSetBaseValue		( CScriptStackFrame& stack, void* result );
	void funcAddNewStat			( CScriptStackFrame& stack, void* result );
	void funcAddStatValue		( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CStatsContainerComponent )
	PARENT_CLASS( CComponent );
	PROPERTY_EDIT( m_statsLists, TXT("Stats") );//

	NATIVE_FUNCTION( "I_GetStatValue"			, funcGetStatValue		);
	NATIVE_FUNCTION( "I_GetStatBaseValue"		, funcGetStatBaseValue	);
	NATIVE_FUNCTION( "I_SetBaseValue"			, funcSetBaseValue		);
	NATIVE_FUNCTION( "I_AddNewStat"				, funcAddNewStat		);
	NATIVE_FUNCTION( "I_AddStatValue"			, funcAddStatValue		);
END_CLASS_RTTI();