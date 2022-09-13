
#include<stdio.h>           // Standard input output library

#include <stdlib.h>         // For random number generator
#include <time.h>           // For initialization for randum number generator
#include <math.h>           // For mathematical functions

#include <SDL2/SDL.h>       // SDL2 for graphical window
#include <sys/queue.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// #define CAM_WIDTH 640
// #define CAM_HEIGHT 320


#define CAM_WIDTH 1280
#define CAM_HEIGHT 840


#define RECT_WIDTH 2
#define RECT_HEIGHT 2

#define ROWS CAM_HEIGHT/RECT_HEIGHT
#define COLUMNS CAM_WIDTH/RECT_WIDTH

#define GRID_SIZE  ROWS*COLUMNS

#define UPDATES_PER_FRAME 200

#define BG_SHADE 0

typedef struct Rectangle
{
    SDL_Rect rect;
    Uint8 bw;
    char changed;
} Rectangle;

#define N_AGENTS 7500
#define SPEED 0.05

// Affects the tail
#define TAIL_LENGTH 300
#define DIFFUSE_SPEED 0.015
#define EVAPORATE_SPEED 0.2

// Affects the following system
#define SENSOR_SCOPE M_PI/6
#define TURN_SPEED 0.3
#define SENSOR_OFFSET_DIST 3
#define SENSOR_SIZE 3

typedef struct Agent
{
    float xPos;
    float yPos;
    float angle;

    float xPrev[TAIL_LENGTH];
    float yPrev[TAIL_LENGTH];

    float speed;
} Agent;

Agent agents[N_AGENTS];

Rectangle grid[GRID_SIZE];
Rectangle tempGrid[GRID_SIZE];

SDL_Window *g_window;
SDL_Renderer *g_renderer;

float ColorMask[3] = {0.2, 0.6, 0.9};


float Lerp(float a, float b, float f)
{
    return a + f * (b - a);
}

float Rand01()
{
    return (float)(rand()%1000)/1000;
}

// This function could be inproved
void ChangeShade(int x, int y, Uint8 bw)
{
    // Finding apropriate rectangle
    Rectangle *rec = &tempGrid[y*COLUMNS + x];
    if (rec->changed == 0)
    {
        //printf("hello\n");
        rec->bw = bw;
    }
    rec->changed = 1;
}

void ChangeShadeBlur(int i, Uint8 bw)
{
    // Finding apropriate rectangle
    Rectangle *rec = &tempGrid[i];
    if (rec->changed == 0)
    {
        rec->bw = bw;
    }
    else
    {
        rec->bw = MAX(rec->bw, bw);
    }
    rec->changed = 1;
}

void ResetUpdate()
{
    // Updates grid
    memcpy(grid, tempGrid, sizeof(grid));

    //Resets rectangles in grid
    for (int i = 0; i < GRID_SIZE; i++)
    {
        Rectangle *rec = &tempGrid[i];
        rec -> changed = 0;
    }
}

void UpdateTail(float *xPrev, float *yPrev, float xOld, float yOld)
{
    for (int i = TAIL_LENGTH-1; i > 0; i--)
    {
        xPrev[i] = xPrev[i-1];
        yPrev[i] = yPrev[i-1];
    }

    xPrev[0] = xOld;
    yPrev[0] = yOld;
}

float Sense(Agent agent, float sensorAngleOffset)
{
    float sensorAngle = agent.angle + sensorAngleOffset;

    float sensorDirX = cos(sensorAngle);
    float sensorDirY = sin(sensorAngle);

    int sensorCentreX = agent.xPos + sensorDirX*SENSOR_OFFSET_DIST;
    int sensorCentreY = agent.yPos + sensorDirY*SENSOR_OFFSET_DIST;

    float sum = 0;

    for (int offsetX = -SENSOR_SIZE; offsetX <= SENSOR_SIZE; offsetX++)
    {
        for (int offsetY = -SENSOR_SIZE; offsetY <= SENSOR_SIZE; offsetY++)
        {
            int posX = sensorCentreX + offsetX;
            int posY = sensorCentreY + offsetY;

            if (posX >= 0 && posX < COLUMNS && posY >= 0 && posY < ROWS)
            {
                sum += grid[posY*COLUMNS + posX].bw;
            }
        }
    }

    return sum;
}

