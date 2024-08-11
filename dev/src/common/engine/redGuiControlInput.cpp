/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiControlInput.h"
#include "redGuiControl.h"

namespace RedGui
{
	CRedGuiControlInput::CRedGuiControlInput()
		: m_needToolTip(false)
		, m_inheritPick(false)
		, m_needKeyFocus(false)
		, m_needMouseFocus(true)
		, m_rootKeyFocus(false)
		, m_rootMouseFocus(false)
	{
		/* intentionally empty */
	}

	CRedGuiControlInput::~CRedGuiControlInput()
	{
		/*intentionally empty*/
	}

	void CRedGuiControlInput::PropagateMouseLostFocus(CRedGuiControl* newControl)
	{
		OnMouseLostFocus(newControl);
		EventMouseLostFocus(static_cast<CRedGuiControl*>(this), newControl);
	}

	void CRedGuiControlInput::PropagateMouseSetFocus(CRedGuiControl* oldControl)
	{
		OnMouseSetFocus(oldControl);
		EventMouseSetFocus(static_cast<CRedGuiControl*>(this), oldControl);
	}

	void CRedGuiControlInput::PropagateMouseDrag( const Vector2& mousePosition, EMouseButton button)
	{
		OnMouseDrag(mousePosition, button);
		EventMouseDrag(static_cast<CRedGuiControl*>(this), mousePosition, button);
	}

	void CRedGuiControlInput::PropagateMouseMove( const Vector2& mousePosition)
	{
		OnMouseMove(mousePosition);
		EventMouseMove(static_cast<CRedGuiControl*>(this), mousePosition);
	}

	void CRedGuiControlInput::PropagateMouseWheel(Int32 delta)
	{
		OnMouseWheel(delta);
		EventMouseWheel(static_cast<CRedGuiControl*>(this), delta);
	}

	void CRedGuiControlInput::PropagateMouseButtonPressed( const Vector2& mousePosition, EMouseButton button )
	{
		OnMouseButtonPressed(mousePosition, button);
		EventMouseButtonPressed(static_cast<CRedGuiControl*>(this), mousePosition, button);
	}

	void CRedGuiControlInput::PropagateMouseButtonReleased( const Vector2& mousePosition, EMouseButton button )
	{
		OnMouseButtonReleased(mousePosition, button);
		EventMouseButtonReleased(static_cast<CRedGuiControl*>(this), mousePosition, button);
	}

	void CRedGuiControlInput::PropagateMouseButtonClick( const Vector2& mousePosition, EMouseButton button )
	{
		OnMouseButtonClick( mousePosition, button );
		EventMouseButtonClick(static_cast<CRedGuiControl*>(this), mousePosition, button );
	}

	void CRedGuiControlInput::PropagateMouseButtonDoubleClick( const Vector2& mousePosition, enum EMouseButton button )
	{
		OnMouseButtonDoubleClick( mousePosition, button );
		EventMouseButtonDoubleClick( static_cast<CRedGuiControl*>(this), mousePosition, button );
	}

	void CRedGuiControlInput::PropagateKeyLostFocus(CRedGuiControl* newControl)
	{
		OnKeyLostFocus(newControl);
		EventKeyLostFocus(static_cast<CRedGuiControl*>(this), newControl);
	}

	void CRedGuiControlInput::PropagateKeySetFocus(CRedGuiControl* oldControl)
	{
		OnKeySetFocus(oldControl);
		EventKeySetFocus(static_cast<CRedGuiControl*>(this), oldControl);
	}

	void CRedGuiControlInput::PropagateKeyButtonPressed(EInputKey key, Char text)
	{
		OnKeyButtonPressed(key, text);
		EventKeyButtonPressed(static_cast<CRedGuiControl*>(this), key, text);
	}

	void CRedGuiControlInput::PropagateKeyButtonReleased(EInputKey key)
	{
		OnKeyButtonReleased(key);
		EventKeyButtonReleased(static_cast<CRedGuiControl*>(this), key);
	}

	void CRedGuiControlInput::PropagateMouseChangeRootFocus(Bool focus)
	{
		OnMouseChangeRootFocus(focus);
		EventRootKeyChangeFocus(static_cast<CRedGuiControl*>(this), focus);
	}

	void CRedGuiControlInput::PropagateKeyChangeRootFocus(Bool focus)
	{
		OnKeyChangeRootFocus(focus);
		EventRootMouseChangeFocus(static_cast<CRedGuiControl*>(this), focus);
	}

	void CRedGuiControlInput::PropagateToolTip( Bool visible, const Vector2& position )
	{
		OnToolTip(visible, position);
		EventToolTip(static_cast<CRedGuiControl*>(this), visible, position);
	}

	Bool CRedGuiControlInput::PropagateInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data )
	{
		return OnInternalInputEvent( event, data );
	}

	void CRedGuiControlInput::SetNeedToolTip(Bool value)
	{
		m_needToolTip = value;
	}

	Bool CRedGuiControlInput::GetNeedToolTip() const
	{
		return m_needToolTip;
	}

	void CRedGuiControlInput::SetPointer(EMousePointer value)
	{
		m_pointer = value;
	}
	
	EMousePointer CRedGuiControlInput::GetPointer() const
	{
		return m_pointer;
	}

	void CRedGuiControlInput::SetNeedKeyFocus(Bool value)
	{
		m_needKeyFocus = value;
	}

	Bool CRedGuiControlInput::GetNeedKeyFocus() const
	{
		return m_needKeyFocus;
	}

	void CRedGuiControlInput::SetNeedMouseFocus(Bool value)
	{
		m_needMouseFocus = value;
	}

	Bool CRedGuiControlInput::GetNeedMouseFocus() const
	{
		return m_needMouseFocus;
	}

	void CRedGuiControlInput::SetInheritPick(Bool value)
	{
		m_inheritPick = value;
	}

	Bool CRedGuiControlInput::GetInheritPick() const
	{
		return m_inheritPick;
	}

	void CRedGuiControlInput::SetRootMouseFocus(Bool value)
	{
		m_rootMouseFocus = value;
	}

	void CRedGuiControlInput::SetRootKeyFocus(Bool value)
	{
		m_rootKeyFocus = value;
	}

	Bool CRedGuiControlInput::GetRootMouseFocus() const
	{
		return m_rootMouseFocus;
	}

	Bool CRedGuiControlInput::GetRootKeyFocus() const
	{
		return m_rootKeyFocus;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
