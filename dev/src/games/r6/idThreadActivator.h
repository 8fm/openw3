#pragma once


class IIDContition;

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
enum EIDPriority
{
	IDP_ExtremelyImportant	= 0	,
	IDP_VeryImportant			,
	IDP_Important				,
	IDP_NotVeryImportant		,
	IDP_NotImportantAtAll		,
	IDP_Irrelevant				,
	IDP_Invalid					,
};

BEGIN_ENUM_RTTI( EIDPriority );
	ENUM_OPTION( IDP_ExtremelyImportant );
	ENUM_OPTION( IDP_VeryImportant );
	ENUM_OPTION( IDP_Important );
	ENUM_OPTION( IDP_NotVeryImportant );
	ENUM_OPTION( IDP_NotImportantAtAll );
	ENUM_OPTION( IDP_Irrelevant );
	ENUM_OPTION( IDP_Invalid );
END_ENUM_RTTI();


//------------------------------------------------------------------------------------------------------------------
// Contains the starting condition for a thread
//------------------------------------------------------------------------------------------------------------------
class CIDActivator : public CObject
{	
	DECLARE_ENGINE_CLASS( CIDActivator, CObject, 0 )

private:
	//------------------------------------------------------------------------------------------------------------------
	// Variables
	//------------------------------------------------------------------------------------------------------------------
	IIDContition*			m_condition;
	TInstanceVar< Bool >	i_isLocked;
	EIDPriority				m_priorityDefault;

public:

	//------------------------------------------------------------------------------------------------------------------
	// Methods
	//------------------------------------------------------------------------------------------------------------------
public:
	CIDActivator();

	virtual void	OnInitInstance		( InstanceBuffer& data )							const;
	virtual void	OnBuildDataLayout	( InstanceDataLayoutCompiler& compiler );

	Bool			IsFulfilled			( InstanceBuffer& data, Uint32	dialogInstanceID )	const;
	void			SetLocked			( InstanceBuffer& data, Bool _locked )				const;

	EIDPriority		GetPriority			( )													const		{	return m_priorityDefault;	};
};

BEGIN_CLASS_RTTI( CIDActivator )
	PARENT_CLASS( CObject )
	PROPERTY_INLINED( m_condition, TXT("Condition") )
	PROPERTY_EDIT( m_priorityDefault, TXT("Default priority") )
END_CLASS_RTTI()
