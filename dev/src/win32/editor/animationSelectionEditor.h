/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdAnimBrowser;
class CAnimatedComponent;


class IAnimationComponentAwareCustomPropertyEditor
{
protected:
	virtual CAnimatedComponent* RetrieveAnimationComponent() const = 0;
};

class IAnimationSelectionEditor : public wxEvtHandler, public ICustomPropertyEditor, public IAnimationComponentAwareCustomPropertyEditor

{
private:
	wxBitmap			m_iconAnimBrowser;
	wxBitmap			m_iconReset;

protected:
	CEdAnimBrowser*		m_animBrowser;
	CName				m_animationChosen;

public:
	IAnimationSelectionEditor( CPropertyItem* propertyItem );

	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;

	virtual void CloseControls() override;

	virtual Bool GrabValue( String& displayData ) override;

	virtual Bool SaveValue() override;

protected:
	void OnAnimationConfirmed( wxCommandEvent &event );

	void OnAnimationAbandoned( wxCommandEvent &event );

	void OnSpawnAnimBrowser( wxCommandEvent &event );

	void OnResetAnimation( wxCommandEvent &event );
};