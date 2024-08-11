#pragma once

#ifndef PHYSX_INCLUDES_H
#define PHYSX_INCLUDES_H

#include "../redSystem/os.h"
#include "../redSystem/architecture.h"

#define USE_PHYSX
#define USE_APEX 

#define PC_SCOPE_PHYSICS( name )
//#define PC_SCOPE_PHYSICS( name ) PC_SCOPE_PIX( name )

#ifndef RED_PLATFORM_ORBIS
//#define PC_SCOPE_PHYSICS( name ) PcScopePhysicsLogger prof( ( #name ) );
#endif

#ifndef USE_PHYSX
	#ifdef USE_APEX
		#error "USE_APEX define cant be used without USE_PHYSX define"
	#endif
#else

#include "PxCpuDispatcher.h"
#include "PxPhysics.h"
#include "PvdConnectionManager.h"
#include "PxDefaultAllocator.h"
#include "PxDefaultCpuDispatcher.h"
#include "PxDefaultErrorCallback.h"
#include "PxDefaultSimulationFilterShader.h"
#include "PxExtensionsAPI.h"
#include "PxControllerManager.h"
#include "PxPhysicsAPI.h"
#include "PxBoxController.h"
#include "PxRigidBodyExt.h"
#include "PxIO.h"
#include "PxSimpleTypes.h"


#define TO_PX_VECTOR( v ) PxVec3( v.X, v.Y, v.Z )
#define TO_PX_MAT( v ) PxMat44(TO_PX_VECTOR(v.V[0]),TO_PX_VECTOR(v.V[1]),TO_PX_VECTOR(v.V[2]),TO_PX_VECTOR(v.V[3]))
#define TO_PX_EXT_VECTOR( v ) PxExtendedVec3( PxExtended( v.X ), PxExtended( v.Y ), PxExtended( v.Z ) )
#define TO_VECTOR( v ) Vector( v.x, v.y, v.z )
#define TO_VECTOR_FROM_EXT_VEC( v ) Vector( (Float)v.x, (Float)v.y, (Float)v.z )
#define COPY_TO_VECTOR( dst, src ) { dst.X = src.x; dst.Y = src.y; dst.Z = src.z; } 
#define TO_PX_MAT( v ) PxMat44(TO_PX_VECTOR(v.V[0]),TO_PX_VECTOR(v.V[1]),TO_PX_VECTOR(v.V[2]),TO_PX_VECTOR(v.V[3]))

#define TO_MAT( v ) Matrix(Vector(v.column0.x,v.column0.y,v.column0.z,0.0f),Vector(v.column1.x,v.column1.y,v.column1.z,0.0f),Vector(v.column2.x,v.column2.y,v.column2.z,0.0f),TO_VECTOR(v.column3))
#define TO_PX_QUAT( q ) PxQuat( q.X, q.Y, q.Z, q.W )
#define TO_QUAT( q ) Vector( q.x, q.y, q.z, q.w )

#ifdef RED_PROFILE_BUILD
	#define PHYSICS_RELEASE
#elif NO_EDITOR
	#define PHYSICS_RELEASE
#endif

//#define PHYSICS_RELEASE

//#define PHYSICS_PROFILE

//#ifndef RED_PROFILE_BUILD
	#define PHYSICS_NAN_CHECKS
//#endif

#ifdef RED_PLATFORM_WIN64
	#define PHYSX_LIB_PATH "..\\..\\..\\external\\PhysX3\\lib\\win64\\"
	#if defined(PHYSICS_PROFILE)
		#define PHYSX_LIB_EXT "PROFILE.lib"
		#define PHYSX_LIB_EXTB "PROFILE_x64.lib"
		#pragma comment(lib, PHYSX_LIB_PATH "PhysXProfileSDK" PHYSX_LIB_EXT )
	#elif defined(_DEBUG)
		#define PHYSX_LIB_EXT "DEBUG.lib"
		#define PHYSX_LIB_EXTB "DEBUG_x64.lib"
	#elif defined(PHYSICS_RELEASE)
		#define PHYSX_LIB_EXT ".lib"
		#define PHYSX_LIB_EXTB "_x64.lib"
	#else
		#define PHYSX_LIB_EXT "CHECKED.lib"
		#define PHYSX_LIB_EXTB "CHECKED_x64.lib"
	#endif
#elif defined( RED_PLATFORM_WIN32 )
	#define PHYSX_LIB_PATH "..\\..\\..\\external\\PhysX3\\lib\\win32\\"
	#if defined(PHYSICS_PROFILE)
		#define PHYSX_LIB_EXT "PROFILE.lib"
		#define PHYSX_LIB_EXTB "PROFILE_x86.lib"
		#pragma comment(lib, PHYSX_LIB_PATH "PhysXProfileSDK" PHYSX_LIB_EXT )
	#elif defined(_DEBUG)
		#define PHYSX_LIB_EXT "DEBUG.lib"
		#define PHYSX_LIB_EXTB "DEBUG_x86.lib"
	#elif defined(PHYSICS_RELEASE)
		#define PHYSX_LIB_EXT ".lib"
		#define PHYSX_LIB_EXTB "_x86.lib"
	#else
		#define PHYSX_LIB_EXT "CHECKED.lib"
		#define PHYSX_LIB_EXTB "CHECKED_x86.lib"
	#endif
