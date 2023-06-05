#include <SDL2/SDL.h>
#include <math.h>
#include <vector>
#include <iostream>

//Convenience
struct rgba { Uint8 r,g,b,a; } ;

//Point structure that acts as a point and doubles as a 2d vector structure
struct Point
{
    double x,y;
    Point operator*(const double& a) const
    {
        return {a*x, a*y};
    }
    Point operator+(const Point& a) const
    {
        return {a.x+x, a.y+y};
    }
};

//Game Class to clean things up a bit
class Game
{
protected:
    SDL_Renderer* renderer = nullptr;
    SDL_Window* window = nullptr;
    const int SCREEN_WIDTH;
    const int SCREEN_HEIGHT;
    void(*eventMethod)(SDL_Event) = nullptr;
public:
    Game(int w, int h, SDL_Window* win, SDL_Renderer* r) : renderer(r), SCREEN_WIDTH(w), SCREEN_HEIGHT(h), window(win) {} 
    //Clears screen with certain color
    void clrScreen(Uint8 r, Uint8 g, Uint8 b, Uint8 a);
    //Takes a function pointer to your gameplay loop
    void gameplayLoop(void(*ptr)(void));
    //Takes a function pointer to your event handler (must accept an SDL_Event)
    void setEventHandler(void(*ptr)(SDL_Event));
    ~Game();
};

/*
Data structure to hold 2d maps
canvas woks as such:
(0,0)x+--------->
y+
|
|
|
ˇ
*/
class Map
{
public:
    Map(std::vector<std::vector<int>> m) : map(m) {}
    int getTileAt(int x, int y) { return map[y][x]; };
    int ySize() { return map[0].size(); };
    int xSize() { return map.size(); };
    ~Map();
private:
    //Could eventually swap int for a Tile class
    std::vector<std::vector<int>> map;
};

//Little object to tidy up raycast return
struct CollisionEvent
{
    Point intersect; //point of intersection
    bool hit = false; //Hit or not
    int sideHit; //side hit
};

//Specific type of game that contains a 2d map and various functions to build a game from such a 2d map
class GridGame : public Game
{
private:
    Map* map = nullptr;
    Point playerPos;
public:
    GridGame(int w, int h, SDL_Window* win, SDL_Renderer* r) : Game(w, h, win, r) {}
    //sets the current map pointer
    void setMap(Map* m) { map = m; };
    //returns the current map pointer
    Map* getCurMap() { return map; };
    //draws a full screen grid
    void drawGrid(int rows, int cols, rgba color);
    //Fills rectangles in the grid where the map is not a floor (0)
    void mapGrid(rgba c);
    //Draws a filled rectangle
    void drawRect(SDL_Rect, rgba);
    //Performs a raycast from start point at angle on current map
    //Returns point of intersection
    CollisionEvent ddaRaycast(Point start, double angle);
    void setPlayerPos(Point p) { playerPos = p; };
    Point getPlayerPos() { return playerPos; };
    int getCellWidth() { return SCREEN_WIDTH / map->xSize(); };
    int getCellHeight() { return SCREEN_HEIGHT / map->ySize(); };
    ~GridGame();
};
