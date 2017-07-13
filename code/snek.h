#ifndef SNEK_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;
typedef int32_t int32;
typedef int16_t int16;
typedef int8_t int8;
typedef float real32;
typedef double real64;
typedef uint32_t bool32;

#define true 1
#define false 0

#define ArrayCount(Array) (sizeof(Array) / (sizeof((Array)[0])))

#define Assert(Condition) if(!(Condition)) { *(uint32 *)0 = 0; }

#define Kilobytes(Value) (Value * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)

union v2
{
	struct
	{
		real32 X, Y;
	};
	real32 E[2];
};

inline union v2
V2(real32 A, real32 B)
{
	union v2 Result = {};
	Result.X = A;
	Result.Y = B;

	return(Result);
}

union v3
{
	struct
	{
		real32 X, Y, Z;
	};
	struct
	{
		real32 R, G, B;
	};
	real32 E[3];
};

inline union v3
V3(real32 A, real32 B, real32 C)
{
	union v3 Result = {};
	Result.X = A;
	Result.Y = B;
	Result.Z = C;

	return(Result);
}

struct game_screen_buffer
{
	void *BitmapMemory;
	uint32 Width;
	uint32 Height;
	uint32 Pitch;
	uint32 BytesPerPixel;
};

struct memory_arena
{
	void *Base;
	uint32 Used;
	uint32 Size;
};

#define PushStruct(Arena, Type) (Type *)_PushOnArena(Arena, sizeof(Type))
#define PushArray(Arena, Type, Count) (Type *)_PushOnArena(Arena, (sizeof(Type) * (Count)))

void *
_PushOnArena(struct memory_arena *Arena, uint32 Size)
{
	Assert(Arena->Used + Size < Arena->Size);

	void *Result = (void *)((uint8 *)Arena->Base + Arena->Used);
	Arena->Used += Arena->Used + Size;
	Arena->Base = (void *)((uint8 *)Arena->Base + Arena->Used);

	return(Result);
}

struct snake_part
{
	v2 Position;
	v2 Direction;
};

struct keyboard_input
{
	bool32 IsDown;
};

union game_input
{
	struct
	{
		struct keyboard_input KeyUp;
		struct keyboard_input KeyDown;
		struct keyboard_input KeyLeft;
		struct keyboard_input KeyRight;
	};
	struct keyboard_input Keys[4];
};

struct game_state
{
	bool32 IsInitialized;
	uint32 GridSize;

	v2 Food;

	struct snake_part *Snake;
	uint32 SnakeMaxSize;
	uint32 SnakeSize;
	bool32 Alive;

	struct memory_arena WorldArena;
	union game_input Input;
};

#define SNEK_H
#endif  // SNEK_H
