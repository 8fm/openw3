/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiButton.h"
#include "redGuiList.h"
#include "redGuiImage.h"
#include "redGuiManager.h"
#include "redGuiComboBox.h"

namespace RedGui
{
	CRedGuiComboBox::CRedGuiComboBox(Uint32 x, Uint32 y, Uint32 width, Uint32 height, Uint32 listHeight)
		: CRedGuiControl(x, y, width, height)
		, m_listShow(true)
		, m_itemIndex(-1)
	{
		SetNeedKeyFocus(true);
		SetBorderVisible(false);
		SetBackgroundColor(Color::CLEAR);

		m_button = new CRedGuiButton(0,0, GetWidth(), GetHeight());
		m_button->SetPadding( Box2(5, 0, 5,0 ) );
		m_button->SetMargin( Box2( 2, 2, 2, 2 ) );
		m_button->SetTextAlign(IA_MiddleLeft);
		m_button->SetBorderVisible(false);
		m_button->SetDock(DOCK_Fill);
		m_button->SetNeedKeyFocus( false );
		m_button->EventMouseButtonPressed.Bind(this, &CRedGuiComboBox::NotifyButtonPressed);
		AddChild(m_button);

		CRedGuiImage* imageArrow = new CRedGuiImage(0,0, 16, 16);
		AddChild(imageArrow);
		imageArrow->SetAlign(IA_MiddleRight);
		imageArrow->SetImage( Resources::GDownArrowIcon );
		imageArrow->SetEnabled(false);

		// add list of items and attach callback function to select item
		m_list = new CRedGuiList(0, GetHeight(), GetWidth(), listHeight);
		m_list->AppendColumn( TXT(""), GetWidth() );
		m_list->SetColLabelsVisible( false );
		m_list->SetBorderVisible(false);
		AddChild(m_list);
		m_list->EventSelectedItem.Bind(this, &CRedGuiComboBox::NotifySelectedItemChanged);
		m_list->SetSelectionMode( SM_None );
		m_list->SetTextAlign( IA_MiddleLeft );
		m_list->SetVisible(false);
		m_list->SetBackgroundColor(Color::CLEAR);
		m_list->SetBorderVisible(false);
		m_list->AttachToLayer(TXT("Menus"));
		m_list->SetCroppedParent(nullptr);
		m_list->SetNeedKeyFocus( false );

		EventSizeChanged.Bind( this, &CRedGuiComboBox::NotifySizeChanged );
	}

	CRedGuiComboBox::~CRedGuiComboBox()
	{
		
	}

	void CRedGuiComboBox::OnPendingDestruction()
	{
		m_button->EventMouseButtonPressed.Unbind(this, &CRedGuiComboBox::NotifyButtonPressed);
		m_list->EventSelectedItem.Unbind(this, &CRedGuiComboBox::NotifySelectedItemChanged);
	}

	void CRedGuiComboBox::AddItem(const String& item, RedGuiAny userData/* = nullptr*/)
	{
		m_list->AddItem(item, Color::WHITE, userData);
	}

	void CRedGuiComboBox::ClearAllItems()
	{
		m_list->RemoveAllItems();
	}

	Int32 CRedGuiComboBox::GetSelectedIndex() const
	{
		return m_itemIndex;
	}

	void CRedGuiComboBox::SetSelectedIndex(Int32 value)
	{
		if(value >= 0)
		{
			m_itemIndex = value;
			m_list->SetSelection( value );
			m_button->SetText( m_list->GetItemText( value ) );
		}
		else
		{
			ClearSelection();
		}
	}

	void CRedGuiComboBox::ClearSelection()
	{
		m_itemIndex = -1;
		m_list->DeselectAll();
		m_button->SetText(TXT(""));
	}

	void CRedGuiComboBox::ShowList()
	{
		if(m_list->GetItemCount() == 0)
		{
			return;
		}

		// clear selection in the list
		m_list->DeselectAll();
		m_list->SetFirstItem( 0 );

		// set list position and size
		m_list->SetPosition(Vector2(0.0, (Float)GetHeight()));
		Uint32 width = Max<Uint32>((Uint32)m_list->GetSize().X, (Uint32)m_button->GetSize().X);
		m_list->SetSize(Vector2((Float)width, m_list->GetSize().Y));
		SetSize(Vector2(m_list->GetSize().X, m_button->GetSize().Y + m_list->GetSize().Y));
		
		m_list->SetVisible(true);
		m_listShow = false;

		GRedGui::GetInstance().GetInputManager()->SetKeyFocusControl(m_list);
		SetOutOfDate();
	}

	void CRedGuiComboBox::HideList()
	{
		SetSize(m_button->GetSize());
		m_list->SetVisible(false);
		m_listShow = true;
		SetOutOfDate();
	}

	void CRedGuiComboBox::NotifyButtonPressed( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, EMouseButton button)
	{
		RED_UNUSED( eventPackage );

		if(button != MB_Left)
		{
			return;
		}

		if(m_listShow == false)
		{
			HideList();
		}
		else
		{
			ShowList();
		}
	}

	void CRedGuiComboBox::NotifySelectedItemChanged( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedIndex)
	{
		if(m_itemIndex == selectedIndex)
		{
			m_list->SetVisible(false);
			m_listShow = true;
			return;
		}

		m_itemIndex = selectedIndex;
		m_button->SetText((selectedIndex != -1) ? m_list->GetItemText( selectedIndex ) : TXT(""));
		EventSelectedIndexChanged(this, selectedIndex);
		HideList();

		GRedGui::GetInstance().GetInputManager()->SetKeyFocusControl(this);
	}

	void CRedGuiComboBox::Draw()
	{
		GetTheme()->DrawPanel( this );
	}

	String CRedGuiComboBox::GetSelectedItemName() const
	{
		return m_button->GetText();
	}

	Uint32 CRedGuiComboBox::GetItemCount() const
	{
		return m_list->GetItemCount();
	}

	void CRedGuiComboBox::NotifySizeChanged( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& oldSize, const Vector2& newSize )
	{
		RED_UNUSED( eventPackage );

		m_list->SetSize( Vector2( newSize.X, m_list->GetSize().Y ) );
		m_list->SetColumnWidth( 0, (Int32)newSize.X );
	}

	void CRedGuiComboBox::OnKeyChangeRootFocus( Bool focus )
	{
		if( focus == false )
		{
			HideList();
		}
	}

	Bool CRedGuiComboBox::OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data )
	{
		Bool result = false;

		if( event == RGIE_Down || event == RGIE_Select || event == RGIE_Execute )
		{
			ShowList();
			result = true;
		}
		if( event == RGIE_Back )
		{
			HideList();
			result = true;
		}

		if( m_list->GetVisible() == true )
		{
			if( m_list->PropagateInternalInputEvent( event, data ) == true )
			{
				result = true;
			}
		}

		return result;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
