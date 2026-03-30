#include <thediva.h>
#include <raylib.h>

#include <time.h>
#include <stdio.h>
#include <math.h>

#include "dsc.h"

long long game_clock_start = 0;
void game_clock_init()
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    game_clock_start = (long long)t.tv_sec * 1000000000L + t.tv_nsec;
}

long long game_clock_get_current_time_us()
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    long long nanoseconds = (long long)t.tv_sec * 1000000000L + t.tv_nsec;
    return (nanoseconds - game_clock_start) / 1000;
}

// todo: refactor all of this mess
typedef struct {
    the_diva_target_t* items;
    size_t count;
    size_t capacity;
} targets_t;

typedef struct {
    the_diva_flying_time_change_t* items;
    size_t count;
    size_t capacity;
} flying_time_changes_t;

#define ARRAY_PUSH(array, value) \
{ \
    if ((array)->count >= (array)->capacity) \
    { \
        if ((array)->capacity == 0) (array)->capacity = 100; \
        else (array)->capacity *= 2; \
        (array)->items = realloc((array)->items, (array)->capacity * sizeof((value))); \
    } \
    (array)->items[(array)->count++] = (value); \
} 

the_diva_button_type_t determine_button_type(uint32_t type)
{
    switch (type)
    {
        case 0: case 4: case 8: case 18: return THE_DIVA_BUTTON_TYPE_TRIANGLE;
        case 1: case 5: case 9: case 19: return THE_DIVA_BUTTON_TYPE_CIRCLE;
        case 2: case 6: case 10: case 20: return THE_DIVA_BUTTON_TYPE_CROSS;
        case 3: case 7: case 11: case 21: return THE_DIVA_BUTTON_TYPE_SQUARE;
        case 12: return THE_DIVA_BUTTON_TYPE_SLIDE_LEFT;
        case 13: return THE_DIVA_BUTTON_TYPE_SLIDE_RIGHT;
        case 15: return THE_DIVA_BUTTON_TYPE_SLIDE_CHAIN_LEFT;
        case 16: return THE_DIVA_BUTTON_TYPE_SLIDE_CHAIN_RIGHT;
        default: return THE_DIVA_BUTTON_TYPE_TRIANGLE;
    }
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

    targets_t targets = {0};
    flying_time_changes_t flying_time_changes = {0};

    uint32_t last_time = 0;
    uint32_t target_id = 0;

    uint32_t end_time;
    for (size_t i = 0; i < commands_length; ++i)
    {
        the_diva_dsc_command_t *command = &commands[i];
        if (command->opcode == THE_DIVA_DSC_OPCODE_TIME)
        {
            last_time = command->parameters[0] * 10; // a bit unsure what time PD uses. Multiplying by 10 seems to work to align it to microseconds. Maybe it's just my sleepy head can't figure out things atm... TODO: figure this out
            continue;
        }
        
        if (command->opcode == THE_DIVA_DSC_OPCODE_TARGET)
        {
            the_diva_target_t target = {
                .id = target_id,
                .button_type = determine_button_type(command->parameters[0]),
                .time = last_time,
                .judgement = THE_DIVA_TARGET_JUDGEMENT_NONE,
                .chord_start = target_id,
                .chord_size = 1,
                .x = command->parameters[1] / 1000.0f,
                .y = command->parameters[2] / 1000.0f,
                .angle = command->parameters[3] / 1000.0f * 3.14f / 180.f, // PI precision
                .distance = command->parameters[4] / 1000.0f,
                .amplitude = command->parameters[5],
                .frequency = command->parameters[6],
            };
            if (target_id > 0)
            {
                the_diva_target_t* previous_target = &targets.items[target_id - 1];
                if (previous_target->time == target.time)
                {
                    target.chord_start = previous_target->chord_start;
                    previous_target->chord_size++;
                    target.chord_size = previous_target->chord_size;
                }
                else if (previous_target->chord_size > 2)
                {
                    for (size_t j = 0; j < previous_target->chord_size; ++j)
                    {
                        targets.items[previous_target->chord_start + j].chord_size = previous_target->chord_size;
                    }
                }
            }
            ARRAY_PUSH(&targets, target);
            target_id++;
            continue;
        }

        if (command->opcode == THE_DIVA_DSC_OPCODE_TARGET_FLYING_TIME)
        {
            the_diva_flying_time_change_t flying_time_change = {
                .time = last_time,
                .flying_time = THE_DIVA_MS(command->parameters[0])
            };
            ARRAY_PUSH(&flying_time_changes, flying_time_change);
            continue;
        }

        if (command->opcode == THE_DIVA_DSC_OPCODE_BAR_TIME_SET)
        {
            the_diva_flying_time_change_t flying_time_change = {
                .time = last_time,
                .flying_time = THE_DIVA_MS((int)((command->parameters[1] + 1) * (60.0f / (float)command->parameters[0]) * 1000.0f))
            };
            ARRAY_PUSH(&flying_time_changes, flying_time_change);
            continue;
        }

        if (command->opcode == THE_DIVA_DSC_OPCODE_END)
        {
            end_time = last_time;
            continue;
        }
    }

    the_diva_target_t *last_target = &targets.items[targets.count - 1];
    if (last_target->chord_size > 2)
    {
        for (size_t j = 0; j < last_target->chord_size; ++j)
        {
            targets.items[last_target->chord_start + j].chord_size = last_target->chord_size;
        }
    }

    last_time = -1;
    for (size_t i = 0; i < targets.count; ++i)
    {
        the_diva_target_t *target = &targets.items[i];
        if (last_time != target->time)
        {
            last_time = target->time;
            printf("==== TIME: %d ====\n", last_time);
        }
        printf("TARGET: %d (chord start: %ld, chord size: %ld)\n", target->id, target->chord_start, target->chord_size);
    }

    the_diva_chart_t chart = {
        .duration = end_time,
        .targets = targets.items,
        .targets_count = targets.count,
        .flying_time_changes = flying_time_changes.items,
        .flying_time_changes_count = flying_time_changes.count
    };

    the_diva_state_config_t config;
    the_diva_state_config_fill_default(&config);

    the_diva_state_t* state = NULL;
    the_diva_state_create(&chart, &config, &state);

    InitWindow(480, 272, "THE DIVA - raylib");
    game_clock_init();
    
    while (!WindowShouldClose())
    {
        the_diva_time_t time = game_clock_get_current_time_us();

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            printf("PRESS AT %ld\n\n", time);
            the_diva_state_press(state, THE_DIVA_BUTTON_TYPE_TRIANGLE, time);
            printf("END PRESS AT %ld\n\n", time);
        }

        the_diva_state_tick(state, time);

        BeginDrawing();
        {
            ClearBackground(BLACK);

            for (size_t i = 0; i < targets.count; ++i)
            {
                the_diva_target_t *target = &targets.items[i];
                if (target->time > time) break;
                if (target->judgement != THE_DIVA_TARGET_JUDGEMENT_NONE) continue;
                
                DrawRectangle(target->x - 5, target->y - 5, 10, 10, target->chord_size == 1 ? GREEN : PURPLE);

                float progress = 1.0f - (time - target->time) / (float)the_diva_state_current_flying_time(state);
                float x = progress * target->distance * sinf(target->angle) + target->x;
                float y = -progress * target->distance * cosf(target->angle) + target->y;
                DrawRectangle(x - 5, y - 5, 10, 10, RED);
                DrawText(TextFormat("%.2f", progress), x - 5, y - 15, 10, GREEN);
            }

            DrawText("THE DIVA - raylib", 0, 0, 20, WHITE);
            DrawText(TextFormat("%lld", time), 0, 30, 20, WHITE);
        }

        EndDrawing();
    }

    CloseWindow();

    the_diva_state_destroy(&state);
    free(targets.items);
    free(flying_time_changes.items);

    return 0;
}