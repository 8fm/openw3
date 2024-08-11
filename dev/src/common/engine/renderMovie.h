/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Movie grabber
class IRenderMovie : public IRenderObject
{
public:
	//! Get movie width
	virtual Uint32 GetWidth() const=0;

	//! Get movie height
	virtual Uint32 GetHeight() const=0;

	//! Get file name
	virtual String GetFileName() const=0;
};