#elif defined( RED_PLATFORM_DURANGO )
	#define PHYSX_LIB_PATH "..\\..\\..\\external\\PhysX3\\lib\\xboxone\\"
	#if defined(PHYSICS_PROFILE)
		#define PHYSX_LIB_EXT "PROFILE.lib"
		#define PHYSX_LIB_EXTB "PROFILE.lib"
		#pragma comment(lib, PHYSX_LIB_PATH "PhysXProfileSDK" PHYSX_LIB_EXT )
	#elif defined(_DEBUG)
		#define PHYSX_LIB_EXT "DEBUG.lib"
		#define PHYSX_LIB_EXTB "DEBUG.lib"
	#elif defined(PHYSICS_RELEASE)
		#define PHYSX_LIB_EXT ".lib"
		#define PHYSX_LIB_EXTB ".lib"
	#else
		#define PHYSX_LIB_EXT "CHECKED.lib"
		#define PHYSX_LIB_EXTB "CHECKED.lib"
	#endif
#elif defined( RED_PLATFORM_ORBIS )
	#define PHYSX_LIB_PATH "..\\..\\..\\external\\PhysX3\\lib\\ps4\\lib"
	#if defined(PHYSICS_PROFILE)
		#define PHYSX_LIB_EXT "PROFILE.a"
		#define PHYSX_LIB_EXTB "PROFILE.a"
		#pragma comment(lib, PHYSX_LIB_PATH "PhysXProfileSDK" PHYSX_LIB_EXT )
	#elif defined(_DEBUG)
		#define PHYSX_LIB_EXT "DEBUG.a"
		#define PHYSX_LIB_EXTB "DEBUG.a"
	#elif defined(PHYSICS_RELEASE)
		#define PHYSX_LIB_EXT ".a"
		#define PHYSX_LIB_EXTB ".a"
	#else
		#define PHYSX_LIB_EXT "CHECKED.a"
		#define PHYSX_LIB_EXTB "CHECKED.a"
	#endif
#endif

#pragma comment(lib, PHYSX_LIB_PATH "PhysX3Common" PHYSX_LIB_EXTB )
#pragma comment(lib, PHYSX_LIB_PATH "PhysX3CharacterKinematic" PHYSX_LIB_EXTB )
#pragma comment(lib, PHYSX_LIB_PATH "PhysX3Cooking" PHYSX_LIB_EXTB )
#pragma comment(lib, PHYSX_LIB_PATH "PhysX3" PHYSX_LIB_EXTB )
#pragma comment(lib, PHYSX_LIB_PATH "PhysX3Extensions" PHYSX_LIB_EXT )
#pragma comment(lib, PHYSX_LIB_PATH "PxTask" PHYSX_LIB_EXT )
#pragma comment(lib, PHYSX_LIB_PATH "PhysXVisualDebuggerSDK" PHYSX_LIB_EXT )

#ifdef USE_APEX

#ifdef RED_PLATFORM_WIN64
	#define APEX_LIB_PATH "..\\..\\..\\external\\Apex\\lib\\vc10win64-PhysX_3.3\\"
#elif defined( RED_PLATFORM_WIN32 )
	#define APEX_LIB_PATH "..\\..\\..\\external\\Apex\\lib\\vc10win32-PhysX_3.3\\"
#elif defined( RED_PLATFORM_ORBIS )
	#define APEX_LIB_PATH "..\\..\\..\\external\\Apex\\lib\\vc10ps4-PhysX_3.3\\lib"
#elif defined( RED_PLATFORM_DURANGO )
#define APEX_LIB_PATH "..\\..\\..\\external\\Apex\\lib\\vc11xboxone-PhysX_3.3\\"
#endif


#pragma comment(lib, APEX_LIB_PATH "ApexFramework" PHYSX_LIB_EXTB )

#ifdef RED_PLATFORM_CONSOLE
#pragma comment(lib, APEX_LIB_PATH "APEX_Clothing"		PHYSX_LIB_EXT )
#pragma comment(lib, APEX_LIB_PATH "APEX_Destructible" PHYSX_LIB_EXT )
#pragma comment(lib, APEX_LIB_PATH "APEX_Legacy"		PHYSX_LIB_EXT )
#else
#pragma comment(lib, APEX_LIB_PATH "APEX_Clothing"		PHYSX_LIB_EXTB )
#pragma comment(lib, APEX_LIB_PATH "APEX_Destructible"	PHYSX_LIB_EXTB )
#pragma comment(lib, APEX_LIB_PATH "APEX_Legacy"		PHYSX_LIB_EXTB )
#endif

#endif

#endif

#endif