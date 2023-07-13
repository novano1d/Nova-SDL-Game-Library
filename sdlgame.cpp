#include "sdlgame.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Game class implementation

double Game::frameTime()
{
    oldTime = time;
    time = SDL_GetTicks();
    return (time - oldTime) / 1000.0;
}

// Sets screen color
void Game::clrScreen(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    SDL_RenderClear(renderer);
}

bool nva::loadImage(std::vector<unsigned char>& image, const std::string& filename, int& x, int&y)
{
    int n;
    unsigned char* data = stbi_load(filename.c_str(), &x, &y, &n, 4);
    if (data != nullptr)
    {
        image = std::vector<unsigned char>(data, data + x * y * 4);
    }
    stbi_image_free(data);
    return (data != nullptr);
}

void Game::gameplayLoop(void(*ptr)(void)) 
{
    bool keepRunning = true;
    while (keepRunning)
    {
        ptr();
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT) 
            { 
                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(window);
                SDL_Quit();
                keepRunning = false;
                break;
            }
            else if (event.type && eventMethod)
            {
                eventMethod(event);
            }
        }
    }
}

void Game::setEventHandler(void(*ptr)(SDL_Event))
{
    eventMethod = ptr;
}


//GridGame implementation

void GridGame::drawGrid(int rows, int cols, rgba c)
{
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
    int cellWidth = SCREEN_WIDTH / cols;
    int cellHeight = SCREEN_HEIGHT / rows;
    for (int i = 1; i <= rows; i++) SDL_RenderDrawLine(renderer, 0, i*cellHeight, SCREEN_WIDTH, i*cellHeight);
    for (int i = 1; i <= cols; i++) SDL_RenderDrawLine(renderer, i*cellWidth, 0, i*cellWidth, SCREEN_HEIGHT);
}

void GridGame::drawRect(SDL_Rect r, rgba c)
{
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(renderer, &r);
}

void GridGame::mapGrid(rgba c)
{
    if (map) //safeguard
    {
        int cellWidth = SCREEN_WIDTH / map->xSize();
        int cellHeight = SCREEN_HEIGHT / map->ySize();
        for (int i = 0; i < map->ySize(); i++){
            for (int j = 0; j < map->xSize(); j++){
                if (map->getTileAt(j, i))
                {
                    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
                    SDL_Rect square = { j * cellWidth, i*cellHeight, cellWidth, cellHeight};
                    SDL_RenderFillRect(renderer, &square);
                }
            }
        }
    }
}

//Simple DDA
inline CollisionEvent GridGame::ddaRaycast(Point start, double angle)
{
    double angleRadians = angle * M_PI / 180.0;
    //using point as 2d vector to keep clean
    Point rayDir = { cos(angleRadians), sin(angleRadians) };
    Point rayUnitStepSize = { sqrt( 1 + (rayDir.y / rayDir.x) * (rayDir.y / rayDir.x)), sqrt( 1 + (rayDir.x / rayDir.y) * (rayDir.x / rayDir.y)) };
    Point mapCheck = { floor(start.x), floor(start.y) };
    Point rayLength;
    Point step;
    if (rayDir.x < 0) 
    {
        step.x = -1;
        rayLength.x = (start.x - mapCheck.x) * rayUnitStepSize.x;
    }
    else
    {
        step.x = 1;
        rayLength.x = (mapCheck.x + 1 - start.x) * rayUnitStepSize.x;
    } 
    if (rayDir.y < 0) 
    {
        step.y = -1;
        rayLength.y = (start.y - mapCheck.y) * rayUnitStepSize.y;
    }
    else 
    {
        step.y = 1;
        rayLength.y = (mapCheck.y + 1 - start.y) * rayUnitStepSize.y;
    }
    bool tileFound = false;
    int maxDistance = (map->xSize() > map->ySize()) ? map->xSize() : map->ySize();
    double distance = 0;
    int side;
    while (!tileFound && distance < maxDistance)
    {
        if (rayLength.x < rayLength.y)
        {
            mapCheck.x += step.x;
            distance = rayLength.x;
            rayLength.x += rayUnitStepSize.x;
            side = 0;
        }
        else
        {
            mapCheck.y += step.y;
            distance = rayLength.y;
            rayLength.y += rayUnitStepSize.y;
            side = 1;
        }
        if (mapCheck.x >= 0 && mapCheck.x < map->xSize() && mapCheck.y >= 0 && mapCheck.y < map->ySize())
        {
            if (map->getTileAt(mapCheck.x, mapCheck.y))
            {
                return {true, start + rayDir * distance, side, distance * cos(angleRadians - getAngle()*M_PI/180), map->getTileAt(mapCheck.x, mapCheck.y)}; //code fixes fish eye effect
            }
        }
        else return CollisionEvent(); //invalid
    }
    return CollisionEvent(); //invalid
}

