/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiComboBox : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiComboBox(Uint32 x, Uint32 y, Uint32 width, Uint32 height, Uint32 listHeight=150);
		virtual ~CRedGuiComboBox();

		// Events
		Event2_PackageInt32 EventSelectedIndexChanged;

		// add items
		void AddItem(const String& item, RedGuiAny userData = nullptr);
		void ClearAllItems();
		Uint32 GetItemCount() const;

		// selecting
		Int32 GetSelectedIndex() const;
		String GetSelectedItemName() const;
		void SetSelectedIndex(Int32 value);
		void ClearSelection();

		void Draw();

	protected:
		void ShowList();
		void HideList();

		void NotifyButtonPressed( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, EMouseButton button);
		void NotifySelectedItemChanged( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedIndex);
		void NotifySizeChanged( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& oldSize, const Vector2& newSize );

		void OnKeyChangeRootFocus( Bool focus );
		Bool OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data );

		virtual void OnPendingDestruction() override final;

	private:
		CRedGuiButton*	m_button;		//!<
		CRedGuiList*	m_list;			//!<

		Bool			m_listShow;		//!<
		Int32			m_itemIndex;	//!<
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
