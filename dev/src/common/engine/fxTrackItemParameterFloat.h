/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "fxTrackItemCurveBase.h"

/// Parameter based track item in the FX
class CFXTrackItemParameterFloat : public CFXTrackItemCurveBase
{
	DECLARE_ENGINE_CLASS( CFXTrackItemParameterFloat, CFXTrackItemCurveBase, 0 );

private:
	CName	m_parameterName;		//!< Name of the parameter being edited
	Bool	m_restoreAtEnd;			//!< Restore parameter to default value after this block ends
	Bool	m_allComponents;		//!< Use all components

public:
	CFXTrackItemParameterFloat();

	//! Get track name
	virtual String GetName() const { return m_parameterName.AsString(); }

	//! Change track name
	virtual void SetName( const String& name ) { m_parameterName = CName( name ); } 

public:
	//! Start track item effect, can spawn play data that will be ticked
	virtual IFXTrackItemPlayData* OnStart( CFXState& fxState ) const;
};

BEGIN_CLASS_RTTI( CFXTrackItemParameterFloat );
	PARENT_CLASS( CFXTrackItemCurveBase );
	PROPERTY_CUSTOM_EDIT( m_parameterName, TXT("Parameter name"), TXT("EffectParameterFloatList") );
	PROPERTY_EDIT( m_restoreAtEnd, TXT("Restore parameter to default value after this track item ends") );
	PROPERTY_EDIT( m_allComponents, TXT("Use ALL components?") );
END_CLASS_RTTI();

class CFXTrackItemParameterFloatPlayData : public IFXTrackItemPlayData
{
public:
	const CFXTrackItemParameterFloat*	m_trackItem;		//!< Data
	CName								m_paramName;		//!< Param name to update
	Float								m_initialValue;		//!< Initial value of parameter
	Float								m_previousValue;	//!< Previous vlaue of parameter
	Bool								m_restoreAtEnd;		//!< Restore on block end
	TDynArray< THandle< CComponent > >	m_components;		//!< Components when 'all components' are selected
	TDynArray< Float >					m_initialValues;	//!< Components when 'all components' are selected
	THandle< CEntity >					m_entity;			//!< Parent entity of the components

public:
	CFXTrackItemParameterFloatPlayData( CComponent* component, const CFXTrackItemParameterFloat* trackItem, CName paramName, Bool restoreAtEnd );

	CFXTrackItemParameterFloatPlayData( CEntity* entity, const TDynArray< CComponent* >& components, const CFXTrackItemParameterFloat* trackItem, CName paramName, Bool restoreAtEnd );

	~CFXTrackItemParameterFloatPlayData();

	void AddComponent( CComponent* component );
	void RemoveComponent( CComponent* component );

	//! Called when entity is partially unstreamed, causing certain components to be destroyed
	virtual void OnPreComponentStreamOut( CComponent* component );

	virtual void OnStop() {}

	virtual void OnTick( const CFXState& fxState, Float timeDelta );
};
