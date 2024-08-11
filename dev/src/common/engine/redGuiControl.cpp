/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiControl.h"
#include "redGuiManager.h"
#include "redGuiLayer.h"
#include "redGuiPanel.h"
#include "redGuiLabel.h"
#include "redGuiTheme.h"
#include "redGuiAnchor.h"
#include "redGuiDock.h"

namespace
{
	struct STabIndexCompareFunc
	{
		RED_INLINE Bool operator()( const RedGui::CRedGuiControl* ctrl1, const RedGui::CRedGuiControl* ctrl2 ) const { return ctrl1->GetTabIndex() < ctrl2->GetTabIndex(); }
	};
}

namespace RedGui
{
	CRedGuiControl::CRedGuiControl(Uint32 left, Uint32 top, Uint32 width, Uint32 height)
		: CRedGuiCroppedRect( Box2( (Float)left, (Float)top, (Float)width, (Float)height ) )
		, m_visible(true)
		, m_enabled(true)
		, m_parent(nullptr)
		, m_controlClient(nullptr)
		, m_inheritsVisible(true)
		, m_inheritsEnabled(true)
		, m_state(STATE_Normal)
		, m_toolTip(nullptr)
		, m_dock(DOCK_None)
		, m_anchor(ANCHOR_Default)
		, m_isOutOfDate(true)
		, m_tabIndex( 0 )
		, m_isDisposed( false )
	{
		// set default theme for control
		SetTheme( RED_NAME( RedGuiDefaultTheme ) );
		SetOutOfDate();
	}

	CRedGuiControl::~CRedGuiControl()
	{
		GRedGui::GetInstance().InvalidateControl( this );
	}

	void CRedGuiControl::SetPosition(const Vector2& position)
	{
		Vector2 oldPosition = m_coord.Min;
		CRedGuiCroppedRect::SetPosition(position);
		UpdateAbsolutePosition();

		PropagatePositionChanged( oldPosition, m_coord.Min );

		SetOutOfDate();
	}

	void CRedGuiControl::SetSize(const Vector2& size)
	{
		Vector2 oldSize = m_coord.Max;
		CRedGuiCroppedRect::SetSize(size);

		PropagateSizeChanged( oldSize, m_coord.Max );

		SetOutOfDate();
	}

	void CRedGuiControl::SetCoord(const Box2& coord)
	{
		Box2 oldCoord = m_coord;
		SetPosition(coord.Min);
		SetSize(coord.Max);
	}

	void CRedGuiControl::SetPosition(Int32 left, Int32 top)
	{
		CRedGuiControl::SetPosition(Vector2((Float)left, (Float)top));
	}

	void CRedGuiControl::SetSize(Int32 width, Int32 height)
	{
		CRedGuiControl::SetSize(Vector2((Float)width, (Float)height));
	}

	void CRedGuiControl::SetCoord(Int32 left, Int32 top, Int32 width, Int32 height)
	{
		CRedGuiControl::SetCoord(Box2((Float)left, (Float)top, (Float)width, (Float)height));
	}

	void CRedGuiControl::SetVisible(Bool value)
	{
		if(m_visible == value)
		{
			return;
		}

		m_visible = value;
		UpdateVisible();
		EventVisibleChanged(this, m_visible);

		if(GetParent() != nullptr)
		{
			GetParent()->SetOutOfDate();
		}
		SetOutOfDate();
	}

	void CRedGuiControl::ForcePick(CRedGuiControl* control)
	{
		if(m_controlClient != nullptr)
		{
			m_controlClient->ForcePick(control);
		}

		if(m_children.Exist(control) == true)
		{
			m_children.Remove(control);
			m_children.Insert(0, control);
		}
	}

	void CRedGuiControl::UpdateAnchor( const Vector2& oldSize, const Vector2& newSize )
	{
		Bool needMove = false;
		Bool needSize = false;
		Box2 coord = m_coord;

		CRedGuiAnchor align(m_anchor);

		// horizontal
		Float horizontalDelta = newSize.X - oldSize.X;
		if(align.IsHStretch() == true)
		{
			coord.Max.X = m_coord.Max.X + (newSize.X - oldSize.X);
			needSize = true;
		}
		else if(align.IsRight() == true)
		{
			coord.Min.X = m_coord.Min.X + (newSize.X - oldSize.X);
			needMove = true;
		}

		// vertical
		Float verticalDelta = newSize.Y - oldSize.Y;
		if(align.IsVStretch() == true)
		{
			coord.Max.Y = m_coord.Max.Y + (newSize.Y - oldSize.Y);
			needSize = true;
		}
		else if(align.IsBottom() == true)
		{
			coord.Min.Y = m_coord.Min.Y + (newSize.Y - oldSize.Y);
			needMove = true;
		}

		// anchor
		if(needMove == true)
		{
			if(needSize == true)
			{
				CRedGuiCroppedRect::SetCoord(coord);
			}
			else
			{
				CRedGuiCroppedRect::SetPosition(coord.Min);
			}
		}
		else if(needSize == true)
		{
			CRedGuiCroppedRect::SetSize(coord.Max);
		}
	}

