// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.

#ifndef NX_MODULE_PARTICLES_H
#define NX_MODULE_PARTICLES_H

#include "NxApex.h"

namespace physx
{


namespace apex
{

PX_PUSH_PACK_DEFAULT

class NxModule;
class NxApexScene;
class NxEffectPackageActor;
/**
\brief Defines the API for the Particles Module

The particles module combines the features of a number of particle related APEX modules
into one location; with the exception of APEX turbulence.  It also provides support for
the EffectPackage actor and asset as well as integration with the ParticleEffectAuthoring
tool.

Loading the APEX Particles module will now automatically cause the following related modules
to be instantiated.

APEX_Emitter
APEX_BasicIOS
APEX_IOFX
APEX_FieldSampler
APEX_BasicIOS
APEX_ParticleIOS : PhysX 3.x only
APEX_ForceField : PhysX 3.x only
APEX_FieldBoundary : PhysX 2.8.x only
APEX_Explosion : PhysX 2.8.x only
APEX_NxFluidIOS : PhysX 2.8.x only

These modules are no longer loaded as separate DLLs and instantiated individually like they were
prior to APEX 1.3.  Instead simply loading the APEX_Particles DLL or calling 'instantiateModuleParticles'
will cause the other particle related modules to be loaded.

The rationale for making this change was to simplify the startup/shutdown sequence for applications
using APEX particle related effects.  It also dramatically simplifies the distribution process for
developers as they now only have to deal with a single DLL rather than as many as 8 which were
required before.

The turbulence module was not folded into the unified APEX_Particles module at this time since
the source code for it is not yet made available to licensees.

It is important to note that since the APEX_Particles module does not load the Turbulence module,
the application *must* load the Turbulence module first so that it will already be registered.
*/
class NxModuleParticles : public NxModule
{
public:

	/**
	\brief Return one of the particle related modules, by name, created by the particles module
	*/
	virtual physx::apex::NxModule* getModule(const char* moduleName) = 0;

	/**
	\brief Registers and adds either a single GraphicsMaterial definition or an entire array of them to the particles module.

	The EffectPackage system works with a set of predefined assets which have been authored using
	the ParticleEffectTool.  This method adds or revises either a single asset or an array of assets.
	When an EffectPackageActor is instantiated it may refer to assets which have been previously authored and
	registered using this method.

	\return Returns true if during the registration of this asset the contents were modified.
	*/
	virtual bool setEffectPackageGraphicsMaterialsDatabase(const NxParameterized::Interface* dataBase) = 0;

	/**
	\brief Registers and adds either a single IOS definition or an entire array of them to the particles module.

	The EffectPackage system works with a set of predefined assets which have been authored using
	the ParticleEffectTool.  This method adds or revises either a single asset or an array of assets.
	When an EffectPackageActor is instantiated it may refer to assets which have been previously authored and
	registered using this method.

	\return Returns true if during the registration of this asset the contents were modified.
	*/
	virtual bool setEffectPackageIOSDatabase(const NxParameterized::Interface* dataBase) = 0;

	/**
	\brief Registers and adds either a single IOFX definition or an entire array of them to the particles module.

	The EffectPackage system works with a set of predefined assets which have been authored using
	the ParticleEffectTool.  This method adds or revises either a single asset or an array of assets.
	When an EffectPackageActor is instantiated it may refer to assets which have been previously authored and
	registered using this method.

	\return Returns true if during the registration of this asset the contents were modified.
	*/
	virtual bool setEffectPackageIOFXDatabase(const NxParameterized::Interface* dataBase) = 0;

	/**
	\brief Registers and adds either a single Emitter definition or an entire array of them to the particles module.

	The EffectPackage system works with a set of predefined assets which have been authored using
	the ParticleEffectTool.  This method adds or revises either a single asset or an array of assets.
	When an EffectPackageActor is instantiated it may refer to assets which have been previously authored and
	registered using this method.

	\return Returns true if during the registration of this asset the contents were modified.
	*/
	virtual bool setEffectPackageEmitterDatabase(const NxParameterized::Interface* dataBase) = 0;

	/**
	\brief Registers and adds either a single EffectPackage definition or an entire array of them to the particles module.

	The EffectPackage system works with a set of predefined assets which have been authored using
	the ParticleEffectTool.  This method adds or revises either a single asset or an array of assets.
	When an EffectPackageActor is instantiated it may refer to assets which have been previously authored and
	registered using this method.

	\return Returns true if during the registration of this asset the contents were modified.
	*/
	virtual bool setEffectPackageDatabase(const NxParameterized::Interface* dataBase) = 0;

	/**
	\brief Registers and adds either a single FieldSampler definition or an entire array of them to the particles module.

	The EffectPackage system works with a set of predefined assets which have been authored using
	the ParticleEffectTool.  This method adds or revises either a single asset or an array of assets.
	When an EffectPackageActor is instantiated it may refer to assets which have been previously authored and
	registered using this method.

	\return Returns true if during the registration of this asset the contents were modified.
	*/
	virtual bool setEffectPackageFieldSamplerDatabase(const NxParameterized::Interface* dataBase) = 0;

	/**
	\brief Returns the NxParameterized::Interface representing the array of GraphicsMaterial assets registered by the Paricles modules
	*/
	virtual const NxParameterized::Interface* getEffectPackageGraphicsMaterialsDatabase() const = 0;

