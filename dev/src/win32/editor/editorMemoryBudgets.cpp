#include "build.h"
#include "editorMemoryBudgets.h"
#include "../../common/engine/memoryBudgetStorage.h"

enum BudgetPlatform {
	Platform_PC,
	Platform_XB1,
	Platform_PS4,
};

#define ADD_TO_ALL_PLATFORMS( groupInstance )	\
	SMemoryBudgets::GetInstance().AddGroup( Platform_PC, groupInstance );	\
	SMemoryBudgets::GetInstance().AddGroup( Platform_XB1, groupInstance );	\
	SMemoryBudgets::GetInstance().AddGroup( Platform_PS4, groupInstance );

void InitialiseMemoryBudgets()
{
	SMemoryBudgets::GetInstance().AddPlatform( Platform_PC, TXT( "PC" ) );
	SMemoryBudgets::GetInstance().AddPlatform( Platform_XB1, TXT( "XBone" ) );
	SMemoryBudgets::GetInstance().AddPlatform( Platform_PS4, TXT( "PS4" ) );

	// ctremblay usesless now.
}