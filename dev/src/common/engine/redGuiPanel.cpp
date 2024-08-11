/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiPanel.h"

namespace RedGui
{
	CRedGuiPanel::CRedGuiPanel(Uint32 left, Uint32 top, Uint32 width, Uint32 height)
		: CRedGuiControl(left, top, width, height)
		, m_autoSize(false)
	{
		SetNeedKeyFocus( false );
	}

	CRedGuiPanel::CRedGuiPanel( const CRedGuiPanel& pattern )
		: CRedGuiControl((Uint32)pattern.GetPosition().X, (Uint32)pattern.GetPosition().Y, (Uint32)pattern.GetSize().X, (Uint32)pattern.GetSize().Y)
		, m_autoSize(pattern.m_autoSize)
	{
		SetNeedKeyFocus( false );
	}

	CRedGuiPanel::~CRedGuiPanel()
	{

	}

	void CRedGuiPanel::SetSize( const Vector2& size)
	{
		if(m_autoSize == true)
		{
			SetOriginalRect(Box2(GetPosition(), size));
			AdjustSizeToContent();
		}
		else
		{
			CRedGuiControl::SetSize(size);
		}
	}

	void CRedGuiPanel::SetSize(Int32 width, Int32 height)
	{
		SetSize(Vector2((Float)width, (Float)height));
	}

	void CRedGuiPanel::SetCoord(const Box2& coord)
	{
		if(m_autoSize == true)
		{
			SetOriginalRect(coord);
			AdjustSizeToContent();
		}
		else
		{
			CRedGuiControl::SetCoord(coord);
		}
	}

	void CRedGuiPanel::SetCoord(Int32 left, Int32 top, Int32 width, Int32 height)
	{
		SetCoord(Box2((Float)left, (Float)top, (Float)width, (Float)height));
	}

	void CRedGuiPanel::Draw()
	{
		GetTheme()->DrawPanel(this);
	}

	void CRedGuiPanel::SetAutoSize(Bool value)
	{
		if(m_autoSize == value)
		{
			return;
		}

		m_autoSize = value;

		if(m_autoSize == false)
		{
			SetCoord(GetOriginalRect());
		}
		else
		{
			SetOriginalRect(GetCoord());
			AdjustSizeToContent();
		}
	}

	Bool CRedGuiPanel::GetAutoSize() const
	{
		return m_autoSize;
	}

	void CRedGuiPanel::AddChild(CRedGuiControl* child)
	{
		child->EventPositionChanged.Bind(this, &CRedGuiPanel::NotifyPositionChanged);
		child->EventSizeChanged.Bind(this, &CRedGuiPanel::NotifySizeChanged);

		CRedGuiControl::AddChild(child);
		AdjustSizeToContent();
	}

	void CRedGuiPanel::RemoveChild(CRedGuiControl* child)
	{
		child->EventPositionChanged.Unbind(this, &CRedGuiPanel::NotifyPositionChanged);
		child->EventSizeChanged.Unbind(this, &CRedGuiPanel::NotifySizeChanged);

		CRedGuiControl::RemoveChild(child);
		AdjustSizeToContent();
	}

	void CRedGuiPanel::AdjustSizeToContent()
	{
		if(m_autoSize == true)
		{
			Vector2 newSize = GetOriginalRect().Max;

			for(Uint32 i=0; i<GetChildCount(); ++i)
			{
				CRedGuiControl* ctrl = GetChildAt(i);
				Vector2 ctrlSize;
				if(ctrl->GetDock() != DOCK_None)
				{
					ctrlSize = Vector2(ctrl->GetSize().X + ctrl->GetPosition().X, ctrl->GetSize().Y + ctrl->GetPosition().Y);
				}
				else
				{
					ctrlSize = Vector2(ctrl->GetPosition().X + ctrl->GetSize().X +
						ctrl->GetMargin().Min.X + ctrl->GetMargin().Max.X,
						ctrl->GetPosition().Y + ctrl->GetSize().Y +
						ctrl->GetMargin().Min.Y + ctrl->GetMargin().Max.Y);
				}

				newSize.X = Max<Float>(newSize.X, ctrlSize.X);
				newSize.Y = Max<Float>(newSize.Y, ctrlSize.Y);
			}

			if(newSize != GetSize())
			{
				CRedGuiControl::SetSize(newSize);
			}
		}
	}

	void CRedGuiPanel::NotifyPositionChanged( RedGui::CRedGuiEventPackage& eventPackage, Vector2 oldPosition, Vector2 newPosition)
	{
		RED_UNUSED( eventPackage );

		AdjustSizeToContent();
	}

	void CRedGuiPanel::NotifySizeChanged( RedGui::CRedGuiEventPackage& eventPackage, Vector2 oldSize, Vector2 newSize)
	{
		RED_UNUSED( eventPackage );

		AdjustSizeToContent();
	}

} // namespace RedGui

#endif	// NO_RED_GUI
