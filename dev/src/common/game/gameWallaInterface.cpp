#include "build.h"
#include "gameWallaInterface.h"
#include "..\engine\soundSystem.h"


CGameWallaInterface CGameWallaInterface::sm_WallaInterface;

CGameWallaInterface::CGameWallaInterface(void)
{
}


CGameWallaInterface::~CGameWallaInterface(void)
{
}

void CGameWallaInterface::InitClass()
{
	CSoundWallaSystem::Init(&sm_WallaInterface);
}

SoundWallaMetrics CGameWallaInterface::CalculateSoundWallaMetricsForBox(const Vector &position, const Box &box, const SoundWallaProcessParams &params) const
{
	CActorsManager* actorsManager = GCommonGame->GetActorsManager();

	ActorMetricFunctor actorMetricBuilder(params);

	actorsManager->TQuery( position, actorMetricBuilder, box, true, nullptr, 0 );

	return actorMetricBuilder.ComputeMetrics();
}

Bool CGameWallaInterface::ActorMetricFunctor::operator()(const CActorsManagerMemberData& element)
{
	CNewNPC * npc = Cast<CNewNPC>(element.Get());
	if(npc && npc->IsAlive() && npc->GetMonsterCategory() == MC_Human)
	{
		Vector direction = npc->GetWorldPosition() - GSoundSystem->GetListenerPosition();
		direction.SetZ(0.f);

		Float dist = direction.Mag2();

		if(dist > params.maxDistance)
		{
			return true;
		}
		else
		{
			direction = CSoundWallaSystem::RotateNormalizedVector(direction, -1.f*params.wallaRotation);

			Uint32 npcValue =  1;

			
			
			auto updateMetrics = [npc, this, npcValue](ESoundWallaDirection dir)
			{
				if(npc->IsAfraid())
				{
					metrics[dir].numAfraidActors += npcValue;
				}
				else if(npc->IsInInterior())
				{
					metrics[dir].numInteriorActors+= npcValue;
					metrics[dir].sumInteriorDistance += npc->GetWorldPosition();
				}
				else
				{
					metrics[dir].numActors+= npcValue;
					metrics[dir].sumDistance += npc->GetWorldPosition();
				}
			};

			if(dist < params.minDistance)
			{
				//npcValue = 2;
				updateMetrics(Walla_DirFR);
				updateMetrics(Walla_DirBR);
				updateMetrics(Walla_DirBL);
				updateMetrics(Walla_DirFL);
			}
			
			//else
			{
				//Normalized vector x,y is equivalent to cos,sin of the direction angle
				if(direction.X > 0.f && direction.Y > 0.f)
				{
					updateMetrics(Walla_DirFR);
				}
				else if(direction.X > 0.f && direction.Y <= 0.f)
				{
					updateMetrics(Walla_DirBR);
				}
				else if(direction.X <= 0.f && direction.Y <= 0.f)
				{
					updateMetrics(Walla_DirBL);
				}
				else if(direction.X <= 0.f && direction.Y > 0.f)
				{
					updateMetrics(Walla_DirFL);
				}
			}

			if(!npc->IsInInterior())
			{
				totalNumActors++;
			}
			else
			{
				totalInteriorActors++;
			}
		}
	}

	return true;
}

SoundWallaMetrics CGameWallaInterface::ActorMetricFunctor::ComputeMetrics()
{
	SoundWallaMetrics wallaMetrics;

	for(int i=0; i < Num_WallaDirections; i++)
	{
		wallaMetrics.numActors[i] = metrics[i].numActors;
		wallaMetrics.numInteriorActors[i] = metrics[i].numInteriorActors;
		wallaMetrics.numAfraidActors[i] = Float(metrics[i].numAfraidActors);
		
		//We're ignoring the vertical component when calculating the average distance from the listener
		if(metrics[i].numActors > 0)
		{
			Vector avgPosition = (metrics[i].sumDistance/Float(metrics[i].numActors));
			wallaMetrics.avgDistance[i] = (avgPosition - GSoundSystem->GetListenerPosition()).Mag2();
		}
		else
		{
			wallaMetrics.avgDistance[i] = 0.f;
		}

		if(metrics[i].numInteriorActors > 0)
		{
			Vector avgInteriorPosition = (metrics[i].sumInteriorDistance/Float(metrics[i].numInteriorActors));
			wallaMetrics.avgInteriorDistance[i] = (avgInteriorPosition - GSoundSystem->GetListenerPosition()).Mag2();
		}
		else
		{
			wallaMetrics.avgInteriorDistance[i] = 0.f;
		}

	}

	wallaMetrics.totalNumActors = totalNumActors;
	wallaMetrics.totalNumInteriorActors = totalInteriorActors;

	return wallaMetrics;
}