void GridGame::setPlayerPos(Point p)
{
    const float WALL_CLOSENESS = 0.1; // how close the player can get (in game grid units) to the wall  
    if(map)
    {
        bool stuckOnVerticalWall = map->getTileAt(playerPos.x - WALL_CLOSENESS, playerPos.y) || map->getTileAt(playerPos.x + WALL_CLOSENESS, playerPos.y);
        bool stuckOnHorizontalWall = map->getTileAt(playerPos.x, playerPos.y - WALL_CLOSENESS) || map->getTileAt(playerPos.x, playerPos.y + WALL_CLOSENESS);

        if (!stuckOnVerticalWall || (stuckOnVerticalWall && ((p.x < playerPos.x && !map->getTileAt(p.x - WALL_CLOSENESS, p.y)) || (p.x > playerPos.x && !map->getTileAt(p.x + WALL_CLOSENESS, p.y)))))
            playerPos.x = p.x;
        if (!stuckOnHorizontalWall || (stuckOnHorizontalWall && ((p.y < playerPos.y && !map->getTileAt(p.x, p.y - WALL_CLOSENESS)) || (p.y > playerPos.y && !map->getTileAt(p.x, p.y + WALL_CLOSENESS)))))
            playerPos.y = p.y;
    }
}


// DEPRECIATED or bug testing only
void GridGame::pseudo3dRender(int FOV, double wallheight)
{
    FOV /= 2;
    SDL_SetRenderDrawColor(renderer, 74, 74, 74, 255);
    SDL_RenderClear(renderer);
    for (int i = 0; i < SCREEN_WIDTH; i++)
    {
        double scanDir = 2 * i / (double)SCREEN_WIDTH - 1; // -1 ---- 0 ---- 1 for the scan across the screen
        CollisionEvent collision = ddaRaycast(getPlayerPos(), getAngle() + FOV * scanDir);
        int lineHeight = (int)(wallheight * (SCREEN_HEIGHT / collision.perpWallDist));
        int drawStart = -lineHeight / 2 + SCREEN_HEIGHT / 2;
        if (drawStart < 0)
            drawStart = 0;
        int drawEnd = lineHeight / 2 + SCREEN_HEIGHT / 2;
        if (drawEnd > SCREEN_HEIGHT)
            drawEnd = SCREEN_HEIGHT;
        Uint8 r = (collision.sideHit) ? 255 : 175;
        SDL_SetRenderDrawColor(renderer, r, 0, 0, 255);
        SDL_RenderDrawLine(renderer, i, drawStart, i, drawEnd);
        // std::cout << collision.intersect.x << " " << collision.intersect.y << std::endl; //debug
        // int textureToRender = map->getTileAt((int)collision.intersect.x, (int)collision.intersect.y);
        // double texCoord = collision.intersect.x - floor(collision.intersect.x);
        // std::cout << collision.tileData << std::endl;
    }
    SDL_RenderPresent(renderer); // fast enough we don't need a buffer
}


