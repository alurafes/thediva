#include <thediva.h>
#include <raylib.h>

#include <time.h>
#include <stdio.h>

#include "dsc.h"

long long game_clock_start = 0;
void game_clock_init()
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    game_clock_start = (long long)t.tv_sec * 1000000000L + t.tv_nsec;
}

long long game_clock_get_current_time()
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    long long nanoseconds = (long long)t.tv_sec * 1000000000L + t.tv_nsec;
    return nanoseconds - game_clock_start;
}

int main(int argc, char** argv)
{
    if (argc != 2) 
    {
        printf("Usage: %s <.dsc>\n", argv[0]);
        return 1;
    }

    FILE *dsc_file = fopen(argv[1], "rb");
    fseek(dsc_file, 0, SEEK_END);
    long dsc_file_size = ftell(dsc_file);
    rewind(dsc_file);

    char *dsc_buffer = (char *)malloc(dsc_file_size);
    fread(dsc_buffer, 1, dsc_file_size, dsc_file);

    size_t commands_length = 0;
    the_diva_dsc_read_from_buffer(dsc_buffer, dsc_file_size, NULL, &commands_length);
    the_diva_dsc_command_t *commands = malloc(sizeof(the_diva_dsc_command_t) * commands_length);
    the_diva_dsc_read_from_buffer(dsc_buffer, dsc_file_size, commands, &commands_length);

    InitWindow(1280, 720, "THE DIVA - raylib");
    game_clock_init();
    
    while (!WindowShouldClose())
    {
        BeginDrawing();
        {
            ClearBackground(BLACK);
            long long time = game_clock_get_current_time();
            DrawText("THEDIVA - raylib", 0, 0, 20, WHITE);
            DrawText(TextFormat("%lld", time / 1000000000L), 0, 30, 20, WHITE);
        }

        EndDrawing();
    }

    CloseWindow();
}