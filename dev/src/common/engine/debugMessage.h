#pragma once

enum DebugMessageType
{
	DMT_Info = 0,
	DMT_Warning,
	DMT_Error,
	DMT_User,
};

class CDebugMessage
{
public:

	// Constructor
	CDebugMessage( const String& content = String::EMPTY, DebugMessageType type = DMT_Info );

	// Public getters
	RED_INLINE const String& GetContent() const { return m_content; }
	RED_INLINE DebugMessageType GetType() const { return m_type; }


private:

	// Private members
	String m_content;
	DebugMessageType m_type;

};
