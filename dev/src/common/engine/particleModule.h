/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../core/object.h"

class CurveParameter;

/// An element of particle emitter
class IParticleModule : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IParticleModule, CObject );

protected:
	String		m_editorName;		//!< Module name in particle editor
	String		m_editorGroup;		//!< Module group in particle editor menu
	Color		m_editorColor;		//!< Module color in particle editor
	Bool		m_isEnabled;		//!< Is this module enabled
	Bool		m_isShowing;		//!< Is this module showing
	Bool		m_enabledState;		//!< Get the enabled state of the emitter
	Bool		m_isSelected;		//!< Is the module selected and thus showing in the graph

public:
	//! Is the module enabled ?
	RED_INLINE Bool IsEnabled() const { return m_isEnabled; }

	//! This is for the editor to have the ability to show only certain emitters.
	RED_INLINE Bool IsShowing() const { return m_isShowing; }

	//! This is for the editor to have the ability to get the enabled state for certain emitters.
	RED_INLINE Bool EnabledState() const { return m_enabledState; }

	//! Is the module enabled ?
	RED_INLINE Bool IsSelected() const { return m_isSelected; }

	//! Get module name in editor
	RED_INLINE const String& GetEditorName() const { return m_editorName; }

	//! Get module group in editor
	RED_INLINE const String& GetEditorGroup() const { return m_editorGroup; }

	//! Get module color in editor
	RED_INLINE const Color& GetEditorColor() const { return m_editorColor; }

	//! Get emitter
	CParticleEmitter* GetEmitter() const;

public:
	IParticleModule();

	//! Enable/Disable module
	void SetEnable( Bool flag );

	//! Show/Hide emitter 
	void SetShowing( Bool flag );

	//! Set state when showing/hiding the emitter 
	void SetEnabledState( Bool flag );

	//! Enable/Disable module
	void SetSelected( Bool selected );

	//! Set module editor color
	void SetEditorColor( const Color& color );

	//! Set module editor name
	void SetEditorName( const String& name );

public:
	//! Get the curves associated with this module
	virtual void GetCurves( TDynArray< CurveParameter* >& curves );
};

BEGIN_ABSTRACT_CLASS_RTTI( IParticleModule );
PARENT_CLASS( CObject );
PROPERTY_EDIT( m_editorName, TXT("Get the editor name") );
PROPERTY_EDIT( m_editorColor, TXT("Change the Color") );
PROPERTY( m_editorGroup );
PROPERTY( m_isEnabled );
PROPERTY( m_isShowing );
PROPERTY( m_isSelected );
END_CLASS_RTTI();
