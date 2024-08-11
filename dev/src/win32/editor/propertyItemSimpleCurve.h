/**
* Copyright © 20010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

struct SSimpleCurveChangeData
{
	SSimpleCurveChangeData( SCurveBase*	curve, CName propertyName )
		: m_curve( curve )
		, m_propertyName( propertyName )
	{}

	SCurveBase*				m_curve;
	CName					m_propertyName;
};

class CPropertyItemSimpleCurve : public CBasePropItem
{
private:
	typedef CBasePropItem TBaseClass;

	Int32					m_selectedGraph;
	Int32					m_mouseMove;
	wxPoint					m_mouseLastPos;
	Float					m_lastColorPickerEntryTime;
	CEdAdvancedColorPicker*	m_ctrlColorPicker;
	bool					m_isInEditionMode;
	SCurveBase*				m_curve;
	IProperty*				m_property;

	struct SGraphEnv*		m_graphEnv;

	Bool					m_isScalingValue;
	wxPoint					m_scalingValuePt;

public:
	CPropertyItemSimpleCurve ( CEdPropertiesPage* page, CBasePropItem* parent, SCurveBase* curve );
	CPropertyItemSimpleCurve ( CEdPropertiesPage* page, CBasePropItem* parent, IProperty* property );
	~CPropertyItemSimpleCurve();

public:
	virtual Int32  GetHeight() const;
	virtual void DrawLayout( wxDC& dc );
	virtual void UpdateLayout( Int32& yOffset, Int32 x, Int32 width );
	virtual String GetCaption() const;
	virtual Bool ShouldSuppressMouseScrollEvent( const wxMouseEvent& event ) const;
	virtual void OnBrowserMouseEvent( wxMouseEvent& event );
	virtual void Expand();
	virtual void Collapse();
	virtual Bool SerializeXML( IXMLFile& file );

	virtual Bool CanUseSelectedResource() const { return false; }
	virtual Bool IsReadOnly() const { return true; }

public:
	RED_INLINE Float GetRangeMin() const { return m_property->GetRangeMin(); }
	RED_INLINE Float GetRangeMax() const { return m_property->GetRangeMax(); }


public:
	SCurveBase*			GetCurve() { return m_curve; }
	const SCurveBase*	GetCurve() const { return m_curve; }
	wxRect				CalcValueRect() const;
	wxRect				CalcCaptionRect() const;

	RED_INLINE bool IsInEditionMode() const { return m_isInEditionMode; }
	void SetEditionMode( bool editionMode );

protected:
	void OnCopyCurve( wxCommandEvent& event );
	void OnPasteCurve( wxCommandEvent& event );
	void OnCopyPoints( wxCommandEvent& event );
	void OnPastePoints( wxCommandEvent& event );
	void OnRemovePoints( wxCommandEvent& event );
	void OnSetPointTimeExplicit( wxCommandEvent& event );
	void OnSetPointValueExplicit( wxCommandEvent& event );
	void OnInvertSelectedPoints( wxCommandEvent& event );
	void OnSetTangentType( wxCommandEvent& event );
	void OnSetCurveType( wxCommandEvent& event );

	void GraphPreChange( bool valueDataChange );
	void GraphPostChange( bool valueDataChange );

private:
	void	UpdateCachedHeightExpanded();
	wxPoint GetLocalMousePos() const;
	bool	OnBrowserMouseEventLocal( wxMouseEvent& event );
	void	OpenColorPicker( const Color &color );
	void	OnColorPicked( wxCommandEvent& event );
};
