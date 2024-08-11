
#include "build.h"
#include "dyngConstraint.h"
#include "dyngResource.h"
#include "skeleton.h"
#include "environmentManager.h"
#include "baseEngine.h"

#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CAnimationSyncToken_Dyng );
IMPLEMENT_ENGINE_CLASS( CAnimDangleConstraint_Dyng );

CAnimDangleConstraint_Dyng::CAnimDangleConstraint_Dyng()
	: m_syncToken( nullptr )
	, m_cachedAllBones( false )
	, m_dyng( nullptr )
	, m_dampening( 0.95f )
	, m_gravity( 1.f )
	, m_speed( 1.f )
	, m_forceReset( false )
	, m_forceRelaxedState( false )
	, m_planeCollision( false )
	, m_useOffsets( false )
	, m_max_links_iterations( 10 )
	, m_wind( 0.0f )
	, m_dt( 0.02f )
	, m_was_cashed( false )
{
	
}

CAnimDangleConstraint_Dyng::~CAnimDangleConstraint_Dyng()
{
	if ( m_syncToken )
	{
		m_syncToken->Reset();
		
		ASSERT( !m_syncToken );
	}
}

const CSkeleton* CAnimDangleConstraint_Dyng::GetSkeleton() const
{
	const CSkeleton* s = TBaseClass::GetSkeleton();
	return s ? s : (m_dyng? m_dyng->m_dyngSkeleton.Get() : nullptr);
}      

void CAnimDangleConstraint_Dyng::SetResource( CDyngResource* res )
{
	m_dyng = res;
}

Bool CAnimDangleConstraint_Dyng::HasCachedBones() const
{
	return m_cachedAllBones;
}

void CAnimDangleConstraint_Dyng::CacheBones( const CComponent* parent, const CSkeleton* skeleton )
{
	m_skeletonModelSpace.Clear();
	m_skeletonWorldSpace.Clear();
	m_dirtyBones.Clear();

	m_cachedAllBones = false;
	m_was_cashed = false;

	if ( parent && m_dyng && m_dyng->m_dyngSkeleton )
	{
		const Int32 numNodes = m_dyng->m_nodeNames.Size();
		const Int32 numLinks = m_dyng->m_linkTypes.Size();
		const Int32 numShapes = m_dyng->m_collisionTransforms.Size();
		const Int32 numTriangles = m_dyng->m_triangleAs.Size();
		Int32 i;

		for( i=0;i<numNodes;++i )
		{
			RED_ASSERT( m_dyng->m_nodeTransforms[i].IsOk() , TXT("Dyng Resource Invalid: %s"), m_dyng->GetFile()->GetFileName().AsChar() );
			Int32 modelBone = FindBoneByName( ( m_dyng->m_nodeNames[i].AsChar()) );
			m_simulator.indices.PushBack( modelBone );
			m_simulator.distances.PushBack( m_dyng->m_nodeDistances[i] );
			m_simulator.radiuses.PushBack( 0.3f );
			m_simulator.masses.PushBack( m_dyng->m_nodeMasses[i] );
			m_simulator.stifnesses.PushBack( m_dyng->m_nodeStifnesses[i] );
			m_simulator.positions.PushBack( m_dyng->m_nodeTransforms[i].GetRow(3) );
			m_simulator.velocities.PushBack( Vector::ZEROS );
			m_simulator.globalTransforms.PushBack( m_dyng->m_nodeTransforms[i] );
			m_simulator.lookats.PushBack( -1 );
			m_simulator.shakness.PushBack( 0.0f );
			m_simulator.offsets.PushBack( Matrix::IDENTITY );
		}
		for( i=0;i<numLinks;++i )
		{
			m_simulator.lengths.PushBack( m_dyng->m_linkLengths[i] );
			m_simulator.types.PushBack( m_dyng->m_linkTypes[i] );
			m_simulator.nodeA.PushBack( m_dyng->m_linkAs[i] );
			m_simulator.nodeB.PushBack( m_dyng->m_linkBs[i] );
			m_simulator.weight.PushBack( 0.0f );
		}

		for( i=0;i<numShapes;++i )
		{
			m_simulator.offsets[i]  = m_dyng->m_collisionTransforms[i];
			m_simulator.radiuses[i] = ( m_dyng->m_collisionHeights[i] );
			m_simulator.shakness[i] = ( m_dyng->m_collisionRadiuses[i] );
		}
		for( i=0;i<numTriangles;++i )
		{
			Int32 indA = m_dyng->m_triangleAs[i];
			Int32 indB = m_dyng->m_triangleBs[i];
			Int32 indC = m_dyng->m_triangleCs[i];
			if( indA>=0 && indA<numNodes )
			{
				if( indB>=0 && indB<numNodes )
				{
					m_simulator.lookats[indA] = indB;
				}
			}
		}

		Int32* numchildren = new Int32[ numNodes ];
		for( i=0;i<numNodes;++i ){ numchildren[i] = 0; }
		for( i=0;i<numNodes;++i )
		{
			Int32 par = -1;
			for( Int32 j=0;j<numNodes;++j )
			{
				if( m_dyng->m_nodeNames[j] == m_dyng->m_nodeParents[i] )
				{
					par = j;
					break;
				}
			}

			if( par>=0 )
			{
				if( numchildren[par]==0 )
				{
					if( m_simulator.radiuses[i]>0.0f && m_simulator.lookats[i]==-1 )
					{
						m_simulator.lookats[par] = i;
					}
					numchildren[par]++;
				}
				else
				{
					m_simulator.lookats[par] = -1;
					numchildren[par]++;
				}
			}
		}

		m_simulator.drawcolls = m_drawcolls;
		m_simulator.drawlinks = m_drawlinks;
		m_simulator.drawlimits = m_drawlimits;
		m_simulator.dampening = m_dampening;
		m_simulator.gravity   = m_gravity;
		m_simulator.PostLoad();

		if ( GetSkeleton() )
		{
			m_skeletonModelSpace.Resize( GetSkeleton()->GetBonesNum() );
			m_skeletonWorldSpace.Resize( GetSkeleton()->GetBonesNum() );
			m_dirtyBones.Reserve( GetSkeleton()->GetBonesNum() );
			m_cachedAllBones = true;
			m_was_cashed = true;
		}
		else
		{
			m_skeletonModelSpace.Clear();
			m_skeletonWorldSpace.Clear();
			m_dirtyBones.Clear();
		}
		delete [] numchildren;
	}
}