	Bool CRedGuiControl::CheckPoint( const Vector2& position ) const
	{
		const Int32 left = (Int32)position.X;
		const Int32 top = (Int32)position.Y;

		return ( (GetAbsoluteLeft() <= left) 
			&& (GetAbsoluteTop() <= top) 
			&& (GetAbsoluteLeft() + GetWidth() >= left) 
			&& (GetAbsoluteTop() + GetHeight() >= top) ) == true;
	}

	void CRedGuiControl::SetState(ERedGuiState state)
	{
		if(GetEnabled() == true)
		{
			if(state != STATE_Disabled)
			{
				m_state = state;
			}
		}
		else
		{
			m_state = STATE_Disabled;
		}
	}

	IRedGuiLayerItem* CRedGuiControl::GetLayerItemByPoint( const Vector2& position ) const
	{
		if(m_enabled == false || m_visible == false ||
			(GetNeedMouseFocus() == false && GetInheritPick() == false) ||
			CheckPoint( position ) == false)
		{
			return nullptr;
		}

		for(Int32 i=m_children.Size()-1; i>=0; --i)
		{
			if( m_children[i]->IsDisposed() == false )
			{
			IRedGuiLayerItem* item = m_children[i]->GetLayerItemByPoint( position );
			if(item != nullptr)
			{
				return item;
			}
		}
		}

		return (GetInheritPick() == true) ? nullptr : const_cast<CRedGuiControl*>(this);
	}
	
	void CRedGuiControl::SetControlClient(CRedGuiControl* control)
	{
		if(control != nullptr)
		{
			AddChild(control);
			m_controlClient = control;
			m_controlClient->m_croppedParent = this;
			m_controlClient->SetOutOfDate();
		}
		else
		{
			m_controlClient = nullptr;
		}
		SetOutOfDate();
	}

	void CRedGuiControl::UpdateAbsolutePosition()
	{
		if(m_parent == nullptr)
		{
			if(m_croppedParent == nullptr)
			{
				this->m_absolutePosition = m_coord.Min;
			}
			else
			{
				this->m_absolutePosition = this->m_croppedParent->GetAbsolutePositionViewArea() + m_coord.Min;
			}
		}
		else
		{
			this->m_absolutePosition = this->m_parent->GetAbsolutePositionViewArea() + m_coord.Min;
		}

		for(Uint32 i=0; i<m_children.Size(); ++i)
		{
			m_children[i]->UpdateAbsolutePosition();
		}
	}

	void CRedGuiControl::UpdateVisible()
	{
		m_inheritsVisible = (m_parent == nullptr) || (m_parent->GetVisible() == true && m_parent->GetInheritedVisible() == true);
		Bool value = m_visible && m_inheritsVisible;

		for(Uint32 i=0; i<m_children.Size(); ++i)
		{
			m_children[i]->UpdateVisible();
		}

		if(value == false && GRedGui::GetInstance().GetInputManager()->GetMouseFocusControl() == this)
		{
			GRedGui::GetInstance().GetInputManager()->ResetMouseFocusControl();
		}
	}

	void CRedGuiControl::UpdateEnabled()
	{
		m_inheritsEnabled = (m_parent ==  nullptr) || (m_parent->GetEnabled() == true && m_parent->GetInheritedEnabled() == true);
		Bool value = m_enabled && m_inheritsEnabled;

		for(Uint32 i=0; i<m_children.Size(); ++i)
		{
			m_children[i]->UpdateEnabled();
		}
	}

	void CRedGuiControl::ResizeLayerItemView(const Vector2& oldView, const Vector2& newView)
	{
		SetOutOfDate();
	}

	Bool CRedGuiControl::GetVisible() const
	{
		return m_visible;
	}

	Bool CRedGuiControl::GetInheritedVisible() const
	{
		return m_inheritsVisible;
	}

	void CRedGuiControl::SetAnchor(CRedGuiAnchor value)
	{
		m_dock = DOCK_None;
		m_align = IA_None;
		m_anchor = value;

		SetOutOfDate();
	}

