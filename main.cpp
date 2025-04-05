#define _USE_MATH_DEFINES
using namespace std;
#include<math.h>
#include<stdio.h>
#include<string.h>
#include <cstdlib>
#include <fstream>
#include <ctime>
#include <Windows.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH    640
#define SCREEN_HEIGHT   480
#define SPEED                   1.5
#define FRIENDLY_SPEED  0.5
#define ENEMY_SPEED     0.1
#define ACC                             0.3
#define BRK                             0.2
#define FULLSCREEN      0

SDL_Surface* screen;
SDL_Surface* charset;
SDL_Texture* scrtex;
SDL_Window* window;
SDL_Renderer* renderer;


const int road[3][2] = { {-95, 95}, {-75, 75}, {-60, 60} };

SDL_Rect road1, road2;


struct car_t {
    int x;
    int y;
    bool alive = true;
    double speed;
    SDL_Rect hitbox;
    SDL_Surface* bmp;
    double distance;
};

struct SaveState {
    double distance;
    double points;
    double worldTime;
    int cur_map;
    int prev_map;
    int poz;
    int lives;
    double friendly_distance;
    int friendly_x;
    double enemy_distance;
    int enemy_x;
} save;


struct Colors {
    int black;
    int green;
    int red;
    int blue;
    int white;
    int purple;
} color;

bool SetSDL() {
    int rc;
    if (FULLSCREEN) {
        rc = SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP,
            &window, &renderer);
    }
    else {
        rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0,
            &window, &renderer);
    }

    if (rc != 0) {
        printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
        return false;
    };
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    SDL_SetWindowTitle(window, "SpyHunter");

    screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH, SCREEN_HEIGHT);

    return true;
}

bool SetBMP(SDL_Surface*& map1, SDL_Surface*& map2, SDL_Surface*& map3, SDL_Surface*& charset, SDL_Surface*& player_car, SDL_Surface*& friendly_car,
    SDL_Surface*& enemy_car, SDL_Surface*& pause_scr, SDL_Surface*& start_scr) {

    charset = SDL_LoadBMP("assets/cs8x8.bmp");
    pause_scr = SDL_LoadBMP("assets/pause.bmp");
    start_scr = SDL_LoadBMP("assets/start.bmp");
    player_car = SDL_LoadBMP("assets/car.bmp");
    friendly_car = SDL_LoadBMP("assets/friendly_car.bmp");
    enemy_car = SDL_LoadBMP("assets/enemy_car.bmp");
    map1 = SDL_LoadBMP("assets/map1.bmp");
    map2 = SDL_LoadBMP("assets/map2.bmp");
    map3 = SDL_LoadBMP("assets/map3.bmp");

    if (charset == NULL || pause_scr == NULL || start_scr == NULL || player_car == NULL || friendly_car == NULL || enemy_car == NULL
        || map1 == NULL || map2 == NULL || map3 == NULL) {

        return false;
    };

    return true;
}

void FreeBMP(SDL_Surface*& map1, SDL_Surface*& map2, SDL_Surface*& map3, SDL_Surface*& charset, SDL_Surface*& player_car, SDL_Surface*& friendly_car,
    SDL_Surface*& enemy_car, SDL_Surface*& pause_scr, SDL_Surface*& start_scr) {
    if (charset != NULL) SDL_FreeSurface(charset);
    if (pause_scr != NULL) SDL_FreeSurface(pause_scr);
    if (start_scr != NULL) SDL_FreeSurface(start_scr);
    if (player_car != NULL) SDL_FreeSurface(player_car);
    if (friendly_car != NULL) SDL_FreeSurface(friendly_car);
    if (enemy_car != NULL) SDL_FreeSurface(enemy_car);
    if (map1 != NULL) SDL_FreeSurface(map1);
    if (map2 != NULL) SDL_FreeSurface(map2);
    if (map3 != NULL) SDL_FreeSurface(map3);
}

