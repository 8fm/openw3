// InputMgr.h
// Copyright (C) 2010 Audiokinetic Inc 
/// \file 
/// Contains the declaration of the Metro Input Manager class.
/// This class handles the relaying of input to the Menu System.

#pragma once

#include <vector>
#include "UniversalInput.h"
#include <Xinput.h>

/// Metro-Specific Input Manager class that handles all input devices and relays input
/// data to the Menu system.
class InputMgr
{
public:

	/// InputMgr class constructor
	InputMgr();

	/// InputMgr class destructor
	~InputMgr();

	/// Initializes the Input Manager object.
	/// \return True if the Input Manager has been properly initialized and false otherwise.
	bool Init( 
		void* in_pParam,						///< - Extra parameter, in this case for the application's main window handle
		AkOSChar* in_szErrorBuffer,				///< - Buffer where error details will be written (if the function returns false)
		unsigned int in_unErrorBufferCharCount	///< - Number of characters available in in_szErrorBuffer, including terminating NULL character
		);

	/// Accessor method for the UniversalInput object.
	/// \return A pointer to the UniversalInput object being used by the Input Manager.
	UniversalInput* UniversalInputAdapter() const;

	/// Updates the input from all devices and relays the new input states to the Menu system.
	/// \remark This method should be called every frame before the Menu system's Update call.
	void Update();

	/// Ends the Input Manager class and releases all used resources.
	void Release();

private:

	/// Retrieves input state from an input device then translates it to a Button State 
	/// usable by the Universal Input class and sets it.
	void TranslateInput( 
		void * in_pad,
		int in_iPlayerIndex					///< - 0-Based Player Index of the device to be polled

		);

	/// Pointer to the Universal Input object to interface with the Menu System
	/// \sa UniversalInput
	UniversalInput* m_pUInput;

	/// The maximum number of controllers
	static const int MAX_INPUT = XUSER_MAX_COUNT;
};