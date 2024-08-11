/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_LOG

#define SCENE_LOG( format, ... )	RED_LOG( Scene, format, ## __VA_ARGS__ )
#define SCENE_WARN( format, ... )	RED_LOG( Scene, format, ## __VA_ARGS__ )
#define SCENE_ERROR( format, ... )	RED_LOG( Scene, format, ## __VA_ARGS__ )

#else

#define SCENE_LOG( format, ... )	
#define SCENE_WARN( format, ... )	
#define SCENE_ERROR( format, ... )	

#endif