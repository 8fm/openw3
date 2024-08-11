#include "build.h"
#include "dyngSimulator.h"
#include "renderFrame.h"
#include "animSkeletalDangleConstraint.h"

#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",off)
#endif

#ifdef DYNG_CHECK_NODES
void DyngNode::CheckData()
{
	ASSERT( position.IsOk() );
	ASSERT( velocity.IsOk() );
	ASSERT( globalTransform.IsOk() );
	ASSERT( offset.IsOk() );
	ASSERT( IsFinite( stifness ) );
	ASSERT( IsFinite( initDistance ) );
	ASSERT( IsFinite( currDistance ) );
}
#endif	


///////////////////////////////////////////////////
////////////////////////////////////////////////

DyngSimulator::DyngSimulator()
{
	drawlinks = true;
	drawcolls = true;
	drawlimits = true;
	shaking = 0.0f;
	useoffsets = false;
	planeCollision = false;
	max_link_iterations = 10;
	gravity = 1.0f;
	weightdist = 1.0f;
	relaxing = false;
	dt = 0.16f;
}

DyngSimulator::~DyngSimulator()
{
}

void DyngSimulator::SetWeight( Float w )
{
	ASSERT( w >= 0.f && w <= 1.f );
	weightdist = Clamp( w, 0.f, 1.f );
}

void DyngSimulator::ForceReset()
{
	Uint32 numn = globalTransforms.Size();
	for( Uint32 i=0; i<numn; ++i )
	{
		velocities[i] = Vector::ZEROS;
		positions[i] = globalTransforms[i].GetTranslation();
	}
}

void DyngSimulator::Evaluate( Bool offs, Bool update )
{
	if ( dt > 0.00001f )
	{
		if( !offs )
		{
			EvaluateDistances();
		}
		else
		{
			EvaluateDistancesOffsets();
		}
		EvaluateNodes();
		if( !offs )
		{
			EvaluateDistances();
		}
		else
		{
			EvaluateDistancesOffsets();
		}
		for( Int32 i=0;i<max_link_iterations;++i )
		{
			Float err = EvaluateLinks();
		}

		const Uint32 numn = globalTransforms.Size();
		for( Uint32 j=0;j<numn;++j )
		{
			velocities[j] *= dampening*(relaxing ? 0.9f : 1.0f);
		}
		if( !offs )
		{
			EvaluateDistances();
		}
		else
		{
			EvaluateDistancesOffsets();
		}
		if( update )
		{
			EvaluateTransforms();
		}
	}
}

void DyngSimulator::EvaluateNodes()
{
	Int32 numn = globalTransforms.Size();
	Int32 i;
	for( i=0;i<numn;++i )
	{
		Float g = gravity;
		{
			if( dt>0.000000001f )
			{
				Float del = -0.327f*masses[i]*g*dt;
				velocities[i].Z += del/dt;
			}
		}
		if( shakness[i]> 0.0f )
		{
			CStandardRand frandom;
			Vector rand;
			rand.X = frandom.Get< Float >( -1.0f , 1.0f );
			rand.Y = frandom.Get< Float >( -1.0f , 1.0f );
			rand.Z = frandom.Get< Float >( -1.0f , 1.0f );
			rand *= shaking*shakness[i];
			velocities[i] += rand*dt;
		}
		positions[i] += velocities[i]*(dt);		
	}
}
void DyngSimulator::EvaluateDistances()
{
	Float mul = 1.0f;
	Uint32 numn = globalTransforms.Size();
	for( Uint32 i=0; i<numn; ++i )
	{
		if( distances[i]>0.0f )
		{
			const Vector& target = globalTransforms[i].GetRow(3);
			Vector delta = target - positions[i];
			Float len = delta.Normalize3();
			if( planeCollision )
			{
				Vector row3 = globalTransforms[i].GetRow(1);
				Float pp = Vector::Dot3( row3, positions[i] - target );
				if( pp<0.0f )
				{
					Vector kor = row3*(-pp);
					positions[i] += kor;
					velocities[i] += (kor/dt)*mul;
				}
			}
			delta = target - positions[i];
			len = delta.Normalize3();
			if( len>distances[i] )
			{
				delta *= len - distances[i];
				positions[i] += delta;
				velocities[i] += (delta/dt)*mul;
			}
		}
		else
		{
			const Vector& target = globalTransforms[i].GetRow(3);
			positions[i] = target;
			velocities[i] = Vector::ZEROS;
		}
	}
}
void DyngSimulator::EvaluateDistancesOffsets()
{
	Float mul = 1.0f;
	Uint32 numn = globalTransforms.Size();
	for( Uint32 i=0; i<numn; ++i )
	{
		if( distances[i]>0.0f )
		{
			const Vector& target3 = globalTransforms[i].GetRow(3);
			Matrix globalColl = Matrix::Mul( globalTransforms[i], offsets[i] );
			Vector del = positions[i] - globalColl.GetTranslation();
			Float x = Vector::Dot3( del, globalColl.GetRow(0))/ Vector::Dot3( globalColl.GetRow(0), globalColl.GetRow(0));
			Float y = Vector::Dot3( del, globalColl.GetRow(1))/ Vector::Dot3( globalColl.GetRow(1), globalColl.GetRow(1));
			Float z = Vector::Dot3( del, globalColl.GetRow(2))/ Vector::Dot3( globalColl.GetRow(2), globalColl.GetRow(2));
			// try asserting x,y,z for __qnan
			Vector delta(x,y,z,1.0f);
			Float len = delta.Normalize3();
			if( planeCollision )
			{
				Vector row3 = globalTransforms[i].GetRow(1);
				Float pp = Vector::Dot3( row3, positions[i] - target3 );
				if( pp<0.0f )
				{
					Vector kor = row3*(-pp);
					positions[i] += kor;
					velocities[i] += (kor/dt)*mul;
				}
			}
			if( len>distances[i] )
			{
				delta *= distances[i];
				Vector target = globalColl.TransformPoint( delta );
				Vector korr = target - positions[i];
				positions[i] += korr;
				velocities[i] += (korr/dt)*mul;
			}

		}
		else
		{
			Matrix globalColl = Matrix::Mul( globalTransforms[i], offsets[i] );
			const Vector& target = globalColl.GetRow(3);
			positions[i] = target;
			velocities[i] = Vector::ZEROS;
		}
	}

}

