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
        case 0: case 4: case 18: return THE_DIVA_BUTTON_TYPE_TRIANGLE;
        case 1: case 5: case 19: return THE_DIVA_BUTTON_TYPE_CIRCLE;
        case 2: case 6: case 20: return THE_DIVA_BUTTON_TYPE_CROSS;
        case 3: case 7: case 21: return THE_DIVA_BUTTON_TYPE_SQUARE;
        case 12: case 23: return THE_DIVA_BUTTON_TYPE_SLIDE_LEFT;
        case 13: case 24: return THE_DIVA_BUTTON_TYPE_SLIDE_RIGHT;
        case 15: return THE_DIVA_BUTTON_TYPE_SLIDE_CHAIN_LEFT;
        case 16: return THE_DIVA_BUTTON_TYPE_SLIDE_CHAIN_RIGHT;
        default: return THE_DIVA_BUTTON_TYPE_TRIANGLE;
    }
}

const char* judgement_name(the_diva_target_judgement_t judgement)
{
    switch (judgement)
    {
        case THE_DIVA_TARGET_JUDGEMENT_COOL: return "COOL";
        case THE_DIVA_TARGET_JUDGEMENT_FINE: return "FINE";
        case THE_DIVA_TARGET_JUDGEMENT_SAFE: return "SAFE";
        case THE_DIVA_TARGET_JUDGEMENT_SAD:  return "SAD";
        case THE_DIVA_TARGET_JUDGEMENT_MISS: return "MISS";
        default: return "";
    }
}

Vector2 note_position(const the_diva_target_t* target, float progress)
{
    float x = progress * target->distance;
    float y = sinf(progress * PI * target->frequency) / 36.0f * target->amplitude;
 
    return (Vector2){
        .x = target->x + sinf(target->angle) * x + cosf(target->angle) * y,
        .y = target->y - cosf(target->angle) * x + sinf(target->angle) * y
    };
}
 
float note_progress(the_diva_state_t* state, size_t index, the_diva_time_t time)
{
    const the_diva_target_t* target = &state->chart->targets[index];
    the_diva_time_t hit_time = the_diva_state_target_hit_time(state, index);
    the_diva_time_t flight = hit_time - target->time;
    if (flight <= 0) return 0.0f;
    return (hit_time - time) / (float)flight;
}

Color button_color(the_diva_button_type_t button)
{
    switch (button)
    {
        case THE_DIVA_BUTTON_TYPE_TRIANGLE: return GREEN;
        case THE_DIVA_BUTTON_TYPE_CIRCLE:   return RED;
        case THE_DIVA_BUTTON_TYPE_CROSS:    return SKYBLUE;
        case THE_DIVA_BUTTON_TYPE_SQUARE:   return PINK;
        default:                            return ORANGE;
    }
}
 
