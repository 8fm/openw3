
#include "build.h"
#include "cameraOrientedComponent.h"
#include "world.h"
#include "entity.h"
#include "layer.h"
#include "tickManager.h"
#include "renderFragment.h"

IMPLEMENT_ENGINE_CLASS( CCameraOrientedComponent );

void CCameraOrientedComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
	world->GetTickManager()->AddToGroup( this, TICK_Main );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Behavior );
}

void CCameraOrientedComponent::OnDetached( CWorld* world )
{
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Behavior );
	world->GetTickManager()->Remove( this );
	TBaseClass::OnDetached( world );
}

void CCameraOrientedComponent::OnTick( Float timeDelta )
{
	TBaseClass::OnTick( timeDelta );
	Vector target(0.0f,0.0f,0.0f,1.0f);
	if( GetEntity() && GetEntity()->GetLayer() && GetEntity()->GetLayer()->GetWorld() )
	{
		target = GetEntity()->GetLayer()->GetWorld()->GetCameraPosition();
		target.Z = 0.0f;
	}
	Matrix l2w = GetLocalToWorld();
	Vector pos = l2w.GetTranslation();
	pos.Z = 0.0f;
	Vector dir = target - pos;
	dir.Normalize3();
	Matrix loc = GetRotation().ToMatrix();
	loc.SetTranslation( GetPosition() );
	loc.Invert();
	Matrix partm = Matrix::Mul( l2w, loc );
	partm.Invert();
	Vector row2 = Vector::Cross( dir, Vector(0.0f,0.0f,1.0f,0.0f) );
	row2.Normalize3();
	Matrix mat;
	dir.W = 0.0f;
	row2.W = 0.0f;
	pos.W = 1.0f;
	mat.SetRow( 0, dir );
	mat.SetRow( 1, row2 );
	mat.SetRow( 2, Vector(0.0f,0.0f,1.0f,0.0f) );
	mat.SetRow( 3, pos );
	Matrix fin = Matrix::Mul( partm, mat );
	this->SetRotation( fin.ToEulerAngles() );
}
/*
void CCameraOrientedComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	TBaseClass::OnGenerateEditorFragments( frame, flag );
}
*/