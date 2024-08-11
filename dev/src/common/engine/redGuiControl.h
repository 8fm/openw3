/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiTypes.h"
#include "redGuiControlInput.h"
#include "redGuiUserData.h"
#include "redGuiCroppedRect.h"
#include "redGuiLayerItem.h"
#include "redGuiControlSkin.h"
#include "redGuiAnchor.h"
#include "redGuiDock.h"
#include "redGuiTheme.h"
#include "redGuiThemeManager.h"
#include "redGuiEmbeddedResources.h"

namespace RedGui
{
	class CRedGuiControl 
		: private Red::System::NonCopyable
		, public CRedGuiControlInput
		, public CRedGuiUserData
		, public CRedGuiCroppedRect
		, public CRedGuiLayerItem
		, public CRedGuiControlSkin
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiControl(Uint32 left, Uint32 top, Uint32 width, Uint32 height);
		virtual	~CRedGuiControl	();

		// Events
		Event2_PackageBool				EventVisibleChanged;
		Event3_PackageVector2Vector2	EventPositionChanged;
		Event3_PackageVector2Vector2	EventSizeChanged;

		// Set control position (position of left top corner)
		virtual void SetPosition(const Vector2& position);
		virtual void SetPosition(Int32 left, Int32 top);

		// Set control size
		virtual void SetSize(const Vector2& size);
		virtual void SetSize(Int32 width, Int32 height);

		// Set control position and size
		virtual void SetCoord(const Box2& coord);
		virtual void SetCoord(Int32 left, Int32 top, Int32 width, Int32 height);

		// Get name
		const String& GetName() const;
		// Set name
		void SetName(const String& value);

		// hide or show control
		virtual void SetVisible(Bool value);
		// return true if visible
		Bool GetVisible() const;
		// return control's visibility based on it's and parent visibility
		Bool GetInheritedVisible() const;

		// set align
		void SetAnchor(CRedGuiAnchor value);
		// get align
		CRedGuiAnchor GetAnchor() const;

		// set dock
		void SetDock(CRedGuiDock value);
		// get dock
		CRedGuiDock GetDock() const;

		// set align
		void SetAlign(EInternalAlign align);
		// get align
		EInternalAlign GetAlign() const;

		// set theme
		Bool SetTheme(const CName& themeName);
		// get theme
		IRedGuiTheme* GetTheme() const;

		// Get parent
		CRedGuiControl* GetParent() const;
		// Get parent size
		Vector2 GetParentSize() const;
		// Get parent padding
		Box2 GetParentPadding() const;

		// Get out of date
		Bool GetOutOfDate() const;
		// Set out of date
		virtual void SetOutOfDate();

		// Get padding
		Box2 GetPadding() const;
		// Set padding
		void SetPadding(const Box2& padding);

		// Get tool tip
		CRedGuiControl* GetToolTip() const;
		// Set tool tip
		virtual void SetToolTip(CRedGuiControl* tooltip);
		// Set simple tooltip - only one row of text
		void SetSimpleToolTip(const String& tooltipText);

		// get child count
		Uint32 GetChildCount();
		// get child by index
		CRedGuiControl* GetChildAt(Uint32 index);

		// set enable or disable control
		void SetEnabled(Bool value);
		// get control enabled
		Bool GetEnabled() const;
		// If control enabled and all it's parents is enabled
		Bool GetInheritedEnabled() const;

		// Get control state
		virtual ERedGuiState GetState() const;
		// Set control state
		void SetState(ERedGuiState state);

		// Get client area control
		CRedGuiControl* GetClientControl() const;
		// Set client area control
		void SetControlClient(CRedGuiControl* control);

		// Add new control as child to this control
		virtual void AddChild(CRedGuiControl* child);
		virtual void RemoveChild(CRedGuiControl* child);

		// Draw control
		void DrawLayerItem();

		// Report this control to gui manager as control to removed
		void Dispose();
		Bool IsDisposed() const;

		//
		void ForcePick(CRedGuiControl* control);

		// checks whether the control is under the position 
		Bool CheckPoint( const Vector2& position ) const;

		// set position and size in relation to parent
		virtual void UpdateControl();

		// calculate absolute position (absolute parent position + control position)
		void UpdateAbsolutePosition();

		// FIX ME!!! - this function should be in protected section
		void NotifyEventParentSizeChange( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& oldSize, const Vector2& newSize);

		// move control to top in the children hierarchy
		void BringToFront( CRedGuiControl* control );
		void MoveToTop( Bool recursiveToUp = true );

		// HACK
		virtual Bool IsAWindow();

		Bool GetGamepadLevel() const;
		void SetGamepadLevel( Bool value );

		Uint32 GetTabIndex() const;
		void SetTabIndex( Uint32 value );

		void UpdateKeyFocusedHierarchy();

		void NotifyPendingDestruction();

	protected:
		virtual void OnPositionChanged( const Vector2& oldPosition, const Vector2& newPosition );
		virtual void OnSizeChanged( const Vector2& oldSize, const Vector2& newSize );
		
		virtual void OnPendingDestruction(); // Override this function if you need to manipulate your children before being destroyed.

		void PropagatePositionChanged( const Vector2& oldPosition, const Vector2& newPosition );
		void PropagateSizeChanged( const Vector2& oldSize, const Vector2& newSize );

		//
		void UpdateVisible();
		void UpdateEnabled();

		//
		void UpdateDocking();
		void UpdateAnchor(const Vector2& oldSize, const Vector2& newSize);
		void UpdateAlign();

		//
		virtual IRedGuiLayerItem* GetLayerItemByPoint( const Vector2& position ) const;

		//
		virtual void ResizeLayerItemView(const Vector2& oldView, const Vector2& newView);

		//
		virtual void OnToolTip(Bool visible, const Vector2& position);
		
		//
		Uint32 CheckDockabledSiblings(CRedGuiDock dockCodition);

		void RegisterControlInParentWindowContext( RedGui::CRedGuiControl* control );
		void UnregisterControlInParentWindowContext( RedGui::CRedGuiControl* control );

		Bool OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data );
		void SelectNextControl();
		void SelectPreviousControl();
		void ResetChildrenFocusHierarchy();

		Uint32 GetKeyFocusedControlCount() const;

	private:
		CRedGuiControl* m_controlClient;	//!< active area for this control
		ArrayControlPtr m_children;			//!< container with all children controls
		CRedGuiControl* m_toolTip;			//!< tool tip control with additional info
		CRedGuiControl* m_parent;			//!< parent for this control
		CRedGuiAnchor m_anchor;				//!< type of anchoring control to parent
		EInternalAlign m_align;				//!< type of alignment control to parent
		IRedGuiTheme* m_theme;				//!< drawing theme for control
		ERedGuiState m_state;				//!< actually control state
		CRedGuiDock m_dock;					//!< type of docking control to parent
		String m_name;						//!< control name
		Bool m_isOutOfDate;					//!< defines the control should be update
		Bool m_enabled;						//!< defines the control can get input events
		Bool m_inheritsEnabled;				//!< control enabled related with parent enabled
		Bool m_inheritsVisible;				//!< control visible related with parent visible
		Bool m_visible;						//!< defines the controls can be draw
		Bool m_isDisposed;					//!< 

		TDynArray< CRedGuiControl*, MC_RedGuiControls, MemoryPool_RedGui >	m_keyFocusedControls;
		Int32																m_activeKeyFocusedControl;
		CRedGuiControl*														m_focusedControlInScope;
		Bool																m_gamepadLevel;
		Uint32																m_tabIndex;
	};

} // namespace RedGui

#endif	// NO_RED_GUI