	CRedGuiAnchor CRedGuiControl::GetAnchor() const
	{
		return m_anchor;
	}

	void CRedGuiControl::SetDock(CRedGuiDock value)
	{
		m_align = IA_None;
		m_anchor = ANCHOR_None;
		m_dock = value;
		SetOutOfDate();
	}

	CRedGuiDock CRedGuiControl::GetDock() const
	{
		return m_dock;
	}

	Bool CRedGuiControl::SetTheme(const CName& themeName)
	{
		IRedGuiTheme* theme = GRedGui::GetInstance().GetThemeManager()->GetThemeByName( themeName );
		if(theme != nullptr)
		{
			m_theme = theme;
			return true;
		}

		return false;
	}

	IRedGuiTheme* CRedGuiControl::GetTheme() const
	{
		return m_theme;
	}

	CRedGuiControl* CRedGuiControl::GetParent() const
	{
		return m_parent;
	}

	Vector2 CRedGuiControl::GetParentSize() const
	{
		if(m_croppedParent != nullptr)
		{
			CRedGuiControl* parent = static_cast<CRedGuiControl*>(m_croppedParent);
			return parent->GetViewSize();
		}
		if(GetLayer() != nullptr)
		{
			return GetLayer()->GetSize();
		}

		return Vector2(0,0);
	}

	Box2 CRedGuiControl::GetParentPadding() const
	{
		if(m_croppedParent != nullptr)
		{
			CRedGuiControl* parent = static_cast<CRedGuiControl*>(m_croppedParent);
			return parent->GetPadding();
		}

		return Box2::ZERO;
	}

	void CRedGuiControl::SetPadding(const Box2& padding)
	{
		if(m_controlClient != nullptr)
		{
			m_controlClient->SetPadding(padding);
		}
		CRedGuiCroppedRect::SetPadding(padding);

		SetOutOfDate();
	}

	Box2 CRedGuiControl::GetPadding() const
	{
		if(m_controlClient != nullptr)
		{
			return m_controlClient->GetPadding();
		}
		return CRedGuiCroppedRect::GetPadding();
	}

	CRedGuiControl* CRedGuiControl::GetToolTip() const
	{
		return m_toolTip;
	}

	void CRedGuiControl::SetToolTip(CRedGuiControl* tooltip)
	{
		SetNeedToolTip(true);
		if ( m_toolTip != nullptr )
		{
			if ( m_toolTip->GetParent() == this )
			{
				Dispose();
			}
		}
		m_toolTip = tooltip;
	}

	Uint32 CRedGuiControl::GetChildCount()
	{
		return m_children.Size();
	}

	CRedGuiControl* CRedGuiControl::GetChildAt(Uint32 index)
	{
		return m_children[index];
	}

	void CRedGuiControl::SetEnabled(Bool value)
	{
		m_enabled = value;
	}
	
	Bool CRedGuiControl::GetEnabled() const
	{
		return m_enabled;
	}

	const String& CRedGuiControl::GetName() const
	{
		return m_name;
	}

	void CRedGuiControl::SetName(const String& value)
	{
		m_name = value;
	}

	Bool CRedGuiControl::GetInheritedEnabled() const
	{
		return m_inheritsEnabled;
	}

	ERedGuiState CRedGuiControl::GetState() const
	{
		return m_state;
	}

	CRedGuiControl* CRedGuiControl::GetClientControl() const
	{
		return m_controlClient;
	}

	void CRedGuiControl::AddChild(CRedGuiControl* child)
	{
		CRedGuiControl* parent = this;
		if(GetClientControl() != nullptr)
		{
			parent = GetClientControl();
			parent->AddChild(child);
		}
		else
		{
			// remove child from previous parent
			if ( child->GetParent() != nullptr )
			{
				child->GetParent()->RemoveChild( child );
			}
			// update child
			child->m_parent = parent;
			child->m_croppedParent = parent;
			parent->m_children.PushBack(child);
			AddChildItem(child);

			EventSizeChanged.Bind(child, &CRedGuiControl::NotifyEventParentSizeChange);
			child->SetOutOfDate();
			SetOutOfDate();
		}

		RegisterControlInParentWindowContext( child );
	}

	void CRedGuiControl::RemoveChild(CRedGuiControl* child)
	{
		UnregisterControlInParentWindowContext( child );

		CRedGuiControl* parent = this;
		if(GetClientControl() != nullptr)
		{
			parent = GetClientControl();
			parent->RemoveChild(child);
		}
		else
		{
			EventSizeChanged.Unbind(child, &CRedGuiControl::NotifyEventParentSizeChange);

			child->m_parent = nullptr;
			child->m_croppedParent = nullptr;
			parent->m_children.Remove(child);
			RemoveChildItem(child);
			SetOutOfDate();
		}
	}

