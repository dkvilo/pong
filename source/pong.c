#include "dk.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define V2_IMPLEMENTATION
#include "dk_linmath.h"

#define DK_TEXT_IMPLEMENTATION
#include "dk_text.h"

#define WINDOW_WIDTH 1080
#define WINDOW_HEIGHT 720

enum GameState
{
  MENU = 0,
  PAUSE,
  IN_GAME,
  GAME_OVER,
  STATE_COUNT
};

enum EntityFlag
{
  PADDLE_1 = 1 << 0,
  PADDLE_2 = 1 << 1,
  BALL = 1 << 2,
  WALL = 1 << 3,       // not used
  UI_ELEMENT = 1 << 4, // not used
  FLAG_COUNT
};

typedef struct
{
  u32 flags;
  struct v2 position;
  struct v2 rotation;
  struct v2 dimentions;
  struct v2 velocity;
  struct v2 acceleration;
  SDL_Color color;
} entity_t;

void
set_flag(entity_t* entity, u32 flag)
{
  entity->flags |= flag;
}

void
clear_flag(entity_t* entity, u32 flag)
{
  entity->flags &= ~flag;
}

bool
has_flag(entity_t* entity, u32 flag)
{
  return entity->flags & flag;
}

typedef struct
{
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Event event;
  bool running;
  u32 state;
  dk_text_t text;
} game_t;

typedef struct
{
  u32 player_1;
  u32 player_2;
} score_t;

entity_t paddle_1 = { .flags = PADDLE_1,
                      .color = { 19, 15, 64, 100 },
                      .dimentions = { 30, 250 } };
entity_t paddle_2 = { .flags = PADDLE_2,
                      .color = { 19, 15, 64, 100 },
                      .dimentions = { 30, 250 } };
entity_t ball = { .flags = BALL,
                  .color = { 255, 255, 255, 255 },
                  .dimentions = { 30, 30 } };

score_t score = { .player_1 = 0, .player_2 = 0 };

void
make_paddle(entity_t* paddle, i32 x, i32 y)
{
  paddle->position.x = x;
  paddle->position.y = y;
  paddle->velocity.x = 0.0f;
  paddle->velocity.y = 0.0f;
}

void
draw_paddle(game_t* game, entity_t* paddle)
{
  SDL_Rect rect = { paddle->position.x,
                    paddle->position.y,
                    paddle->dimentions.x,
                    paddle->dimentions.y };

  SDL_SetRenderDrawColor(game->renderer,
                         paddle->color.r,
                         paddle->color.g,
                         paddle->color.b,
                         paddle->color.a);
  SDL_RenderFillRect(game->renderer, &rect);
}

void
make_ball(entity_t* ball, i32 x, i32 y)
{
  ball->position.x = x;
  ball->position.y = y;
  ball->velocity.x = 0;
  ball->velocity.y = 0;
  set_flag(ball, BALL);
}

void
draw_ball(game_t* game, entity_t* ball)
{
  SDL_Rect rect = {
    ball->position.x, ball->position.y, ball->dimentions.x, ball->dimentions.y
  };
  SDL_SetRenderDrawColor(
    game->renderer, ball->color.r, ball->color.g, ball->color.b, ball->color.a);
  SDL_RenderFillRect(game->renderer, &rect);
}

void
game_init(game_t* game)
{

  game->state = MENU;

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    printf("SDL_Init Error: %s ", SDL_GetError());
    exit(1);
  }

  game->window = SDL_CreateWindow("The Pong",
                                  SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED,
                                  WINDOW_WIDTH,
                                  WINDOW_HEIGHT,
                                  SDL_WINDOW_SHOWN);
  if (game->window == NULL) {
    printf("SDL_CreateWindow Error: %s ", SDL_GetError());
    exit(1);
  }

  game->renderer = SDL_CreateRenderer(
    game->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (game->renderer == NULL) {
    printf("SDL_CreateRenderer Error: %s ", SDL_GetError());
    exit(1);
  }

  if (TTF_Init() != 0) {
    printf("TTF_Init Error: %s ", TTF_GetError());
    exit(1);
  }

  TTF_Font* font = TTF_OpenFont("assets/font/Monogram-Extended.ttf", 120);
  if (font == NULL) {
    fprintf(stderr, "Font Error: Unable to find a font file\n");
    exit(EXIT_FAILURE);
  }

  game->running = true;

  dk_text_init(&game->text, game->renderer, font, (SDL_Color){ 255, 255, 255 });

  make_paddle(&paddle_1, 10, (WINDOW_HEIGHT / 2) - (paddle_1.dimentions.x / 2));
  make_paddle(&paddle_2,
              WINDOW_WIDTH - paddle_2.dimentions.x - 10,
              (WINDOW_HEIGHT / 2) - (paddle_2.dimentions.y / 2));

  make_ball(&ball,
            (WINDOW_WIDTH / 2) - (ball.dimentions.x / 2),
            (WINDOW_HEIGHT / 2) - (ball.dimentions.y / 2));
}

