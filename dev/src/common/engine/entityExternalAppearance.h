/**
* Copyright c 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "appearanceComponent.h"

///////////////////////////////////////////////////////////
/// Entity external appearance
class CEntityExternalAppearance : public CResource
{	
	DECLARE_ENGINE_RESOURCE_CLASS( CEntityExternalAppearance, CResource, "w3app", "Entity external appearance" );

	friend class CEntity;
	friend class CAppearanceComponent;

private:
	CEntityAppearance 	   m_appearance;

public:
	CEntityExternalAppearance();
		
	RED_INLINE const CName& GetName() const { return m_appearance.GetName(); }
	RED_INLINE const CEntityAppearance& GetAppearance() const { return m_appearance; }

	void PrintLogInfo() const;
};

BEGIN_CLASS_RTTI( CEntityExternalAppearance );
	PARENT_CLASS( CResource );
	PROPERTY( m_appearance );
END_CLASS_RTTI();