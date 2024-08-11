/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "entityTemplate.h"

class CCharacterEntityTemplate : public CEntityTemplate
{
	DECLARE_ENGINE_RESOURCE_CLASS( CCharacterEntityTemplate, CEntityTemplate, "w2cent", "Character entity template" );

public:

	CCharacterEntityTemplate();

	virtual void RebuildCharacter();

	void SetBaseEntityOverride( CEntityTemplate* tpl )
	{
		m_baseEntityOverride = tpl;
	}
	void ClearBaseEntityOverride()
	{
		m_baseEntityOverride = THandle< CEntityTemplate >::Null();
	}
	RED_INLINE CEntityTemplate* GetBaseEntityOverride() const {
		return m_baseEntityOverride.IsValid() ? m_baseEntityOverride.Get() : nullptr;
	}

private:
	THandle< CEntityTemplate >	m_baseEntityOverride;
};

BEGIN_CLASS_RTTI( CCharacterEntityTemplate );
	PARENT_CLASS( CEntityTemplate );
	PROPERTY( m_baseEntityOverride );
END_CLASS_RTTI();