void AgentUpdate(deltaTime)
{
    // Updating every agent
    for (int i = 0; i < N_AGENTS; i++)
    {
        // Draw curret position

        // Calculate directions
        float Xdirection = cosf(agents[i].angle);
        float Ydirection = sinf(agents[i].angle);

        // Calculate new position
        float newXPos = agents[i].xPos + Xdirection*agents[i].speed*deltaTime;
        float newYPos = agents[i].yPos + Ydirection*agents[i].speed*deltaTime;

        // Following system
        float weightForward = Sense(agents[i], 0);
        float weightLeft = Sense(agents[i], SENSOR_SCOPE);
        float weightRight = Sense(agents[i], -SENSOR_SCOPE);

        float steeringStrength = Rand01();

        if (weightForward > weightLeft && weightForward > weightRight)
        {
            // No change in direction
            agents[i].angle += 0;
        }
        else if (weightForward < weightLeft && weightForward < weightRight)
        {
            // Turn randomly
            agents[i].angle += (steeringStrength - 0.5) * 2 * TURN_SPEED * deltaTime;
        }
        else if (weightRight > weightLeft)
        {
            //Turn left
            agents[i].angle -= steeringStrength * TURN_SPEED * deltaTime;
        }
        else if (weightLeft > weightRight)
        {
            agents[i].angle += steeringStrength * TURN_SPEED * deltaTime;
        }

        // Check for collision with boundary
        if (newXPos < 0 || newXPos > COLUMNS || newYPos < 0 || newYPos > ROWS)
        {
            newXPos = MIN(COLUMNS-0.01, MAX(0, newXPos));
            newYPos = MIN(ROWS-0.01, MAX(0, newYPos));

            // Calculate new direction
            agents[i].angle = 2 * M_PI * ((float)(rand()%1000)) / 1000;
        }

        // Update previous positions
        UpdateTail(agents[i].xPrev, agents[i].yPrev, agents[i].xPos, agents[i].yPos);

        //ChangeShade((int)agents[i].xPos, (int)agents[i].yPos, 0);
        agents[i].xPrev[0] = agents[i].xPos;
        agents[i].yPrev[0] = agents[i].yPos;

        agents[i].xPos = newXPos;           // Setting new x postion
        agents[i].yPos = newYPos;           // Setting new y position
        ChangeShade((int)newXPos, (int)newYPos, 255);
    }
}

void Blur(double deltaTime)
{

    // Looping over agents
    for (int i = 0; i < N_AGENTS; i++)
    {
        // Looping over previous positions
        for (int j = 0; j < TAIL_LENGTH; j++)
        {
            // Checking if previous position has been recorded
            if(agents[i].xPrev[j] != -1 && agents[i].yPrev[j] != -1)
            {
                // Change shade for previous agent position
                int grid_index = (int)agents[i].yPrev[j]*COLUMNS + (int)agents[i].xPrev[j];
                float bw = grid[grid_index].bw-EVAPORATE_SPEED*deltaTime;
                ChangeShadeBlur(grid_index, MAX(0, bw));
            }
        }
    }


    // Looping over grid
    for (int i = 0; i < GRID_SIZE; i++)
    {
        int sum = 0;

        float origionalVal = (float)(grid[i].bw);

        // Finding neighbours
        for (int x = -1; x <= 1; x++)
        {
            for (int y = -1; y <= 1; y++)
            {
                int nx = i%(int)(COLUMNS) + x;
                int ny = i/(int)(COLUMNS) + y;

                if (x != 0 || y != 0)
                {
                    if (nx >= 0 && nx < COLUMNS && ny >= 0 && ny < ROWS)
                    {
                        sum += grid[i+COLUMNS*y+x].bw;
                    }
                }
            }
        }

        float blurVal = ((float)(sum)/9);

        float diffusedVal = Lerp(origionalVal, blurVal, DIFFUSE_SPEED*deltaTime);

        float diffusedEvaporatedVal = MAX(0, diffusedVal - EVAPORATE_SPEED*deltaTime);

        //tempGrid[i].bw = diffusedEvaporatedVal;
        ChangeShadeBlur(i, diffusedEvaporatedVal);
    }
}

void Update(double deltaTime)
{
    AgentUpdate(deltaTime);

    Blur(deltaTime);

    ResetUpdate();
}

void Draw()
{
    // Clear screen
    SDL_RenderClear(g_renderer);

    // Drawing rectangles
    for (int i = 0; i < GRID_SIZE; i++)
    {
        SDL_SetRenderDrawColor(g_renderer,
                               (Uint8)(grid[i].bw*ColorMask[0]),
                               (Uint8)(grid[i].bw*ColorMask[1]),
                               (Uint8)(grid[i].bw*ColorMask[2]),
                               255);
        SDL_RenderDrawRect(g_renderer, &grid[i].rect);
        SDL_RenderFillRect(g_renderer, &grid[i].rect);
    }

    // Background Color
    SDL_SetRenderDrawColor(g_renderer, 255, 0, 0, 255);

    // Drawing to window
    SDL_RenderPresent(g_renderer);
}

