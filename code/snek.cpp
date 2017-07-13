static void
ClearScreenToColor(struct game_screen_buffer *Buffer, v3 Background)
{
	uint32 BackgroundColor = ( (0xff << 24) |
							   ((int32)Background.R << 16) |
							   ((int32)Background.G << 8) |
							   ((int32)Background.B << 0) );

	uint8 *Row = (uint8 *)Buffer->BitmapMemory;
	for(uint32 Y = 0; Y < Buffer->Height; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for(uint32 X = 0; X < Buffer->Width; ++X)
		{
			*Pixel++ = BackgroundColor;
		}
		Row += Buffer->Pitch;
	}
}

static void
DrawGridOverlay(struct game_screen_buffer *Buffer, uint32 GridSize, v3 Color)
{
	uint32 GridColor = ( (0xff << 24) |
						 ((int32)Color.R << 16) |
						 ((int32)Color.G << 8) |
						 ((int32)Color.B << 0) );

	uint8 *Row = (uint8 *)Buffer->BitmapMemory;
	for(uint32 Y = 0; Y < Buffer->Height; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for(uint32 X = 0; X < Buffer->Width; ++X)
		{
			if((X % GridSize == 0) ||
			   (Y % GridSize == 0))
			{
				*Pixel = GridColor;
			}
			Pixel++;
		}
		Row += Buffer->Pitch;
	}
}

static void
DrawRectangle(struct game_screen_buffer *Buffer, uint32 XPosition, uint32 YPosition,
			  uint32 Width, uint32 Height, v3 Color)
{
	uint32 MinX = XPosition;
	if(MinX < 0) { MinX = 0; }

	uint32 MinY = YPosition;
	if(MinY < 0) { MinY = 0; }

	uint32 MaxX = XPosition + Width;
	if(MaxX > Buffer->Width) { MaxX = Buffer->Width; }

	uint32 MaxY = YPosition + Height;
	if(MaxY > Buffer->Height) { MaxY = Buffer->Height; }

	uint32 FillColor = ( (0xff << 24) |
						 ((int32)Color.R << 16) |
						 ((int32)Color.G << 8) |
						 ((int32)Color.B << 0) );

	uint8 *Base = (uint8 *)Buffer->BitmapMemory;
	for(uint32 Y = MinY; Y < MaxY; ++Y)
	{
		uint32 *Pixel = (uint32 *)(Base + (Y * Buffer->Pitch) + (MinX * Buffer->BytesPerPixel));
		for(uint32 X = MinX; X < MaxX; ++X)
		{
			*Pixel++ = FillColor;
		}
	}
}

static void
PlaceFood(struct game_state *GameState, uint32 Width, uint32 Height)
{
	GameState->Food.X = rand() % Width;
	GameState->Food.Y = rand() % Height;
}

void
GameUpdateAndRender(struct game_state *GameState, struct game_screen_buffer *Buffer)
{
	uint32 GridWidth = (Buffer->Width / GameState->GridSize);
	uint32 GridHeight = (Buffer->Height / GameState->GridSize);

	if(!GameState->IsInitialized)
	{
		GameState->IsInitialized = true;
		//srand(time(0));
		srand(5);

		GameState->SnakeMaxSize = GridWidth * GridHeight;
		GameState->Snake = PushArray(&GameState->WorldArena, struct snake_part, GameState->SnakeMaxSize);

		GameState->Alive = true;
		GameState->Snake->Position.X = GridWidth / 2;
		GameState->Snake->Position.Y = GridHeight / 2;
		++GameState->SnakeSize;

		PlaceFood(GameState, GridWidth, GridHeight);
	}

	if(GameState->Alive)
	{
		for(uint32 SnakePartIndex = GameState->SnakeSize;
			SnakePartIndex > 0;
			--SnakePartIndex)
		{
			(GameState->Snake + SnakePartIndex)->Position = (GameState->Snake + (SnakePartIndex - 1))->Position;
		}

		struct snake_part *SnakeHead = GameState->Snake;
		if((GameState->Input.KeyUp.IsDown) &&
		(SnakeHead->Direction.Y != 1))
		{
			SnakeHead->Direction.X = 0;
			SnakeHead->Direction.Y = -1;
		}
		else if((GameState->Input.KeyDown.IsDown) &&
				(SnakeHead->Direction.Y != -1))
		{
			SnakeHead->Direction.X = 0;
			SnakeHead->Direction.Y = 1;
		}
		else if((GameState->Input.KeyLeft.IsDown) &&
				(SnakeHead->Direction.X != 1))
		{
			SnakeHead->Direction.X = -1;
			SnakeHead->Direction.Y = 0;
		}
		else if((GameState->Input.KeyRight.IsDown) &&
				(SnakeHead->Direction.X != -1))
		{
			SnakeHead->Direction.X = 1;
			SnakeHead->Direction.Y = 0;
		}

		SnakeHead->Position.X += SnakeHead->Direction.X;
		SnakeHead->Position.Y += SnakeHead->Direction.Y;

		{
			if((SnakeHead->Position.X == GameState->Food.X) &&
			(SnakeHead->Position.Y == GameState->Food.Y))
			{
				PlaceFood(GameState, GridWidth, GridHeight);
				Assert(GameState->SnakeSize + 1 < GameState->SnakeMaxSize);

				if(GameState->SnakeSize + 1 < GameState->SnakeMaxSize)
				{
					++GameState->SnakeSize;
				}
			}

			for(uint32 SnakePartIndex = 1;
				SnakePartIndex < GameState->SnakeSize;
				++SnakePartIndex)
			{
				struct snake_part *BodySegment = GameState->Snake + SnakePartIndex;
				if((SnakeHead->Position.X == BodySegment->Position.X) &&
				(SnakeHead->Position.Y == BodySegment->Position.Y))
				{
					// TODO(rick): End game here
					GameState->Alive = false;
					SnakeHead->Direction.X = 0;
					SnakeHead->Direction.Y = 0;
				}
			}
		}
	}

	ClearScreenToColor(Buffer, V3(31, 31, 31));
	DrawRectangle(Buffer, GameState->Food.X * GameState->GridSize,
				  GameState->Food.Y * GameState->GridSize,
				  GameState->GridSize, GameState->GridSize, V3(255, 50, 50));
	
	for(uint32 SnakePartIndex = 0;
		SnakePartIndex < GameState->SnakeSize;
		++SnakePartIndex)
	{
		v3 Color = V3(0, 170, 0);
		if(SnakePartIndex == 0)
		{
			Color = V3(20, 200, 20);
		}

		struct snake_part *BodySegment = GameState->Snake + SnakePartIndex;
		DrawRectangle(Buffer, BodySegment->Position.X * GameState->GridSize,
					  BodySegment->Position.Y * GameState->GridSize,
					  GameState->GridSize, GameState->GridSize, Color);
	}
	DrawGridOverlay(Buffer, GameState->GridSize, V3(18, 18, 18));
}
