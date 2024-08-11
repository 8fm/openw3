//
// ApplicationView.h
//

#pragma once

#include "pch.h"
#include "Game.h"


// Application - implements the required functionality for a application
ref class ApplicationView sealed : public Windows::ApplicationModel::Core::IFrameworkView
{
public:

    ApplicationView();

    // IFrameworkView Methods
    virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
    virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
    virtual void Load(Platform::String^ entryPoint);
    virtual void Run();
    virtual void Uninitialize();

protected:

    // Event Handlers
    void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);

private:

    Game^ m_game;
};

// ApplicationSource - responsible for creating the Application instance 
// and passing it back to the system
ref class ApplicationViewSource : Windows::ApplicationModel::Core::IFrameworkViewSource
{
public:
    virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();
};