	void CRedGuiControl::BringToFront( CRedGuiControl* control )
	{
		if( (m_children.Size() < 2) || (m_children.Back() == control))
		{
			return;
		}

		// modal window mode
		if( GRedGui::GetInstance().GetInputManager()->CanBeOnTop( control ) == false )
		{
			return;
		}

		m_children.Remove(control);
		m_children.PushBack(control);
		SetOutOfDate();
	}

	void CRedGuiControl::MoveToTop( Bool recursiveToUp /*= true*/ )
	{
		if(recursiveToUp == true)
		{
			CRedGuiControl* parentControl = this;
			for( CRedGuiControl* control = this; control != nullptr; control = control->GetParent() )
			{
				parentControl = control;
				if( control->IsAWindow() == true )
				{
					break;
				}
			}
			if( parentControl->IsAWindow() == true )
			{
				if( parentControl->GetParent() != nullptr )
				{
					parentControl->GetParent()->BringToFront(parentControl);
				}
			}
		}
	}

	void CRedGuiControl::DrawLayerItem()
	{
		if(GetVisible() == true)
		{
			// inside check out of date condition
			UpdateControl();

			if(GetCroppedParent() != nullptr)
			{
				GetTheme()->SetCroppedParent(GetCroppedParent());
			}

			Draw();

			for(Uint32 i=0;i<m_children.Size(); ++i)
			{
				if( m_children[i]->IsDisposed() == false )
				{
				m_children[i]->DrawLayerItem();
			}
			}

			if(GetCroppedParent() != nullptr)
			{
				GetTheme()->ResetCroppedParent();
			}
		}
	}

	void CRedGuiControl::OnToolTip( Bool visible, const Vector2& position )
	{
		if(m_toolTip != nullptr)
		{
			if(visible == true)
			{
				m_toolTip->SetPosition(position);
				m_toolTip->SetVisible(true);
			}
			else
			{
				m_toolTip->SetVisible(false);
			}
		}
	}

	void CRedGuiControl::UpdateControl()
	{
		if(m_isOutOfDate == true)
		{
			if(GetDock() != DOCK_None)
			{
				UpdateDocking();
			}
			else if(GetAlign() != IA_None)
			{
				UpdateAlign();
			}

			UpdateAbsolutePosition();

			for(Uint32 i=0; i<GetChildCount(); ++i)
			{
				GetChildAt(i)->SetOutOfDate();
			}

			m_isOutOfDate = false;
		}
	}

	void CRedGuiControl::UpdateDocking()
	{
		Box2 padding = GetParentPadding();
		Box2 margin = GetMargin();
		Vector2 parentSize = GetParentSize();

		Box2 newCoord = GetOriginalRect();
		EDock dockValue = m_dock.GetValue();

		switch(dockValue)
		{
		case DOCK_Left:
			newCoord.Min = Vector2(padding.Min.X + margin.Min.X + CheckDockabledSiblings(DOCK_Left), padding.Min.Y + margin.Min.Y + CheckDockabledSiblings(DOCK_Top));
			newCoord.Max = Vector2(GetOriginalRect().Max.X, parentSize.Y - 
				(padding.Min.Y + padding.Max.Y + margin.Min.Y + margin.Max.Y 
				+ CheckDockabledSiblings(DOCK_Bottom) + CheckDockabledSiblings(DOCK_Top) ));
			break;
		case DOCK_Right:
			newCoord.Min = Vector2(parentSize.X - (GetOriginalRect().Max.X + padding.Max.X + margin.Max.X + CheckDockabledSiblings(DOCK_Right)), 
				padding.Min.Y + margin.Min.Y + CheckDockabledSiblings(DOCK_Top));
			newCoord.Max = Vector2(GetOriginalRect().Max.X, parentSize.Y - 
				(padding.Min.Y + padding.Max.Y + margin.Min.Y + margin.Max.Y
				+ CheckDockabledSiblings(DOCK_Bottom) + CheckDockabledSiblings(DOCK_Top) ));
			break;
		case DOCK_Top:
			newCoord.Min = Vector2(padding.Min.X + margin.Min.X + CheckDockabledSiblings(DOCK_Left), padding.Min.Y + margin.Min.Y + CheckDockabledSiblings(DOCK_Top));
			newCoord.Max = Vector2(parentSize.X - (padding.Min.X + padding.Max.X + margin.Min.X + margin.Max.X +
				CheckDockabledSiblings(DOCK_Left) + CheckDockabledSiblings(DOCK_Right) ),
				GetOriginalRect().Max.Y);
			break;
		case DOCK_Bottom:
			newCoord.Min = Vector2(padding.Min.X + margin.Min.X + CheckDockabledSiblings(DOCK_Left), 
				parentSize.Y - (GetOriginalRect().Max.Y + padding.Max.Y + margin.Max.Y + CheckDockabledSiblings(DOCK_Bottom)));
			newCoord.Max = Vector2(parentSize.X - 
				(padding.Min.X + padding.Max.X + margin.Min.X + margin.Max.X +
				CheckDockabledSiblings(DOCK_Left) + CheckDockabledSiblings(DOCK_Right) ), 
				GetOriginalRect().Max.Y);
			break;
		case DOCK_Fill:
			newCoord.Min = Vector2(padding.Min.X + margin.Min.X + CheckDockabledSiblings(DOCK_Left), padding.Min.Y + margin.Min.Y + CheckDockabledSiblings(DOCK_Top));
			newCoord.Max = Vector2(parentSize.X - 
				(padding.Min.X + padding.Max.X + margin.Min.X + margin.Max.X + 
				CheckDockabledSiblings(DOCK_Left) + CheckDockabledSiblings(DOCK_Right) ),
				parentSize.Y - 
				(padding.Min.Y + padding.Max.Y + margin.Min.Y + margin.Max.Y +
				CheckDockabledSiblings(DOCK_Bottom) + CheckDockabledSiblings(DOCK_Top)));
			break;
		}
			
		SetCoord(newCoord);
	}

