/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleModule.h"
#include "particleEmitter.h"
#include "evaluator.h"
#include "curve.h"

IMPLEMENT_ENGINE_CLASS( IParticleModule );

IParticleModule::IParticleModule()
	: m_isEnabled( true ),
	m_isShowing( false ),
	m_enabledState( true )
{
}

void IParticleModule::SetEnable( Bool flag )
{
	m_isEnabled = flag;
}

void IParticleModule::SetShowing( Bool flag )
{
	m_isShowing = flag;
}

void IParticleModule::SetEnabledState( Bool flag )
{
	m_enabledState = flag;
}

void IParticleModule::SetSelected( Bool selected )
{
	m_isSelected = selected;
}

void IParticleModule::SetEditorColor( const Color& color )
{
	m_editorColor = color;
}

void IParticleModule::SetEditorName( const String& name )
{
	m_editorName = name;
}

static Bool IsEvaluatorProperty( IProperty* property )
{
	IRTTIType* propertyType = property->GetType();
	if ( propertyType->GetType() == RT_Pointer )
	{
		CRTTIPointerType* pointerType = static_cast< CRTTIPointerType* >( propertyType );
		if ( pointerType->GetPointedType() && pointerType->GetPointedType()->GetType() == RT_Class )
		{
			CClass* pointedClass = static_cast< CClass* >( pointerType->GetPointedType() );
			if ( pointedClass->IsA( ClassID< IEvaluator >() ) )
			{
				// It's a pointer to an evaluator !
				return true;
			}
		}
	}

	// Other type
	return false;
}

void IParticleModule::GetCurves( TDynArray< CurveParameter* >& curves )
{
	// Get the object properties
	TDynArray< IProperty* > properties;
	GetClass()->GetProperties( properties );

	// Scan for the evaluator properties with curves
	for ( Uint32 i=0; i<properties.Size(); i++ )
	{
		IProperty* property = properties[i];
		if ( IsEvaluatorProperty( property ) )
		{
			// Get the evaluator
			IEvaluator* evaluator = NULL;
			void* propData = property->GetOffsetPtr( this );
			property->GetType()->Copy( &evaluator, propData );

			// Get curve parameters from evaluator			
			if ( evaluator )
			{
				CurveParameter* curve = evaluator->GetCurves();
				if ( curve )
				{
					// Change the name of the curves to the name of the property
					curve->SetName( property->GetName() );

					// Add the curve to the list of curves
					curves.PushBack( curve );
				}
			}
		}
	}
}
CParticleEmitter* IParticleModule::GetEmitter() const
{
	return FindParent< CParticleEmitter >(); 
}
