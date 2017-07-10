#ifndef WIN32_SNEK_H

struct win32_window_dimensions
{
	uint32 Width;
	uint32 Height;
};

struct win32_screen_buffer
{
	uint32 Width;
	uint32 Height;
	uint32 Pitch;
	uint32 BytesPerPixel;
	void *BitmapMemory;
	BITMAPINFO BitmapInfo;
};

#define WIN32_SNEK_H
#endif  // WIN32_SNEK_H