	Uint32 CRedGuiControl::CheckDockabledSiblings(CRedGuiDock dockCodition)
	{
		CRedGuiControl* parent = GetParent();

		if(parent != nullptr)
		{
			Uint32 siblingsOffset = 0;

			for(Uint32 i=0; i<parent->GetChildCount(); ++i)
			{
				CRedGuiControl* sibling = parent->GetChildAt(i);
				if(sibling == this)
				{
					return siblingsOffset;
				}

				if(sibling->GetDock() == dockCodition && sibling->GetVisible() == true)
				{
					switch(dockCodition.GetValue())
					{
					case DOCK_Left:
						siblingsOffset += (Uint32)(sibling->GetSize().X + sibling->GetMargin().Max.X + sibling->GetMargin().Min.X);
						break;
					case DOCK_Right:
						siblingsOffset += (Uint32)(sibling->GetSize().X + sibling->GetMargin().Min.X + sibling->GetMargin().Max.X);
						break;
					case DOCK_Top:
						siblingsOffset += (Uint32)(sibling->GetSize().Y + sibling->GetMargin().Max.Y + sibling->GetMargin().Min.Y);
						break;
					case DOCK_Bottom:
						siblingsOffset += (Uint32)(sibling->GetSize().Y + sibling->GetMargin().Min.Y + sibling->GetMargin().Max.Y);
						break;
					}
				}
			}
		}
		else
		{
			IRedGuiLayer* layer = GetLayer();
			if(layer != nullptr)
			{
				Uint32 siblingsOffset = 0;

				for(Uint32 i=0; i<layer->GetChildItemCount(); ++i)
				{
					CRedGuiControl* sibling = static_cast<CRedGuiControl*>(layer->GetChildItemAt(i));
					if(sibling == this)
					{
						return siblingsOffset;
					}

					if(sibling->GetDock() == dockCodition && sibling->GetVisible() == true)
					{
						switch(dockCodition.GetValue())
						{
						case DOCK_Left:
							siblingsOffset += (Uint32)(sibling->GetSize().X + sibling->GetMargin().Min.X + sibling->GetMargin().Max.X);
							break;
						case DOCK_Right:
							siblingsOffset += (Uint32)(sibling->GetSize().X + sibling->GetMargin().Min.X + sibling->GetMargin().Max.X);
							break;
						case DOCK_Top:
							siblingsOffset += (Uint32)(sibling->GetSize().Y + sibling->GetMargin().Min.Y + sibling->GetMargin().Max.Y);
							break;
						case DOCK_Bottom:
							siblingsOffset += (Uint32)(sibling->GetSize().Y + sibling->GetMargin().Min.Y + sibling->GetMargin().Max.Y);
							break;
						}
					}
				}
			}
		}

		return 0;
	}

