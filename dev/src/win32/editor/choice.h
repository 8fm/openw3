/**
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CChoiceChangedEvent : public wxNotifyEvent
{
public:
	CChoiceChangedEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 )
		: wxNotifyEvent( commandType, winid )
	{}
};

wxDECLARE_EVENT( wxEVT_CHOICE_CHANGED, CChoiceChangedEvent );

class CEdChoice : public wxComboCtrl, public wxItemContainer
{
protected:
	wxComboPopup*		m_comboPopup;
	bool				m_editableText;
	
	virtual int DoInsertItems(const wxArrayStringsAdapter & items,
                              unsigned int pos,
                              void **clientData,
                              wxClientDataType type);

    virtual void DoSetItemClientData(unsigned int n, void *clientData);
    virtual void *DoGetItemClientData(unsigned int n) const;

    virtual void DoClear();
    virtual void DoDeleteOneItem(unsigned int pos);

	virtual void SetClientDataType( wxClientDataType clientDataItemsType );

	void OnKeyDown( wxKeyEvent& event );
	void OnMouseWheel( wxMouseEvent& event );

public:
	CEdChoice( wxWindow* parent, const wxPoint& pos, const wxSize& size, bool editableText = false, long style = 0 );
	virtual ~CEdChoice();

	virtual bool SetBackgroundColour( const wxColour& colour );

    virtual unsigned int GetCount() const;

    virtual void SetSelection(int n);
    virtual int GetSelection() const;
	
    virtual wxString GetString(unsigned int n) const;
    virtual void SetString(unsigned int n, const wxString& s);

	wxClientData *GetClientObject( unsigned int n ) const;
	void SetClientObject( unsigned int n, wxClientData* data );

	virtual wxClientDataType GetClientDataType() const;
};