void CircleSpawn()
{
    int radius = 150;
    int i = 0;
    for (float theta = 0; theta < 2*M_PI; theta += 2*M_PI/(N_AGENTS))
    {
        if (i < N_AGENTS)
        {
            float randomAngle = 2*M_PI*Rand01();
            agents[i].xPos = COLUMNS/2 + (rand()%radius)*cos(randomAngle);
            agents[i].yPos = ROWS/2 + (rand()%radius)*sin(randomAngle);


            float vx = (COLUMNS/2 - agents[i].xPos) / sqrt(pow(COLUMNS/2, 2) + pow(agents[i].xPos, 2));
            float vy = (ROWS/2 - agents[i].yPos) / sqrt(pow(ROWS/2, 2) + pow(agents[i].yPos, 2));

            agents[i].angle = atan2(vx, vy);
            agents[i].speed = SPEED;

            for (int j = 0; j < TAIL_LENGTH; j++)
            {
                agents[i].xPrev[j] = -1;
                agents[i].yPrev[j] = -1;
            }

            i++;
        }
    }
}

void RandomSpawn()
{
    int i = 0;
    for (float theta = 0; theta < 2*M_PI; theta += 2*M_PI/(N_AGENTS))
    {
        if (i < N_AGENTS)
        {
            // agents[i].xPos = COLUMNS/2;
            // agents[i].yPos = ROWS/2;

            agents[i].xPos = rand()%COLUMNS;
            agents[i].yPos = rand()%ROWS;

            agents[i].angle = 2*M_PI*Rand01(); //atan2(vx, vy);
            agents[i].speed = SPEED;

            for (int j = 0; j < TAIL_LENGTH; j++)
            {
                agents[i].xPrev[j] = -1;
                agents[i].yPrev[j] = -1;
            }

            i++;
        }
    }
}

void CreateGrid()
{
    int count = 0;
    for (int y = 0; y < CAM_HEIGHT; y+= RECT_HEIGHT)
    {
        for (int x = 0; x < CAM_WIDTH; x+= RECT_WIDTH)
        {
            Rectangle rect;

            rect.rect.x = x;
            rect.rect.y = y;

            rect.rect.w = RECT_WIDTH;
            rect.rect.h = RECT_HEIGHT;


            //rect.bw = rand()%256;                 // Noise
            rect.bw = BG_SHADE;                     // Black tiles
            rect.changed = 0;

            tempGrid[count] = rect;
            grid[count] = rect;

            count ++;
        }
    }
}

int GameWindow()
{
        // Initialize random number generator
    srand(time(NULL));

    // Initialize window
    SDL_Init(SDL_INIT_VIDEO);

    // Creating window
    g_window = SDL_CreateWindow("Window",
                                SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED,
                                CAM_WIDTH,
                                CAM_HEIGHT,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
    );

    // Checking if window is created
    if (g_window == NULL) {
        printf("Could not create window: %s\n", SDL_GetError());
        return -1;
    }

    // Creating renderer window
    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);

    // Creating grid
    CreateGrid();


    // Initialize agents
    CircleSpawn();


    SDL_Event e;            // General Event Structure
    char quit = 0;

    //SDL_EnableKeyRepeat(500, 30);

    clock_t time1 = clock();
    clock_t time2 = clock();
    double deltaTime = 0;

    char first = 0;

    float time = 0;

    // Event loop
    while (quit == 0){

        time2 = clock();
        deltaTime = ((float)(time2-time1))/10000;

        if (time > 50)
        {
            while (SDL_PollEvent(&e))
            {
                if (e.type == SDL_QUIT){
                    quit = 1;
                }

                if (e.type == SDL_KEYDOWN){
                    for (int i = 0; i < UPDATES_PER_FRAME; i++)
                    {
                        Update(deltaTime);
                    }
                }

                if (e.type == SDL_MOUSEBUTTONDOWN){
                    quit = 1;
                }
            }

            Update(deltaTime);

            Draw();
        }
        else
        {
            time += deltaTime;
        }

        time1 = time2;
    }

    // Destroying window
    SDL_DestroyWindow(g_window);

    // Quitting SDL ...
    SDL_Quit();

    return 0;
}

int main(void)
{
    int ExitCode = GameWindow();
    return ExitCode;
}
