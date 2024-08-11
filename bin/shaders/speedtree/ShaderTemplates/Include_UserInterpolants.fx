///////////////////////////////////////////////////////////////////////
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with
//  the terms of that agreement.
//
//      Copyright (c) 2003-2014 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      Web: http://www.idvinc.com

#ifndef ST_INCLUDE_USER_INTERPOLANTS
#define ST_INCLUDE_USER_INTERPOLANTS


//	Due to the way shaders are generated from templates in the SpeedTree 6.x and 7.x systems, it's
//	more or less impossible to modify the templates to add extra interpolants (those values
//	passed from the vertex to the pixel shaders). We added this system to make it manageable.
//
//	Users have the option of adding up to four extra float4 interpolants by modifying the
//	source contained in this file. By adding "#define ST_USER_INTERPOLANT0" to this file, an
//	extra float4 will be passed from the vertex shader to the pixel shader. If #define'd, 
//	float4 v2p_vUserInterpolant ("v2p" = vertex-to-pixel) will be created. It must be
//	fully assigned in the vertex shader, else the compilation is likely to fail. Search 
//  for "ST_USER_INTERPOLANT0" in Template_3dGeometry_Vertex.fx or
//  Template_3dGeometry_Vertex_Deferred.fx) to find where this should be done.
//
//	Keep in mind that users may put conditions on when the user interpolants are active.
//	That is, interpolants don't have to be active for every generated shader. If, for example,
//	you're passing a new value to be used with specular lighting, you can check for the
//	presence of specular lighting by using code like:
//	
//	#ifdef SPECULAR // defined by the Compiler when generating
//					// each shader, based on effect LOD dialog
//		#define ST_USER_INTERPOLANT0
//	#endif
//	
//	Then, within the vertex shader, only assign the interpolant when present:
//
//	#ifdef ST_USER_INTERPOLANT0
//		v2p_vUserInterpolant0 = float4(0, 0, 0, 0);
//	#endif
//
//	Be sure to only access the v2p_vUserInterpolantN variables in the pixel shader when they're
//	defined.
//
//	Note that these changes apply to the forward- and deferred-render templates used for the
//	3D tree geometry only. Billboard templates are not affected since they do not use dynamic
//	interpolants. Users are free to modify the less complex billboard templates as needed.
//
//	Below is a nearly-complete list of the macros defined by the Compiler when various render 
//	& wind states are set by the user. They can be used in your own expressions to manage when
//	interpolants should be used.
//
//	todo: what to do with all this? a lot is out of date
//
//	  Macro								Additional Description
//  -------------------------------------------------------------------------------------------
//	ALPHA_TEST_NOISE					Alpha test noise is active (when a2c isn't available)
//	BACKFACES_CULLED					
//	LIGHTING_MODEL_PER_VERTEX			
//	LIGHTING_MODEL_PER_PIXEL
//	LIGHTING_MODEL_GRADUAL_CHANGE		Forward rendering; transition between per-vertex and per-pixel lighting
//	LIGHTING_MODEL_DEFERRED				Deferred rendering is active (always per-pixel)
//	AMBIENT_CONTRAST					Ambient contrast is active, labeled "Contrast" in Effect LOD dialog
//	AMBIENT_CONTRAST_FADE				Ambient contrast effect is fading in/out in this LOD
//	AMBIENT_OCCLUSION
//	DIFFUSE_MAP_IS_OPAQUE				There is no alpha channel in the diffuse texture (can skip texkill)
//	DETAIL_LAYER						
//	DETAIL_LAYER_FADE					Detail layer is fading in/out in this LOD
//	DETAIL_NORMAL_LAYER					Detail layer has and is using its own normal map
//	SPECULAR							Specular lighting is in effect (forward rendering)
//	SPECULAR_FADE						Specular effect is fading in/out in this LOD
//	TRANSMISSION						Transmission lighting is in effect (forward rendering)
//	TRANSMISSION_FADE					Transmission effect is fading in/out in this LOD
//	BRANCH_SEAM_SMOOTHING				
//	BRANCH_SEAM_SMOOTHING_FADE
//	LOD_METHOD_SMOOTH					LOD setting is smooth as opposed to popping
//	FADE_TO_BILLBOARD					This shader is the last LOD and fades out to make way for billboard
//	HAS_HORZ_BB							Indicates a horizontal billboard is present
//	FOG_COLOR_TYPE_CONSTANT				Using SpeedTree's fog system, the color is constant
//	FOG_COLOR_TYPE_DYNAMIC				Using SpeedTree's fog system, the color is computed per-pixel (see sky color in ref app)
//	SHADOWS_ENABLED						Geometry rendered in this shader receives shadows
//	SHADOWS_SMOOTH						Shadows will be filtered above standard PCF (expensive)
//	ST_SHADOWS_NUM_MAPS					Number of cascaded shadow maps, one to four
//	ST_BRANCHES_PRESENT					This shader intended to be used to render branch geometry (may be mixed)
//	ONLY_BRANCHES_PRESENT				This shader intended to be used to render *only* branch geometry
//	ST_FRONDS_PRESENT
//	ONLY_FRONDS_PRESENT
//	ST_LEAVES_PRESENT
//	ONLY_LEAVES_PRESENT
//	FACING_ST_LEAVES_PRESENT
//	ONLY_FACING_ST_LEAVES_PRESENT
//	RIGID_MESHES_PRESENT
//	ONLY_RIGID_MESHES_PRESENT
//	VERTEX_PROPERTY_TANGENT_PRESENT		Flag indicating that a per-vertex tangent value is in the vertex decl
//	USED_AS_GRASS						This shader is intended to render grass geometry
//
//
// Wind state macros appear in this section:
//
//	  Macro					Description
//  -------------------------------------------------------------------------
//	WIND_LOD_GLOBAL
//	WIND_LOD_BRANCH
//	WIND_LOD_FULL
//	WIND_LOD_NONE_X_GLOBAL
//	WIND_LOD_GLOBAL
//	WIND_LOD_NONE_X_BRANCH
//	WIND_LOD_BRANCH
//	WIND_LOD_NONE_X_FULL
//	WIND_LOD_FULL
//	WIND_LOD_GLOBAL_X_BRANCH
//	WIND_LOD_BRANCH
//	WIND_LOD_GLOBAL_X_FULL
//	WIND_LOD_FULL
//	WIND_LOD_BRANCH_X_FULL
//	WIND_LOD_FULL
//	WIND_LOD_NONE
//	ST_WIND_EFFECT_GLOBAL_WIND
//	ST_WIND_EFFECT_GLOBAL_PRESERVE_SHAPE
//	ST_WIND_EFFECT_BRANCH_SIMPLE_1
//	ST_WIND_EFFECT_BRANCH_DIRECTIONAL_1
//	ST_WIND_EFFECT_BRANCH_DIRECTIONAL_FROND_1
//	ST_WIND_EFFECT_BRANCH_TURBULENCE_1
//	ST_WIND_EFFECT_BRANCH_WHIP_1
//	ST_WIND_EFFECT_BRANCH_ROLLING_1
//	ST_WIND_EFFECT_BRANCH_OSC_COMPLEX_1
//	ST_WIND_EFFECT_BRANCH_SIMPLE_2
//	ST_WIND_EFFECT_BRANCH_DIRECTIONAL_2
//	ST_WIND_EFFECT_BRANCH_DIRECTIONAL_FROND_2
//	ST_WIND_EFFECT_BRANCH_TURBULENCE_2
//	ST_WIND_EFFECT_BRANCH_WHIP_2
//	ST_WIND_EFFECT_BRANCH_ROLLING_2
//	ST_WIND_EFFECT_BRANCH_OSC_COMPLEX_2
//	ST_WIND_EFFECT_LEAF_RIPPLE_VERTEX_NORMAL_1
//	ST_WIND_EFFECT_LEAF_RIPPLE_COMPUTED_1
//	ST_WIND_EFFECT_LEAF_TUMBLE_1
//	ST_WIND_EFFECT_LEAF_TWITCH_1
//	ST_WIND_EFFECT_LEAF_ROLL_1
//	ST_WIND_EFFECT_LEAF_OCCLUSION_1
//	ST_WIND_EFFECT_LEAF_RIPPLE_VERTEX_NORMAL_2
//	ST_WIND_EFFECT_LEAF_RIPPLE_COMPUTED_2
//	ST_WIND_EFFECT_LEAF_TUMBLE_2
//	ST_WIND_EFFECT_LEAF_TWITCH_2
//	ST_WIND_EFFECT_LEAF_ROLL_2
//	ST_WIND_EFFECT_LEAF_OCCLUSION_2
//	ST_WIND_EFFECT_FROND_RIPPLE_ONE_SIDED
//	ST_WIND_EFFECT_FROND_RIPPLE_TWO_SIDED
//	ST_WIND_EFFECT_FROND_RIPPLE_ADJUST_LIGHTING
//	WIND_LOD_BILLBOARD_GLOBAL
//
//
//	The following macros are not render state related, but may be helpful:
//
//	  Macro					Description
//  -------------------------------------------------------------------------
//	DEFERRED_RENDER
//	FORWARD_RENDER
//	OPENGL
//	DIRECTX9
//	DIRECTX11
//	XBOX_360
//	PS3
//	PSP2
//	BILLBOARD_SHADER
//	TREE_SHADER
//	VERTEX_SHADER
//	PIXEL_SHADER
//	ST_COORDSYS_Y_UP
//	ST_COORDSYS_Z_UP
//	ST_COORDSYS_RIGHT_HANDED
//	ST_COORDSYS_LEFT_HANDED


//	Define all or none of the four user interpolants; use #ifdef conditions to restrict
//	use if viable (see top of this file for details)

#define ST_USER_INTERPOLANT0 1		// LAVA: random colorization
#define ST_USER_INTERPOLANT1 1		// LAVA: worldPosition
#define ST_USER_INTERPOLANT2 1		// LAVA: pigment for grass or tree fading for trees
#define ST_USER_INTERPOLANT3 0

#endif // ST_INCLUDE_USER_INTERPOLANTS
