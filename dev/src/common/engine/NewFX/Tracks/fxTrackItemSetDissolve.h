#pragma once
#include "..\..\fxTrackItem.h"

//////////////////////////////////////////////////////////////////////////
// Allows to enable/disable dissolves on all entity's drawable components.
//////////////////////////////////////////////////////////////////////////
class CFXTrackItemSetDissolve: public CFXTrackItem
{
	DECLARE_ENGINE_CLASS( CFXTrackItemSetDissolve, CFXTrackItem, 0 );

protected:
	Bool m_DisableAllDissolves;

public:
	CFXTrackItemSetDissolve();

	void SetName( const String& name ) override {}
	String GetName() const override { return TXT("SetDissolve"); } 

public:
	IFXTrackItemPlayData* OnStart( CFXState& fxState ) const override;
};

BEGIN_CLASS_RTTI( CFXTrackItemSetDissolve );
	PARENT_CLASS( CFXTrackItem );
	PROPERTY_EDIT( m_DisableAllDissolves, TXT("Enable/disable dissolves on all entity's drawable components") );
END_CLASS_RTTI();