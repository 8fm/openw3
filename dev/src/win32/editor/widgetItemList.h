#pragma once

wxDECLARE_EVENT( edEVT_COMMAND_ITEMCONTAINERWITHBUTTON_CLICKED, wxCommandEvent );

class CEdItemContainerWithButton : public wxControl
{
public:
	class CMiniButton : public wxButton
	{
	public:
		CMiniButton( wxWindow* parent, const wxString& title, long style = 0 );
		bool SetBackgroundColour( const wxColour& colour );
	};

	class CItemContainer : public wxControl
	{
		wxWindow*					m_widget;

	public:
		CItemContainer( wxWindow* parent );

		void						SetWidget( wxWindow* widget );
		inline wxWindow*			GetWidget() const { return m_widget; }
		inline void					OnEraseBackground( wxEraseEvent& event ){}
	};

protected:
	CMiniButton*					m_removeButton;
	CItemContainer*					m_itemContainer;

public:
	CEdItemContainerWithButton( wxWindow* parent );

	inline wxButton*				GetButton() const { return m_removeButton; }
	inline CItemContainer*			GetItemContainer() const { return m_itemContainer; }

	virtual wxSize					DoGetBestSize() const;

private:
	void							OnSize( wxSizeEvent& event );
	void							OnRemoveClicked( wxCommandEvent& event );

	DECLARE_EVENT_TABLE()
};

//////////////////////////////////////////////////////////////////////////

class CEdWidgetItemListItem
{
	friend class CEdWidgetItemList;

	CEdItemContainerWithButton*	m_containerWithButton;

public:
	inline void SetWidget( wxWindow* widget )
	{
		m_containerWithButton->GetItemContainer()->SetWidget( widget );
	}

	inline wxWindow* GetWidget() const
	{
		return m_containerWithButton->GetItemContainer()->GetWidget();
	}
};

WX_DECLARE_LIST( CEdWidgetItemListItem, CEdWidgetItemListItemList );


class CEdWidgetItemList : public wxScrolledWindow
{
private:

	class Timer : public CEdTimer
	{
		friend class CEdWidgetItemList;
		CEdWidgetItemList*	owner;
	public:

		void NotifyOnce()
		{
			owner->UpdateAddButtonPopupPosition();
		}
	};

	void							OnAddButtonClicked( wxCommandEvent& event );
	void							OnRemoveButtonClicked( wxCommandEvent& event );

protected:
	wxString						m_addButtonTitle;
	wxString						m_removeButtonTip;
	CEdWidgetItemListItemList*		m_items;
	wxButton*						m_addButton;
	wxPanel*						m_addButtonPopup;
	Timer							m_timer;

	void							OnAddButtonPopupPaint( wxEraseEvent& event );
	void							OnSize( wxSizeEvent& event );

	void							UpdateAddButtonPopupPosition();
	void							CreateAddButton();
	void							DestroyAddButton();

	virtual void					OnAddItem();

public:
	CEdWidgetItemList( wxWindow* parent, wxPoint pos = wxDefaultPosition, wxSize size = wxDefaultSize, long style = wxHSCROLL | wxSIMPLE_BORDER | wxVSCROLL );
	virtual ~CEdWidgetItemList();

	virtual bool					Enable( bool enable = true );

	void							SetAddButtonTitle( const wxString& title );
	wxString						GetAddButtonTitle() const;
	void							SetRemoveButtonTip( const wxString& tip );
	wxString						GetRemoveButtonTip() const;

	inline size_t GetItemCount() const
	{
		return m_items->GetCount();
	}

	inline CEdWidgetItemListItem* GetItem( size_t index ) const
	{
		return index < m_items->GetCount() ? (*m_items)[index] : NULL;
	}

	inline wxWindow* GetItemWidget( size_t index ) const
	{
		CEdWidgetItemListItem* item = GetItem( index );
		return item ? item->GetWidget() : NULL;
	}

	inline void SetItemWidget( size_t index, wxWindow* widget )
	{
		CEdWidgetItemListItem* item = GetItem( index );
		if ( item )
		{
			item->SetWidget( widget );
		}
	}

	virtual void					RemoveAllItems();
	virtual CEdWidgetItemListItem*	AddItem( wxWindow* widget );
	virtual bool					ShouldRemoveItem( CEdWidgetItemListItem* item ) const;
	virtual void					RemoveItem( CEdWidgetItemListItem* item );

};
