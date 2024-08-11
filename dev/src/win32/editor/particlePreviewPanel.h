/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */
#pragma once

class CEdParticleEditor;

/// Preview panel that renders particle system preview
class CEdParticlePreviewPanel : public CEdPreviewPanel, public IEdEventListener
{
	DECLARE_EVENT_TABLE();

	CEdParticleEditor*		m_particleEditor;
	CParticleComponent*		m_particleComponent;
	CEntity*				m_entity;
	CEntity*				m_targetEntity;

	Float					m_currentAveragedTime;
	Float					m_averagedTime;
	Int32						m_averagedCount;

	Float					m_timeMultiplier;
	CEdSpinSliderControl	m_timeMultiplierControl;

	Bool					m_showHelpers;

public:
	CEdParticlePreviewPanel( wxWindow *parent, CEdParticleEditor* particleEditor );
	~CEdParticlePreviewPanel();

	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

	void SetTargetVisible( Bool flag );

	RED_INLINE CEntity* GetEntity() const { return m_entity; }
	RED_INLINE CEntity* GetTargetEntity() const { return m_targetEntity; }
	RED_INLINE CParticleComponent* GetParticleComponent() const { return m_particleComponent; }

	virtual void HandleContextMenu( Int32 x, Int32 y );

	virtual void OnViewportTick( IViewport* view, Float timeDelta );

	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );

	virtual Color GetClearColor() const;

	virtual Bool ShouldDrawGrid() const;

	void Tick( Float timeDelta );

	void	SetTimeMultiplier( Float newTimeMultiplier ) { m_timeMultiplier = newTimeMultiplier; m_timeMultiplierControl.UpdateValue( newTimeMultiplier ); m_particleComponent->SetTimeMultiplier( m_timeMultiplier ); }
	Float	GetTimeMultiplier() { return m_timeMultiplier; }
	Bool	IsPause() { return m_timeMultiplier > 0; }

	virtual void Reload();

	void OnUpdateEmitter( CParticleEmitter* emitter );
	void OnRemoveEmitter( CParticleEmitter * emitter );

protected:
	void LoadEntity( const String &filename );
	void SpawnTargetEntity();

	void OnLoadEntity( wxCommandEvent& event );
};