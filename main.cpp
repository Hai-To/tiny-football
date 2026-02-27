#define SDL_DISABLE_OLD_NAMES
#include "SDL3/SDL.h"
#include <iostream>
#include <vector>
#include <cmath>

int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL could not initialize! "
                  << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Tiny Football",
        800,
        600,
        0
    );

    // SDL3 renderers are created by name; NULL chooses default backend
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        std::cout << "Renderer could not be created: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // game constants
    const int SCREEN_W = 800;
    const int SCREEN_H = 600;

    struct Vec2 { float x, y; };

    struct Player {
        Vec2 pos;
        Vec2 vel;
        SDL_Scancode up, down, left, right;
        bool controlled{false};
        int team;
    };

    struct Ball {
        Vec2 pos;
        Vec2 vel;
        float radius;
    } ball;

    std::vector<Player> players;
    int team1Index = 0;
    int team2Index = 0;
    int score1 = 0;
    int score2 = 0;

    auto resetBall = [&]() {
        ball.pos = {SCREEN_W / 2.0f, SCREEN_H / 2.0f};
        ball.vel = { 200.0f * ((rand()%2)?1:-1), 150.0f * ((rand()%2)?1:-1) };
        ball.radius = 10.0f;
    };

    // create players: two per team
    players.push_back({{100, 150}, {0,0}, SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_A, SDL_SCANCODE_D, true, 1});
    players.push_back({{100, 450}, {0,0}, SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_A, SDL_SCANCODE_D, false, 1});
    players.push_back({{700, 150}, {0,0}, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, true, 2});
    players.push_back({{700, 450}, {0,0}, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, false, 2});

    resetBall();

    bool running = true;
    Uint32 lastTicks = SDL_GetTicks();
    while (running) {
        Uint32 current = SDL_GetTicks();
        float delta = (current - lastTicks) / 1000.0f;
        lastTicks = current;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.scancode == SDL_SCANCODE_Q) {
                    // switch team1 controlled player
                    do {
                        team1Index = (team1Index + 1) % players.size();
                    } while (players[team1Index].team != 1);
                    for (auto &p : players) p.controlled = false;
                    players[team1Index].controlled = true;
                } else if (e.key.scancode == SDL_SCANCODE_P) {
                    do {
                        team2Index = (team2Index + 1) % players.size();
                    } while (players[team2Index].team != 2);
                    for (auto &p : players) p.controlled = false;
                    players[team2Index].controlled = true;
                }
            }
        }

        const bool* keys = SDL_GetKeyboardState(NULL);
        for (auto &p : players) {
            if (!p.controlled) continue;
            p.vel = {0,0};
            if (keys[p.up]) p.vel.y = -200;
            if (keys[p.down]) p.vel.y = 200;
            if (keys[p.left]) p.vel.x = -200;
            if (keys[p.right]) p.vel.x = 200;
            if (p.vel.x != 0 && p.vel.y != 0) {
                p.vel.x *= 0.7071f;
                p.vel.y *= 0.7071f;
            }
        }

        // update players
        for (auto &p : players) {
            p.pos.x += p.vel.x * delta;
            p.pos.y += p.vel.y * delta;
            // clamp to field
            if (p.pos.x < 0) p.pos.x = 0;
            if (p.pos.x > SCREEN_W) p.pos.x = SCREEN_W;
            if (p.pos.y < 0) p.pos.y = 0;
            if (p.pos.y > SCREEN_H) p.pos.y = SCREEN_H;
        }

        // update ball
        ball.pos.x += ball.vel.x * delta;
        ball.pos.y += ball.vel.y * delta;
        
        // top/bottom bounce
        if (ball.pos.y - ball.radius < 0) {
            ball.pos.y = ball.radius;
            ball.vel.y = -ball.vel.y;
        }
        if (ball.pos.y + ball.radius > SCREEN_H) {
            ball.pos.y = SCREEN_H - ball.radius;
            ball.vel.y = -ball.vel.y;
        }
        // left/right goal or bounce
        if (ball.pos.x - ball.radius < 0) {
            // goal for team2
            score2++;
            resetBall();
        } else if (ball.pos.x + ball.radius > SCREEN_W) {
            score1++;
            resetBall();
        }

        // simple player-ball collision: reflect ball
        for (auto &p : players) {
            float dx = ball.pos.x - p.pos.x;
            float dy = ball.pos.y - p.pos.y;
            float dist2 = dx*dx + dy*dy;
            float minDist = ball.radius + 15.0f; // player radius
            if (dist2 < minDist*minDist) {
                float dist = std::sqrt(dist2);
                if (dist == 0) continue;
                // push ball out
                ball.pos.x = p.pos.x + dx / dist * minDist;
                ball.pos.y = p.pos.y + dy / dist * minDist;
                // simple reflection based on relative velocity
                ball.vel.x = dx / dist * 200;
                ball.vel.y = dy / dist * 200;
            }
        }

        // render
        SDL_SetRenderDrawColor(renderer, 0, 128, 0, 255); // green field
        SDL_RenderClear(renderer);

        // draw players
        for (auto &p : players) {
            SDL_FRect r{ p.pos.x-15.0f, p.pos.y-15.0f, 30.0f, 30.0f };
            if (p.team == 1) SDL_SetRenderDrawColor(renderer, 0,0,255,255);
            else SDL_SetRenderDrawColor(renderer, 255,0,0,255);
            SDL_RenderFillRect(renderer, &r);
            if (p.controlled) {
                SDL_SetRenderDrawColor(renderer, 255,255,255,255);
                SDL_RenderRect(renderer, &r); // new SDL3 name
            }
        }

        // draw ball
        SDL_SetRenderDrawColor(renderer, 255,255,255,255);
        for (int w = 0; w < ball.radius*2; w++) {
            for (int h = 0; h < ball.radius*2; h++) {
                int dx = ball.radius - w;
                int dy = ball.radius - h;
                if (dx*dx + dy*dy <= ball.radius*ball.radius) {
                    SDL_RenderPoint(renderer, (int)(ball.pos.x - ball.radius + w),
                                              (int)(ball.pos.y - ball.radius + h));
                }
            }
        }

        // draw scoreboard as bars
        SDL_FRect s1{20.0f,20.0f, float(score1) * 20.0f, 20.0f};
        SDL_FRect s2{SCREEN_W-20.0f - float(score2) * 20.0f,20.0f, float(score2) * 20.0f,20.0f};
        SDL_SetRenderDrawColor(renderer, 0,0,255,255);
        SDL_RenderFillRect(renderer, &s1);
        SDL_SetRenderDrawColor(renderer, 255,0,0,255);
        SDL_RenderFillRect(renderer, &s2);

        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}