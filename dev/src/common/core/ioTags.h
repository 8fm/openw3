/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#ifndef _IO_TAGS_H
#define _IO_TAGS_H

/// Tags for IO operations
/// NOTE: tags do not directly map to priorities
/// Instead they are resolved using priority matrix + some heuristics that tries to figure out what is the best thing to load

enum EIOTag
{
	// Generic tag - roughly matches normal priority, use for generic stuff, duh
	eIOTag_Generic=0,

	// Bundle preloading
	eIOTag_BundlePreload,

	// Resource loading tag
	eIOTag_ResourceLow,
	eIOTag_ResourceNormal,
	eIOTag_ResourceImmediate,

	// Asset loading tags
	eIOTag_TexturesLow,
	eIOTag_TexturesNormal,
	eIOTag_TexturesImmediate, eIOTag_EnvProbes = eIOTag_TexturesImmediate,

	eIOTag_MeshesLow,
	eIOTag_MeshesNormal,
	eIOTag_MeshesImmediate,

	eIOTag_AnimationsLow,
	eIOTag_AnimationsNormal,
	eIOTag_AnimationsImmediate,

	eIOTag_SoundNormal,
	eIOTag_SoundImmediate,

	eIOTag_CollisionNormal,
	eIOTag_CollisionImmediate,

	eIOTag_TerrainHeight,
	eIOTag_TerrainControl,
	eIOTag_TerrainColor,

	eIOTag_UmbraBuffer,
	eIOTag_StreamingData,

	eIOTag_MAX,
};

/// Context for IO operations
/// NOTE: do not switch this lighttly :)

enum EIOContext
{
	// initial state - booting of the engine
	eIOContext_Boot,

	// loading the world
	eIOContext_WorldLoading,

	// world streaming (gameplay)
	eIOContext_WorldStreaming,

	// specialized state for scene/cutscene loading
	eIOContext_SceneLoading,

	eIOContext_MAX,
};

namespace Helper
{
	extern const AnsiChar* GetIOTagName( const EIOTag tag );
	extern const AnsiChar* GetIOContextName( const EIOContext context );
}

#endif