	/**
	\brief Returns the NxParameterized::Interface representing the array of IOS assets registered by the Paricles modules
	*/
	virtual const NxParameterized::Interface* getEffectPackageIOSDatabase() const = 0;

	/**
	\brief Returns the NxParameterized::Interface representing the array of IOFX assets registered by the Paricles modules
	*/
	virtual const NxParameterized::Interface* getEffectPackageIOFXDatabase() const = 0;

	/**
	\brief Returns the NxParameterized::Interface representing the array of Emitter assets registered by the Paricles modules
	*/
	virtual const NxParameterized::Interface* getEffectPackageEmitterDatabase() const = 0;

	/**
	\brief Returns the NxParameterized::Interface representing the array of EffectPackage assets registered by the Paricles modules
	*/
	virtual const NxParameterized::Interface* getEffectPackageDatabase() const = 0;

	/**
	\brief Returns the NxParameterized::Interface representing the array of FieldSampler assets registered by the Paricles modules
	*/
	virtual const NxParameterized::Interface* getEffectPackageFieldSamplerDatabase() const = 0;

	/**
	\brief This helper method will return the NxParameterized::Interface for a previously defined 'GraphicsMaterial'
	This 'GraphicsMaterial' contains settings which were defined in the ParticleEffectTool.  The ParticleEffectTool
	will let the user define things like a source texture, a blend mode, user properties, and celled animation settings,
	relative to a particular named material.  The application can use all of these settings or none of them as they prefer.
	These settings are used by the ParticleEffectTool to provide a prototype visualization of a sprite based particle effect.

	\param [in] name : The name of the predefined graphics material

	\return A pointer to the NxParameterized::Interface representing this material or NULL if not found.
	*/
	virtual const NxParameterized::Interface* locateGraphicsMaterialData(const char* name) const = 0;

	/**
	\brief This helper method will return the NxParameterized::Interface data for a predefined asset in one of the registered 'databases'
	When an application gets a callback asking for an asset which was authored using the ParticleEffectTool it can use
	this method to extract the current parameter data necessary to create that asset.

	\param [in] resourceName : The name of the previously defined/registered asset
	\param [in] nameSpace : The namespace of the asset.

	\return Returns the NxParamterized::Interface pointer for that asset or NULL if not found.
	*/
	virtual NxParameterized::Interface* locateResource(const char* resourceName,
	        const char* nameSpace) = 0;


	/**
	\brief This is a helper method to return the names of all assets registered relative to a particular namespace.

	\param [in] nameSpace : The namespace of the asset type we are searching for.
	\param [in] nameCount : A reference to a counter which will contain the number of assets found matching this namespace
	\param [in] variants : A reference to an array of const char pointers which will contain what reference variant each returned asset matches.

	\return Returns an array of const char pointers representing the asset names matching this resource type.
	*/
	virtual const char** getResourceNames(const char* nameSpace, physx::PxU32& nameCount, const char** &variants) = 0;

	/**
	\brief This method globally enabled or disables the screen-space culling LOD feature for EffectPackageActors
	EffectPackageActors can be culled based on whether or not they are on screen.  For this to work the application
	must provide a valid projection matrix, in addition to a current view matrix, into the SDK.
	The projection matrix in some engines define Z positive as being in front of the screen and others Z negative.

	\param [in] state : Whether to enable or disable the screen culling feature
	\param [in] znegative : Whether or not Z is positive or negative when a point projected into screen-space is in front or behind the camera.
	*/
	virtual void setEnableScreenCulling(bool state, bool znegative) = 0;

	/**
	\brief Resets the emitter pool; causing all cached emitters to be released.
	*/
	virtual void resetEmitterPool() = 0;

	/**
	\brief Sets the state to determine whether or not the particles module is going to use the emitter pool.
	The emitter pool is an optimization which prevents the effect system from constantly creating and the immediately releasing a great deal of APEX related
	assets and resources.  Take for example the case of a short lived particle effect.  Without an emitter pool, triggering this effect would cause a number of
	assets and rendering resources to be allocated sufficient to render and simulate this effect.  But, in just a very short time, the effect lifespan is over and
	all of those assets and rendering resources are deleted.  Then the same effect gets fired again causing the assets and render resources to be immediately re-allocated
	yet once again.  With the emitter pool enabled, the original emitter is not actually released when it times out.  Therefore the render resources and associated assets
	stay resident in memory.  When the same effect gets triggered again, it simply reuses the previously allocated emitter from the emitter pool.
	*/
	virtual void setUseEmitterPool(bool state) = 0;

	/**
	\brief Returns the current state of the emitter pool status.
	*/
	virtual bool getUseEmitterPool() const = 0;

	/**
	\brief Used by the ParticleEffectTool to initialize the default database values for the editor
	*/
	virtual void initializeDefaultDatabases() = 0;


protected:
	virtual ~NxModuleParticles() {}
};

#if !defined(_USRDLL)
/* If this module is distributed as a static library, the user must call this
* function before calling NxApexSDK::createModule("Particles")
*/
void instantiateModuleParticles();
#endif


PX_POP_PACK

}
} // end namespace physx::apex

#endif // NX_MODULE_PARTICLES_H
