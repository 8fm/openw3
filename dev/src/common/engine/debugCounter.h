/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_DEBUG_PAGES

/// Debug counter
class IDebugCounter
{
protected:
	String		m_name;			//!< Name
	Float		m_max;			//!< Maximum value
	Float		m_current;		//!< Current value

public:
	//! Get name
	RED_INLINE const String& GetName() const { return m_name; }

	//! Get maximum value
	RED_INLINE Float GetMax() const { return m_max; }

	//! Get current value
	RED_INLINE Float GetCurrent() const { return m_current; }

public:
	IDebugCounter( const String& name, Float max );
	virtual ~IDebugCounter();

	//! Get the color for drawing the bar
	virtual Color GetColor() const;

	// Get the text to display
	virtual String GetText() const;
};

#endif