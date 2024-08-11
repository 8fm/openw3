/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

enum EInputKey : Int32;
enum EInputAction : Int32;

class CRenderFrame;
class IDebugPage;

#ifndef NO_DEBUG_PAGES

/// Item of check box tree
class IDebugCheckBox
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Debug );
public:
	/// Render options
	struct RenderOptions
	{
		IDebugCheckBox*		m_selected;		//!< Selected item
		Uint32 m_maxX;						//!< Fit in this size X
		Uint32 m_maxY;						//!< Fit in this size Y
		Uint32 m_startLine;				//!< Start drawing from this element number

		RenderOptions()
			: m_selected( NULL )
		{};
	};

	static const Uint32 LINE_HEIGHT;

protected:
	String							m_name;			//!< Name to display
	IDebugCheckBox*					m_parent;		//!< Parent item
	TDynArray< IDebugCheckBox* >	m_children;		//!< Child items
	Bool							m_canExpand;	//!< Can we expand this item
	Bool							m_canCheck;		//!< Can we check this item
	Bool							m_isExpanded;	//!< Is this item expanded

public:
	//! Get name
	RED_INLINE const String& GetName() const { return m_name; }

	//! Get parent item
	RED_INLINE IDebugCheckBox* GetParent() const { return m_parent; }

	//! Get children
	RED_INLINE const TDynArray< IDebugCheckBox* >& GetChildren() const { return m_children; }

	//! Does this item have any children
	RED_INLINE Bool HasChildren() const { return !m_children.Empty(); }

	//! Is the check box
	RED_INLINE Bool IsExpanded() const { return m_isExpanded; }

	//! Can we expand this node
	RED_INLINE Bool CanExpand() const { return m_canExpand; }

public:
	IDebugCheckBox( IDebugCheckBox* parent, const String& name, Bool canExpand, Bool canCheck );
	virtual ~IDebugCheckBox();

	//! Is this item checked ?
	virtual Bool IsChecked() const { return false; }

	//! Toggle state of this item, only for check items
	virtual void OnToggle() {};

	//! Render
	virtual void OnRender( CRenderFrame* frame, Uint32 x, Uint32 &y, Uint32 &counter, const RenderOptions& options );

	//! Render comment
	virtual void OnRenderComment( CRenderFrame* frame, Uint32 x, Uint32 y, const Color& color, const RenderOptions& options );

	//! Handle input
	virtual Bool OnInput( enum EInputKey key, EInputAction action, Float data );

	//! General notify
	virtual Bool OnNotify( const Uint32 notificationCode, void* ptr );

	//! Tick the crap
	virtual void OnTick( Float timeDelta );

	//! Focus was lost
	virtual void OnLostFocus() {};

	//! Focus was gained
	virtual void OnGetFocus() {};

	//! Remove all children
	virtual void Clear();

	//! Calculate drawing color
	virtual void CalculateColor( Color& color, Bool isSelected );

public:
	//! Find child node by name
	IDebugCheckBox* FindChildByName( const String& name );

public:
	// Get num expanded following children and their children
	Uint32 GetNumLinear();

	// Get linear index of item
	void GetLinearIndex( IDebugCheckBox*, Uint32 &index );

public:
	//! Expand/Collapse this item
	virtual void Expand( Bool isExpanded );

	//! Linearize tree into list
	void Linearize( TDynArray< IDebugCheckBox* >& items );
};

/// Debug options tree
class CDebugOptionsTree
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Debug );
public:
	struct KeyRepeaterInfo
	{
		// 4 pressed keys
		Bool m_keyIsPressed[4];
		Float m_Timer[4];
		Float m_startTime[4];
		Bool m_isKeyRepeated[4];
	};

public:
	TDynArray< IDebugCheckBox* >		m_roots;
	IDebugCheckBox*						m_active;
	Uint32								m_width;
	Uint32								m_height;
	Uint32								m_x;
	Uint32								m_y;
	Uint32								m_startLine;
	KeyRepeaterInfo						m_keyRepeaterInfo;
	IDebugPage*							m_parentPage;
	Bool								m_drawBackground;

public:
	CDebugOptionsTree( Uint32 x, Uint32 y, Uint32 width, Uint32 height, IDebugPage* debugPage = NULL, Bool drawBackground = true );
	virtual ~CDebugOptionsTree();

	//! Clear items
	void Clear();

	//! Add root
	void AddRoot( IDebugCheckBox* root );

	//! Select item
	void SelectItem( IDebugCheckBox* item );

	//! Find root by it's name
	IDebugCheckBox* FindRootByName( const String& name );

	Uint32 GetNumLinear();

	// Get linear index of item
	void GetLinearIndex( IDebugCheckBox*, Uint32 &index );

public:
	//! Render the tree
	virtual void OnRender( CRenderFrame* frame );

	//! Process input
	virtual Bool OnInput( enum EInputKey key, enum EInputAction action, Float data );

	//! General update
	virtual void OnTick( Float timeDelta );

	//! Send notification code
	virtual Bool OnNotify( const Uint32 notificationCode, void* ptr );
};

#endif