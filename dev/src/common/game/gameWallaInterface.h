#pragma once

#include "actorsManager.h"

#include "../engine/soundWallaSystem.h"

class CGameWallaInterface :
	public CSoundWallaInterface
{
public:
	CGameWallaInterface(void);
	~CGameWallaInterface(void);

	static void InitClass();


	virtual SoundWallaMetrics CalculateSoundWallaMetricsForBox(const Vector &position, const Box &box, const SoundWallaProcessParams &params) const;


private:

	struct ActorMetricFunctor : public CQuadTreeStorage< CActor, CActorsManagerMemberData >::DefaultFunctor
	{
		enum { SORT_OUTPUT = false };

		struct DirectionActorMetrics
		{
			Uint32 numActors;
			Vector sumDistance;
			Vector sumInteriorDistance;
			Uint32 numInteriorActors;
			Uint32 numAfraidActors;
		};

		ActorMetricFunctor(const SoundWallaProcessParams &inParams)
			:params(inParams)
		{
			for(int i=0; i < Num_WallaDirections; i++)
			{
				metrics[i].sumDistance.SetZeros();
				metrics[i].sumInteriorDistance.SetZeros();
				metrics[i].numActors = 0;
				metrics[i].numInteriorActors = 0;
				metrics[i].numAfraidActors = 0;
			}
			
			totalNumActors = 0;
			totalInteriorActors = 0;
		}



		Bool operator()( const CActorsManagerMemberData& element );

		SoundWallaMetrics ComputeMetrics();

	private:

		DirectionActorMetrics metrics[Num_WallaDirections];
		const SoundWallaProcessParams params;
		Float  totalNumActors;
		Float  totalInteriorActors;

	};


	static CGameWallaInterface sm_WallaInterface;

};