void GridGame::renderStripe(int i)
{
    double scanDir = 2 * i / static_cast<double>(renderWidth) - 1; // -1 ---- 0 ---- 1 for the scan across the screen
    CollisionEvent collision = ddaRaycast(getPlayerPos(), angle + FOV * scanDir);
    ZBuffer[i] = collision.perpWallDist; //set zbuffer value
    int lineHeight = static_cast<int>(wallheight * (renderHeight / collision.perpWallDist));
    int drawStart = -lineHeight / 2 + renderHeight / 2;
    if (drawStart < 0) drawStart = 0;
    int drawEnd = lineHeight / 2 + renderHeight / 2;
    if (drawEnd > renderHeight) drawEnd = renderHeight;
    double texCoord;
    const Uint32 black = SDL_MapRGBA(format, 0, 0, 0, 255);
    if (collision.sideHit)
        texCoord = collision.intersect.x - static_cast<int>(collision.intersect.x);
    else
        texCoord = collision.intersect.y - static_cast<int>(collision.intersect.y);
    int textureToRender = collision.tileData;
    int texX = static_cast<int>(texCoord * currentTextureSet->widthHeightAt(textureToRender - 1).first);
    texX = nva::clamp<int>(texX, 0, currentTextureSet->widthHeightAt(textureToRender - 1).first);
    if (textureToRender)
    {
        for (int y = drawStart; y < drawEnd; y++)
        {
            int texY = (((y * 2 - renderHeight + lineHeight) * currentTextureSet->widthHeightAt(textureToRender - 1).second) / lineHeight) / 2;
            texY = nva::clamp<int>(texY, 0, currentTextureSet->widthHeightAt(textureToRender - 1).second);
            rgba textureColor;
            textureColor = currentTextureSet->colorAt(textureToRender - 1, texX, texY);
            if (collision.sideHit)
                pixels[y * renderWidth + i] = (textureColor.r << rshift) |
                                                    (textureColor.g << gshift) |
                                                    (textureColor.b << bshift) |
                                                    (textureColor.a << ashift);
                else
                    pixels[y * renderWidth + i] = ((textureColor.r / 2) << rshift) |
                                                    ((textureColor.g / 2) << gshift) |
                                                    ((textureColor.b / 2) << bshift) |
                                                    (textureColor.a << ashift);
            }
            //floor casting
            for (int y = drawEnd + 1; y <= renderHeight; y++)
            {
                // Calculate the current distance from the player to the floor/ceiling
                double currentDist = renderHeight / (2.0 * y - renderHeight);
                double weight = currentDist / collision.perpWallDist;
                double floorX = weight * collision.intersect.x + (1 - weight) * playerPos.x;
                double floorY = weight * collision.intersect.y + (1 - weight) * playerPos.y;
                int floorTexX = static_cast<int>(floorX * currentTextureSet->widthHeightAt(1).first) % currentTextureSet->widthHeightAt(1).first;
                int floorTexY = static_cast<int>(floorY * currentTextureSet->widthHeightAt(1).second) % currentTextureSet->widthHeightAt(1).second;
                floorTexX = nva::clamp<int>(floorTexX, 0, currentTextureSet->widthHeightAt(1).first);
                floorTexY = nva::clamp<int>(floorTexY, 0, currentTextureSet->widthHeightAt(1).second);
                rgba ftex = currentTextureSet->colorAt(1, floorTexX, floorTexY);
                rgba ctex = currentTextureSet->colorAt(1, floorTexX, floorTexY);
                pixels[(y-1) * renderWidth + i] = (ftex.r << rshift) | //floor
                                                        (ftex.g << gshift) |
                                                        (ftex.b << bshift) |
                                                        (ftex.a << ashift);
                pixels[(renderHeight - y) * renderWidth + i] = (ctex.r << rshift) | //ceiling
                                                        (ctex.g << gshift) |
                                                        (ctex.b << bshift) |
                                                        (ctex.a << ashift);
            }
        }
        else
        {
            for (int y = 0; y < renderHeight; y++)
            {
                pixels[y * renderWidth + i] = black;
            }
        }
}