void
game_destroy(game_t* game)
{
  dk_text_destroy(&game->text);
  SDL_DestroyRenderer(game->renderer);
  SDL_DestroyWindow(game->window);
  TTF_Quit();
  SDL_Quit();
}

void
game_handle_events(game_t* game)
{
  while (SDL_PollEvent(&game->event)) {
    switch (game->event.type) {
      case SDL_QUIT:
        game->running = false;
        break;
      case SDL_KEYDOWN:
        switch (game->event.key.keysym.sym) {
          case SDLK_ESCAPE:
            game->state = PAUSE;
            break;
          case SDLK_SPACE:
            if (game->state == MENU || game->state == PAUSE) {
              game->state = IN_GAME;
            }
            break;
        }
        break;
    }
  }
}

void
game_update(game_t* game)
{
  switch (game->state) {
    case MENU:
      break;
    case PAUSE:
      break;
    case IN_GAME: {

      static f32 paddle_1_speed = 20.0f;
      static f32 paddle_2_speed = 20.0f;
      static f32 ball_speed = 5.5f;

      if (ball.velocity.x == 0 && ball.velocity.y == 0) {
        ball.velocity.x = rand() % 2 == 0.0f ? 1.0f : -1.0f;
        ball.velocity.y = rand() % 2 == 0.0f ? 1.0f : -1.0f;
      }

      if (has_flag(&paddle_1, PADDLE_1)) {
        const Uint8* state = SDL_GetKeyboardState(NULL);
        if (state[SDL_SCANCODE_W]) {
          paddle_1.position.y -= 1.0f * paddle_1_speed;
        }
        if (state[SDL_SCANCODE_S]) {
          paddle_1.position.y += 1.0f * paddle_1_speed;
        }
      }

      if (has_flag(&paddle_2, PADDLE_2)) {
        const Uint8* state = SDL_GetKeyboardState(NULL);
        if (state[SDL_SCANCODE_I]) {
          paddle_2.position.y -= 1.0f * paddle_2_speed;
        }
        if (state[SDL_SCANCODE_K]) {
          paddle_2.position.y += 1.0f * paddle_2_speed;
        }
      }

      if (paddle_1.position.y < 0) {
        paddle_1.position.y = 0.0f;
      }

      if (paddle_1.position.y > WINDOW_HEIGHT - paddle_1.dimentions.y) {
        paddle_1.position.y = WINDOW_HEIGHT - paddle_1.dimentions.y;
      }

      if (paddle_2.position.y < 0) {
        paddle_2.position.y = 0.0f;
      }

      if (paddle_2.position.y > WINDOW_HEIGHT - paddle_2.dimentions.y) {
        paddle_2.position.y = WINDOW_HEIGHT - paddle_2.dimentions.y;
      }

      if (has_flag(&ball, BALL)) {
        ball.position.x += ball.velocity.x * ball_speed;
        ball.position.y += ball.velocity.y * ball_speed;
      }

      if (ball.position.y <= 0 ||
          ball.position.y >= WINDOW_HEIGHT - ball.dimentions.y) {
        // invert y velocity when ball hits top or bottom (do not increase
        // speed)
        ball.velocity.y *= -1.0f;
      }

      if (ball.position.x <= 0 ||
          ball.position.x >= WINDOW_WIDTH - ball.dimentions.x) {
        // invert ball velocity x (do not increase speed)
        ball.velocity.x *= -1.0f;
      }

      if (ball.position.x <= paddle_1.position.x + paddle_1.dimentions.x &&
          ball.position.y >= paddle_1.position.y &&
          ball.position.y <= paddle_1.position.y + paddle_1.dimentions.y) {
        f32 paddle_1_center = paddle_1.position.y + (paddle_1.dimentions.y / 2);
        f32 ball_center = ball.position.y + (ball.dimentions.y / 2);
        f32 paddle_1_collision = paddle_1_center - ball_center;
        f32 paddle_1_collision_percentage =
          paddle_1_collision / (paddle_1.dimentions.y / 2);
        f32 ball_angle = paddle_1_collision_percentage * 45.0f;
        ball.velocity.x = cos(ball_angle * (M_PI / 180.0f));
        ball.velocity.y = sin(ball_angle * (M_PI / 180.0f));
        ball.velocity.x *= 1.0f;
        ball.velocity.y *= 1.0f;
        ball_speed += 1.5f;
      }

      if (ball.position.x + ball.dimentions.x >= paddle_2.position.x &&
          ball.position.y >= paddle_2.position.y &&
          ball.position.y <= paddle_2.position.y + paddle_2.dimentions.y) {
        f32 paddle_2_center = paddle_2.position.y + (paddle_2.dimentions.y / 2);
        f32 ball_center = ball.position.y + (ball.dimentions.y / 2);
        f32 paddle_2_collision = paddle_2_center - ball_center;
        f32 paddle_2_collision_percentage =
          paddle_2_collision / (paddle_2.dimentions.y / 2);
        f32 ball_angle = paddle_2_collision_percentage * 45.0f;
        ball.velocity.x = cos(ball_angle * (M_PI / 180.0f));
        ball.velocity.y = sin(ball_angle * (M_PI / 180.0f));
        ball.velocity.x *= -1.0f;
        ball.velocity.y *= -1.0f;
        ball_speed += 1.5f;
      }

      if (ball.position.x <= 0) {
        ball.position.x = (f32)(WINDOW_WIDTH / 2);
        ball.position.y = (f32)(WINDOW_HEIGHT / 2);
        ball.velocity.x = 0.0f;
        ball.velocity.y = 0.0f;
        score.player_2++;
        ball_speed = 5.0f;
      }

      if (ball.position.x >= WINDOW_WIDTH - ball.dimentions.x) {
        ball.position.x = (f32)(WINDOW_WIDTH / 2);
        ball.position.y = (f32)(WINDOW_HEIGHT / 2);
        ball.velocity.x = 0.0f;
        ball.velocity.y = 0.0f;
        score.player_1++;
        ball_speed = 5.0f;
      }

    } break;
    case GAME_OVER:
      break;
  }
}

