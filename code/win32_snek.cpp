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

static struct memory_arena
Win32InitializeMemoryArena(uint32 Size)
{
	struct memory_arena Result = {};

	Result.Base = (void *)VirtualAlloc(0, Size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	Result.Used = 0;
	Result.Size = Size;

	return(Result);
}

static void
Win32ProcessKeyboardInput(struct keyboard_input *Key, bool32 IsDown)
{
	//	TODO(rick): Figure this out??? it's always down
//	if(Key->IsDown != IsDown)
	{
		Key->IsDown = IsDown;
	}
}

static void
Win32DrawText(struct game_screen_buffer *Buffer, char *Text, int32 X, int32 Y, text_anchor AnchorPoint, v3 Color)
{
	static HDC TextDeviceContext = 0;
	if(!TextDeviceContext)
	{
		uint32 TextBitmapWidth = 512;
		uint32 TextBitmapHeight = 32;

		TextDeviceContext = CreateCompatibleDC(0);
		HBITMAP TextBitmap = CreateCompatibleBitmap(TextDeviceContext, TextBitmapWidth, TextBitmapHeight);
		Assert(TextBitmap != NULL);
		SelectObject(TextDeviceContext, TextBitmap);
	}

	uint32 TextLength = strlen(Text);

	SIZE TextSize = {};
	GetTextExtentPoint32(TextDeviceContext, Text, TextLength, &TextSize);
	
	SetBkColor(TextDeviceContext, RGB(0, 0, 0));
	SetTextColor(TextDeviceContext, RGB(255, 255, 255));
	TextOut(TextDeviceContext, 0, 0, Text, TextLength);

	uint32 DestColor = ( (0xff << 24) |
						 ((int32)Color.R << 16) |
						 ((int32)Color.G << 8) |
						 ((int32)Color.B << 0) );
	if(AnchorPoint == TextAnchor_Center)
	{
		X = X - (TextSize.cx / 2);
		Y = Y - (TextSize.cy / 2);
	}
	else if(AnchorPoint == TextAnchor_Right)
	{
		X = X - TextSize.cx;
	}

	if(X < 0) { X = 0; }
	if(X + TextSize.cx > Buffer->Width) { X = X - TextSize.cx; }
	if(Y < 0) { Y = 0; }
	if(Y + TextSize.cy > Buffer->Height) { Y = Y - TextSize.cy; }

	uint8 *Base = (uint8 *)Buffer->BitmapMemory;
	for(uint32 SrcY = 0;
		SrcY < TextSize.cy;
		++SrcY)
	{
		uint32 *Pixel = (uint32 *)(Base + ((Y + SrcY) * Buffer->Pitch) + (X * Buffer->BytesPerPixel));
		for(uint32 SrcX = 0;
			SrcX < TextSize.cx;
			++SrcX)
		{
			COLORREF SourcePixel = GetPixel(TextDeviceContext, SrcX, SrcY);
			if(SourcePixel != 0)
			{
				*Pixel = DestColor;
			}
			++Pixel;
		}
	}
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
ProcessPendingMessages(union game_input *Input)
{
	MSG Message;
	while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
	{
		switch(Message.message)
		{
			case WM_KEYDOWN:
			case WM_KEYUP:
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			{
				uint32 VKCode = (uint32)Message.wParam;
				bool32 WasDown = ((Message.lParam & (1 << 30)) != 0);
				bool32 IsDown = ((Message.lParam & (1 << 31)) == 0);
				if(WasDown != IsDown)
				{
					if((VKCode == 'W') ||
					   (VKCode == VK_UP))
					{
						Win32ProcessKeyboardInput(&Input->KeyUp, IsDown);
					}
					else if((VKCode == 'A') ||
							(VKCode == VK_LEFT))
					{
						Win32ProcessKeyboardInput(&Input->KeyLeft, IsDown);
					}
					else if((VKCode == 'S') ||
							(VKCode == VK_DOWN))
					{
						Win32ProcessKeyboardInput(&Input->KeyDown, IsDown);
					}
					else if((VKCode == 'D') ||
							(VKCode == VK_RIGHT))
					{
						Win32ProcessKeyboardInput(&Input->KeyRight, IsDown);
					}

					if(VKCode == VK_SPACE)
					{
						Win32ProcessKeyboardInput(&Input->KeyPause, IsDown);
					}
					if(VKCode == VK_RETURN)
					{
						Win32ProcessKeyboardInput(&Input->KeyAction, IsDown);
					}
				}
			} break;
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

	struct game_state GameState = {};
	GameState.WorldArena = Win32InitializeMemoryArena(Kilobytes(70));
	GameState.GridSize = 10;
	GameState.PlatformDrawText = Win32DrawText;
	GameState.CurrentScene = GameScene_MainMenu;

	GlobalRunning = 1;
	while(GlobalRunning)
	{
		ProcessPendingMessages(&GameState.Input);

		HDC WindowDC = GetDC(Window);
		struct win32_window_dimensions WindowDims = Win32GetWindowDimensions(Window);

		game_screen_buffer GameScreenBuffer = {};
		GameScreenBuffer.BitmapMemory = GlobalBackBuffer.BitmapMemory;
		GameScreenBuffer.Width = GlobalBackBuffer.Width;
		GameScreenBuffer.Height = GlobalBackBuffer.Height;
		GameScreenBuffer.Pitch = GlobalBackBuffer.Pitch;
		GameScreenBuffer.BytesPerPixel = GlobalBackBuffer.BytesPerPixel;
		GameUpdateAndRender(&GameState, &GameScreenBuffer);

		Win32DrawBufferToScreen(&GlobalBackBuffer, WindowDC, WindowDims.Width, WindowDims.Height);

		Sleep(33);
	}

	return(0);
}