	void CRedGuiControl::UpdateAlign()
	{
		Box2 padding = GetParentPadding();
		Box2 margin = GetMargin();
		Vector2 parentSize = GetParentSize();

		Vector2 newPosition = GetOriginalRect().Min;
		Vector2 actualPositin = GetPosition();
		Vector2 mySize = GetSize();

		// temporary variables
		Float xPos = 0.0f;
		Float yPos = 0.0f;

		switch(m_align)
		{
		case IA_None:
			newPosition = actualPositin;
			break;
		case IA_TopLeft:
			newPosition = Vector2(padding.Min.X + margin.Min.X, padding.Min.Y + margin.Min.Y);
			break;
		case IA_TopCenter:
			xPos = (parentSize.X - (padding.Min.X + padding.Max.X + margin.Min.X + margin.Max.X + mySize.X))/2;
			xPos += padding.Min.X + margin.Min.X;
			newPosition = Vector2(xPos, padding.Min.Y + margin.Min.Y);
			break;
		case IA_TopRight:
			newPosition = Vector2((parentSize.X - (padding.Max.X + margin.Max.X + mySize.X)), padding.Min.Y + margin.Min.Y);
			break;
		case IA_MiddleLeft:
			yPos = (parentSize.Y - (padding.Min.Y + padding.Max.Y + margin.Min.Y + margin.Max.Y + mySize.Y))/2;
			yPos += padding.Min.Y + margin.Min.Y;
			newPosition = Vector2(padding.Min.X + margin.Min.X, yPos);
			break;
		case IA_MiddleCenter:
			xPos = (parentSize.X - (padding.Min.X + padding.Max.X + margin.Min.X + margin.Max.X + mySize.X))/2;
			yPos = (parentSize.Y - (padding.Min.Y + padding.Max.Y + margin.Min.Y + margin.Max.Y + mySize.Y))/2;
			xPos += padding.Min.X + margin.Min.X;
			yPos += padding.Max.Y + margin.Max.Y;
			newPosition = Vector2(xPos, yPos);
			break;
		case IA_MiddleRight:
			yPos = (parentSize.Y - (padding.Min.Y + padding.Max.Y + margin.Min.Y + margin.Max.Y + mySize.Y))/2;
			yPos += padding.Min.Y + margin.Min.Y;
			newPosition = Vector2((parentSize.X - (padding.Max.X + margin.Max.X + mySize.X)), yPos);
			break;
		case IA_BottomLeft:
			newPosition = Vector2(padding.Min.X + margin.Min.X, (parentSize.Y - (padding.Max.Y + margin.Max.Y + mySize.Y)));
			break;
		case IA_BottomCenter:
			xPos = (parentSize.X - (padding.Min.X + padding.Max.X + margin.Min.X + margin.Max.X + mySize.X))/2;
			xPos += padding.Min.X + margin.Min.X;
			newPosition = Vector2(xPos, (parentSize.Y - (padding.Max.Y + margin.Max.Y + mySize.Y)));
			break;
		case IA_BottomRight:
			newPosition = Vector2((parentSize.X - (padding.Max.X + margin.Max.X + mySize.X)), (parentSize.Y - (padding.Max.Y + margin.Max.Y + mySize.Y)));
			break;
		}

		SetPosition(newPosition);
	}

	void CRedGuiControl::SetAlign( EInternalAlign align )
	{
		m_anchor = ANCHOR_None;
		m_dock = DOCK_None;
		m_align = align;
		SetOutOfDate();
	}

	RedGui::EInternalAlign CRedGuiControl::GetAlign() const
	{
		return m_align;
	}

	Bool CRedGuiControl::GetOutOfDate() const
	{
		return m_isOutOfDate;
	}

	void CRedGuiControl::SetOutOfDate()
	{
		m_isOutOfDate = true;
	}

	void CRedGuiControl::NotifyEventParentSizeChange( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& oldSize, const Vector2& newSize )
	{
		RED_UNUSED( eventPackage );

		if(GetAnchor() != ANCHOR_None)
		{
			UpdateAnchor(oldSize, newSize);
			UpdateAbsolutePosition();

			for(Uint32 i=0; i<GetChildCount(); ++i)
			{
				GetChildAt(i)->SetOutOfDate();
				GetChildAt(i)->UpdateControl();
			}

			m_isOutOfDate = false;
		}
	}