void SDL_Destroy() {
    SDL_FreeSurface(charset);
    SDL_FreeSurface(screen);
    SDL_DestroyTexture(scrtex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
}

void SetColors() {
    color.black = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
    color.green = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
    color.red = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
    color.blue = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);
    color.white = SDL_MapRGB(screen->format, 255, 255, 255);
    color.purple = SDL_MapRGB(screen->format, 102, 102, 255);
}
// narysowanie napisu txt na powierzchni screen, zaczynaj¹c od punktu (x, y)
// charset to bitmapa 128x128 zawieraj¹ca znaki
// draw a text txt on surface screen, starting from the point (x, y)
// charset is a 128x128 bitmap containing character images
void DrawString(SDL_Surface* screen, int x, int y, const char* text,
    SDL_Surface* charset) {
    int px, py, c;
    SDL_Rect s, d;
    s.w = 8;
    s.h = 8;
    d.w = 8;
    d.h = 8;
    while (*text) {
        c = *text & 255;
        px = (c % 16) * 8;
        py = (c / 16) * 8;
        s.x = px;
        s.y = py;
        d.x = x;
        d.y = y;
        SDL_BlitSurface(charset, &s, screen, &d);
        x += 8;
        text++;
    };
};


// narysowanie na ekranie screen powierzchni sprite w punkcie (x, y)
// (x, y) to punkt œrodka obrazka sprite na ekranie
// draw a surface sprite on a surface screen in point (x, y)
// (x, y) is the center of sprite on screen
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
    SDL_Rect dest;
    dest.x = x - sprite->w / 2;
    dest.y = y - sprite->h / 2;
    dest.w = sprite->w;
    dest.h = sprite->h;
    SDL_BlitSurface(sprite, NULL, screen, &dest);
};


// rysowanie pojedynczego pixela
// draw a single pixel
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
    int bpp = surface->format->BytesPerPixel;
    Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
    *(Uint32*)p = color;
};


// rysowanie linii o d³ugoœci l w pionie (gdy dx = 0, dy = 1) 
// b¹dŸ poziomie (gdy dx = 1, dy = 0)
// draw a vertical (when dx = 0, dy = 1) or horizontal (when dx = 1, dy = 0) line
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color) {
    for (int i = 0; i < l; i++) {
        DrawPixel(screen, x, y, color);
        x += dx;
        y += dy;
    };
};


// rysowanie prostok¹ta o d³ugoœci boków l i k
// draw a rectangle of size l by k
void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k,
    Uint32 outlineColor, Uint32 fillColor) {
    int i;
    DrawLine(screen, x, y, k, 0, 1, outlineColor);
    DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
    DrawLine(screen, x, y, l, 1, 0, outlineColor);
    DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
    for (i = y + 1; i < y + k - 1; i++)
        DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};

void DrawMenu(double points, double worldTime, int lives, char text[128]) {
    // tekst informacyjny / info text
    DrawRectangle(screen, -1, -1, SCREEN_WIDTH + 2, 36, color.white, color.purple);
    //            "template for the second project, elapsed time = %.1lf s  %.0lf frames / s"
    sprintf(text, "%.0lf  % .1lf s  %dx", points, worldTime, lives);
    DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, charset);
    //            "Esc - exit, \030 - faster, \031 - slower"
    sprintf(text, "Esc - wyjscie, \030 - przyspieszenie, \031 - zwolnienie");
    DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 26, text, charset);

    //wykonane podpunkty
    //DrawRectangle(screen, SCREEN_WIDTH - 181, SCREEN_HEIGHT - 14, 180, 12, color.white, color.purple);
    //sprintf(text, "g h i j l m");
    //DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2 + 230, 468, text, charset);
}

