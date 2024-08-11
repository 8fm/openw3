/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderCommandInterface.h"
#include "renderer.h"
#include "renderThread.h"
#include "renderCommandBuffer.h"

static Uint8 GTheFakeMemForRenderCommands[ 2048 ];

IRenderCommand::IRenderCommand()
{
}

IRenderCommand::~IRenderCommand()
{
}

void IRenderCommand::Commit()
{	
	// Cleanup the guard command (JTS)
	if ( GRender )
	{
		if ( (void*)this != GTheFakeMemForRenderCommands )
		{
			GRender->GetRenderThread()->GetCommandBuffer().Commit( this );
		}
	}
}

IRenderCommand* IRenderCommand::Alloc( Uint32 size )
{
	// Failsafe
	if ( !GRender || ! GRender->GetRenderThread() )
	{
		RED_FATAL_ASSERT( size < sizeof(GTheFakeMemForRenderCommands), "Size of the command is to big for the fake buffer" );
		Red::System::MemoryZero( GTheFakeMemForRenderCommands, sizeof(GTheFakeMemForRenderCommands) );

		// ugly hacks
		return (IRenderCommand*) GTheFakeMemForRenderCommands;
	}

	// Allocate memory for command
	void *mem = GRender->GetRenderThread()->GetCommandBuffer().Alloc( size );

	// Bind commit address
	return ( IRenderCommand* ) mem;
}