//speed could almost certainly be improved with multithreading or decreasing # of raycasts
void GridGame::pseudo3dRenderTextured(int FOV, double wallheight)
{
    // Calculate the render dimensions
    double ZBuffer[renderWidth]; // store Z distances for sprite rendering (necessary for occlusion)
    if (!textureBuffer)
        textureBuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, renderWidth, renderHeight);

    Uint32* pixels;
    int pitch;
    SDL_LockTexture(textureBuffer, nullptr, reinterpret_cast<void**>(&pixels), &pitch);
    FOV /= 2;
    //wall casting
    for (int i = 0; i < renderWidth; i++)
    {
        double scanDir = 2 * i / static_cast<double>(renderWidth) - 1; // -1 ---- 0 ---- 1 for the scan across the screen
        CollisionEvent collision = ddaRaycast(getPlayerPos(), angle + FOV * scanDir);
        ZBuffer[i] = collision.perpWallDist; //set zbuffer value
        int lineHeight = static_cast<int>(wallheight * (renderHeight / collision.perpWallDist));
        int drawStart = -lineHeight / 2 + renderHeight / 2;
        if (drawStart < 0) drawStart = 0;
        int drawEnd = lineHeight / 2 + renderHeight / 2;
        if (drawEnd > renderHeight) drawEnd = renderHeight;
        double texCoord;
        const Uint32 black = SDL_MapRGBA(format, 0, 0, 0, 255);
        if (collision.sideHit)
            texCoord = collision.intersect.x - static_cast<int>(collision.intersect.x);
        else
            texCoord = collision.intersect.y - static_cast<int>(collision.intersect.y);
        int textureToRender = collision.tileData;
        int texX = static_cast<int>(texCoord * currentTextureSet->widthHeightAt(textureToRender - 1).first);
        texX = nva::clamp<int>(texX, 0, currentTextureSet->widthHeightAt(textureToRender - 1).first);
        if (textureToRender)
        {
            for (int y = drawStart; y < drawEnd; y++)
            {
                int texY = (((y * 2 - renderHeight + lineHeight) * currentTextureSet->widthHeightAt(textureToRender - 1).second) / lineHeight) / 2;
                texY = nva::clamp<int>(texY, 0, currentTextureSet->widthHeightAt(textureToRender - 1).second);
                rgba textureColor;
                textureColor = currentTextureSet->colorAt(textureToRender - 1, texX, texY);
                if (collision.sideHit)
                    pixels[y * renderWidth + i] = (textureColor.r << rshift) |
                                                    (textureColor.g << gshift) |
                                                    (textureColor.b << bshift) |
                                                    (textureColor.a << ashift);
                else
                    pixels[y * renderWidth + i] = ((textureColor.r / 2) << rshift) |
                                                    ((textureColor.g / 2) << gshift) |
                                                    ((textureColor.b / 2) << bshift) |
                                                    (textureColor.a << ashift);
            }
            //floor casting
            for (int y = drawEnd + 1; y <= renderHeight; y++)
            {
                // Calculate the current distance from the player to the floor/ceiling
                double currentDist = renderHeight / (2.0 * y - renderHeight);
                double weight = currentDist / collision.perpWallDist;
                double floorX = weight * collision.intersect.x + (1 - weight) * playerPos.x;
                double floorY = weight * collision.intersect.y + (1 - weight) * playerPos.y;
                int floorTexX = static_cast<int>(floorX * currentTextureSet->widthHeightAt(1).first) % currentTextureSet->widthHeightAt(1).first;
                int floorTexY = static_cast<int>(floorY * currentTextureSet->widthHeightAt(1).second) % currentTextureSet->widthHeightAt(1).second;
                floorTexX = nva::clamp<int>(floorTexX, 0, currentTextureSet->widthHeightAt(1).first);
                floorTexY = nva::clamp<int>(floorTexY, 0, currentTextureSet->widthHeightAt(1).second);
                rgba ftex = currentTextureSet->colorAt(1, floorTexX, floorTexY);
                rgba ctex = currentTextureSet->colorAt(1, floorTexX, floorTexY);
                pixels[(y-1) * renderWidth + i] = (ftex.r << rshift) | //floor
                                                        (ftex.g << gshift) |
                                                        (ftex.b << bshift) |
                                                        (ftex.a << ashift);
                pixels[(renderHeight - y) * renderWidth + i] = (ctex.r << rshift) | //ceiling
                                                        (ctex.g << gshift) |
                                                        (ctex.b << bshift) |
                                                        (ctex.a << ashift);
            }
        }
        else
        {
            for (int y = 0; y < renderHeight; y++)
            {
                pixels[y * renderWidth + i] = black;
            }
        }
    }
    std::vector<Sprite> *temp = &(map->getSprites()); //pointer so we don't sort each time :)
    //sort sprites by distance from player
    //std::vector<double> distance; //parallel distance vector
    // std::transform(temp.begin(), temp.end(), distance.begin(), [this](Sprite s){ return hypot(s.x - getPlayerPos().x, s.y - getPlayerPos().y); });
    // std::sort(distance.begin(), distance.end(), [](double &a, double &b){ return a > b; }); //sort distances to sprites in descending order
    std::sort(temp->begin(), temp->end(), [this](Sprite &a, Sprite &b){ 
        return hypot(a.x - getPlayerPos().x, a.y - getPlayerPos().y) > hypot(b.x - getPlayerPos().x, b.y - getPlayerPos().y); }); //could eliminate hypot and just use pow
    //rendering
    double planeLength = tan(FOV * M_PI / 180); 
    double planeX = -sin(angle * M_PI / 180) * planeLength; 
    double planeY = cos(angle * M_PI / 180) * planeLength;
    for (auto it = temp->begin(); it != temp->end(); it++)
    {
        double spriteX = it->x - getPlayerPos().x;
        double spriteY = it->y - getPlayerPos().y;
        double invDet = 1.0 / (planeX * sin(angle * M_PI / 180) - cos(angle * M_PI / 180) * planeY);
        double transformX = invDet * (sin(angle * M_PI / 180) * spriteX - cos(angle * M_PI / 180) * spriteY);
        double transformY = invDet * (-planeY * spriteX + planeX * spriteY);
        int spriteScreenX = int((renderWidth / 2) * (1 + transformX / transformY));
        int spriteHeight = abs(int(renderHeight / transformY));
        int drawStartY = -spriteHeight / 2 + renderHeight / 2;
        if(drawStartY < 0) drawStartY = 0;
        int drawEndY = spriteHeight / 2 + renderHeight / 2;
        if(drawEndY >= renderHeight) drawEndY = renderHeight - 1;
        int spriteWidth = abs(int(renderHeight / transformY));
        int drawStartX = -spriteWidth / 2 + spriteScreenX;
        if(drawStartX < 0) drawStartX = 0;
        int drawEndX = spriteWidth / 2 + spriteScreenX;
        if(drawEndX >= renderWidth) drawEndX = renderWidth - 1;
        for(int stripe = drawStartX; stripe < drawEndX; stripe++)
        {
            int texX = int(256 * (stripe - (-spriteWidth / 2 + spriteScreenX)) * currentTextureSet->widthHeightAt(it->texIndex).first / spriteWidth) / 256;
            texX = nva::clamp<int>(texX, 0, currentTextureSet->widthHeightAt(it->texIndex).first);
            if(transformY > 0 && stripe > 0 && stripe < renderWidth && transformY < ZBuffer[stripe])
            {
                for(int y = drawStartY; y < drawEndY; y++)
                {
                    int d = (y - renderHeight / 2) * 256 + spriteHeight * 128;
                    int texY = ((d * currentTextureSet->widthHeightAt(it->texIndex).second) / spriteHeight) / 256;
                    texY = nva::clamp<int>(texY, 0, currentTextureSet->widthHeightAt(it->texIndex).second);
                    rgba textureColor;
                    textureColor = currentTextureSet->colorAt(it->texIndex, texX, texY);
                    if(textureColor.a != 0) // If the pixel is not transparent
                        pixels[y * renderWidth + stripe] = (textureColor.r << rshift) |
                                                            (textureColor.g << gshift) |
                                                            (textureColor.b << bshift) |
                                                            (textureColor.a << ashift);
                }
            }
        }
    }

    SDL_UnlockTexture(textureBuffer);

    // Calculate the target area to maintain aspect ratio
    int windowWidth, windowHeight;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    float windowAspectRatio = static_cast<float>(windowWidth) / windowHeight;
    float bufferAspectRatio = static_cast<float>(renderWidth) / renderHeight;

    SDL_Rect targetRect;
    if (windowAspectRatio > bufferAspectRatio)
    {
        // Window is wider, keep the height and center horizontally
        int targetWidth = windowHeight * bufferAspectRatio;
        targetRect.x = (windowWidth - targetWidth) / 2;
        targetRect.y = 0;
        targetRect.w = targetWidth;
        targetRect.h = windowHeight;
    }
    else
    {
        // Window is taller or same aspect ratio, keep the width and center vertically
        int targetHeight = windowWidth / bufferAspectRatio;
        targetRect.x = 0;
        targetRect.y = (windowHeight - targetHeight) / 2;
        targetRect.w = windowWidth;
        targetRect.h = targetHeight;
    }

    // Clear the window with black color
    //SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Render the back buffer texture in the window
    SDL_RenderCopy(renderer, textureBuffer, nullptr, &targetRect);

    // Present the rendered frame
    SDL_RenderPresent(renderer);
}


