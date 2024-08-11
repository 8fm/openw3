/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CEdParticleEditor;

class CEdEmitterGraphEditor : public CEdCanvas
{
	DECLARE_EVENT_TABLE()

public:

	CEdEmitterGraphEditor(CEdParticleEditor* editor, wxWindow *parent, CParticleSystem *particleSystem);
	virtual ~CEdEmitterGraphEditor();

	// Public methods
	IParticleModule *GetEditedModule();
	void UpdateLayout();
	void ScaleToClientView();
	
	//! Selects single emitter
	void Select( CParticleEmitter *emitter, Bool state );

protected:
	// CEdCanvas virtual methods
	virtual void PaintCanvas( Int32 width, Int32 height ) override;
	virtual void MouseClick( wxMouseEvent& event ) override;
	virtual void MouseMove( wxMouseEvent& event, wxPoint delta ) override;
	void ShowOnlySelectedEmitter();
	Bool IsAnyEmitterShowing();
	void GetEmittersEnabledState();
	void ResetEmitterEnabledState();

	// Modificator block layout info
	struct ModificatorLayoutInfo
	{
		static const Int32 ButtonSize = 16;
		wxRect m_localRect;
		wxPoint m_moveUpPoint;
		wxPoint m_moveDownPoint;
		wxPoint m_curvePoint;
		wxPoint m_enablePoint;
		wxPoint m_showOnlyPoint;
	};

	// Emitter block layout info
	struct EmitterLayoutInfo
	{
		EmitterLayoutInfo() : m_collapsed(false) {}
		static const Int32 ThumbnailSize = 64;
		static const Int32 ModificatorHeight = 20;
		static const Int32 ButtonSize = 16;
		wxSize m_size;
		wxPoint m_enablePoint;
		wxPoint m_showOnlyPoint;
		wxPoint m_disableAllButThisPoint;	// to be implemented
		wxPoint m_triggerPoint;
		wxPoint m_birthRatePoint;
		wxPoint m_nameOffset;
		wxPoint m_thumbnailOffset;
		THashMap<IParticleModule *, ModificatorLayoutInfo> m_modificators;
		Bool m_collapsed;
	};

	/// Mouse action mode
	enum EMouseAction
	{
		MA_None,				//!< Nothing, free mouse
		MA_BackgroundScroll,	//!< Scrolling background
		MA_MovingBlocks,		//!< Moving selected blocks
		MA_SelectingBlocks,		//!< Selecting multiple blocks
	};

	enum EHitArea
	{
		HA_Emitter,
		HA_Trigger,
		HA_Enable,
		HA_ShowOnly,
		HA_DisableAllButThis,
		HA_Modificator,
		HA_ModificatorEnable,
		HA_MoveUp,
		HA_MoveDown,
		HA_BirthRateCurve,
		HA_ModificatorCurve,
		HA_Outside,
	};

private:
	static TDynArray< THandle< CParticleEmitter > > m_emittersInClipboard; //!< Emitters that we can copy from one particle system to another

	// Const members
	const wxColor m_selectRectColor;
	const wxColor m_selectedColor;
	const wxColor m_editedColor;
	
	// Members
	CEdParticleEditor *m_editor;
	CParticleSystem *m_particleSystem;
	THashMap<CParticleEmitter *, EmitterLayoutInfo> m_layout;
	THashSet<CParticleEmitter *> m_selected;
	THashMap<CThumbnail *, Gdiplus::Bitmap *> m_thumbnails;
	Gdiplus::Bitmap *m_chessboard;
	wxPoint m_lastMousePosition;
	wxPoint m_selectRectStart;
	wxPoint m_selectRectEnd;
	EMouseAction m_action;
	Int32 m_moveTotal;
	Float m_wheelTotal;
	Bool m_blocksMoved;
	IParticleModule *m_activeModule;
	IParticleModule *m_selectedModificator;
	EHitArea m_activeHitArea;

	// Inline methods
	Bool IsEmitter(IParticleModule *module) const;
	Bool IsModificator(IParticleModule *module) const;
	Bool IsEmitterSelected(CParticleEmitter *emitter) const;
	IParticleModule *GetModificator(IParticleModule *module) const;

	// Internal methods
	void MouseDown( wxMouseEvent& event );
	void MouseUp( wxMouseEvent& event );
	void CheckClickOverEmitterBlock(CParticleEmitter *emitter, IParticleModule *module, wxPoint point);
	void UpdateEmitterLayout(CParticleEmitter *emitter);
	void OpenContextMenu();
	CParticleEmitter *GetEmitter(IParticleModule *module) const;
	wxRect GetBoundingRect(CParticleEmitter *emitter) const;	
	void SelectModificator(IParticleModule *modificator);
	void SelectEmitter(CParticleEmitter *emitter, Bool select, Bool exclusive = false);
	void SelectAll(Bool select);
	void SelectEmittersFromArea(const wxRect &area);
	void EditModule(IParticleModule *module);
	void DrawEmitterBlock(CParticleEmitter *emitter);
	void DrawCurveRect(wxRect rect, wxColor color, Bool singleCurve, Bool active);
	Gdiplus::Bitmap* GetThumbnailForMaterial( IMaterial* material );
	Gdiplus::Bitmap* GetThumbnail(CResource *resource);
	void CalculateTriangleInsideRect(const wxRect &rect, wxPoint &p1, wxPoint &p2, wxPoint &p3, Bool up, Int32 factor);
	EHitArea GetEmitterHitArea(CParticleEmitter *emitter, const wxPoint &point, IParticleModule *&activeModule);
	void ScaleGraph(Float scale, wxPoint point);
	void DrawCheckInRect( wxRect rect, wxColor color, Float width );
	void DrawEyeInRect( wxRect rect, wxColor color, Float width );
	void DrawClosedEyeInRect( wxRect rect, wxColor color, Float width );

	// Event handlers
	void OnMouseWheel( wxMouseEvent& event );
	void OnKeyDown( wxKeyEvent &event );

	void OnEmitterAdd( wxCommandEvent& event );
	void OnEmitterClone( wxCommandEvent& event );
	void OnEmitterRemove( wxCommandEvent& event );
	void OnEmitterDebug( wxCommandEvent& event );
	void OnEmitterGlobalDisable( wxCommandEvent& event );

	void OnModificatorAdd( wxCommandEvent& event );
	void OnModificatorRemove( wxCommandEvent& event );
	void OnModificatorCopy( wxCommandEvent& event );

	void OnCopyToClipboard( wxCommandEvent& event );
	void OnPasteFromClipboard( wxCommandEvent& event );
	
	void OnEditCurve( wxCommandEvent& event );
	void OnLayoutReset( wxCommandEvent& event );
	void OnFitToWindow( wxCommandEvent& event );

private:
	void RemoveModule( IParticleModule* module );

};
