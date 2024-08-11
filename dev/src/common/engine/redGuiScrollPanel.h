/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiScrollPanel : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiScrollPanel(Uint32 left, Uint32 top, Uint32 width, Uint32 height);
		virtual ~CRedGuiScrollPanel();

		// Event
		Event2_PackageVector2 EventMoveDelta;

		// Set control size
		virtual void SetSize( const Vector2& size);
		virtual void SetSize(Int32 width, Int32 height);
		// Set control position and size
		virtual void SetCoord(const Box2& coord);
		virtual void SetCoord(Int32 left, Int32 top, Int32 width, Int32 height);

		void SetVisibleVScroll(Bool value);
		Bool IsVisibleVScroll() const;

		void SetVisibleHScroll(Bool value);
		Bool IsVisibleHScroll() const;

		Box2 GetViewCoord() const;

		void ResetScrollPosition();

		void SetPointer(EMousePointer pointer);
	
		void Draw();

		virtual void AddChild(CRedGuiControl* child);
		virtual void RemoveChild(CRedGuiControl* child);

		void NotifyMouseWheel( RedGui::CRedGuiEventPackage& eventPackage, Int32 delta);

		virtual void UpdateControl();

		void SetOutOfDate();

		//
		void ScrollToDown();
		void Move( const Vector2& delta);

	protected:
		void NotifyMousePressed( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button);
		void NotifyMouseReleased(CRedGuiControl* sender, const Vector2& mousePosition, enum EMouseButton button);
		void NotifyEventSizeChanged( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& oldsize, Vector2 newSize);
		void NotifyScrollChangePosition( RedGui::CRedGuiEventPackage& eventPackage, Uint32 position);
		void NotifyCroppRectEventSizeChanged( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& oldsize, const Vector2& newSize );
		void NotifyKeyButtonPressed( RedGui::CRedGuiEventPackage& eventPackage, enum EInputKey key, Char text );
		void NotifyKeyButtonReleased( RedGui::CRedGuiEventPackage& eventPackage, enum EInputKey key);
		void NotifyKeyLostFocus( RedGui::CRedGuiEventPackage& eventPackage, CRedGuiControl* newFocused);
		void NotifyMouseLostFocus( RedGui::CRedGuiEventPackage& eventPackage, CRedGuiControl* newFocused);
		void NotifyKeySetFocus( RedGui::CRedGuiEventPackage& eventPackage, CRedGuiControl* oldFocused);
		void NotifyMouseSetFocus( RedGui::CRedGuiEventPackage& eventPackage, CRedGuiControl* oldFocused);
		void NotifyMouseButtonPressed( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button );
		void NotifyMouseButtonReleased( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button );

		void EraseContent();
		void UpdateView();

		void UpdateScrollPosition();
		void UpdateScrollSize();

		virtual Vector2 GetContentSize();
		virtual Vector2 GetContentPosition();

		virtual Vector2 GetViewSize();
		virtual void SetContentPosition( const Vector2& value);

		virtual Uint32 GetVScrollPage();
		virtual Uint32 GetHScrollPage();

		Bool OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data );
		
		virtual void OnPendingDestruction() override;

	private:
		CRedGuiPanel* m_realClient;
		CRedGuiPanel* m_croppClient;

		CRedGuiScrollBar* m_vScroll;
		CRedGuiScrollBar* m_hScroll;

		Bool m_visibleHScroll;
		Bool m_visibleVScroll;

		Uint32 m_vRange;
		Uint32 m_hRange;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
