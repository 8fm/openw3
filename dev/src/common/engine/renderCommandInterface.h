/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

//#define RED_RENDER_COMMAND_DEBUG_NAMES

/// Render command
class IRenderCommand
{
public:
	IRenderCommand();
	virtual ~IRenderCommand();

public:
	//! Commit this command to the render thread
	virtual void Commit();

public:
#ifdef RED_RENDER_COMMAND_DEBUG_NAMES
	//! Describe command
	virtual const Char* Describe() const=0;
#endif

	//! Execute command, called from render thread
	virtual void Execute()=0;

public:
	//! Allocate command
	static IRenderCommand* Alloc( Uint32 size );
};
