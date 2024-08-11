/**
 * Copyright © 2007 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#include "showFlags.h"

class CRenderFrame;

/// Editor fragments filter for world nodes
class CWorldEditorFragmentsFilter
{
protected:

#ifndef NO_EDITOR_FRAGMENTS
	THashMap< MemUint, THandle< CNode > >	m_categories[ SHOW_MAX_INDEX ];		//!< List of registered categories
#endif	

public:
	CWorldEditorFragmentsFilter();
	~CWorldEditorFragmentsFilter();

	//! Register node in show flag category
	void RegisterEditorFragment( CNode* node, EShowFlags showFlag );

	//! Unregister node from show flag category
	void UnregisterEditorFragment( CNode* node, EShowFlags showFlag );

	//! Unregister all nodes from show flag category
	void UnregisterAllEditorFragmentsOfCategory( EShowFlags showFlag );

	//! Generate editor fragments from registered categories
	void GenerateEditorFragments( CRenderFrame* frame );
};