void DyngSimulator::EvaluateTransforms()
{
	Uint32 numn = globalTransforms.Size();
	for( Uint32 i=0; i<numn; ++i )
	{
		RED_ASSERT( globalTransforms[i].IsOk(), TXT("1st globalTransforms.IsOk() failed") );
		//bool isok1 = globalTransforms[i].IsOk();
		globalTransforms[i].SetTranslation( positions[i] );
		//ASSERT( globalTransforms[i].IsOk() && "Dyng Lookat zjebal macierz: rot jest nieprawidlowe pre." );
		if( lookats[i]>=0 && radiuses[i]>0.001f )
		{
			// do the look at
			Vector toLookat = positions[ lookats[i] ] - positions[i];
			Float len = toLookat.Normalize3();
			if( len>0.0001f )
			{
				Vector row1 = globalTransforms[i].GetRow( 0 );
				Vector ax = Vector::Cross( row1, toLookat );
				Float axlen = ax.Normalize3();
				if( axlen>0.0001f )
				{
					Float an = MAcos_safe(Vector::Dot3( row1, toLookat ))*0.5f;
					Float san = sinf( an );
					Vector q( ax.X*san, ax.Y*san, ax.Z*san, -cosf(an) );
					Matrix rot = Matrix::IDENTITY;
					rot.BuildFromQuaternion( q );
					//ASSERT( rot.IsOk() && "Dyng Lookat zjebal macierz: rot jest nieprawidlowe." );
					Matrix frot = globalTransforms[i];
					frot.SetTranslation( Vector(0.0f,0.0f,0.0f) );
					frot = Matrix::Mul( rot, frot );
					frot.SetTranslation( positions[i] );
					globalTransforms[i] = frot;
				}
			}
		}
		RED_ASSERT( globalTransforms[i].IsOk(), TXT("2nd globalTransforms.IsOk() failed") );
		//ASSERT( globalTransforms[i].IsOk() && "Dyng Lookat zjebal macierz: rot jest nieprawidlowe post." );
		/*
		bool isok2 = globalTransforms[i].IsOk();
		if( isok1 && !isok2 )
		{
			ASSERT( 0 && "Dyng Lookat zjebal macierz." );
		}
		*/
	}
}

