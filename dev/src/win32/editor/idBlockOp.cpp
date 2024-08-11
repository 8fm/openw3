#include "build.h"
#include "interactiveDialogEditor.h"
#include "idBlockOp.h"

void SIDBlockEditorOp::CopyOptions( SIDOptionStub* dst, const SIDOptionStub* src )
{
	for ( Uint32 i = 0; i < CHOICE_Max; ++i )
	{
		dst[ i ] = src[ i ];
	}
}
