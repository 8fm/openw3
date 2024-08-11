#ifndef _H_EDITOR_NAMES_REGISTRY
#define _H_EDITOR_NAMES_REGISTRY

#if !defined( REGISTER_NAME )
#define REGISTER_NAME( name_ ) RED_DECLARE_NAME( name_ )
#define REGISTER_NAMED_NAME( varname_, string_ ) RED_DECLARE_NAMED_NAME( varname_, string_ )
#define REGISTER_NOT_REGISTERED
#endif

REGISTER_NAME( EditorPropertyPostChange )
REGISTER_NAME( EditorPropertyPostTransaction )
REGISTER_NAME( EditorPropertyChanging )
REGISTER_NAME( EditorPropertyPreChange )
REGISTER_NAME( EditorPreUndoStep )
REGISTER_NAME( EditorPostUndoStep )
REGISTER_NAME( EditorSpawnTreeResourceModified )
REGISTER_NAME( SceneExplorerSelectionChanged )

REGISTER_NAME( FlyingSpawn1 )
REGISTER_NAME( FlyingSpawn2 )
REGISTER_NAME( FlyingSpawn3 )
REGISTER_NAME( FlyingSpawn4 )
REGISTER_NAME( FlyingSpawn5 )
REGISTER_NAME( FlyingSpawn6 )
REGISTER_NAME( FlyingSpawn7 )
REGISTER_NAME( FlyingSpawn8 )
REGISTER_NAME( FlyingSpawn9 )

REGISTER_NAME( DialogEditorDestroyed )

#if defined( REGISTER_NOT_REGISTERED )
#undef REGISTER_NAME
#undef REGISTER_NAMED_NAME
#undef REGISTER_NOT_REGISTERED
#endif

#endif // _H_EDITOR_NAMES_REGISTRY