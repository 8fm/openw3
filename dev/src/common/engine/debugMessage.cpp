#include "build.h"
#include "debugMessage.h"

CDebugMessage::CDebugMessage(const String &content, DebugMessageType type)
: m_content(content)
, m_type(type)
{
}