GridGame::~GridGame()
{
    SDL_DestroyTexture(textureBuffer);
}

//Texture handler constructor takes in vector of filenames and loads them in
TextureHandler::TextureHandler(std::vector<std::string> in)
{
    int width, height;
    for (std::string filename : in)
    {
        std::vector<unsigned char> image;
        bool success = nva::loadImage(image, filename, width, height);
        if (!success)
        {
            std::cout << "Error loading image " + filename + "\n";
        }
        loadedTextures.emplace_back(image);
        loadedTextureSizes.emplace_back(std::make_pair(width, height));
    }
}

//might prove to be a bottleneck in performance since this function is called for every pixel being rendered on the wall.... therefore we may need to reduce
//the call time as much as possible and change the loaded textures class to store in an array instead of a vector
rgba TextureHandler::colorAt(int textureIndex, int x, int y)
{
    const int RGBA = 4; //This might change if you change the loadimage function
    int r, g, b, a;
    int index = RGBA * ( y * widthHeightAt(textureIndex).first + x);
    r = static_cast<int>(loadedTextures[textureIndex][index + 0]);
    g = static_cast<int>(loadedTextures[textureIndex][index + 1]);
    b = static_cast<int>(loadedTextures[textureIndex][index + 2]);
    a = static_cast<int>(loadedTextures[textureIndex][index + 3]);
    // std::cout << r << " "
    //           << g << " "
    //           << b << " "
    //           << a << " "
    //           << *(Uint32*)&(loadedTextures[textureIndex][index]) << " "
    //           << SDL_MapRGBA(SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888), r, g , b , a) << " "
    //           << '\n';
    return { r, g, b, a };
}