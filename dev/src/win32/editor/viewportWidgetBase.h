/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "undoManager.h"

class CViewportWidgetManager;

/// Base viewport widget class
class CViewportWidgetBase
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Editor );
	friend class CViewportWidgetManager;

private:
	CViewportWidgetManager*		m_manager;		//!< Widget manager
	String						m_groupName;	//!< Group name
	Bool						m_isEnabled;	//!< Widget is enabled
	Bool						m_duplicateOnStart;

public:	
	CViewportWidgetBase( const String& groupName, Bool enableDuplicateOnStart );

	virtual ~CViewportWidgetBase();

	// Get widget manager
	RED_INLINE CViewportWidgetManager* GetManager() const { return m_manager; }

	// Get group name
	RED_INLINE const String& GetGroupName() const { return m_groupName; }

	// Is widget active
	RED_INLINE Bool IsEnabled() const { return m_isEnabled; }

public:
	// Check collision with widget
	virtual Bool CheckCollision( const CRenderFrameInfo &frameInfo, const wxPoint &mousePos )=0;

	// Generate rendering fragments
	virtual Bool GenerateFragments( CRenderFrame *frame, Bool isActive )=0;

	// Activate action
	virtual Bool Activate();

	// Deactivate
	virtual void Deactivate();

	// Set mouse cursor
	virtual Bool SetCursor()=0;

	// Input from viewport, return true if you handle the message
	virtual Bool OnViewportMouseMove( const CMousePacket& packet )=0;

public:
	//! Enable/Disable widget
	void EnableWidget( Bool state );

public:
	//! Calculate movement factor for given mouse movement, camera, and move axis
	static Float CalcMovement( const CRenderFrameInfo &frameInfo, const Vector &axis, Int32 dx, Int32 dy );

	// Get line rendering matrix
	virtual Matrix CalcLocalToWorld() const;

	// Get local axis
	virtual Vector CalcLocalAxis( const Vector& axis ) const;

protected:
	void SendPropertyEvent( const CName &propertyName, const CName& eventName );
};

