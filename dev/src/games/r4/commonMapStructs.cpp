#include "build.h"

#include "commonMapStructs.h"
#if !defined( NO_SECOND_SCREEN )
	#include "../../common/platformCommon/secondScreenManager.h"
#endif

IMPLEMENT_ENGINE_CLASS( SCommonMapPinInstance )

class CCommonMapManager;

Bool SCommonMapPinInstance::IsValid()
{
	for ( Uint32 i = 0; i < m_entities.Size(); i++ )
	{
		const CEntity* entity = m_entities[ i ].Get();
		if ( !entity )
		{
			return false;
		}
	}
	return true;
}

Bool SCommonMapPinInstance::GetEntityPosition( Vector& pos ) const
{
	Vector centerPosition( Vector::ZEROS );
	Uint32 count = 0;

	for ( Uint32 i = 0; i < m_entities.Size(); i++ )
	{
		const CEntity* entity = m_entities[ i ].Get();
		if ( entity )
		{
			centerPosition += entity->GetWorldPositionRef();
			count++;
		}
	}
	if ( count > 0 )
	{
		pos = centerPosition.Div4( ( Float )count );
		return true;
	}
	return false;
}

#ifndef NO_SECOND_SCREEN

void SCommonMapPinInstance::AddToSecondScreen()
{
	SSecondScreenMapPin questMapPin;
	questMapPin.m_id			= m_id;
	questMapPin.m_tag			= m_tag;
	questMapPin.m_type			= m_type;
	questMapPin.m_position		= m_position;
	questMapPin.m_isDiscovered  = m_isDiscovered;
	SCSecondScreenManager::GetInstance().SendActualAreaDynamicMapPin( questMapPin );
	m_usedBySecondScreen = true;
}

void SCommonMapPinInstance::UpdateOnSecondScreen()
{	
	if( m_usedBySecondScreen )
	{
		SSecondScreenMapPin questMapPin;
		questMapPin.m_id			= m_id;
		questMapPin.m_tag			= m_tag;
		questMapPin.m_type			= m_type;
		questMapPin.m_position		= m_position;
		questMapPin.m_isDiscovered  = m_isDiscovered;
		SCSecondScreenManager::GetInstance().SendUpdateActualAreaDynamicMapPin( questMapPin );
	}
}

void SCommonMapPinInstance::MoveOnSecondScreen()
{
	if( m_usedBySecondScreen )
	{
		SCSecondScreenManager::GetInstance().SendMoveActualAreaDynamicMapPin( m_id, m_type, m_position );
	}
}

#endif //! NO_SECOND_SCREEN