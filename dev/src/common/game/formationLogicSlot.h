/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "formationLogic.h"

class CSlotFormationLogic : public IFormationLogic
{
	DECLARE_ENGINE_CLASS( CSlotFormationLogic, IFormationLogic, 0 )
protected:
	IFormationPatternNode*			m_formationPattern;

	CClass*							GetFormationLeaderType() const override;
	CFormationMemberData*			SpawnMemberData( CActor* actor ) const override;
public:
	CSlotFormationLogic()
		: m_formationPattern( NULL )															{}

	IFormationPatternNode*			GetFormationPattern() const									{ return m_formationPattern; }
};


BEGIN_CLASS_RTTI( CSlotFormationLogic )
	PARENT_CLASS( IFormationLogic )
	PROPERTY_INLINED( m_formationPattern, TXT("Formation pattern definition root") )
END_CLASS_RTTI()
