#include "PsShare.h"
#include "NxApex.h"
#include "ApexLegacyModule.h"

// AUTO_GENERATED_INCLUDES_BEGIN
#include "NoiseFSAssetParams_0p0.h"
#include "NoiseFSAssetParams_0p1.h"
#include "ConversionNoiseFSAssetParams_0p0_0p1.h"
// AUTO_GENERATED_INCLUDES_END

namespace physx
{
namespace apex
{
namespace legacy
{

// AUTO_GENERATED_OBJECTS_BEGIN
static NoiseFSAssetParams_0p0Factory factory_NoiseFSAssetParams_0p0;
static NoiseFSAssetParams_0p1Factory factory_NoiseFSAssetParams_0p1;
// AUTO_GENERATED_OBJECTS_END

static LegacyClassEntry ModuleBasicFSLegacyObjects[] = {
	// AUTO_GENERATED_TABLE_BEGIN
	{
		0,
		1,
		&factory_NoiseFSAssetParams_0p0,
		NoiseFSAssetParams_0p0::freeParameterDefinitionTable,
		ConversionNoiseFSAssetParams_0p0_0p1::Create,
		0
	},
	// AUTO_GENERATED_TABLE_END

	{ 0, 0, 0, 0, 0, 0} // Terminator
};

class ModuleBasicFSLegacy : public ApexLegacyModule
{
public:
	ModuleBasicFSLegacy( NiApexSDK* sdk );

protected:
	void releaseLegacyObjects();

private:

	// Add custom conversions here

};

	DEFINE_INSTANTIATE_MODULE(ModuleBasicFSLegacy)

ModuleBasicFSLegacy::ModuleBasicFSLegacy( NiApexSDK* inSdk )
{
	name = "BasicFS_Legacy";
	mSdk = inSdk;
	mApiProxy = this;

	// Register legacy stuff

	NxParameterized::Traits *t = mSdk->getParameterizedTraits();
	if( !t )
		return;

	// Register auto-generated objects
	registerLegacyObjects(ModuleBasicFSLegacyObjects);

	// Register custom conversions here
}

void ModuleBasicFSLegacy::releaseLegacyObjects()
{
	//Release legacy stuff

	NxParameterized::Traits *t = mSdk->getParameterizedTraits();
	if( !t )
		return;

	// Unregister auto-generated objects
	unregisterLegacyObjects(ModuleBasicFSLegacyObjects);

	// Unregister custom conversions here
}

}
}
} // end namespace physx::apex
