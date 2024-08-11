#pragma once

// This enum needs to be shared by headers that cannot include each other

// Viewport window mode
enum EViewportWindowMode
{
	VWM_Windowed,		// Regular windowed mode with title, border and such
	VWM_Borderless,		// "Borderless fullscreen windowed" mode, basically a borderless window that covers the screen
	VWM_Fullscreen,		// Classic fullscreen mode
};
