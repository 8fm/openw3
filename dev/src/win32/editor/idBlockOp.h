/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../games/r6/idGraphBlockChoice.h"

// TODO: refactor
// This is temporary solution. Should use different classes instead of one struct for everything.
// Then it could be used >>potentially<< as undo/redo steps?

enum EIDBlockEditorOP
{
	BLOCK_DoNothing,
	BLOCK_Update,
	BLOCK_AddText,
	BLOCK_Delete,
	BLOCK_AddChoice,
};

enum EIDBlockEditorUpdateFlags
{
	UPDATE_Nothing		= 0,
	UPDATE_Name			= FLAG( 0 ),
	UPDATE_Comment		= FLAG( 1 ),
	UPDATE_Lines		= FLAG( 2 ),
	UPDATE_Options		= FLAG( 3 ),
};

struct SIDBlockEditorOp
{
	EIDBlockEditorOP				m_op;
	Uint32							m_flags;	
	
	// optional data fileds ( filled or not - depending on op and flags )
	THandle< CIDGraphBlock >		m_block;
	CName							m_name;
	String							m_comment;
	TDynArray< SIDLineStub >		m_stubs;
	SIDOptionStub					m_options[ CHOICE_Max ];

	static void	CopyOptions( SIDOptionStub* dst, const SIDOptionStub* src );
};