Float DyngSimulator::EvaluateLink( Int32 i )
{
	//Float beta = 100.0f;
	Float len = lengths[i];
	const Float w =  weight[i];
	Vector delta = positions[ nodeB[i] ] - positions[ nodeA[i] ];
	Float dist = delta.Mag3();
	Float diff = dist - len;
	if( types[i] == 0 && fabs(diff) >0.000000001f && dist>0.0f )
	{
		delta /= dist;
		delta *= diff;// * HyperbolicTangentPolyAproxxNorm( fabs(diff*beta) );
		const Vector velA = delta * (1.0f-w);
		const Vector velB = delta * (-w);
		positions[ nodeA[i] ] += velA;
		velocities[ nodeA[i] ] += velA/dt;
		positions[ nodeB[i] ] += velB;
		velocities[ nodeB[i] ] += velB/dt;
	}
	if( types[i] == 1 && (diff) < 0.0f && dist > 0.f )
	{
		delta /= dist;
		delta *= diff;// * HyperbolicTangentPolyAproxxNorm( fabs(diff*beta) );
		const Vector velA = delta * (1.0f-w);
		const Vector velB = delta * (-w);
		positions[ nodeA[i] ] += velA;
		velocities[ nodeA[i] ] += velA/dt;
		positions[ nodeB[i] ] += velB;
		velocities[ nodeB[i] ] += velB/dt;
	}
	if( types[i] == 2 && (diff) > 0.0f && dist > 0.f )
	{
		delta /= dist;
		delta *= diff;// * HyperbolicTangentPolyAproxxNorm( fabs(diff*beta) );
		const Vector velA = delta * (1.0f-w);
		const Vector velB = delta * (-w);
		positions[ nodeA[i] ] += velA;
		velocities[ nodeA[i] ] += velA/dt;
		positions[ nodeB[i] ] += velB;
		velocities[ nodeB[i] ] += velB/dt;
	}
	return diff*diff*100.0f;
}
Float DyngSimulator::EvaluateLinks()
{
	Float err = 0.0f;
	Uint32 numl = nodeA.Size();
	for( Uint32 i=0; i<numl; ++i )
	{
		err += EvaluateLink( i );
	}
	return err;
}

void DyngSimulator::PostLoad()
{
	Uint32 numl = nodeA.Size();
	for( Uint32 i=0; i<numl; ++i )
	{
		Float w = 0.0f;
		if ( masses[nodeA[i]] <= 0.0f && masses[nodeB[i]] <= 0.0f ) 
		{ 
			w = 0.0f; 
		}
		else if ( distances[nodeA[i]] <= 0.0f ) 
		{ 
			w = 1.0f; 
		}
		else if ( distances[ nodeB[i]] <= 0.0f ) 
		{ 
			w = 0.0f; 
		}
		else
		{
			Float sum = masses[nodeA[i]] + masses[nodeB[i]];
			RED_ASSERT( sum != 0.f, TXT("Dividing by zero in dyng. Please fix it.") );
			w = masses[nodeA[i]]/sum;
		}
		weight[i] = w;
	}
}

void DyngSimulator::SetShakeFactor( Float factor )
{
	shaking = factor;
}
void DyngSimulator::SetGravityFactor( Float factor )
{
	gravity = factor;
}

void DyngSimulator::DebugDraw( const Matrix& l2w, CRenderFrame* frame )
{
	const Uint32 numn = globalTransforms.Size();
	if( drawlinks )
	{
		for( Uint32 i=0;i<numn;++i )
		{
			frame->AddDebugSphere( positions[i], 0.002f, Matrix::IDENTITY, Color(255,255,255));
		}
		const Uint32 numLinks = nodeA.Size();
		for( Uint32 i=0;i<numLinks;++i )
		{
			Color c = Color::RED;
			if( types[i]==1 ){ c = Color::GREEN; }
			if( types[i]==2 ){ c = Color::BLUE; }
			frame->AddDebugLine( l2w.TransformPoint(positions[nodeA[i]] ), l2w.TransformPoint( positions[nodeB[i]] ), c);
		}
	}
	if( drawcolls )
	{
		for( Uint32 i=0;i<numn;++i )
		{
			const Matrix& glo = Matrix::Mul( globalTransforms[i], offsets[i] );
			const Vector& p = glo.GetTranslationRef();
			Matrix rot = glo;
			rot.SetTranslation( Vector::ZERO_3D_POINT );
			frame->AddDebugSphere( p, distances[i], rot, Color(255,255,255) );
		}
	}
	if( velocities.Size() > 0.f )
	{
		Float avgVel = 0.f;
		for( Vector v : velocities )
		{
			avgVel += v.Mag3();
		}
		avgVel /= velocities.Size();
		Vector pos = l2w.GetTranslation();
		pos.Z -= 0.5f;
		frame->AddDebugText( pos, String::Printf( TXT("%.3f"), avgVel, 0, 0, true, Color::RED ) );
	}
}


#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",on)
#endif