void draw_button_symbol(the_diva_button_type_t button, Vector2 center, float radius, float thickness, Color color)
{
    switch (button)
    {
        case THE_DIVA_BUTTON_TYPE_TRIANGLE:
            DrawPolyLinesEx(center, 3, radius, -90.0f, thickness, color);
            break;
 
        case THE_DIVA_BUTTON_TYPE_CIRCLE:
            DrawRing(center, radius - thickness * 0.5f, radius + thickness * 0.5f, 0.0f, 360.0f, 32, color);
            break;
 
        case THE_DIVA_BUTTON_TYPE_CROSS:
        {
            float d = radius * 0.75f;
            DrawLineEx((Vector2){ center.x - d, center.y - d }, (Vector2){ center.x + d, center.y + d }, thickness, color);
            DrawLineEx((Vector2){ center.x - d, center.y + d }, (Vector2){ center.x + d, center.y - d }, thickness, color);
            break;
        }
 
        case THE_DIVA_BUTTON_TYPE_SQUARE:
            DrawPolyLinesEx(center, 4, radius, 45.0f, thickness, color);
            break;
 
        case THE_DIVA_BUTTON_TYPE_SLIDE_LEFT:
        case THE_DIVA_BUTTON_TYPE_SLIDE_CHAIN_LEFT:
            DrawPoly(center, 3, radius + thickness * 0.5f, 180.0f, color);
            break;
 
        case THE_DIVA_BUTTON_TYPE_SLIDE_RIGHT:
        case THE_DIVA_BUTTON_TYPE_SLIDE_CHAIN_RIGHT:
            DrawPoly(center, 3, radius + thickness * 0.5f, 0.0f, color);
            break;
 
        default:
            DrawCircleV(center, radius + thickness * 0.5f, color);
            break;
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
                .chord_start = target_id,
                .chord_size = 1,
                .x = command->parameters[1] / 1000.0f,
                .y = command->parameters[2] / 1000.0f,
                .angle = command->parameters[3] / 1000.0f * PI / 180.0f,
                .distance = command->parameters[4] / 1000.0f,
                .amplitude = command->parameters[5],
                .frequency = command->parameters[6],
                .hold = command->parameters[0] >= 4 && command->parameters[0] <= 7 ? THE_DIVA_TRUE : THE_DIVA_FALSE
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

    const char* last_judgement_text = "";
    the_diva_bool_t last_judgement_wrong = THE_DIVA_FALSE;
    uint32_t combo = 0;
    
    while (!WindowShouldClose())
    {
        the_diva_time_t time = game_clock_get_current_time_us();

        if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_I)) the_diva_state_press(state, THE_DIVA_BUTTON_TYPE_TRIANGLE, time);
        if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_J)) the_diva_state_press(state, THE_DIVA_BUTTON_TYPE_SQUARE, time);
        if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_K)) the_diva_state_press(state, THE_DIVA_BUTTON_TYPE_CROSS, time);
        if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_L)) the_diva_state_press(state, THE_DIVA_BUTTON_TYPE_CIRCLE, time);

        if (IsKeyReleased(KEY_W) || IsKeyReleased(KEY_I)) the_diva_state_release(state, THE_DIVA_BUTTON_TYPE_TRIANGLE, time);
        if (IsKeyReleased(KEY_A) || IsKeyReleased(KEY_J)) the_diva_state_release(state, THE_DIVA_BUTTON_TYPE_SQUARE, time);
        if (IsKeyReleased(KEY_S) || IsKeyReleased(KEY_K)) the_diva_state_release(state, THE_DIVA_BUTTON_TYPE_CROSS, time);
        if (IsKeyReleased(KEY_D) || IsKeyReleased(KEY_L)) the_diva_state_release(state, THE_DIVA_BUTTON_TYPE_CIRCLE, time);

        the_diva_state_tick(state, time);

        the_diva_state_event_t event;
        while (the_diva_state_event_poll(state, &event) == THE_DIVA_RESULT_OK)
        {
            if (event.type == THE_DIVA_STATE_EVENT_JUDGEMENT)
            {
                the_diva_target_judgement_t judgement = event.event.judgement.judgement;
                last_judgement_text = judgement_name(judgement);
                last_judgement_wrong = event.event.judgement.wrong;
 
                if (judgement == THE_DIVA_TARGET_JUDGEMENT_MISS || judgement == THE_DIVA_TARGET_JUDGEMENT_SAD || event.event.judgement.wrong)
                {
                    combo = 0;
                }
                else
                {
                    combo++;
                }
            }
        }

        the_diva_time_t song_time = time + config.calibration_offset;

        BeginDrawing();
        {
            ClearBackground(BLACK);

            for (size_t i = 0; i < targets.count; ++i)
            {
                the_diva_target_t *target = &targets.items[i];
                if (target->time > time) break;
                if (the_diva_state_target_judgement(state, i) != THE_DIVA_TARGET_JUDGEMENT_NONE) continue;

                float progress = 1.0f - (time - target->time) / (float)the_diva_state_current_flying_time(state);
                Color color = button_color(target->button_type);
                Vector2 target_center = { target->x, target->y };
                
                draw_button_symbol(target->button_type, target_center, 7.0f, 4.0f, Fade(color, 0.5f));
                DrawRectanglePro((Rectangle){.x = target->x, .y = target->y, .width = 2, .height = 10}, (Vector2){1, 10}, progress * -360.0f, WHITE);
                if (target->hold == THE_DIVA_TRUE) DrawText("HOLD", target->x - 13, target->y + 6, 1, WHITE);

                Vector2 position = note_position(target, progress);

                if (target->chord_size > 1)
                {
                    size_t next = i + 1 < target->chord_start + target->chord_size ? i + 1 : target->chord_start;
                    if (next != i && the_diva_state_target_judgement(state, next) == THE_DIVA_TARGET_JUDGEMENT_NONE)
                    {
                        Vector2 other = note_position(&targets.items[next], note_progress(state, next, song_time));
                        DrawLineV(position, other, PURPLE);
                    }
                }

                draw_button_symbol(target->button_type, position, 7.0f, 6.0f, WHITE);
                draw_button_symbol(target->button_type, position, 7.0f, 4.0f, color);
            }

            DrawText("THE DIVA - raylib", 0, 0, 10, WHITE);
            DrawText(TextFormat("%lld", time), 0, 11, 10, WHITE);
            DrawText(TextFormat("TRIANGLE: %d | CIRCLE: %d | CROSS: %d | SQUARE: %d\n", state->holds[0], state->holds[1], state->holds[2], state->holds[3]), 0, 22, 10, WHITE);
            DrawText(TextFormat("%s%s  combo %u", last_judgement_wrong ? "WRONG " : "", last_judgement_text, combo), 0, 33, 10, WHITE);
        }

        EndDrawing();
    }

    CloseWindow();

    the_diva_state_destroy(&state);
    free(targets.items);
    free(flying_time_changes.items);

    return 0;
}