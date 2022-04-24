#include <windows.h>
#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <stdint.h>
#include <stdbool.h>

#define BOARD_W 23
#define BOARD_H 15

#define TILE_W 25
#define TILE_H 25

// Pixel positions of the game board in the browser
#define IMG_X 917
#define IMG_Y 150
#define IMG_W (BOARD_W * TILE_W)
#define IMG_H (BOARD_H * TILE_H)

// Pixel positons of the reset and play buttons
#define RESET_POS 1389,556
#define PLAY_POS 1198,343

// Structure to hold the data for playing the game
typedef struct {
    uint8_t tile;
    uint8_t value;
    uint8_t D[4];
} Tile;

// Do a single left mouse click
void ClickAt(int x, int y) {
    SetCursorPos(x, y);

    INPUT Inputs[2] = {0};

    Inputs[0].type = INPUT_MOUSE;
    Inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

    Inputs[1].type = INPUT_MOUSE;
    Inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

    SendInput(3, Inputs, sizeof(INPUT));
}

// Do a single mouse click on the game board
void ClickBoard(int x, int y) {
    ClickAt(
        IMG_X + TILE_W / 2 + TILE_W * x,
        IMG_Y + TILE_H / 2 + TILE_H * y
    );
}

// Check if a tile is the background color
bool IsWhite(uint32_t colour) {
    uint8_t* p = (uint8_t*)&colour;
    return p[0] >= 0xE0 && p[1] >= 0xE0 &&p[2] >= 0xE0;
}

// Check if two tiles are the same colour
bool ColourMatch(uint32_t colour0, uint32_t colour1) {
    uint8_t* p0 = (uint8_t*)&colour0;
    uint8_t* p1 = (uint8_t*)&colour1;
    int diff = 0;
    for (int i = 0; i < 4; i++) {
        int d = (int)p0[i] - (int)p1[i];
        diff += d < 0 ? -d : d;
    }
    return diff < 16;
}

// Update the tiles that would be removed if the current one is clicked
void UpdateLeftTile(Tile* TILES, Tile* tile, int x, int y) {
    int t = 1;

    while (
        x - t >= 0 &&
        TILES[(x - t) + y * BOARD_W].tile == 0x00
    ) {
        t = t + 1;
    }

    if (x - t >= 0) {
        tile->D[0] = t;
    } else {
        tile->D[0] = 0;
    }
}
void UpdateUpTile(Tile* TILES, Tile* tile, int x, int y) {
    int t = 1;
    while (
        y - t >= 0 &&
        TILES[x + (y - t) * BOARD_W].tile == 0x00
    ) {
        t = t + 1;
    }

    if (y - t >= 0) {
        tile->D[1] = t;
    } else {
        tile->D[1] = 0;
    }
}
void UpdateRightTile(Tile* TILES, Tile* tile, int x, int y) {
    int t = 1;
    while (
        x + t < BOARD_W &&
        TILES[(x + t) + y * BOARD_W].tile == 0x00
    ) {
        t = t + 1;
    }

    if (x + t < BOARD_W) {
        tile->D[2] = t;
    } else {
        tile->D[2] = 0;
    }
}
void UpdateDownTile(Tile* TILES, Tile* tile, int x, int y) {
    int t = 1;
    while (
        y + t < BOARD_H &&
        TILES[x + (y + t) * BOARD_W].tile == 0x00
    ) {
        t = t + 1;
    }

    if (y + t < BOARD_H) {
        tile->D[3] = t;
    } else {
        tile->D[3] = 0;
    }
}

// Update all the values inside a tile
void UpdateTile(Tile* TILES, Tile* tile, int x, int y) {
    if (tile->tile == 0x00) {
        UpdateLeftTile(TILES, tile, x, y);
        UpdateUpTile(TILES, tile, x, y);
        UpdateRightTile(TILES, tile, x, y);
        UpdateDownTile(TILES, tile, x, y);
    } else {
        tile->D[0] = 0;
        tile->D[1] = 0;
        tile->D[2] = 0;
        tile->D[3] = 0;
    }

    uint8_t C[4];

    C[0] = TILES[(x - tile->D[0]) + y * BOARD_W].tile;
    C[1] = TILES[x + (y - tile->D[1]) * BOARD_W].tile;
    C[2] = TILES[(x + tile->D[2]) + y * BOARD_W].tile;
    C[3] = TILES[x + (y + tile->D[3]) * BOARD_W].tile;

    for (int i = 0; i < 4; i++) {
        bool isValid = false;
        for (int j = 0; j < 4; j++) {
            if (
                i != j &&
                C[i] == C[j] &&
                C[i] != 0x00
            ) {
                isValid = true;
                break;
            }
        }
        if (!isValid) {
            tile->D[i] = 0;
        }
    }

    // if (tile->tile == 0x00) {
    //     printf("(%d, %d): [%d %d %d %d] [%02X %02X %02X %02X]\n",
    //         x, y,
    //         tile->D[0], tile->D[1],
    //         tile->D[2], tile->D[3],
    //         C[0], C[1], C[2], C[3]
    //     );
    // }

    tile->value = 0;
    for (int i = 0; i < 4; i++) {
        tile->value += tile->D[i] != 0 ? 1 : 0;
    }
}

