/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiTypes.h"

namespace RedGui
{
	class CRedGuiControlInput
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiInternals );
	public:
		CRedGuiControlInput();
		virtual ~CRedGuiControlInput();

		// Desc: Control lost mouse focus
		// Signature: void func(CRedGuiControl* sender, CRedGuiControl* newFocusedControl)
		// Param0: control that called this event
		// Param1: new control with mouse focus
		Event2_PackageControl EventMouseLostFocus;

		// TODO
		Event2_PackageControl EventMouseSetFocus;

		// TODO
		Event2_PackageVector2 EventMouseMove;

		// TODO
		Event2_PackageInt32 EventMouseWheel;

		// TODO
		Event3_PackageVector2MouseButton EventMouseButtonPressed;

		// TODO
		Event3_PackageVector2MouseButton EventMouseButtonReleased;

		// TODO
		Event3_PackageVector2MouseButton EventMouseDrag;

		// TODO
		Event3_PackageVector2MouseButton EventMouseButtonClick;

		// TODO
		Event3_PackageVector2MouseButton EventMouseButtonDoubleClick;

		// TODO
		Event2_PackageControl EventKeyLostFocus;

		// TODO
		Event2_PackageControl EventKeySetFocus;

		// TODO
		Event3_PackageInputKeyChar EventKeyButtonPressed;

		// TODO
		Event2_PackageInputKey EventKeyButtonReleased;

		// TODO
		Event2_PackageBool EventRootKeyChangeFocus;

		// TODO
		Event2_PackageBool EventRootMouseChangeFocus;

		// TODO
		Event3_PackageBoolVector2 EventToolTip;

		// Set need tool tip mode flag
		void SetNeedToolTip(Bool value);
		// Get need tool tip mode flag
		Bool GetNeedToolTip() const;

		// Set mouse pointer for this control
		void SetPointer(EMousePointer value);
		// Get mouse pointer name for this control
		EMousePointer GetPointer() const;

		// Set need key focus flag
		void SetNeedKeyFocus(Bool value);
		// Get need key focus
		Bool GetNeedKeyFocus() const;

		// Set need mouse focus flag
		void SetNeedMouseFocus(Bool value);
		// Get need mouse focus
		Bool GetNeedMouseFocus() const;

		// Set inherit mode flag
		void SetInheritPick(Bool value);
		// Get inherit mode flag
		Bool GetInheritPick() const;

		void SetRootMouseFocus(Bool value);
		void SetRootKeyFocus(Bool value);

		Bool GetRootMouseFocus() const;
		Bool GetRootKeyFocus() const;

		//////////////////////////////////////////////////////////////////////////
		// internal
		// 
		virtual void PropagateMouseLostFocus(CRedGuiControl* newControl);
		virtual void PropagateMouseSetFocus(CRedGuiControl* oldControl);
		virtual void PropagateMouseDrag( const Vector2& mousePosition, enum EMouseButton button);
		virtual void PropagateMouseMove( const Vector2& mousePosition);
		virtual void PropagateMouseWheel(Int32 delta);
		virtual void PropagateMouseButtonPressed( const Vector2& mousePosition, enum EMouseButton button );
		virtual void PropagateMouseButtonReleased( const Vector2& mousePosition, enum EMouseButton button );
		virtual void PropagateMouseButtonClick( const Vector2& mousePosition, enum EMouseButton button );
		virtual void PropagateMouseButtonDoubleClick( const Vector2& mousePosition, enum EMouseButton button );
		virtual void PropagateKeyLostFocus(CRedGuiControl* newControl);
		virtual void PropagateKeySetFocus(CRedGuiControl* oldControl);
		virtual void PropagateKeyButtonPressed(enum EInputKey key, Char text);
		virtual void PropagateKeyButtonReleased(enum EInputKey key);
		virtual void PropagateMouseChangeRootFocus(Bool focus);
		virtual void PropagateKeyChangeRootFocus(Bool focus);
		virtual void PropagateToolTip(Bool visible, const Vector2& position);
		virtual Bool PropagateInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data );

	protected:
		virtual void OnMouseLostFocus(CRedGuiControl* newControl)										{ /*intentionally empty*/ };
		virtual void OnMouseSetFocus(CRedGuiControl* oldControl)										{ /*intentionally empty*/ };
		virtual void OnMouseDrag( const Vector2& mousePosition, enum EMouseButton button)				{ /*intentionally empty*/ };
		virtual void OnMouseMove( const Vector2& mousePosition)											{ /*intentionally empty*/ };
		virtual void OnMouseWheel(Int32 delta)															{ /*intentionally empty*/ };
		virtual void OnMouseButtonPressed( const Vector2& mousePosition, enum EMouseButton button)		{ /*intentionally empty*/ };
		virtual void OnMouseButtonReleased( const Vector2& mousePosition, enum EMouseButton button)		{ /*intentionally empty*/ };
		virtual void OnMouseButtonClick( const Vector2& mousePosition, enum EMouseButton button )		{ /*intentionally empty*/ };
		virtual void OnMouseButtonDoubleClick( const Vector2& mousePosition, enum EMouseButton button )	{ /*intentionally empty*/ };
		virtual void OnKeyLostFocus(CRedGuiControl* newControl)											{ /*intentionally empty*/ };
		virtual void OnKeySetFocus(CRedGuiControl* oldControl)											{ /*intentionally empty*/ };
		virtual void OnKeyButtonPressed(enum EInputKey key, Char text)									{ /*intentionally empty*/ };
		virtual void OnKeyButtonReleased(enum EInputKey key)											{ /*intentionally empty*/ };
		virtual void OnMouseChangeRootFocus(Bool focus)													{ /*intentionally empty*/ };
		virtual void OnKeyChangeRootFocus(Bool focus)													{ /*intentionally empty*/ };
		virtual void OnToolTip(Bool visible, const Vector2& position)									{ /*intentionally empty*/ };

		virtual Bool OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data )			{ return false;			  };

	private:
		EMousePointer m_pointer;

		Bool m_needToolTip;
		Bool m_inheritPick;
		Bool m_needKeyFocus;
		Bool m_needMouseFocus;

		Bool m_rootMouseFocus;
		Bool m_rootKeyFocus;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