void CAnimDangleConstraint_Dyng::RecreateSkeleton()
{
	if ( m_dyng )
	{
		m_dyng->RecreateSkeleton();
	}
}

#ifndef NO_EDITOR
void CAnimDangleConstraint_Dyng::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("dyng") )
	{
		RecreateSkeleton();
		CacheBones( GetParentComponent(), GetSkeleton() );
	}
	
	m_simulator.drawcolls = m_drawcolls;
	m_simulator.drawlinks = m_drawlinks;
	m_simulator.drawlimits = m_drawlimits;
	m_simulator.dampening = m_dampening;
	m_simulator.gravity   = m_gravity;
	m_simulator.shaking   = m_shake;
	m_simulator.max_link_iterations = m_max_links_iterations;
	m_simulator.planeCollision = m_planeCollision;
	m_simulator.useoffsets = m_useOffsets;
}
#endif

void CAnimDangleConstraint_Dyng::OnShowDebugRender( Bool flag )
{
	m_simulator.drawlinks = flag;
}

#ifndef NO_EDITOR_FRAGMENTS
void CAnimDangleConstraint_Dyng::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	TBaseClass::OnGenerateEditorFragments( frame, flags );

	if ( m_cachedAllBones )
	{
		const CComponent* c = GetParentComponent();
		if ( !c )
		{
			c = GetComponent();
		}

		if ( c )
		{
			const Matrix& l2w = c->GetLocalToWorld();
			m_simulator.DebugDraw( l2w, frame );
		}
	}
}
#endif //NO_EDITOR_FRAGMENTS

void CAnimDangleConstraint_Dyng::OnParentUpdatedAttachedAnimatedObjectsLS( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	TBaseClass::OnParentUpdatedAttachedAnimatedObjectsLS( dt, poseLS, bones );
	m_dt = dt > 0.02f ? 0.02f : dt;
}

void CAnimDangleConstraint_Dyng::OnParentUpdatedAttachedAnimatedObjectsLSAsync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	TBaseClass::OnParentUpdatedAttachedAnimatedObjectsLSAsync( dt, poseLS, bones );
	m_dt = dt > 0.02f ? 0.02f : dt;
}

