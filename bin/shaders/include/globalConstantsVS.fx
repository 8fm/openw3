/// List of SYSTEM GLOBAL constants for vertex and pixel shader
/// It's esential to recompile all shaders after changing any of those lines

#include "globalConstants.fx"

/// Register
#ifdef __cplusplus

	#ifndef REG
		#define REG( _name, _reg, _type )				const Red::System::Uint32 _name = (_reg);
		#define REGI( _name, _reg, _type )				const Red::System::Uint32 _name = (_reg);
		#define REGB( _name, _reg, _type )				const Red::System::Uint32 _name = (_reg);
		#define REG_ARRAY( _name,_reg, _type, _size )	const Red::System::Uint32 _name = (_reg);
		#define SAMPLER( _name,_reg, _type )			const Red::System::Uint32 _name = (_reg);
		#define VERTEXSHADER 1
		#define PIXELSHADER 1
	#endif

	#ifndef START_CB
		#define START_CB( _name, _reg )		// EMPTY
		#define END_CB						// EMPTY
	#endif
															
#endif

////////////////////////////////////////////////
/// Vertex shader constants
////////////////////////////////////////////////

#ifdef VERTEXSHADER

#ifndef __cplusplus
#include "globalConstantBuffers.fx"
#endif

START_CB( FrequentVertexConsts, 2 )
// Common
REG( VSC_LocalToWorld, 			0, 	float4x4 )		// every draw
REG( VSC_QS, 					4, 	float4 )		// every buffer
REG( VSC_QB, 					5, 	float4 )		// every buffer
REG( VSC_DissolveParams,		6, 	float4 )		// possibly every draw (every dissolve)
REG( VSC_SkinningData, 			7, 	float4 )		// for skinning (every skinned mesh)
END_CB

#define VSC_Frequent_First		VSC_LocalToWorld
#define VSC_Frequent_Last		VSC_SkinningData

////////////////////////////////////////////////
/// Vertex shader custom constants
////////////////////////////////////////////////

START_CB( CustomVertexConsts, 3 )
REG( VSC_Custom_0 ,		10, float4 )
REG( VSC_Custom_1 ,		11, float4 )
REG( VSC_Custom_2 ,		12, float4 )
REG( VSC_Custom_3 ,		13, float4 )
REG( VSC_Custom_4 ,		14, float4 )
REG( VSC_Custom_5 ,		15, float4 )
REG( VSC_Custom_6 ,		16, float4 )
REG( VSC_Custom_Matrix,	17, float4x4 )
END_CB

#define VSC_Custom_First	VSC_Custom_0
#define VSC_Custom_Last		VSC_Custom_Matrix+3

#ifndef __cplusplus
# define VSGetIsReversedProjectionCamera		(0 == VSC_RevProjCameraInfo.x)
# define VSGetProjectionCameraSkyDepth			(VSC_RevProjCameraInfo.w)
# define VSGetProjectionCameraNearestDepth		(1 - VSC_RevProjCameraInfo.x)
#endif

#endif

#ifdef __cplusplus
	#undef REG
#endif