void
game_render(game_t* game)
{

  SDL_SetRenderDrawColor(game->renderer, 5, 196, 107, 255);
  SDL_RenderClear(game->renderer);

  switch (game->state) {
    case MENU: {
      char* menu_str = "Press Space to Start";
      dk_text_draw(
        &game->text,
        menu_str,
        (WINDOW_WIDTH / 2) - dk_text_width(&game->text, menu_str) / 2,
        WINDOW_HEIGHT / 2 - dk_text_height(&game->text, menu_str) / 2);
    } break;
    case PAUSE: {
      char* pause_str = "Press Space to Resume";
      dk_text_draw(
        &game->text,
        pause_str,
        (WINDOW_WIDTH / 2) - dk_text_width(&game->text, pause_str) / 2,
        (WINDOW_HEIGHT / 2) - dk_text_height(&game->text, pause_str) / 2);
    } break;
    case IN_GAME: {

      game->text.color = (SDL_Color){ 19, 15, 64, 100 };

      SDL_SetRenderDrawColor(game->renderer, 5, 196, 107, 255);
      SDL_RenderClear(game->renderer);

      char* score_val = dk_text_itoa(score.player_1);
      char* score_val2 = dk_text_itoa(score.player_2);

      char buff[250];
      sprintf(buff, "%s   %s", score_val, score_val2);
      dk_text_draw(&game->text,
                   buff,
                   (WINDOW_WIDTH / 2) - dk_text_width(&game->text, buff) / 2,
                   (WINDOW_HEIGHT / 2) - dk_text_height(&game->text, buff) / 2);

      free(score_val);
      free(score_val2);

      draw_paddle(game, &paddle_1);
      draw_paddle(game, &paddle_2);
      draw_ball(game, &ball);

      SDL_SetRenderDrawColor(game->renderer, 19, 15, 64, 100);
      SDL_RenderDrawLine(
        game->renderer, WINDOW_WIDTH / 2, 0, WINDOW_WIDTH / 2, WINDOW_HEIGHT);
      for (int i = 0; i < WINDOW_HEIGHT; i += 20) {
        SDL_RenderDrawPoint(game->renderer, WINDOW_WIDTH / 2, i);
      }

      // top and bottom walls
      SDL_SetRenderDrawColor(game->renderer, 19, 15, 64, 100);
      SDL_Rect top_wall = { 0, 0, WINDOW_WIDTH, 10 };
      SDL_Rect bottom_wall = { 0, WINDOW_HEIGHT - 10, WINDOW_WIDTH, 10 };
      SDL_RenderFillRect(game->renderer, &top_wall);
      SDL_RenderFillRect(game->renderer, &bottom_wall);

    }

    break;
    case GAME_OVER: {
      char* game_over_str = "Game Over";
      dk_text_draw(&game->text,
                   game_over_str,
                   (WINDOW_WIDTH / 2) -
                     dk_text_width(&game->text, game_over_str) / 2,
                   WINDOW_HEIGHT / 2);
    } break;
  }

  SDL_RenderPresent(game->renderer);
}

int
main(int argc, char const* argv[])
{
  srand((unsigned int)time(NULL));

  game_t game;
  game_init(&game);

  while (game.running) {
    game_handle_events(&game);
    game_update(&game);
    game_render(&game);
  }

  game_destroy(&game);

  return 0;
}
