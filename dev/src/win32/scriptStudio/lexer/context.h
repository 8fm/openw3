/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SCRIPT_STUDIO_CONTEXT_H__
#define __SCRIPT_STUDIO_CONTEXT_H__

struct SSFileContext
{
	wstring		m_file;
	int			m_line;

	SSFileContext();
	SSFileContext( const wstring& file, int line );
};

#endif // __SCRIPT_STUDIO_CONTEXT_H__
