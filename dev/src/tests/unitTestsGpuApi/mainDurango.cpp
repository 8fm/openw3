#include "build.h"

#if defined(RED_PLATFORM_DURANGO)

#include <xdk.h>
#include <wrl.h>
#include "memoryInitializationDurango.h"

static int GExitValue = EXIT_SUCCESS;

// Application - implements the required functionality for an application
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
	void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
	void OnResuming(Platform::Object^ sender, Platform::Object^ args);
	void OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);

private:

	bool m_windowClosed;
};

// ApplicationSource - responsible for creating the Application instance
// and passing it back to the system
ref class ApplicationViewSource : Windows::ApplicationModel::Core::IFrameworkViewSource
{
public:
	virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();
};

using namespace Windows::Foundation;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;

ApplicationView::ApplicationView()
{
	m_windowClosed = false;
}

// Called by the system.  Perform application initialization here,
// hooking application wide events, etc.
void ApplicationView::Initialize(CoreApplicationView^ applicationView)
{
	applicationView->Activated += ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &ApplicationView::OnActivated);
	CoreApplication::Suspending += ref new EventHandler<SuspendingEventArgs^>(this, &ApplicationView::OnSuspending);
	CoreApplication::Resuming += ref new EventHandler<Platform::Object^>(this, &ApplicationView::OnResuming);
}

// Called when we are provided a window.
void ApplicationView::SetWindow(CoreWindow^ window)
{
	window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &ApplicationView::OnWindowClosed);
}

// The purpose of this method is to get the application entry point.
void ApplicationView::Load(Platform::String^ entryPoint)
{
}

// Called by the system after initialization is complete.  This
// implements the traditional game loop
void ApplicationView::Run()
{
	CoreDispatcher^ dispatcher = CoreWindow::GetForCurrentThread()->Dispatcher;

	GExitValue = ::Run();
}

void ApplicationView::Uninitialize()
{
	SShutdownPlatform();
}

// Called when the application is activated.
void ApplicationView::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
	LaunchActivatedEventArgs^ launchArgs = static_cast<LaunchActivatedEventArgs^>(args);
	const UniChar * commandLine = launchArgs->Arguments->Data();

	CommandLineArguments arguments;
	ExtractCommandLineArguments( commandLine, arguments );

	MemoryInitializer::InitializeMemory();
	SInitializePlatform( arguments );

	CoreWindow::GetForCurrentThread()->Activate();
}

// Called when the application is suspending.
void ApplicationView::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
}

// Called when the application is resuming from suspended.
void ApplicationView::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
}

void ApplicationView::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
	m_windowClosed = true;
}

// Implements a IFrameworkView factory.
IFrameworkView^ ApplicationViewSource::CreateView()
{
	return ref new ApplicationView();
}

// Application entry point
[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
	auto applicationViewSource = ref new ApplicationViewSource();

	CoreApplication::Run(applicationViewSource);

	return GExitValue;
}

#endif