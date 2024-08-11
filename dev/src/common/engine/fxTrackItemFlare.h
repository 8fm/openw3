/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "fxTrackItemCurveBase.h"
#include "fxSpawner.h"

class CFXTrackItemFlare: public CFXTrackItemCurveBase
{
	DECLARE_ENGINE_CLASS( CFXTrackItemFlare, CFXTrackItem, 0 );

protected:
	THandle< CMaterialInstance >	m_material;
	SFlareParameters				m_parameters;
	IFXSpawner*						m_spawner;

public:
	CFXTrackItemFlare();

	//! Change name of track item
	virtual void SetName( const String& name );

	//! Get name of track item
	virtual String GetName() const;

	//! Get curve name
	virtual String GetCurveName( Uint32 i = 0 ) const;

	//! Get number of curves here
	virtual Uint32 GetCurvesCount() const { return 1; }

	void SetSpawner( IFXSpawner* spawner );

	//! Check if track item uses component with given name
	virtual Bool UsesComponent( const CName& componentName ) const;

public:
	//! Start track item effect, can spawn play data that will be ticked
	virtual IFXTrackItemPlayData* OnStart( CFXState& fxState ) const;
};

BEGIN_CLASS_RTTI( CFXTrackItemFlare );
	PARENT_CLASS( CFXTrackItem );
	PROPERTY_INLINED( m_material,	TXT("Material applied") );
	PROPERTY_EDIT( m_parameters,	TXT("Parameters") );
	PROPERTY_INLINED(m_spawner,		TXT("Spawner for flare effect"))
END_CLASS_RTTI();