#pragma once
#include "animSkeletalDangleConstraint.h"

// CHECK PERFORCE HISTORY FOR PREVIOUS IMPLEMENTATION

class DyngSimulator
{
public:
	Bool	drawlinks;
	Bool	drawcolls;
	Bool	drawlimits;
	Bool	useoffsets;
	Bool	planeCollision;
	Float	dampening;
	Float	dt;
	Float	shaking;
	Int32	max_link_iterations;
	Float	gravity;
	Float	weightdist;
	Bool	relaxing;

public:
	TDynArray<Vector>		positions;
	TDynArray<Vector>		velocities;
	TDynArray<Int32>		lookats;
	TDynArray<Float>		masses;
	TDynArray<Matrix>		globalTransforms;
	TDynArray<Matrix>		offsets;
	TDynArray<Float>		stifnesses;
	TDynArray<Float>		shakness;
	TDynArray<Float>		distances;
	TDynArray<Float>		radiuses;
	TDynArray<Int32>		indices;

	TDynArray<Int32>		nodeA;
	TDynArray<Int32>		nodeB;
	TDynArray<Float>		weight;
	TDynArray<Float>		lengths;
	TDynArray<Int32>		types;

public:
	DyngSimulator();
	~DyngSimulator();

	void Evaluate( Bool offs, Bool update = true );
	void SetWeight( Float w );
	void ForceReset();
	void SetShakeFactor( Float factor );
	void SetGravityFactor( Float factor );
	void DebugDraw( const Matrix& l2w, CRenderFrame* frame );
	void PostLoad();

private:
	void EvaluateNodes();
	void EvaluateDistances();
	void EvaluateDistancesOffsets();
	void EvaluateTransforms();
	Float EvaluateLinks();
	Float EvaluateLink( Int32 i );
};

