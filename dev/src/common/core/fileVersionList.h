/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/****************************/
/* Version list				*/
/****************************/

// TODO: get rid of this define once it is figured out how to shorten the compression time for BC6 and BC7
// #define USE_NEW_COMPRESSION

// Removed bone indices (texture skinning doesn't need)
#define VER_RESAVE_OCT_2011			120

// 
#define VER_APPROXIMATED_CURVES		121

// CNames saved as hash
#define VER_HASH_CNAMES				122

// 64 bit compression for dialog animations
#define VER_COMPR_ROT_TYPES			123

// CNames saved as hash
#define VER_16BIT_OBJECT_FLAGS		124

// FX track groups 'enabled' parameter reset
#define VER_EFFECT_TRACK_GROUP_ENABLED	125

// Entity template streaming
#define VER_STREAMED_ENTITY_TEMPLATE 127

// Added internal variables to behavior graph
#define VER_BEHAVIOR_GRAPH_INTERNAL_VARIABLES_ADDED 128

// RGBA textures without Red/Blue channels swapped
#define VER_NON_RB_SWAPPED_TEXTURES 129

// New shape data for area component
#define VER_NEW_AREA_COMPONENT_SHAPE_DATA 130

// Per-component instance data for templated entities (to save area shapes even with templated entities)
#define VER_ADDITIONAL_COMPONENT_DATA 131

// Changed SIDLine to SIDBaseLine, SIDTextLine and SIDConnectorLine
#define VER_IDLINES_REFACTOR 132

// Mesh bones changed to separate arrays for names, matrices, epsilons.
#define VER_REARRANGE_MESH_BONE_DATA_LAYOUT 133

// Refactored mesh LOD distances to support Apex cloth LODs. Per-platform LOD settings for meshes removed, likely won't have time to set them up.
#define VER_REMOVE_SEPARATE_MESH_LOD_DISTANCES 134

// Compatibility mode for compiled material parameter list
#define VER_RECOMPILE_MATERIAL_PARAMETERS 135

// Improved bitfield serialization + hack for restoring invalid encounters
#define VER_IMPROVED_BITFIELD_SERIALIZATION 136

// Added general extra data members to the vertex, replacing wind data that was hacked
#define VER_ADDITIONAL_VERTEX_DATA 137

// Various little cleanups in the serialization code
#define VER_SERIALIZATION_DATA_CLEANUP 138

// Various little cleanups in the serialization code
#define VER_CLASS_PROPERTIES_DATA_CLEANUP 139

// Hashmap reimplemented to be more optimal (both performance and memory wise)
#define VER_NEW_HASHMAP 140

// TMap typename removed (and in most cases remapped to THashMap)
#define VER_REMOVED_TMAP 141

// Texture cooking reimplemented, cooked data buffers changed.
#define VER_TEXTURE_COOKING 142

// Umbra switched to using hashmaps
#define VER_UMBRA_SWITCHED_TO_HASHMAPS 143

// Removed saving of the reference to CPersistentEntity (they shall now only be saved via EntityHandle)
#define VER_REMOVED_PERSISTENT_ENTITY_REF_SAVING 144

// Behavior variables' names are CNames, not Strings
#define VER_BEHAVIOR_VARS_BY_NAME 145

// Store hashmap of crc of hlsl in material graph
#define VER_MATERIALS_CRCS 146

// Properly cooked curves.
#define VER_CURVE_COOKING 147

// CMesh vertices sort skinning index/weight by decreasing weight
#define VER_MESH_SORTED_SKINNING 148

// Store per templated entity instance parameters
#define VER_TEMPLATE_INSTANCE_PARAMETERS 149

// Pointer serialization using object map for already created objects
#define VER_POINTER_WITH_MAP_SERIALIZATION 150

// Hashset reimplemented (based on new THashMap) to be more optimal (both performance and memory wise)
#define VER_NEW_HASHSET 151

// Bug in GetUTCTime/GetLocalTime, could cause entity templates to not recreate full buffer if an include changes
#define VER_FIX_GET_TIME_FOR_CACHED_ENTITY_BUFFERS 152

// File header contains information about deferred data buffers
#define VER_DEPENDENCY_LINKER_HEADER_BUFFER_ENTRY 153

// Optimized foliage data format
#define VER_FOLIAGE_DATA_OPTIMIZATION 154

// File header contains CRC information
#define VER_UPDATED_RESOURCE_FORMAT	155

// GPU Data Buffers contain base ptr alignment (for in-place loaded assets)
#define VER_GPU_DATA_BUFFERS_ALIGNMENT 156

// IMaterialDefinition no longer contains information about its compiled techniques
#define VER_REMOVED_COOKEDTECHNIQUES 157

// Serialization stream support
#define VER_SERIALIZATION_STREAM 158

// Separated vertex & index data from CMesh chunk array
#define VER_SEPARATED_PAYLOAD_FROM_MESH_CHUNKS 159

// changed Umbra to use DeferredDataBuffer instead of LatentDataBuffer
#define VER_UMBRA_USES_DEFERRED_DATA_BUFFERS 160

// Can skippable blocks be stored in-line
#define VER_SKIPABLE_BLOCKS_NOINLINE_SUPPORT 161

// Removed saving of m_localToWorld from CNode
#define VER_REMOVED_SAVING_OF_LOCAL_TO_WORLD 162

// Removed all RTTI/unused data from lipsync animations
#define VER_OPTIMIZED_LIPSYNC_CACHE 163

/****************************/
/* Current version			*/
/****************************/
#define VER_CURRENT		VER_OPTIMIZED_LIPSYNC_CACHE

/********************************/
/* Minimal supported version	*/
/********************************/
#define VER_MINIMAL		VER_RESAVE_OCT_2011
