#ifndef SNEK_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

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

#define GAME_GRIDSIZE 10

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

#define SNEK_H
#endif  // SNEK_H