void CAnimDangleConstraint_Dyng::OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones )
{
#ifdef DEBUG_ANIM_CONSTRAINTS
	CTimeCounter timer;
#endif
	TBaseClass::OnParentUpdatedAttachedAnimatedObjectsWS( parent, poseLS, poseMS, poseWS, bones );

	if( !m_dyng || !m_dyng->m_dyngSkeleton )
	{
		return;
	}

	// This is a simple version - we check only base skeleton
	if ( m_forceReset || m_forceRelaxedState )
	{
		if ( const CAnimatedComponent* ac = CAnimatedComponent::GetRootAnimatedComponentFromAttachment( GetComponent() ) )
		{
			if ( const CTeleportDetector* detector = ac->GetTeleportDetector() )
			{
#ifndef NO_EDITOR
				RED_ASSERT( detector->GetLastTick() == GEngine->GetCurrentEngineTick() );
#endif
				const Bool poseChanged = detector ? detector->DoesPoseChanged() : false;
				const Bool pelvisChangedMS = detector ? detector->DoesPelvisChangedMS() : false;

				///////////////////////////////////
				//
				//	So we have 4 cases when teleport request appears
				//
				//	1. req: poseChanged=0 && pelvisChanged=0 => do nothing
				//	2. req: poseChanged=1 && pelvisChanged=0 => reset relax
				//	3. req: poseChanged=1 && pelvisChanged=1 => reset relax 
				//	4. req: poseChanged=0 && pelvisChanged=1 => do nothing
				//
				///////////////////////////////////

				if ( poseChanged || pelvisChangedMS )
				{
					// case 2 and 3 doesnt matter if pelvis changed
					m_forceRelaxedState = true;
					m_forceReset = true;
				}
				else
				{
					// case 1 and case 4 do nothing
					// otherwise please clear flags
					m_forceRelaxedState = false;
					m_forceReset = false;
				}
			}
		}
	}
	
	if ( !HasCachedBones() )
	{
		CacheBones( GetParentComponent(), GetSkeleton() );
	}

	const Float dt = m_dt;

	if ( m_cachedAllBones && HasSkeletalMatrices() )
	{
		if ( parent )
		{
			StartUpdateWithBoneAlignmentFull( poseMS, poseWS, bones );
		}
		else
		{
			StartUpdateWithBoneAlignment( poseMS, poseWS, bones );
		}
		const Int32 numNodes = Min< Int32 >( (Int32)GetNumBonesModelSpace(), m_simulator.globalTransforms.SizeInt() );
		{
			if( m_was_cashed )
			{
				for( Int32 i=0; i<numNodes; ++i )
				{
					m_simulator.globalTransforms[i] = GetBoneModelSpace( i );
					m_simulator.positions[i] = m_simulator.globalTransforms[i].GetTranslationRef();
					m_simulator.velocities[i] = Vector::ZEROS;
				}
			}
			m_was_cashed = false;
		}

		if( dt > 0.0001f && m_speed > 0.0001f )
		{
			m_simulator.dt = dt*m_speed;
			m_simulator.planeCollision = m_planeCollision;
			

			// first all bones should be calculated to MS
			if ( m_cachedAllBones && poseMS )
			{
				const CComponent* dyngCmp = GetComponent();
				Matrix loc = dyngCmp->GetLocalToWorld();
				loc.SetTranslation( 0,0,0 );
				loc.Invert();
				
				Vector wind = Vector::ZEROS;
				Matrix globalPos = dyngCmp->GetLocalToWorld();
				if( numNodes>0 )
				{
					if( const CWorld* world = dyngCmp->GetWorld() )
					{
						//wind = world->GetEnvironmentManager()->GetCurrentWindVector( globalPos.TransformPoint( m_simulator.globalTransforms[0].GetTranslation() ) );
						wind = world->GetWindAtPointForVisuals( globalPos.TransformPoint( m_simulator.globalTransforms[0].GetTranslation() ), true );
						wind = loc.TransformPoint( wind );
					}
					for( Int32 i=0; i<numNodes; ++i )
					{
						m_simulator.globalTransforms[i] = GetBoneModelSpace(i);
						RED_ASSERT(  m_simulator.globalTransforms[i].IsOk() , TXT("Dyng transform invalid pre: %s"), m_dyng->GetFile()->GetFileName().AsChar() );
						if( m_wind>0.0001f )
						{
							Float r = Float(rand())/Float(RAND_MAX);
							Float rr = r*r * 0.3f;
							m_simulator.velocities[i] += wind * ( rr>0.5f ? 0.5f : rr ) * m_wind;// + Vector(rr,rr,rr);
						}
					}
				}

				if ( m_forceReset )
				{
					m_simulator.ForceReset();
				}
				m_simulator.relaxing = m_forceRelaxedState;

				const Uint32 numIt = m_forceRelaxedState ? 30 : 1;

				// alwways should be called !
				for ( Uint32 i=0; i<numIt; ++i )
				{
					m_simulator.Evaluate( m_useOffsets, i==numIt-1 );
				}
			}

			m_simulator.relaxing = false;

			m_dirtyBones.Reserve( numNodes );

			for( Int32 i=0;i<numNodes;++i )
			{
				RED_ASSERT(  m_simulator.globalTransforms[i].IsOk() , TXT("Dyng transform invalid post: %s"), m_dyng->GetFile()->GetFileName().AsChar() );
				SetBoneModelSpace( i, mixTransforms( GetBoneModelSpace(i), m_simulator.globalTransforms[i], m_simulator.weightdist ) ); //m_blend
			}
			EndUpdate( poseMS, poseWS, bones );
		}
	}

	// clear reset flags
	m_forceReset = false;
	m_forceRelaxedState = false;

#ifdef DEBUG_ANIM_CONSTRAINTS
	const Float timeElapsed = timer.GetTimePeriod();
	if ( timeElapsed > 0.0005f )
	{
		GScreenLog->PerfWarning( timeElapsed, TXT("ANIM DANGLES"), TXT("Slow anim dangle update CAnimDangleConstraint_Dyng for agent '%ls'"), GetComponent()->GetEntity()->GetFriendlyName().AsChar() );
	}

	//BEH_LOG( TXT("CAnimDangleConstraint_Dyng: %1.5f"), timeElapsed );
#endif
}

