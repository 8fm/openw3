#include "build.h"
#include "soundSettings.h"

Bool SSoundSettings::m_voiceEventInCutscenesFixed = false;
Float SSoundSettings::m_listnerOnPlayerFromCamera = 0.5f;
Float SSoundSettings::m_occlusionDistanceLimiter = 150.0f;
Bool SSoundSettings::m_contactSoundsEnabled = true;
Bool SSoundSettings::m_contactsLogging = false;
Float SSoundSettings::m_contactSoundPerTimeIntervalLimit = 0.08f;
Float SSoundSettings::m_contactSoundsDistanceFromCamera = 40.0f;
Float SSoundSettings::m_contactSoundsMinVelocityLimit = 0.30f;
Float SSoundSettings::m_contactSoundsMaxVelocityClamp = 10000.0f;
Float SSoundSettings::m_occlusionInterpolationSpeed = 1.0f;
