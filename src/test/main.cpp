#include "../OISMHandler.h"
#include "../OISMSimpleSerializer.h"

#include <OISException.h>

#include <iostream>
#include <string>



/////////////////////////////////////////////////////////////////
// Everything except main() is taken from the OIS console demo //
/////////////////////////////////////////////////////////////////



#if defined OIS_WIN32_PLATFORM
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#ifdef min
#undef min
#endif
#include "resource.h"
LRESULT DlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
//////////////////////////////////////////////////////////////////////
////////////////////////////////////Needed Linux Headers//////////////
#elif defined OIS_LINUX_PLATFORM
#include <X11/Xlib.h>
#include <X11/Xatom.h>
void checkX11Events();
//////////////////////////////////////////////////////////////////////
////////////////////////////////////Needed Mac Headers//////////////
#elif defined OIS_APPLE_PLATFORM
#include <Carbon/Carbon.h>
void checkMacEvents();
#endif


//-- OS Specific Globals --//
#if defined OIS_WIN32_PLATFORM
  HWND hWnd = 0;
#elif defined OIS_LINUX_PLATFORM
  Display *xDisp = 0;
  Window xWin = 0;
#elif defined OIS_APPLE_PLATFORM
  WindowRef mWin = 0;
#endif

bool g_run = true;


#if defined OIS_WIN32_PLATFORM
LRESULT DlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    return FALSE;
}
#endif


#if defined OIS_LINUX_PLATFORM
//This is just here to show that you still recieve x11 events, as the lib only needs mouse/key events
void checkX11Events()
{
    if(!g_run)
        return;

    XEvent event;

    while(XPending(xDisp) > 0)
    {
        XNextEvent(xDisp, &event);
        //Handle Resize events
        if(event.type == ConfigureNotify)
        {
            //if(g_m)
            //{
                //const MouseState &ms = g_m->getMouseState();
                //ms.width = event.xconfigure.width;
                //ms.height = event.xconfigure.height;
            //}
        }
        else if(event.type == ClientMessage || event.type == DestroyNotify)
        {   // We only get DestroyNotify for child windows. However, we regeistered earlier to receive WM_DELETE_MESSAGEs
            std::cout << "Exiting...\n";
            g_run = false;
            return;
        }
        else
        {
            std::cout << "\nUnknown X Event: " << event.type << std::endl;
        }
    }
}
#endif


#if defined OIS_APPLE_PLATFORM
void checkMacEvents()
{   
    //TODO - Check for window resize events, and then adjust the members of mousestate
    EventRef event = NULL;
    EventTargetRef targetWindow = GetEventDispatcherTarget();
    
    if( ReceiveNextEvent( 0, NULL, kEventDurationNoWait, true, &event ) == noErr )
    {
        SendEventToEventTarget(event, targetWindow);
        std::cout << "Event : " << GetEventKind(event) << "\n";
        ReleaseEvent(event);
    }
}
#endif


unsigned createWindow()
{
#if defined OIS_WIN32_PLATFORM
    //Create a capture window for Input Grabbing
    hWnd = CreateDialog( 0, MAKEINTRESOURCE(IDD_DIALOG1), 0,(DLGPROC)DlgProc);
    if( hWnd == NULL )
        OIS_EXCEPT(E_General, "Failed to create Win32 Window Dialog!");

    ShowWindow(hWnd, SW_SHOW);
    return hWnd;    

#elif defined OIS_LINUX_PLATFORM
    //Connects to default X window
    if( !(xDisp = XOpenDisplay(0)) )
        OIS_EXCEPT(OIS::E_General, "Error opening X!");
    //Create a window
    xWin = XCreateSimpleWindow(xDisp, DefaultRootWindow(xDisp), 0, 0, 100, 100, 0, 0, 0);
    //bind our connection to that window
    XMapWindow(xDisp, xWin);
    // XInternAtom
    //Select what events we want to listen to locally
    XSelectInput(xDisp, xWin, StructureNotifyMask | SubstructureNotifyMask);
    Atom wmProto = XInternAtom(xDisp, "WM_PROTOCOLS", False);
    Atom wmDelete = XInternAtom(xDisp, "WM_DELETE_WINDOW", False);
    XChangeProperty(xDisp, xWin, wmProto, XA_ATOM, 32, 0, (const unsigned char*)&wmDelete, 1);
    XEvent evtent;
    do
    {
        XNextEvent(xDisp, &evtent);
    } while(evtent.type != MapNotify);
    
    return xWin;

#elif defined OIS_APPLE_PLATFORM
    // create the window rect in global coords
    ::Rect windowRect;
    windowRect.left = 0;
    windowRect.top = 0;
    windowRect.right = 300;
    windowRect.bottom = 300;
    
    // set the default attributes for the window
    WindowAttributes windowAttrs = kWindowStandardDocumentAttributes
        | kWindowStandardHandlerAttribute 
        | kWindowInWindowMenuAttribute
        | kWindowHideOnFullScreenAttribute;
    
    // Create the window
    CreateNewWindow(kDocumentWindowClass, windowAttrs, &windowRect, &mWin);
    
    // Color the window background black
    SetThemeWindowBackground (mWin, kThemeBrushBlack, true);
    
    // Set the title of our window
    CFStringRef titleRef = CFStringCreateWithCString( kCFAllocatorDefault, "OIS Input", kCFStringEncodingASCII );
    SetWindowTitleWithCFString( mWin, titleRef );
    
    // Center our window on the screen
    RepositionWindow( mWin, NULL, kWindowCenterOnMainScreen );
    
    // Install the event handler for the window
    InstallStandardEventHandler(GetWindowEventTarget(mWin));
    
    // This will give our window focus, and not lock it to the terminal
    ProcessSerialNumber psn = { 0, kCurrentProcess };
    TransformProcessType( &psn, kProcessTransformToForegroundApplication );
    SetFrontProcess(&psn);
    
    // Display and select our window
    ShowWindow(mWin);
    SelectWindow(mWin);

    std::cout << "WindowRef: " << mWin << " WindowRef as int: " << wnd.str() << "\n";
    return mWin;
#endif
}


void destroyWindow()
{
#if defined OIS_LINUX_PLATFORM
    // Be nice to X and clean up the x window
    XDestroyWindow(xDisp, xWin);
    XCloseDisplay(xDisp);
#endif
}


int main(int argc, char** argv)
{
    auto& input = oism::Handler::getInstance();
    input.init<oism::SimpleSerializer>(createWindow(), "../");

    // Callbacks are disabled when the shared pointer goes out of scope
    auto cbSharedPointer = input.callback("quit", [&](){g_run = false;});
    {
        // Callback is disabled at the end of this scope
        auto cbOutOfScope = input.callback("disabled", [](){std::cout << "this should disabled!" << std::endl;});
    }

    auto walkBinding = input.getBinding("walk");

    while (g_run)
    {
        input.update();
        std::cout << "walk value=" << walkBinding->getValue();
        std::cout << "                      \r" << std::flush;
    }
    
    delete &input;
    destroyWindow();

    std::cout << std::endl << "Terminated normally" << std::endl;
    return 0;
}