Matrix CAnimDangleConstraint_Dyng::mixTransforms( const Matrix & a, const Matrix & b, Float w )
{
	if( w<0.000001f )
	{
		return a;
	}
	if( w>0.999999f )
	{
		return b;
	}
	Matrix out;

	Vector ta = a.GetTranslation();
	Vector tb = b.GetTranslation();
	Vector qa = a.ToQuat();
	Vector qb = b.ToQuat();

	RedQsTransform aa( RedVector4(ta.X,ta.Y,ta.Z,ta.W), RedQuaternion(qa.X,qa.Y,qa.Z,qa.W) );
	RedQsTransform bb( RedVector4(tb.X,tb.Y,tb.Z,tb.W), RedQuaternion(qb.X,qb.Y,qb.Z,qb.W) );

	RedQsTransform cc;
	cc.Lerp(aa,bb,w);

	out.BuildFromQuaternion( Vector( cc.Rotation.Quat.X, cc.Rotation.Quat.Y, cc.Rotation.Quat.Z, -cc.Rotation.Quat.W ) );
	out.SetTranslation( Vector( cc.Translation.X, cc.Translation.Y, cc.Translation.Z, cc.Translation.W ) );

	return out;
}

void CAnimDangleConstraint_Dyng::SetBlendToAnimationWeight( Float w )
{
	m_simulator.SetWeight( 1.f-w );
}

void CAnimDangleConstraint_Dyng::ForceReset()
{
	m_forceReset = true;
}

void CAnimDangleConstraint_Dyng::ForceResetWithRelaxedState()
{
	m_forceReset = true;
	m_forceRelaxedState = true;
}

void CAnimDangleConstraint_Dyng::Bind( CAnimationSyncToken_Dyng* syncToken )
{
	if ( syncToken )
	{
		ASSERT( !m_syncToken );
	}

	m_syncToken = syncToken;
}

void CAnimDangleConstraint_Dyng::SetAnimation( const CName& animationName, const CSyncInfo& syncInfo, Float weight )
{
	m_animation.Reset();

	if ( animationName )
	{
		m_animation.m_animationName = animationName;
		m_animation.m_syncInfo = syncInfo;
		m_animation.m_weight = weight;
	}
}

void CAnimDangleConstraint_Dyng::SetShakeFactor( Float factor )
{
	m_simulator.SetShakeFactor( factor );
}
void CAnimDangleConstraint_Dyng::SetGravityFactor( Float factor )
{
	m_simulator.SetGravityFactor( factor );
}

#ifndef NO_EDITOR
Bool CAnimDangleConstraint_Dyng::PrintDebugComment( String& str ) const
{
	Bool ret = TBaseClass::PrintDebugComment( str );

	const CDyngResource* dyng = m_dyng.Get();
	if ( !dyng )
	{
		str += TXT("m_dyng is null; ");
		ret = true;
	}

	if ( !m_cachedAllBones )
	{
		str += TXT("can not cache all bones; ");
		ret = true;
	}

	return ret;
}

#endif

//////////////////////////////////////////////////////////////////////////

void CAnimationSyncToken_Dyng::Bind( CAnimDangleConstraint_Dyng* owner )
{
	m_owner = owner;
}

void CAnimationSyncToken_Dyng::Sync( CName animationName, const CSyncInfo& syncInfo, Float weight )
{
	ASSERT( m_owner );
	if ( m_owner )
	{
		m_owner->SetAnimation( animationName, syncInfo, weight );
	}
}

void CAnimationSyncToken_Dyng::Reset()
{
	if ( m_owner )
	{
		m_owner->Bind( nullptr );
	}
	m_owner = nullptr;
}

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",on)
#endif
