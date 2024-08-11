
#pragma once

#include "../core/profiler.h"

#define DEF_BEH_PROFILER_LEVEL_1

#if defined DEF_BEH_PROFILER_LEVEL_0
#define BEH_PROFILER_LEVEL_1( name )
#define BEH_PROFILER_LEVEL_2( name )
#define BEH_PROFILER_LEVEL_3( name )
#elif defined DEF_BEH_PROFILER_LEVEL_1
#define BEH_PROFILER_LEVEL_1( name ) PC_SCOPE( name );
#define BEH_PROFILER_LEVEL_2( name )
#define BEH_PROFILER_LEVEL_3( name )
#elif defined DEF_BEH_PROFILER_LEVEL_2
#define BEH_PROFILER_LEVEL_1( name ) PC_SCOPE( name );
#define BEH_PROFILER_LEVEL_2( name ) PC_SCOPE( name );
#define BEH_PROFILER_LEVEL_3( name )
#elif defined DEF_BEH_PROFILER_LEVEL_3
#define BEH_PROFILER_LEVEL_1( name ) PC_SCOPE( name );
#define BEH_PROFILER_LEVEL_2( name ) PC_SCOPE( name );
#define BEH_PROFILER_LEVEL_3( name ) PC_SCOPE( name );
#else
#define BEH_PROFILER_LEVEL_1( name )
#define BEH_PROFILER_LEVEL_2( name )
#define BEH_PROFILER_LEVEL_3( name )
#endif

#define BEH_NODE_UPDATE( name ) BEH_PROFILER_LEVEL_2( BehNodeUpdate_##name )
#define BEH_NODE_SAMPLE( name ) BEH_PROFILER_LEVEL_2( BehNodeSample_##name )
