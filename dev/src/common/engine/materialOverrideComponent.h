/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "component.h"


/// Material override component
class CMaterialOverrideComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CMaterialOverrideComponent, CComponent, 0 );

private:
	bool m_isGlobalOverride;

protected:
	THandle< CMaterialInstance > m_override;

public:
	//! Default constructor
	CMaterialOverrideComponent ();

	// Property was changed
	virtual void OnPropertyPostChange( IProperty* property );

public:
	//! Set global override
	void SetGlobalOverride( bool enable );

	//! Return is global override flag
	bool IsGlobalOverride() const { return m_isGlobalOverride; }

	//! Get material instance override
	IMaterial* OverrideMaterial( IMaterial *material ) const;
};

BEGIN_CLASS_RTTI( CMaterialOverrideComponent )
	PARENT_CLASS( CComponent );
	PROPERTY_EDIT( m_isGlobalOverride, TXT("Is global override") );
	PROPERTY_INLINED( m_override, TXT("Material override") );
END_CLASS_RTTI();
