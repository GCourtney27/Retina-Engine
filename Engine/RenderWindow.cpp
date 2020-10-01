#include "WindowContainer.h"

bool RenderWindow::Initialize(WindowContainer * pWindowContainer, HINSTANCE hInstance, std::string window_title, std::string window_class, int width, int height)
{
	this->hInstance = hInstance;
	this->width = width;
	this->height = height;
	this->window_title = window_title;
	this->window_title_wide = StringHelper::StringToWide(this->window_title);
	this->window_class = window_class;
	this->window_class_wide = StringHelper::StringToWide(this->window_class);

	this->RegisterWindowClass();

	int centerScreenX = GetSystemMetrics(SM_CXSCREEN) / 2 - this->width / 2;
	int centerScreenY = GetSystemMetrics(SM_CYSCREEN) / 2 - this->height / 2;

	RECT wr; // Window Rectangle
	wr.left = centerScreenX;
	wr.top = centerScreenY;
	wr.right = wr.left + this->width;
	wr.bottom = wr.top + this->height;

	AdjustWindowRect(&wr, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU, FALSE);

	// Create window instance
	this->handle = CreateWindowEx(WS_EX_ACCEPTFILES, // Extended Windows style - we are using the default
		this->window_class_wide.c_str(), // Window class name
		this->window_title_wide.c_str(), // Window title
		WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_SIZEBOX, // Windows style
		wr.left, // Window X position
		wr.top, // Window Y position
		wr.right - wr.left, // Window width
		wr.bottom - wr.top, // Window Height
		NULL, // Handle to parent of this window, Since this is the first window, it has no parent Window
		NULL, // Handle to menu or child window identifier. Can be et to NULL and use menu in WindowClassEX
		this->hInstance, // Handle to the instance of module to be used with this window
		pWindowContainer); // Parameter to create window

	// Check if window was created succefully
	if (this->handle == NULL)
	{
		ErrorLogger::Log(GetLastError(), "CreateWindowEx Failed for window: " + this->window_title);
		return false;
	}

	// Bring the window up on the screen and set it as main Focus
	ShowWindow(this->handle, SW_SHOW);
	SetForegroundWindow(this->handle);
	SetFocus(this->handle);

	return true;
}

LRESULT CALLBACK HandleMsgRedirect(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		// All other messages
	case WM_CLOSE:
		DestroyWindow(hwnd);
		return 0;
	default:
		{
			// Retrieve ptr to window class
		WindowContainer * const pWindow = reinterpret_cast<WindowContainer*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		// Foreward message to window class handler
		return pWindow->WindowProc(hwnd, uMsg, wParam, lParam);
		}
	}
}

LRESULT CALLBACK HandleMessageSetup(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NCCREATE:
	{
		const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
		WindowContainer * pWindow = reinterpret_cast<WindowContainer*>(pCreate->lpCreateParams);
		if (pWindow == nullptr)
		{
			ErrorLogger::Log("Critical Error: Pointer to window container is null durring WM_NCCREATE.");
			exit(-1);
		}
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
		SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HandleMsgRedirect));
		return pWindow->WindowProc(hwnd, uMsg, wParam, lParam);
	}
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

void RenderWindow::RegisterWindowClass()
{
	WNDCLASSEX wc; // Our Window Class
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // Flags [Redraw on width/height change from resize/movement]
	wc.lpfnWndProc = HandleMessageSetup; // Pointer to Window Proc function for handling messages from this window
	wc.cbClsExtra = 0; // # of extra bytes toallocate following the window-class structure
	wc.cbWndExtra = 0; // # of extra bytes toallocate following the window instance
	wc.hInstance = this->hInstance; // Handle to instance that contains the Window Proccedure
	wc.hIcon = NULL; // Handle to the class icon. Must be handle to an icon resource
	wc.hIconSm = NULL; // handle to small icon for this class
	wc.hCursor = LoadCursor(NULL, IDC_ARROW); // Default Cursor = If we leave this null we have to explicetly set the cursor
	wc.hbrBackground = NULL; // Handle to the class background brush to the window's background color
	wc.lpszMenuName = NULL; // Pointer to a null terminated charater string for the menu
	wc.lpszClassName = this->window_class_wide.c_str(); // Pointer to null terminated string of our class name
	wc.cbSize = sizeof(WNDCLASSEX); // Need to fill in the size of our struct for cbSize
	RegisterClassEx(&wc); // Register the class so that it is usable
}

bool RenderWindow::ProccessMessages()
{
	// Handle Windows messages
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG)); // Initialize the message structure

	while (PeekMessage(&msg,  // Where to store message (ifone exists)
		this->handle, //Handle to window we are checking messages for
		0, // Minimum Filter Msg Value - We are not filterinf for specific messages but min and max could be used to do so
		0, // Maximum Filter Msg Value
		PM_REMOVE)) // Remove mesage after captureing it via PeekMessage
	{
		TranslateMessage(&msg);  // Translate message from virtual key message into character messages
		DispatchMessage(&msg); // Dispatch message to our Window Proc for this window
	}

	// Check if the window was closed
	if (msg.message == WM_NULL)
	{
		if (!IsWindow(this->handle))
		{
			this->handle = NULL; // Message proccessing loop takes care of destroying this window
			UnregisterClass(this->window_class_wide.c_str(), this->hInstance);
			return false;
		}
	}
	//assert(_CrtCheckMemory()); // Makse sure the heap isn's corrupted on exit
	return true;
}

HWND RenderWindow::GetHWND() const
{
	return this->handle;
}

RenderWindow::~RenderWindow()
{
	if (this->handle != NULL)
	{
		UnregisterClass(this->window_class_wide.c_str(), this->hInstance);
		DestroyWindow(handle);
	}
}

