#pragma once

enum ESoundWallaDirection
{
	Walla_DirFR, //Vector in direction 1.f, 1.f
	Walla_DirBR, //Vector in direction 1.f, -1.f
	Walla_DirBL, //Vector in direction -1.f, -1.f
	Walla_DirFL, //Vector in direction -1.f, 1.f
	Num_WallaDirections
};

struct SoundWallaProcessParams
{
	Float minDistance;
	Float maxDistance;
	Float wallaRotation;
};

struct SoundWallaMetrics
{
	SoundWallaMetrics() 
	{
		for(int i=0; i < Num_WallaDirections; i++)
		{
			numActors.PushBack(0);
			avgDistance.PushBack(0.f);
			avgInteriorDistance.PushBack(0.f);
			numInteriorActors.PushBack(0);
			numAfraidActors.PushBack(0);
		}
		totalNumActors = 0;
		totalNumInteriorActors = 0;
	}

	void UpdateFromMetrics(const SoundWallaMetrics & metrics);

	Uint32 GetTotalNumActors() const;

	TStaticArray<Uint32, Num_WallaDirections> numActors;
	TStaticArray<Float, Num_WallaDirections> avgDistance;
	TStaticArray<Float, Num_WallaDirections> avgInteriorDistance;
	TStaticArray<Uint32, Num_WallaDirections> numInteriorActors;
	TStaticArray<Float, Num_WallaDirections> numAfraidActors;

	Float totalNumActors;
	Float totalNumInteriorActors;
};

//The game should create an interface that derives from this class which will provide an interface for 
//querying information about game data such as actor metrics
class CSoundWallaInterface
{
public:
	virtual SoundWallaMetrics CalculateSoundWallaMetricsForBox(const Vector &position, const Box &box, const SoundWallaProcessParams &params) const = 0;

};

class CSoundWallaSystem
{
public:
	CSoundWallaSystem(void);
	~CSoundWallaSystem(void);


	//Set the interface to be used for obtaining metrics for ambient zone walla. Should be called once (probably from game code) at startup.
	static void Init(CSoundWallaInterface *gameWallaInterface) { sm_GameInterface = gameWallaInterface; }

	//Returns true if the system has been initialized, otherwise returns false
	bool IsInitialized() const { return sm_GameInterface != nullptr; }


	//Update the sound walla based on the current metrics
	void Tick();

	//Takses a vector and a rotation in radians, returns a vector flattened int the x-y plane, normalized and rotated around z
	static Vector RotateNormalizedVector(const Vector& vec, Float rotation);
	
	//For the given walla direction and rotation in radians, returns the rotated Vector for that direction
	static Vector GetDirectionVector(ESoundWallaDirection dir, Float rotation);

	//Takes a position and a box (TODO: maybe we want other inputs here) and produces the actor metrics for this region
	//position: metrics will use this as the base for actor distance; probably you want this to be the listener position
	//box: an aabb relative to the position parameter which will be used to generate actor metrics
	SoundWallaMetrics CalculateSoundWallaMetrics(const Vector &position, const Box &box, const SoundWallaProcessParams &params) const;

private:

	static CSoundWallaInterface *sm_GameInterface;

	static Vector sm_WallaDirections[Num_WallaDirections];
};



