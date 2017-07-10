#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "snek.h"
#include "snek.cpp"

#include "win32_snek.h"

static bool32 GlobalRunning;
static win32_screen_buffer GlobalBackBuffer;

static win32_window_dimensions
Win32GetWindowDimensions(HWND Window)
{
	struct win32_window_dimensions Result = {};

	RECT ClientRect = {};
	GetClientRect(Window, &ClientRect);

	Result.Width = ClientRect.right- ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return(Result);
}

static void
Win32ResizeDIBSection(struct win32_screen_buffer *Buffer, uint32 Width, uint32 Height)
{
	if(Buffer->BitmapMemory)
	{
		VirtualFree(Buffer->BitmapMemory, 0, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Height;
	Buffer->BytesPerPixel = 4;
	Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;

	Buffer->BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	Buffer->BitmapInfo.bmiHeader.biWidth = Buffer->Width;
	Buffer->BitmapInfo.bmiHeader.biHeight = -Buffer->Height;
	Buffer->BitmapInfo.bmiHeader.biPlanes = 1;
	Buffer->BitmapInfo.bmiHeader.biBitCount = 32;
	Buffer->BitmapInfo.bmiHeader.biCompression = BI_RGB;

	uint32 BitmapMemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;
	Buffer->BitmapMemory = (void *)VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

// TODO(rick): Move this into the game layer, it doesn't need to be in the
// platform layer
static void
Win32ClearScreenToGrid(struct win32_screen_buffer *Buffer, v3 Background, v3 Grid, uint32 GridSize)
{
	uint32 BackgroundColor = ( (0xff << 24) |
							   ((int32)Background.R << 16) |
							   ((int32)Background.G << 8) |
							   ((int32)Background.B << 0) );

	uint32 GridColor = ( (0xff << 24) |
						 ((int32)Grid.R << 16) |
						 ((int32)Grid.G << 8) |
						 ((int32)Grid.B << 0) );

	uint8 *Row = (uint8 *)Buffer->BitmapMemory;
	for(uint32 Y = 0; Y < Buffer->Height; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for(uint32 X = 0; X < Buffer->Width; ++X)
		{
			if((X % GridSize == 0) ||
			   (Y % GridSize == 0))
			{
				*Pixel++ = GridColor;
			}
			else
			{
				*Pixel++ = BackgroundColor;
			}
		}
		Row += Buffer->Pitch;
	}
}

static void
Win32DrawBufferToScreen(struct win32_screen_buffer *Buffer, HDC DeviceContext,
						uint32 WindowWidth, uint32 WindowHeight)
{
	StretchDIBits(DeviceContext,
				  0, 0, WindowWidth, WindowHeight,
				  0, 0, Buffer->Width, Buffer->Height,
				  Buffer->BitmapMemory,
				  &Buffer->BitmapInfo,
				  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK
Win32Callback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
	LRESULT Result = 0;

	switch(Message)
	{
		case WM_QUIT:
		case WM_DESTROY:
		{
			GlobalRunning = 0;
			PostQuitMessage(0);
		} break;
		default:
		{
			Result = DefWindowProc(Window, Message, WParam, LParam);
		} break;
	}

	return(Result);
}

static void
ProcessPendingMessages()
{
	MSG Message;
	while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
	{
		switch(Message.message)
		{
			default:
			{
				TranslateMessage(&Message);
				DispatchMessage(&Message);
			} break;
		}
	}
}

int WINAPI
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow)
{
	WNDCLASSEXA WindowClass = {};
	WindowClass.cbSize = sizeof(WNDCLASSEXA);
	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32Callback;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = "win32_snek_game";

	if(!RegisterClassEx(&WindowClass))
	{
		MessageBox(NULL, "Failed to register window class.", "Error", MB_OK);
		return(-1);
	}

	HWND Window;
	Window = CreateWindowEx(0,
							WindowClass.lpszClassName,
							"Snek",
							WS_OVERLAPPEDWINDOW | WS_VISIBLE,
							CW_USEDEFAULT, CW_USEDEFAULT,
							416, 400,
							0, 0, Instance, 0);

	if(!Window)
	{
		MessageBox(NULL, "Failed to create window.", "Error", MB_OK);
		return(-2);
	}

	struct win32_window_dimensions WindowDims = Win32GetWindowDimensions(Window);
	Win32ResizeDIBSection(&GlobalBackBuffer, WindowDims.Width, WindowDims.Height);

	GlobalRunning = 1;
	while(GlobalRunning)
	{
		ProcessPendingMessages();

		HDC WindowDC = GetDC(Window);
		struct win32_window_dimensions WindowDims = Win32GetWindowDimensions(Window);
		Win32ClearScreenToGrid(&GlobalBackBuffer, V3(31, 31, 31), V3(18, 18, 18), GAME_GRIDSIZE);
		Win32DrawBufferToScreen(&GlobalBackBuffer, WindowDC, WindowDims.Width, WindowDims.Height);

		Sleep(1);
	}

	return(0);
}