void DrawRoad(SDL_Surface* prev_map, SDL_Surface* cur_map, car_t player) {
    DrawSurface(screen, prev_map,
        SCREEN_WIDTH / 2, player.distance * SCREEN_HEIGHT + 480);

    DrawSurface(screen, cur_map,
        SCREEN_WIDTH / 2, (player.distance - 2.0) * SCREEN_HEIGHT);

    DrawSurface(screen, player.bmp,
        player.x + SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
}

void DrawCar(car_t car) {
    DrawSurface(screen, car.bmp,
        car.x + SCREEN_WIDTH / 2, (car.distance) * SCREEN_HEIGHT);
}
bool SaveGame() {
    // Get current time
    time_t now = time(0);
    tm* lt = localtime(&now);

    // Create a file name using current time
    char* fileName = new char[256];
    sprintf(fileName, "saves/%04d-%02d-%02d-%02d-%02d-%02d.bin",
        lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
        lt->tm_hour, lt->tm_min, lt->tm_sec);

    // Open the file
    ofstream file(fileName, ios::out | ios::binary);
    if (!file.is_open()) {
        printf("Failed to save game: %s\n", fileName);
        delete[] fileName;
        return false;
    }

    // Write game state to the file
    file.write((char*)&save, sizeof(SaveState));
    file.close();

    printf("Game saved: %s\n", fileName);
    delete[] fileName;
    return true;
}

bool LoadGame(char* fileName) {
    // Open the file
    ifstream file(fileName, ios::binary);
    if (!file.is_open()) {
        printf("Failed to load game: %s\n", fileName);
        return false;
    }

    // Read game state from the file
    file.read((char*)&save, sizeof(SaveState));
    file.close();

    printf("Game loaded: %s\n", fileName);
    return true;
}

bool ListSavedGames() {
    OPENFILENAME ofn;
    wchar_t fileName[MAX_PATH] = L"";

    ZeroMemory(&ofn, sizeof(ofn)); //inicjalizacja ofn samymi 0

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"Save Files(*.txt)\0*.bin\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrInitialDir = L"saves\\";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileName(&ofn))
    {
        size_t length = wcslen(fileName);
        char* file_name = new char[length + 1]; // 
        wcstombs(file_name, fileName, length);
        file_name[length] = '\0';
        LoadGame(file_name);
        delete[] file_name;
        return true;
    }
    return false;

}

bool IsInside(const SDL_Rect& outerHitbox, const SDL_Rect& innerHitbox)
{
    return outerHitbox.x <= innerHitbox.x && outerHitbox.y <= innerHitbox.y &&
        outerHitbox.x + outerHitbox.w >= innerHitbox.x + innerHitbox.w &&
        outerHitbox.y + outerHitbox.h >= innerHitbox.y + innerHitbox.h;
}

bool OnRoad(const SDL_Rect& car)
{
    return IsInside(road1, car) || IsInside(road2, car);
}

void RoadCords(const double distance, const int prev_i, const int cur_i) {
    road1.x = SCREEN_WIDTH / 2 - road[prev_i][1];
    road1.y = (distance + 1.0) * SCREEN_HEIGHT + 480 - 960;
    road1.w = 2 * road[prev_i][1];
    road1.h = 1920;

    road2.x = SCREEN_WIDTH / 2 - road[cur_i][1];
    road2.y = (distance - 2.0) * SCREEN_HEIGHT - 960;
    road2.w = road[cur_i][1] * 2;
    road2.h = 1920 - (distance - 2.0) * SCREEN_HEIGHT;
}

bool Collision(car_t& hitter, car_t& hit) { //if hitter is player and collision = true, block points or add points
    int xDistance = abs((hitter.hitbox.x + hitter.hitbox.w / 2) - (hit.hitbox.x + hit.hitbox.w / 2));
    int yDistance = abs((hitter.hitbox.y + hitter.hitbox.h / 2) - (hit.hitbox.y + hit.hitbox.h / 2));
    if (2 * xDistance < yDistance) {
        if (hitter.hitbox.y < hit.hitbox.y) {
            hit.alive = false;
            return true;
        }
        else {
            hitter.alive = false;
            return false;
        }
    }
    else {
        if (hitter.hitbox.x < hit.hitbox.x) {
            hit.x++;
            return false;
        }
        else {
            hit.x--;
            return false;
        }
    }

}

void DetectCollision(car_t& player, car_t& friendly, car_t& enemy, double& distance_p, double& points) {
    if (SDL_HasIntersection(&player.hitbox, &friendly.hitbox)) {
        if (Collision(player, friendly) || !friendly.alive) {
            distance_p = -3;
        };

    }
    if (SDL_HasIntersection(&player.hitbox, &enemy.hitbox)) {
        if (Collision(player, enemy)) {
            points += 300;
        }
    }

    if (SDL_HasIntersection(&enemy.hitbox, &friendly.hitbox)) {
        Collision(enemy, friendly);
    }
}

void EnemyAI(car_t& player, car_t& enemy, car_t& friendly) {
    if (player.hitbox.y - enemy.hitbox.y < 120 && player.hitbox.y - enemy.hitbox.y > 50) {
        if (enemy.hitbox.x > player.hitbox.x) {
            enemy.hitbox.x--;
            enemy.x--;
        }
        else {
            enemy.hitbox.x++;
            enemy.x++;
        }
    }
    else if (enemy.hitbox.y - player.hitbox.y < player.hitbox.h && enemy.hitbox.y - player.hitbox.y > 0) {
        if (enemy.hitbox.x > player.hitbox.x) {
            enemy.hitbox.x--;
            enemy.x--;
            if (SDL_HasIntersection(&player.hitbox, &enemy.hitbox)) {
                player.hitbox.x--;
                player.x--;
            }
        }
        else  if (enemy.hitbox.x < player.hitbox.x) {
            enemy.hitbox.x++;
            enemy.x++;
            if (SDL_HasIntersection(&player.hitbox, &enemy.hitbox)) {
                player.hitbox.x++;
                player.x++;
            }

        }
    }
    else if (enemy.hitbox.y - friendly.hitbox.y < 100) {
        if (enemy.hitbox.x - friendly.hitbox.x < 40 && enemy.hitbox.x > 270) {
            enemy.hitbox.x--;
            enemy.x--;
        }
        else if (friendly.hitbox.x - enemy.hitbox.x < 40 && enemy.hitbox.x < 330) {
            enemy.hitbox.x++;
            enemy.x++;
        }
    }
}

// okno konsoli nie jest widoczne, je¿eli chcemy zobaczyæ
        // komunikaty wypisywane printf-em trzeba w opcjach:
        // project -> szablon2 properties -> Linker -> System -> Subsystem
        // zmieniæ na "Console"
        // 
// main
#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char** argv) {
    car_t player, friendly, enemy;
    bool rightB = false, leftB = false, pause = true, newGame = true, onRoad = true;
    int t1, t2, quit, frames, rc, cur_v, prev_v, lives;
    double delta, worldTime, fpsTimer, fps, distance_p, player_speed, points, c_points, friendly_speed, enemy_speed, safeTimer;
    SDL_Event event;
    SDL_Surface* charset, * player_car, * enemy_car, * friendly_car, * pause_scr, * start_scr;
    SDL_Surface* cur_map, * prev_map, * maps[3];

    srand(time(NULL));

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }
    SetSDL();
    SetColors();
    // wy³¹czenie widocznoœci kursora myszy
    SDL_ShowCursor(SDL_DISABLE);
    // wczytanie obrazka cs8x8.bmp
    if (!SetBMP(maps[0], maps[1], maps[2], charset, player_car, friendly_car, enemy_car, pause_scr, start_scr)) {
        FreeBMP(maps[0], maps[1], maps[2], charset, player_car, friendly_car, enemy_car, pause_scr, start_scr);
        SDL_Destroy();
        return 0;
    }
    SDL_SetColorKey(charset, true, 0x000000);


    SDL_Rect car_hitbox;
    SDL_Rect fr_car_hitbox;


    player.bmp = player_car;
    friendly.bmp = friendly_car;
    enemy.bmp = enemy_car;


    t1 = SDL_GetTicks();

    frames = 0;
    fpsTimer = 0;
    quit = 0;
    worldTime = 0;
    distance_p = 0;
    player.distance = 0;
    friendly.distance = 0;
    enemy.distance = 0;
    player_speed = SPEED;
    player.x = 0;
    friendly.x = (rand() % 81) - 40;
    enemy.x = (rand() % 81) - 40;
    points = 0;
    c_points = 0;
    cur_map = maps[0];
    prev_map = maps[0];
    cur_v = 0;
    prev_v = 0;
    lives = 0;
    friendly_speed = FRIENDLY_SPEED;
    enemy_speed = ENEMY_SPEED;
    safeTimer = 0;

    while (!quit) {

        if (worldTime < 5) {
            enemy.alive = false;
            friendly.alive = false;
        }

        player.hitbox.x = player.x + SCREEN_WIDTH / 2 - player.bmp->w / 2;
        player.hitbox.y = SCREEN_HEIGHT / 2 - player.bmp->h / 2;
        player.hitbox.w = player.bmp->w;
        player.hitbox.h = player.bmp->h;

        if (friendly.alive) {
            friendly.hitbox.x = SCREEN_WIDTH / 2 + friendly.x - friendly.bmp->w / 2;
            friendly.hitbox.y = friendly.distance * SCREEN_HEIGHT - friendly.bmp->h / 2;
            friendly.hitbox.w = friendly.bmp->w;
            friendly.hitbox.h = friendly.bmp->h;
        }
        else {
            friendly.hitbox = { 0,0,0,0 };
        }

        if (enemy.alive) {
            enemy.hitbox.x = SCREEN_WIDTH / 2 + enemy.x - enemy.bmp->w / 2;
            enemy.hitbox.y = enemy.distance * SCREEN_HEIGHT - enemy.bmp->h / 2;
            enemy.hitbox.w = enemy.bmp->w;
            enemy.hitbox.h = enemy.bmp->h;
        }
        else {
            enemy.hitbox = { 0,0,0,0 };
        }
        // w tym momencie t2-t1 to czas w milisekundach,
        // jaki uplyna³ od ostatniego narysowania ekranu
        // delta to ten sam czas w sekundach
        t2 = SDL_GetTicks();
        delta = (t2 - t1) * 0.001;
        t1 = t2;

        if (!pause) {
            worldTime += delta;
            safeTimer += delta;
            distance_p += player_speed * delta;
            player.distance += player_speed * delta;
            friendly.distance += friendly_speed * delta;
            enemy.distance += enemy_speed * delta;
        }
        friendly.y = friendly.distance * SCREEN_HEIGHT - friendly.bmp->h / 2;
        enemy.y = enemy.distance * SCREEN_HEIGHT - enemy.bmp->h / 2;

        if (player.distance > 4) {
            player.distance = 0;
            prev_map = cur_map;
            prev_v = cur_v;
            cur_v = rand() % 3;
            cur_map = maps[cur_v];
        }

        RoadCords(player.distance, prev_v, cur_v);

        player.alive = OnRoad(player.hitbox);
        if (friendly.distance > 0 && friendly.distance < 1500) {
            friendly.alive = OnRoad(friendly.hitbox);
        }
        if (enemy.distance > 0 && enemy.distance < 1500) {
            enemy.alive = OnRoad(enemy.hitbox);
        }

        DetectCollision(player, friendly, enemy, distance_p, points);

        if (!pause) EnemyAI(player, enemy, friendly);

        if (!friendly.alive || friendly.distance > 3) {
            friendly.distance = (-1) * (rand() % 2) - 2;
            friendly.x = (rand() % 81) - 40;
            friendly.alive = true;

        }
        if (!enemy.alive || enemy.distance > 2) {
            enemy.distance = (-1) * (rand() % 2);
            enemy.x = (rand() % 81) - 40;
            enemy.alive = true;
        }


        if (!player.alive) {
            friendly.distance = -3;
            enemy.distance = -3;
            if (safeTimer > 3) {
                lives--;
                safeTimer = 0;
                if (lives < 0) {
                    SDL_Delay(4000);
                    newGame = true;
                }
                else {
                    player.x = 0;
                    SDL_Delay(1000);
                    t1 += 1000;

                }
            }
            else {
                player.x = 0;
                SDL_Delay(1000);
                t1 += 1000;

            }

        }

        if (distance_p > 1) {
            c_points += 50.0;
            points += 50.0;
            distance_p = 0;

        }
        if (c_points >= 1000) {
            lives++;
            c_points -= 1000.0;
        }

        if (newGame) {
            player.distance = 0.0;
            points = 0.0;
            worldTime = 0.0;
            player.x = 0;
            lives = 0;
            c_points = 0;
            pause = true;
            safeTimer = 0;
            friendly.distance = 0;
            enemy.distance = 0;
        }

        SDL_FillRect(screen, NULL, color.black);

        DrawRoad(prev_map, cur_map, player);

        if (friendly.alive) {
            DrawCar(friendly);
        }

        if (enemy.alive) {
            DrawCar(enemy);
        }



        char text[128];
        //DrawMenu(points, worldTime, lives, text);
        DrawRectangle(screen, -1, -1, SCREEN_WIDTH + 2, 36, color.white, color.purple);
        sprintf(text, "%.0lf  % .1lf s  %dx", points, worldTime, lives);
        DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 14, text, charset);

        //wykonane podpunkty
        //DrawRectangle(screen, SCREEN_WIDTH - 181, SCREEN_HEIGHT - 14, 180, 12, color.white, color.purple);
        //sprintf(text, "g h i j l m");
        //DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2 + 230, 468, text, charset);

        if (newGame) {
            DrawSurface(screen, start_scr,
                SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
        }
        else if (pause) {
            DrawSurface(screen, pause_scr,
                SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
        }

        SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
        //              SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, scrtex, NULL, NULL);
        SDL_RenderPresent(renderer);

        // obs³uga zdarzeñ (o ile jakieœ zasz³y) / handling of events (if there were any)
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
                else if (event.key.keysym.sym == SDLK_UP) {
                    player_speed = SPEED + ACC;
                    friendly_speed = FRIENDLY_SPEED + 2 * ACC;
                    enemy_speed = ENEMY_SPEED + 1 * ACC;
                }
                else if (event.key.keysym.sym == SDLK_DOWN)
                {
                    player_speed = SPEED - BRK;
                    friendly_speed = FRIENDLY_SPEED - 2 * BRK;
                    enemy_speed = ENEMY_SPEED - 1 * BRK;
                }
                else if (event.key.keysym.sym == SDLK_p) {
                    pause = !pause;
                    newGame = false;
                }
                else if (event.key.keysym.sym == SDLK_n) newGame = true;
                else if (event.key.keysym.sym == SDLK_s) {
                    save = { player.distance, points, worldTime, cur_v, prev_v,player.x, lives, friendly.distance, friendly.x, enemy.distance, enemy.x };
                    SaveGame();
                }
                else if (event.key.keysym.sym == SDLK_l) {
                    pause = true;
                    if (ListSavedGames()) {
                        points = save.points;
                        player.x = save.poz;
                        worldTime = save.worldTime;
                        player.distance = save.distance;
                        cur_v = save.cur_map;
                        prev_v = save.prev_map;
                        lives = save.lives;
                        cur_map = maps[cur_v];
                        prev_map = maps[prev_v];
                        friendly.distance = save.friendly_distance;
                        friendly.x = save.friendly_x;
                        enemy.distance = save.enemy_distance;
                        enemy.x = save.enemy_x;
                    };

                }
                break;
            case SDL_KEYUP:
                if (event.key.keysym.sym == SDLK_UP) {
                    player_speed -= ACC;
                    friendly_speed -= (2 * ACC);
                    enemy_speed -= (1 * ACC);

                }
                else if (event.key.keysym.sym == SDLK_DOWN) {
                    player_speed += BRK;
                    friendly_speed += (2 * BRK);
                    enemy_speed += (1 * BRK);
                }
                break;
            case SDL_QUIT:
                quit = 1;
                break;
            };
        };
        const uint8_t* keystate = SDL_GetKeyboardState(NULL);
        if (keystate[SDL_SCANCODE_LEFT] && !pause) {
            player.x--;
        }
        if (keystate[SDL_SCANCODE_RIGHT] && !pause) {
            player.x++;
        }

        frames++;

    };

    // zwolnienie powierzchni / freeing all surfaces
    SDL_Destroy();
    return 0;
};