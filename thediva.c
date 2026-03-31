#include "thediva.h"

#include <stdlib.h>
#include <string.h>

#include <stdio.h>

typedef struct the_diva_target_judgement_events_t {
    the_diva_target_judgement_event_t value;
    struct the_diva_target_judgement_events_t *next; 
} the_diva_target_judgement_events_t; 

the_diva_target_judgement_events_t* judgement_events = NULL;
the_diva_target_judgement_events_t* judgement_events_end = NULL;

the_diva_result_t queue_judgement_event(the_diva_target_t* target, uint32_t wrong)
{
    the_diva_target_judgement_events_t* new_judgement_event = (the_diva_target_judgement_events_t*)malloc(sizeof(the_diva_target_judgement_events_t));
    if (new_judgement_event == NULL) return THE_DIVA_RESULT_ALLOCATION_FAILED;

    new_judgement_event->value.target = target;
    new_judgement_event->value.wrong = wrong;
    new_judgement_event->next = NULL;

    if (judgement_events_end == NULL)
    {
        judgement_events = new_judgement_event;
        judgement_events_end = new_judgement_event;
    }
    else
    {
        judgement_events_end->next = new_judgement_event;
        judgement_events_end = judgement_events_end->next;
    }

    return THE_DIVA_RESULT_OK;
}

the_diva_result_t the_diva_state_config_fill_default(the_diva_state_config_t* out_state_config)
{
    if (out_state_config == NULL) return THE_DIVA_RESULT_INVALID_ARGUMENT;

    out_state_config->hit_window.cool = 33333; // todo: make these more readable. This is about 2 frames at 60 fps
    out_state_config->hit_window.fine = 66667;
    out_state_config->hit_window.safe = 116667;
    out_state_config->hit_window.sad = 166667;

    out_state_config->calibration_offset = 0;

    out_state_config->lookahead_time = THE_DIVA_SEC(2);

    return THE_DIVA_RESULT_OK;
}

the_diva_result_t the_diva_state_create(the_diva_chart_t* chart, the_diva_state_config_t* config, the_diva_state_t** out_state)
{
    if (out_state == NULL) return THE_DIVA_RESULT_INVALID_ARGUMENT;
    if (chart == NULL) return THE_DIVA_RESULT_INVALID_ARGUMENT;
    if (config == NULL) return THE_DIVA_RESULT_INVALID_ARGUMENT;
    
    the_diva_state_t* state = calloc(1, sizeof(the_diva_state_t));
    if (state == NULL) return THE_DIVA_RESULT_ALLOCATION_FAILED;

    state->chart = chart;
    state->config = config;

    *out_state = state;
    return THE_DIVA_RESULT_OK;
}

void the_diva_state_destroy(the_diva_state_t** state)
{
    free(*state);
    *state = NULL;
}

void set_current_time(the_diva_state_t* state, the_diva_time_t time)
{
    state->current_time = time + state->config->calibration_offset;
}

void judge_missed_notes(the_diva_state_t* state)
{
    for (size_t i = 0; i < state->chart->targets_count; ++i)
    {
        the_diva_target_t* target = &state->chart->targets[i];

        if (target->judgement != THE_DIVA_TARGET_JUDGEMENT_NONE) continue;

        the_diva_time_t deadline = target->time + state->current_flying_time + state->config->hit_window.sad;

        if (state->current_time > deadline) {
            target->judgement = THE_DIVA_TARGET_JUDGEMENT_MISS;
            queue_judgement_event(target, THE_DIVA_FALSE);
            continue;
        }
    }
}

void process_flying_time_change(the_diva_state_t* state)
{
    // todo: instead of iterating it every time i should keep some index
    for (size_t i = state->flying_time_change_index; i < state->chart->flying_time_changes_count; ++i)
    {
        the_diva_flying_time_change_t *flying_time_change = &state->chart->flying_time_changes[i];
        if (flying_time_change->time < state->current_time) continue;
        state->current_flying_time = flying_time_change->flying_time;
        state->flying_time_change_index = i + 1;
        return; 
    }
}

