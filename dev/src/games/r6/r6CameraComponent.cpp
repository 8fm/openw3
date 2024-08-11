#include "build.h"
#include "r6CameraComponent.h"


IMPLEMENT_ENGINE_CLASS( CR6CameraComponent );

CR6CameraComponent::CR6CameraComponent()
	: m_blendInTime( 0.0f )
	, m_isDefault( false )
{
}
