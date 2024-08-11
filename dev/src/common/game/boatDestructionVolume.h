#pragma once

//////////////////////////////////////////////////////////////////////////

class CBoatDestructionComponent;

//////////////////////////////////////////////////////////////////////////

struct SBoatDestructionVolume
{
    DECLARE_RTTI_STRUCT( SBoatDestructionVolume )

    static const Float DEFAULT_HEALTH;

    SBoatDestructionVolume();
    Bool IsLocalPointInVolume( const Vector& point ) const;

    Vector              m_volumeCorners;
    Vector              m_volumeLocalPosition;
    Float               m_areaHealth;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

BEGIN_CLASS_RTTI( SBoatDestructionVolume );
    PROPERTY_EDIT( m_areaHealth, TXT("This area hit points left") );
    PROPERTY_EDIT( m_volumeCorners, TXT("Volume size") );
    PROPERTY_EDIT( m_volumeLocalPosition, TXT("Volume local position") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
