/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SCRIPT_STUDIO_TOKEN_H__
#define __SCRIPT_STUDIO_TOKEN_H__

#include "context.h"

/// Token of code
struct SSToken
{
	wstring			m_text;			//!< Token text
	int				m_token;		//!< Token ID
	int				m_level;		//!< Code level ( bracket level )
	SSFileContext	m_context;		//!< Token context

	SSToken();
};

#endif // __SCRIPT_STUDIO_TOKEN_H__