void the_diva_state_tick(the_diva_state_t* state, the_diva_time_t time)
{
    set_current_time(state, time);
    process_flying_time_change(state);
    judge_missed_notes(state);
}

void the_diva_state_press(the_diva_state_t* state, the_diva_button_type_t button, the_diva_time_t time)
{
    the_diva_time_t input_time = time + state->config->calibration_offset;
    
    if (button <= THE_DIVA_BUTTON_TYPE_SLIDE_RIGHT && state->holds[button] > 0)
    {
        state->holds[button] += 1;
    }

    // todo: instead of iterating the chart every time i should keep some index
    for (size_t i = 0; i < state->chart->targets_count; ++i)
    {
        the_diva_target_t *target = &state->chart->targets[i];
        if (input_time > target->time + state->current_flying_time + state->config->hit_window.sad) continue;
        if (target->judgement != THE_DIVA_TARGET_JUDGEMENT_NONE) continue;

        the_diva_time_t window = llabs(input_time - (target->time + state->current_flying_time));
        if (window > state->config->hit_window.sad) return;

        the_diva_target_judgement_t judgement;
        if (window <= state->config->hit_window.cool) judgement = THE_DIVA_TARGET_JUDGEMENT_COOL;
        else if (window <= state->config->hit_window.fine) judgement = THE_DIVA_TARGET_JUDGEMENT_FINE;
        else if (window <= state->config->hit_window.safe) judgement = THE_DIVA_TARGET_JUDGEMENT_SAFE;
        else if (window <= state->config->hit_window.sad) judgement = THE_DIVA_TARGET_JUDGEMENT_SAD;

        the_diva_target_t *chord = &state->chart->targets[target->chord_start];
        size_t chord_size = target->chord_size;

        the_diva_bool_t button_not_in_chord = THE_DIVA_TRUE;
        for (size_t j = 0; j < chord_size; ++j)
        {
            the_diva_target_t *other_target = &chord[j];
            if (other_target->judgement != THE_DIVA_TARGET_JUDGEMENT_NONE) continue;
            if (other_target->button_type != button) continue;

            button_not_in_chord = THE_DIVA_FALSE;

            other_target->judgement = judgement;
            queue_judgement_event(other_target, THE_DIVA_FALSE);

            if (other_target->hold == THE_DIVA_TRUE && other_target->button_type <= THE_DIVA_BUTTON_TYPE_SLIDE_RIGHT && state->holds[other_target->button_type] == 0)
            {
                state->holds[other_target->button_type] += 1;
            }
            return;
        }

        // fail entire chord in this case
        // right now i am judging all notes in the chord with the same judgement. Not sure how it works in the original game
        if (button_not_in_chord == THE_DIVA_TRUE)
        {
            for (size_t j = 0; j < chord_size; ++j)
            {
                the_diva_target_t *other_target = &chord[j];
                if (other_target->judgement != THE_DIVA_TARGET_JUDGEMENT_NONE) continue;
                other_target->judgement = judgement;
                queue_judgement_event(other_target, THE_DIVA_TRUE);
                return;
            }
        }
    }
}

void the_diva_state_release(the_diva_state_t* state, the_diva_button_type_t button, the_diva_time_t time)
{
    if (button <= THE_DIVA_BUTTON_TYPE_SLIDE_RIGHT && state->holds[button] > 0)
    {
        state->holds[button] -= 1;
    }
}

the_diva_time_t the_diva_state_current_flying_time(the_diva_state_t* state)
{
    return state->current_flying_time;
}

the_diva_result_t the_diva_state_judgement_event_poll(the_diva_state_t* state, the_diva_target_judgement_event_t* out_target_judgement_event)
{
    if (judgement_events == NULL) return THE_DIVA_RESULT_NO_EVENT;
    
    out_target_judgement_event->target = judgement_events->value.target;
    out_target_judgement_event->wrong = judgement_events->value.wrong;

    the_diva_target_judgement_events_t *next = judgement_events->next;
    free(judgement_events);
    judgement_events = next;
    if (judgement_events == NULL) judgement_events_end = NULL;

    return THE_DIVA_RESULT_OK;
}