	void CRedGuiControl::SetSimpleToolTip( const String& tooltipText )
	{
		// create tooltip
		RedGui::CRedGuiPanel* tooltip = new RedGui::CRedGuiPanel(0, 0, 0, 15);
		tooltip->SetBackgroundColor(Color(75, 75, 75, 255));
		tooltip->AttachToLayer(TXT("Pointers"));
		tooltip->SetTheme( RED_NAME( RedGuiGradientTheme ) );
		tooltip->SetVisible(false);
		tooltip->SetAutoSize(true);

		RedGui::CRedGuiLabel* info = new RedGui::CRedGuiLabel(0, 0, 10, 10);
		info->SetText(tooltipText);
		info->SetMargin(Box2(3, 3, 3, 3));
		info->SetAlign(IA_MiddleCenter);
		(*tooltip).AddChild( info );

		SetToolTip(tooltip);
	}

	void CRedGuiControl::Dispose()
	{
		if( m_isDisposed == false )
		{
			for( Uint32 i=0; i<m_children.Size(); ++i )
			{
				RemoveChild( m_children[i] );
			}

			for( Uint32 i=0; i<m_children.Size(); ++i )
			{
				m_children[i]->Dispose();
			}

			DetachFromLayer();
			if( GetParent() != nullptr )
			{
				GetParent()->RemoveChild( this );
			}
			GRedGui::GetInstance().DisposeControl( this );
			m_isDisposed = true;
		}
	}

	void CRedGuiControl::OnPositionChanged( const Vector2& oldPosition, const Vector2& newPosition )
	{
		/* intentionally empty */
	}

	void CRedGuiControl::OnSizeChanged( const Vector2& oldSize, const Vector2& newSize )
	{
		/* intentionally empty */
	}

	void CRedGuiControl::PropagatePositionChanged( const Vector2& oldPosition, const Vector2& newPosition )
	{
		OnPositionChanged( oldPosition, newPosition );
		EventPositionChanged(static_cast<CRedGuiControl*>(this), oldPosition, newPosition );

	}

	void CRedGuiControl::PropagateSizeChanged( const Vector2& oldSize, const Vector2& newSize )
	{
		OnSizeChanged( oldSize, newSize );
		EventSizeChanged(static_cast<CRedGuiControl*>(this), oldSize, newSize );
	}

	Bool CRedGuiControl::IsAWindow()
	{
		return false;
	}

	void CRedGuiControl::RegisterControlInParentWindowContext( RedGui::CRedGuiControl* control )
	{
		if( control->GetNeedKeyFocus() == false )
		{
			return;
		}

		for( RedGui::CRedGuiControl* ctrl = control->GetParent(); ctrl != nullptr; ctrl = ctrl->GetParent() )
		{
			if( ctrl->GetNeedKeyFocus() == true )
			{
				ctrl->m_keyFocusedControls.PushBackUnique( control );
				::Sort( ctrl->m_keyFocusedControls.Begin(), ctrl->m_keyFocusedControls.End(), ::STabIndexCompareFunc() );
				return;
			}
		}
	}

	void CRedGuiControl::UnregisterControlInParentWindowContext( RedGui::CRedGuiControl* control )
	{
		if( control->GetNeedKeyFocus() == false )
		{
			return;
		}

		for( RedGui::CRedGuiControl* ctrl = control->GetParent(); ctrl != nullptr; ctrl = ctrl->GetParent() )
		{
			if( ctrl->GetNeedKeyFocus() == true )
			{
				ctrl->m_keyFocusedControls.Remove( control );
				::Sort( ctrl->m_keyFocusedControls.Begin(), ctrl->m_keyFocusedControls.End(), ::STabIndexCompareFunc() );
				return;
			}
		}
	}

	void CRedGuiControl::SelectNextControl()
	{
		Int32 previousIndex = m_activeKeyFocusedControl;
		m_focusedControlInScope = nullptr;
		do
		{
			--m_activeKeyFocusedControl;
			if( m_activeKeyFocusedControl < 0 )
			{
				m_activeKeyFocusedControl = (Int32)( m_keyFocusedControls.Size() - 1 );
			}
			if( m_activeKeyFocusedControl < 0 || m_activeKeyFocusedControl >= (Int32)m_keyFocusedControls.Size() )
			{
				return;
			}
			CRedGuiControl* activeControl = m_keyFocusedControls[m_activeKeyFocusedControl];
			if( activeControl->GetEnabled() == true 
				&& activeControl->GetVisible() == true
				&& activeControl->GetInheritedVisible() == true 
				&& activeControl->GetInheritedEnabled() == true )
			{
				m_focusedControlInScope = activeControl;
				m_focusedControlInScope->MoveToTop();
				GRedGui::GetInstance().GetInputManager()->SetKeyFocusControl( m_focusedControlInScope );
				break;
			}
		}while( m_activeKeyFocusedControl != previousIndex );
	}

