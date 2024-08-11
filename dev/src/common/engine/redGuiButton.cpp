/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiManager.h"
#include "redGuiLabel.h"
#include "redGuiImage.h"
#include "redGuiButton.h"
#include "fonts.h"

namespace RedGui
{
	CRedGuiButton::CRedGuiButton(Uint32 left, Uint32 top, Uint32 width, Uint32 height)
		: CRedGuiControl(left, top, width, height)
		, m_isMousePressed(false)
		, m_image(nullptr)
		, m_imageMode(false)
		, m_label(nullptr)
		, m_toggleMode(false)
		, m_isOn(false)
	{
		SetTheme( RED_NAME( RedGuiGradientTheme ) );
		SetNeedKeyFocus( true );

		m_label = new CRedGuiLabel(0, 0, GetWidth(), GetHeight());
		m_label->SetEnabled(false);		
		m_label->SetAlign(IA_MiddleCenter);
		AddChild(m_label);
	}

	CRedGuiButton::~CRedGuiButton()
	{
		/* intentionally empty */
	}

	void CRedGuiButton::OnMouseButtonPressed( const Vector2& mousePosition, enum EMouseButton button)
	{
		if(button == MB_Left)
		{
			m_isMousePressed = true;
		}
		GRedGui::GetInstance().GetInputManager()->SetKeyFocusControl( this );
		CRedGuiControl::OnMouseButtonPressed(mousePosition, button);
	}

	void CRedGuiButton::OnMouseButtonReleased( const Vector2& mousePosition, enum EMouseButton button)
	{
		if(button == MB_Left)
		{
			if(m_isMousePressed == true)
			{
				m_isMousePressed = false;
				ExecuteClick();

				SetState(STATE_Normal);
			}
		}
		CRedGuiControl::OnMouseButtonReleased(mousePosition, button);
	}

	ERedGuiState CRedGuiButton::GetState() const
	{
		if(m_toggleMode == true && GetEnabled() == true)
		{
			if(m_isOn == true)
			{
				return STATE_Pushed;
			}
		}

		return CRedGuiControl::GetState();
	}

	void CRedGuiButton::SetImageMode(Bool value)
	{
		m_imageMode = value;
	}

	void CRedGuiButton::SetImage( THandle< CBitmapTexture > texture)
	{
		if(m_image != nullptr)
		{
			RemoveChild(m_image);
			m_image = nullptr;
		}

		SetPadding(Box2(1, 1, 1, 1));
		m_image = new CRedGuiImage(0, 0, GetWidth(), GetHeight());
		m_image->SetImage(texture);
		m_image->SetEnabled(false);
		AddChild(m_image);
		m_image->SetDock(DOCK_Fill);
		SetOutOfDate();
	}

	void CRedGuiButton::SetImage(const String& pathToFile)
	{
		if(m_image != nullptr)
		{
			RemoveChild(m_image);
			m_image = nullptr;
		}

		SetPadding(Box2(1, 1, 1, 1));
		m_image = new CRedGuiImage(0, 0, GetWidth(), GetHeight());
		m_image->SetImage(pathToFile);
		m_image->SetEnabled(false);
		AddChild(m_image);
		m_image->SetDock(DOCK_Fill);
		SetOutOfDate();
	}

	void CRedGuiButton::SetImage( CGatheredResource& resource )
	{
		if(m_image != nullptr)
		{
			RemoveChild(m_image);
			m_image = nullptr;
		}

		SetPadding(Box2(1, 1, 1, 1));
		m_image = new CRedGuiImage(0, 0, GetWidth(), GetHeight());
		m_image->SetImage( resource );
		m_image->SetEnabled(false);
		AddChild(m_image);
		m_image->SetDock(DOCK_Fill);
		SetOutOfDate();
	}

	void CRedGuiButton::SetToggleMode(Bool value)
	{
		m_toggleMode = value;
	}

	Bool CRedGuiButton::GetToggleMode() const
	{
		return m_toggleMode;
	}

	void CRedGuiButton::SetToggleValue( Bool value, Bool silentChange /*= false*/ )
	{
		if( value != m_isOn )
		{
			m_isOn = value;

			if( silentChange == false )
			{
				EventCheckedChanged( this, m_isOn );
			}
		}
	}

	Bool CRedGuiButton::GetToggleValue() const
	{
		return m_isOn;
	}

	Bool CRedGuiButton::GetImageMode() const
	{
		return m_imageMode;
	}

	void CRedGuiButton::SetFitToText(Bool value)
	{
		if(m_fitToText == value)
		{
			return;
		}

		m_fitToText = value;
		FitToText();
	}

	Bool CRedGuiButton::GetFitToText() const
	{
		return m_fitToText;
	}

	void CRedGuiButton::SetText(const String& text, const Color& textColor/* = Color::WHITE*/)
	{
		m_label->SetText(text, textColor);
		FitToText();
	}

	String CRedGuiButton::GetText() const
	{
		return m_label->GetText();
	}

	void CRedGuiButton::SetTextAlign(EInternalAlign align)
	{
		m_label->SetAlign(align);
		SetOutOfDate();
	}

	EInternalAlign CRedGuiButton::GetTextAlign() const
	{
		return m_label->GetAlign();
	}

	CRedGuiImage* CRedGuiButton::GetImageBox()
	{
		return m_image;
	}

	void CRedGuiButton::Draw()
	{
		GetTheme()->DrawButton(this);
	}

	void CRedGuiButton::FitToText()
	{
		if(m_fitToText == true)
		{
			Vector2 oldSize = GetSize();
			Uint32 offsetForBetterView = 20;

			Int32 unusedX, unusedY;
			Uint32 width, height;
			if( CFont* font = GetFont() )
			{
				GetFont()->GetTextRectangle(m_label->GetText(), unusedX, unusedY, width, height);
				SetSize(width + offsetForBetterView, (Uint32)oldSize.Y);
			}
			SetOriginalRect(Box2(GetPosition().X, GetPosition().Y, GetSize().X, GetSize().Y));
		}

		SetOutOfDate();
	}

	void CRedGuiButton::SetTextColor( const Color& color )
	{
		m_label->SetForegroundColor(color);
	}

	Color CRedGuiButton::GetTextColor() const
	{
		return m_label->GetForegroundColor();
	}

	void CRedGuiButton::OnSizeChanged( const Vector2& oldSize, const Vector2& newSize )
	{
		m_label->SetOutOfDate();
		SetOutOfDate();
	}

	void CRedGuiButton::ExecuteClick()
	{
		m_isOn = !m_isOn;

		if(m_toggleMode == false)
		{
			EventButtonClicked(this);
		}
		else
		{
			EventCheckedChanged(this, m_isOn);
		}
	}

	Bool CRedGuiButton::OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data )
	{
		if( CRedGuiControl::OnInternalInputEvent( event, data ) == true )
		{
			return true;
		}

		if( event == RGIE_Execute || event == RGIE_Select )
		{
			ExecuteClick();
			return true;
		}

		return false;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
