/*
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

// Below is a list of all resources needed by engine and editor.
// Please don't add any paths in the code, this header being the only exception.

#define DEFAULT_MATERIAL				TXT("engine\\materials\\gridmat.w2mg")
#define DEFAULT_FONT					TXT("engine\\fonts\\parachute.w2fnt")
#define DEFAULT_BEHAVIOR_ENTITY			TXT("characters\\models\\geralt\\geralt.w2ent")
#define DEFAULT_ENTITY_NAME				TXT("characters\\models\\geralt\\geralt.w2ent")
#define DEFAULT_ANIMSET_NAME			TXT("engine\\characters\\witcher\\animation\\exploration_animset.w2anims")

#define VOICE_TAGS_TABLE				STORY_SCENE_VOICE_TAGS_TABLE
#define VOICE_TAG_ALIASES_TABLE			TXT("gameplay\\globals\\scenes\\scene_aliases.csv")

#define EMOTION_STATES_TABLE			TXT("gameplay\\globals\\scene_emotion_states.csv")
#define ACTION_CATEGORY_TABLE			ACTION_POINT_CATEGORY_TABLE

#define STICKER_TEMPLATE_PATH			TXT("engine\\templates\\editor\\sticker.w2ent")
#define ENTITY_WAYPOINT					TXT("engine\\templates\\editor\\waypoint.w2ent")
#define ENTITY_PARTICLES				TXT("engine\\templates\\editor\\particles.w2ent")

#define MESH_BOX						TXT("engine\\meshes\\editor\\box.w2mesh")
#define MESH_CYLINDER					TXT("engine\\meshes\\editor\\cylinder.w2mesh")
#define MESH_SPHERE						TXT("engine\\meshes\\editor\\sphere.w2mesh")
#define MESH_PLANE						TXT("engine\\meshes\\editor\\plane.w2mesh")

#define DIFFUSEMAP_MATERIAL				TXT("engine\\materials\\diffusemap.w2mg")
#define DIFFUSECUBEMAP_MATERIAL			TXT("engine\\materials\\diffusecubemap.w2mg")
#define NORMALMAP_MATERIAL				TXT("engine\\materials\\normalmap.w2mg")
#define VOLUME_MATERIAL_PATH			TXT( "engine\\materials\\defaults\\volume.w2mg" )

#define BRUSH_PAINT_32F					TXT("engine\\textures\\brushes\\brush_32f.xbm")

#define EDITOR_TEMPLATES_CSV			TXT("engine\\templates\\editor\\editor_templates.csv")
#define EDITOR_QUEST_SHORTCUTS_CSV		TXT("engine\\shortcuts\\questeditor.csv")
#define EDITOR_SCENE_SHORTCUTS_CSV		TXT("engine\\shortcuts\\sceneeditor.csv")
#define DIALOG_EDITOR_LIGHT_PRESETS_CSV	TXT("engine\\dialogeditor\\lightpresets.csv")
#define SCENE_SECTION_TAGS_CSV			TXT("gameplay\\globals\\scenes\\scene_section_tags.csv")
#define SOUND_EVENTS_LIST				TXT("gameplay\\globals\\sounds\\events.csv")
#define STICKERS_CSV					TXT("gameplay\\globals\\stickers.csv")
#define BEHAVIOR_SNAPSHOT_CSV			TXT("gameplay\\globals\\behavior_snapshots.csv")
#define PLAYGO_CHUNKS_CSV				TXT("engine\\misc\\playgo_chunks.csv")
#define TEXT_LANGUAGES_CSV				TXT("engine\\misc\\text_languages.csv")
#define SPEECH_LANGUAGES_CSV			TXT("engine\\misc\\speech_languages.csv")

#define THUMBNAIL_GEN_PLANE				TXT( "engine\\thumbnails\\thumbnail_plane.w2mesh" )
#define THUMBNAIL_GEN_GRASS				TXT( "engine\\thumbnails\\thumbnail_grass.srt" )


#define PROXYMESH_DEFAULT_MATERIAL	TXT( "engine\\materials\\graphs\\pbr_simple.w2mg" )