	void CRedGuiControl::SelectPreviousControl()
	{
		Int32 previousIndex = m_activeKeyFocusedControl;
		m_focusedControlInScope = nullptr;
		do
		{
			++m_activeKeyFocusedControl;
			if( m_activeKeyFocusedControl == (Int32)m_keyFocusedControls.Size() )
			{
				m_activeKeyFocusedControl = 0;
			}
			if( m_activeKeyFocusedControl < 0 || m_activeKeyFocusedControl >= (Int32)m_keyFocusedControls.Size() )
			{
				return;
			}
			CRedGuiControl* activeControl = m_keyFocusedControls[m_activeKeyFocusedControl];
			if( activeControl->GetEnabled() == true 
				&& activeControl->GetVisible() == true
				&& activeControl->GetInheritedVisible() == true 
				&& activeControl->GetInheritedEnabled() == true )
			{
				m_focusedControlInScope = activeControl;
				m_focusedControlInScope->MoveToTop();
				GRedGui::GetInstance().GetInputManager()->SetKeyFocusControl( m_focusedControlInScope );
				break;
			}
		}while( m_activeKeyFocusedControl != previousIndex );
	}

	Bool CRedGuiControl::OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data )
	{
		if( m_focusedControlInScope != nullptr )
		{
			if( m_focusedControlInScope->PropagateInternalInputEvent( event, data ) == true )
			{
				return true;
			}
		}

		if( event == RGIE_Back )
		{
			for( RedGui::CRedGuiControl* ctrl = this; ctrl != nullptr; ctrl = ctrl->GetParent() )
			{
				if( ctrl->GetGamepadLevel() == true )
				{
					m_focusedControlInScope = nullptr;
					m_activeKeyFocusedControl = -1;
					ctrl->SetGamepadLevel( false );
					ctrl->m_focusedControlInScope = nullptr;
					ctrl->m_activeKeyFocusedControl = -1;
					if( ctrl->IsAWindow() == true )
					{
						ctrl->SetVisible( false );
					}
					else
					{
						GRedGui::GetInstance().GetInputManager()->SetKeyFocusControl( ctrl );
					}
					return true;
				}
			}
		}
		else if( event == RGIE_Execute || event == RGIE_Select )
		{
			if( m_focusedControlInScope != nullptr && m_focusedControlInScope->GetKeyFocusedControlCount() != 0 )
			{
				m_focusedControlInScope->SetGamepadLevel( true );
				m_focusedControlInScope->SelectPreviousControl();
				return true;
			}
		}
		
		if( GetGamepadLevel() == true )
		{
			if( event == RGIE_NextControl )
			{
				SelectNextControl();
				return true;

			}
			else if( event == RGIE_PreviousControl )
			{
				SelectPreviousControl();
				return true;
			}
		}

		return false;
	}

	Bool CRedGuiControl::GetGamepadLevel() const
	{
		return m_gamepadLevel;
	}

	void CRedGuiControl::SetGamepadLevel( Bool value )
	{
		m_gamepadLevel = value;
	}

	Uint32 CRedGuiControl::GetKeyFocusedControlCount() const
	{
		return m_keyFocusedControls.Size();
	}

	void CRedGuiControl::UpdateKeyFocusedHierarchy()
	{
		for( CRedGuiControl* ctrl = this; ctrl != nullptr; ctrl = ctrl->GetParent() )
		{
			if( ctrl->GetNeedKeyFocus() == true )
			{
				CRedGuiControl* focusedChild = ctrl;
				for( CRedGuiControl* ctrl2 = ctrl->GetParent(); ctrl2 != nullptr; ctrl2 = ctrl2->GetParent() )
				{
					if( ctrl2->GetNeedKeyFocus() == true )
					{
						ctrl2->m_focusedControlInScope = ctrl;
					}
				}
			}
		}
	}

	void CRedGuiControl::ResetChildrenFocusHierarchy()
	{
		m_activeKeyFocusedControl = (Int32)m_keyFocusedControls.Size();
		SelectPreviousControl();
	}

	Uint32 CRedGuiControl::GetTabIndex() const
	{
		return m_tabIndex;
	}

	void CRedGuiControl::SetTabIndex( Uint32 value )
	{
		m_tabIndex = value;
	}

	Bool CRedGuiControl::IsDisposed() const
	{
		return m_isDisposed;
	}

	void CRedGuiControl::NotifyPendingDestruction()
	{
		OnPendingDestruction();
	}

	void CRedGuiControl::OnPendingDestruction()
	{}

}	// namespace RedGui

#endif	// NO_RED_GUI