// Update all the tiles in the map
void UpdateTiles(Tile* TILES) {
    for (int y = 0; y < BOARD_H; y++) {
        for (int x = 0; x < BOARD_W; x++) {
            Tile* tile = &TILES[x + y * BOARD_W];
            UpdateTile(TILES, tile, x, y);
        }
    }
}

// static variable to hold the score of the current board
static int score = 0;

// execute a single move randomly
bool DoNextMove(Tile* TILES, bool doClicks) {
    UpdateTiles(TILES);

    uint16_t num_moves = 0;
    static uint16_t MOVES[BOARD_W * BOARD_H];

    for (uint16_t i = 0; i < BOARD_W * BOARD_H; i++) {
        if (TILES[i].value > 0) {
            MOVES[num_moves++] = i;
        }
    }

    int mi = num_moves > 0 ? MOVES[rand() % num_moves] : -1;

    if (mi >= 0) {
        int x = mi % BOARD_W;
        int y = mi / BOARD_W;

        if (doClicks) {
            ClickBoard(x, y);
        }

        Tile* tile = &TILES[mi];
        TILES[(x - tile->D[0]) + y * BOARD_W].tile = 0x00;
        TILES[x + (y - tile->D[1]) * BOARD_W].tile = 0x00;
        TILES[(x + tile->D[2]) + y * BOARD_W].tile = 0x00;
        TILES[x + (y + tile->D[3]) * BOARD_W].tile = 0x00;

        for (int i = 0; i < 4; i++) {
            if (tile->D[i] != 0) {
                score = score + 1;
            }
        }

        return true;
    }

    return false;
}

// Do a single playthrough of the game
void PlayGame(uint8_t* BOARD, bool doClicks) {
    score = 0;

    static Tile TILES[BOARD_W * BOARD_H];

    for (uint16_t i = 0; i < BOARD_W * BOARD_H; i++) {
        TILES[i].tile = BOARD[i];
    }

    int count = 0;
    while (DoNextMove(TILES, doClicks) && count < 100) {
        if (doClicks) {
            Sleep(100);
        }
        count = count + 1;
    }

    printf("Score: %d\n", score);
}

// Take a screenshot of the board and then play through it
void RunVictory(bool doClicks) {
    HWND hwnd = GetDesktopWindow();
    HDC hdc[3] = { NULL, NULL };
    HBITMAP hBmp[2] = { NULL, NULL};

    hwnd = GetDesktopWindow();
    hdc[0] = GetWindowDC(hwnd);

    hBmp[0] = CreateCompatibleBitmap(hdc[0], IMG_W, IMG_H);
    hdc[1] = CreateCompatibleDC(hdc[0]);
    SelectObject(hdc[1], hBmp[0]);

    hBmp[1] = CreateCompatibleBitmap(hdc[1], BOARD_W, BOARD_H);
    hdc[2] = CreateCompatibleDC(hdc[1]);
    SelectObject(hdc[2], hBmp[1]);

    BitBlt(
        hdc[1], 0, 0, IMG_W, IMG_H,
        hdc[0], IMG_X, IMG_Y,
        SRCCOPY
    );

    SetStretchBltMode(hdc[1], COLORONCOLOR);
    StretchBlt(
        hdc[2], 0, 0, BOARD_W, BOARD_H,
        hdc[1], TILE_W / 2, TILE_H / 2, IMG_W, IMG_H,
        SRCCOPY
    );

    static uint32_t BMP_BOARD[BOARD_W * BOARD_H];

    LONG len = GetBitmapBits(
        hBmp[1], sizeof(BMP_BOARD),
        BMP_BOARD
    );

    uint8_t num_colours = 0;
    static uint32_t COLOURS[256];
    static uint8_t BOARD[BOARD_W * BOARD_H];

    for (uint16_t y = 0; y < BOARD_H; y++) {
        for (uint16_t x = 0; x < BOARD_W; x++) {
            uint8_t val = 0;
            uint32_t col = BMP_BOARD[x + y * BOARD_W];

            if (!IsWhite(col)) {
                for (uint8_t i = 0; i < num_colours; i++) {
                    if (ColourMatch(col, COLOURS[i])) {
                        val = i + 1; break;
                    }
                }

                if (val == 0) {
                    COLOURS[num_colours++] = col;
                    val = num_colours;
                }
            }

            BOARD[x + y * BOARD_W] = val;
        }
    }

    PlayGame(BOARD, doClicks);

    // ReleaseDC(0, hdc[0]);
    ReleaseDC(0, hdc[1]);
    ReleaseDC(0, hdc[2]);
    DeleteObject(hBmp[0]);
    DeleteObject(hBmp[1]);
}

// Main function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    srand((unsigned int)time(NULL));

    while (score < 190) {
        ClickAt(RESET_POS);
        Sleep(1000);
        ClickAt(PLAY_POS);
        Sleep(1000);

        printf("Starting...\n");

        RunVictory(true);
    }

    return 0;
}