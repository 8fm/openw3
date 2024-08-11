#include "build.h"
#include "soundWallaSystem.h"
#include "soundSystem.h"


CSoundWallaInterface * CSoundWallaSystem::sm_GameInterface = nullptr;


Vector  CSoundWallaSystem::sm_WallaDirections[Num_WallaDirections] =
{
	Vector(1.f, 1.f, 0.f), //Walla_Dir1
	Vector(1.f, -1.f, 0.f), //Walla_Dir2
	Vector(-1.f, -1.f, 0.f), //Walla_Dir3
	Vector(-1.f, 1.f, 0.f) //Walla_Dir4
};

CSoundWallaSystem::CSoundWallaSystem(void)
{
}


CSoundWallaSystem::~CSoundWallaSystem(void)
{
}

void CSoundWallaSystem::Tick()
{
}

Vector CSoundWallaSystem::RotateNormalizedVector(const Vector &vec, Float rotation)
{
	Float cosRot = MCos(rotation), sinRot = MSin(rotation);
	//Use trig identity cos(u+v) = cos(u)*cos(v) - sin(u)*sin(v)
	Vector baseDir = vec.Normalized2();
	Float rotatedX = baseDir.X*cosRot - baseDir.Y*sinRot;
	//Use trig identity sin(u+v) = sin(u)*cos(v) + cos(u)*sin(v)
	Float rotatedY = baseDir.Y*cosRot + baseDir.X*sinRot;

	return Vector(rotatedX, rotatedY, 0.f);
}

Vector CSoundWallaSystem::GetDirectionVector(ESoundWallaDirection dir, Float rotation) 
{
	return RotateNormalizedVector(sm_WallaDirections[dir], rotation);
}

SoundWallaMetrics CSoundWallaSystem::CalculateSoundWallaMetrics(const Vector &position, const Box &box, const SoundWallaProcessParams &params) const
{
	if(IsInitialized())
	{
		SoundWallaMetrics metrics = sm_GameInterface->CalculateSoundWallaMetricsForBox(position, box, params);


		return metrics;
	}

	return SoundWallaMetrics();
}

void SoundWallaMetrics::UpdateFromMetrics(const SoundWallaMetrics & metrics)
{
	for(int i=0; i < Num_WallaDirections; i++)
	{
		numActors[i] = metrics.numActors[i];
		avgDistance[i] = metrics.avgDistance[i];
		avgInteriorDistance[i] = metrics.avgInteriorDistance[i];
		numInteriorActors[i] = metrics.numInteriorActors[i];
		if(numAfraidActors[i] < metrics.numAfraidActors[i])
		{
			numAfraidActors[i] = metrics.numAfraidActors[i];
		}
	}
	totalNumActors = metrics.totalNumActors;
	totalNumInteriorActors = metrics.totalNumInteriorActors;
}

Uint32 SoundWallaMetrics::GetTotalNumActors() const
{
	Uint32 result = 0;
	for(int i =0; i < Num_WallaDirections; i++)
	{
		result+= numActors[i];
	}

	return result;
}
