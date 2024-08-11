	/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeGuardAreaData.h"
#include "behTreeVarsEnums.h"

struct SBehTreeSelectedAreaInstance;

enum EAIAreaSelectionMode
{
	EAIASM_Encounter,
	EAIASM_GuardArea,
	EAIASM_ByTag,
	EAIASM_ByTagInEncounter,
	EAIASM_None,
};

BEGIN_ENUM_RTTI( EAIAreaSelectionMode )
	ENUM_OPTION( EAIASM_Encounter )
	ENUM_OPTION( EAIASM_GuardArea )
	ENUM_OPTION( EAIASM_ByTag )
	ENUM_OPTION( EAIASM_ByTagInEncounter )
	ENUM_OPTION( EAIASM_None )
END_ENUM_RTTI()



class CBehTreeValAreaSelectionMode : public TBehTreeValEnum< EAIAreaSelectionMode, EAIASM_Encounter >
{
	DECLARE_RTTI_STRUCT( CBehTreeValAreaSelectionMode )

private:
	typedef TBehTreeValEnum< EAIAreaSelectionMode, EAIASM_Encounter > TBaseClass;

public:
	CBehTreeValAreaSelectionMode( EAIAreaSelectionMode e = EAIASM_Encounter )
		: TBaseClass( e )																	{}
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeValAreaSelectionMode )
	PROPERTY_EDIT( m_varName, TXT("Variable") )
	PROPERTY_EDIT( m_value, TXT("Move type") )
END_CLASS_RTTI()



struct SBehTreeAreaSelection
{
	DECLARE_RTTI_STRUCT( SBehTreeAreaSelection )

	CBehTreeValAreaSelectionMode			m_selectionMode;
	CBehTreeValCName						m_optionalAreaTag;

	SBehTreeAreaSelection()
		: m_selectionMode( EAIASM_Encounter )																		{}

	void									InitInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, SBehTreeSelectedAreaInstance& outInstance ) const;
};

BEGIN_CLASS_RTTI( SBehTreeAreaSelection )
	PROPERTY_EDIT( m_selectionMode, TXT("Tag") )
	PROPERTY_EDIT( m_optionalAreaTag, TXT("If flag is set to true goal will activate even if no target is found") )
END_CLASS_RTTI()


struct SBehTreeSelectedAreaInstance
{
	friend struct SBehTreeAreaSelection;
protected:
	THandle< CAreaComponent >				m_area;
	CBehTreeGuardAreaDataPtr				m_guardAreaPtr;
public:
	CAreaComponent*							GetArea();
};