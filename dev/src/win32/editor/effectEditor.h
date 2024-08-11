/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CEdEffectTracksEditor;
class CEdCurveEditor;
class CEdPropertiesBrowserWithStatusbar;
class CEdEffectEditorProperties;
class CEdEntityPreviewPanel;
class CFXState;

// Used to synchronize Effect Curve Editor with Curve Editor
class CEffectEditorSynchronizable
{
public:
	CEffectEditorSynchronizable() : m_synchEditors( NULL ) {}

	void SetSynchEditor( CEffectEditorSynchronizable *synchEditors ) { m_synchEditors = synchEditors; }
	void SynchronizeWithOther( wxPoint offset, const Vector& scale ) { ASSERT(m_synchEditors); m_synchEditors->Synchronize( offset, scale ); }

protected:
	virtual void Synchronize( wxPoint offset, const Vector& scale ) = 0;

private:
	CEffectEditorSynchronizable *m_synchEditors; // editors to synchronize with (this can be TDynArray in the future)
};

// Canvas for CEdEffectCurveEditor
class CEdEffectCurveEditorCanvas : public CEdCurveEditorCanvas
{
public:
	CEdEffectCurveEditorCanvas( wxWindow *parent, CEffectEditorSynchronizable *synch )
		: CEdCurveEditorCanvas( parent )
		, m_synch( synch )
	{
		CEdCurveEditor* editor = static_cast< CEdCurveEditor* >( m_parent->GetParent() );
		SetHook( editor );
	}

	virtual void ScrollBackgroundOffset( wxPoint delta )
	{
		CEdCurveEditorCanvas::ScrollBackgroundOffset( delta );
		m_synch->SynchronizeWithOther( GetOffset(), m_scaleCurves );
	}

	virtual void SetZoomedRegion( const Vector& corner1, const Vector& corner2 )
	{
		CEdCurveEditorCanvas::SetZoomedRegion( corner1, corner2 );
		m_synch->SynchronizeWithOther( GetOffset(), m_scaleCurves );
	}

private:
	CEffectEditorSynchronizable *m_synch;
};

class CEdEffectCurveEditor : public CEdCurveEditor, public CEffectEditorSynchronizable
{
public:
	CEdEffectCurveEditor( wxWindow* parent );

	// Info methods
	Int32 GetSidePanelWidth() const { return m_curveEditorCanvas->GetSidePanelWidth(); }
	Int32 GetSidePanelHeight() const { return m_curveEditorCanvas->GetSidePanelHeight(); }
	wxPoint GetOffset() const { return m_curveEditorCanvas->GetOffset(); }

	virtual void Synchronize( wxPoint offset, const Vector& scale );
};

class CEdEffectEditor : public wxPanel, public IEdEventListener
{
	DECLARE_EVENT_TABLE();

protected:
	THandle< CEntity >				m_entity;
	THandle< CEntityTemplate >		m_template;
	CEdEffectCurveEditor*			m_curveEditor;
	CEdEffectEditorProperties*		m_propertyEditor;
	CEdEffectTracksEditor*			m_effectTracksEditor;
	CFXDefinition*					m_fxDefinition;		//!< Definition of effect being edited
	Bool							m_isPlaying;		//!< Is playing effect preview spawned from this editor (not from external source)

public:
	//! Get the effect assigned to this effect editor
	RED_INLINE CFXDefinition* GetFXDefinition() const { return m_fxDefinition; }

	//! Get the entity
	CEntity* GetEntity() const { return m_entity.Get(); }

public:
	CEdEffectEditor( wxWindow* parent, CEntity* entity, CEntityTemplate* templ, CFXDefinition* fxDefinition );
	~CEdEffectEditor();

	//! Set object to edit via properties browser
	void SetObjectToEdit( CObject* object );

	//! Refresh properties
	void RefreshProperties();

	//! Set animation length
	void SetAnimationLength( Float length );

	//! Force stop effects
	void ForceStopEffects();

	//! Set the entity to play effect on
	void SetEntity( CEntity* entity );

private:
	//! Dispatch internal event
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

	//! Save configuration
	virtual void SaveOptionsToConfig();

	//! Load configuration
	virtual void LoadOptionsFromConfig();

private:
	// Get all entities to play the preview effect on
	void GetPreviewEntities( TDynArray< CEntity* >& entities );
	void OnRestartEffect( wxCommandEvent& evnet );
	void OnPlayEffect( wxCommandEvent& evnet );
	void OnPauseEffect( wxCommandEvent& evnet );
	void OnStopEffect( wxCommandEvent& evnet );
	void OnForceStopEffect( wxCommandEvent& evnet );
	void OnUpdateUI( wxUpdateUIEvent& event );

public:
	void CopySelection();
	void PasteSelection();
};
