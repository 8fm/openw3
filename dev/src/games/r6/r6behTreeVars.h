/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../../common/game/behTreeVars.h"


class CBehTreeValAIAction : public TBehTreeValue< CAIAction* >, public ISerializable
{
	DECLARE_RTTI_SIMPLE_CLASS( CBehTreeValAIAction );
public:
	CBehTreeValAIAction()
		: TBehTreeValue( NULL )											{}

	CAIAction* GetVal( const CBehTreeSpawnContext& context ) const;
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeValAIAction );
	PROPERTY_EDIT( m_varName, TXT("Variable name") );
	PROPERTY_INLINED( m_value, TXT("AIAction definition object") );
END_CLASS_RTTI();