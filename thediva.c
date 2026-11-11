#include "thediva.h"

#include <stdlib.h>
#include <string.h>

#include <stdio.h>

the_diva_result_t queue_event(the_diva_state_t* state, the_diva_state_event_t* event)
{
    if (state->events_count == state->events_capacity)
    {
        size_t new_capacity = state->events_capacity ? state->events_capacity * 2 : 64;
        the_diva_state_event_t* grown = realloc(state->events, new_capacity * sizeof(the_diva_state_event_t));
        if (grown == NULL) return THE_DIVA_RESULT_ALLOCATION_FAILED;
        state->events = grown;
        state->events_capacity = new_capacity;
    }
    state->events[state->events_count++] = *event;
    return THE_DIVA_RESULT_OK;
}

the_diva_result_t queue_judgement_event(the_diva_state_t* state, the_diva_time_t time, size_t target_index, the_diva_target_judgement_t judgement, uint32_t wrong)
{
    the_diva_state_event_t event = {
        .type = THE_DIVA_STATE_EVENT_JUDGEMENT,
        .time = time
    };

    event.event.judgement.target_index = target_index;
    event.event.judgement.judgement = judgement;
    event.event.judgement.wrong = wrong;

    return queue_event(state, &event);
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

static the_diva_time_t flying_time_at(const the_diva_chart_t* chart, the_diva_time_t time)
{
    size_t lo = 0, hi = chart->flying_time_changes_count;
    while (lo < hi)
    {
        size_t mid = lo + (hi - lo) / 2;
        if (chart->flying_time_changes[mid].time <= time) lo = mid + 1;
        else hi = mid;
    }
    return lo == 0 ? 0 : chart->flying_time_changes[lo - 1].flying_time;
}


static int compare_target_order(const void* a, const void* b)
{
    const the_diva_target_order_t* x = a;
    const the_diva_target_order_t* y = b;
    if (x->hit_time != y->hit_time) return x->hit_time < y->hit_time ? -1 : 1;
    if (x->index != y->index) return x->index < y->index ? -1 : 1;
    return 0;
}

static void compute_hit_times(the_diva_state_t* state)
{
    const the_diva_chart_t* chart = state->chart;
 
    for (size_t i = 0; i < chart->targets_count; ++i)
    {
        the_diva_time_t spawn = chart->targets[i].time;
        state->hit_times[i] = spawn + flying_time_at(chart, spawn);
        state->order[i].hit_time = state->hit_times[i]; // gotta keep it here as well for sorting reasons
        state->order[i].index = i;
    }
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

    state->order = malloc(chart->targets_count * sizeof(the_diva_target_order_t));
    state->judgements = calloc(chart->targets_count, sizeof(the_diva_target_judgement_t));
    state->hit_times = malloc(chart->targets_count * sizeof(the_diva_time_t));

    compute_hit_times(state);
    qsort(state->order, chart->targets_count, sizeof(the_diva_target_order_t), compare_target_order);

    *out_state = state;
    return THE_DIVA_RESULT_OK;
}

void the_diva_state_destroy(the_diva_state_t** state)
{
    free((*state)->order);
    free((*state)->judgements);
    free((*state)->hit_times);
    free(*state);
    *state = NULL;
}

void set_current_time(the_diva_state_t* state, the_diva_time_t time)
{
    state->current_time = time + state->config->calibration_offset;
}

void judge_missed_notes(the_diva_state_t* state)
{
    while (state->order_cursor < state->chart->targets_count)
    {
        the_diva_target_order_t* entry = &state->order[state->order_cursor];
        if (state->judgements[entry->index] == THE_DIVA_TARGET_JUDGEMENT_NONE)
        {
            the_diva_time_t deadline = entry->hit_time + state->config->hit_window.sad;
            if (state->current_time <= deadline) break;

            state->judgements[entry->index] = THE_DIVA_TARGET_JUDGEMENT_MISS;
            queue_judgement_event(state, deadline, entry->index, THE_DIVA_TARGET_JUDGEMENT_MISS, THE_DIVA_FALSE);
        }

        state->order_cursor++;
    }
}

void process_flying_time_change(the_diva_state_t* state)
{
    while (state->flying_time_change_index < state->chart->flying_time_changes_count && state->chart->flying_time_changes[state->flying_time_change_index].time <= state->current_time)
    {
        state->current_flying_time = state->chart->flying_time_changes[state->flying_time_change_index].flying_time;
        state->flying_time_change_index++;
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

    for (size_t i = state->order_cursor; i < state->chart->targets_count; ++i)
    {
        the_diva_target_order_t* entry = &state->order[i];
        the_diva_target_t *target = &state->chart->targets[entry->index];

        if (state->judgements[entry->index] != THE_DIVA_TARGET_JUDGEMENT_NONE) continue;
        if (input_time > state->hit_times[entry->index] + state->config->hit_window.sad) continue;

        the_diva_time_t window = llabs(input_time - state->hit_times[entry->index]);
        if (window > state->config->hit_window.sad) return;

        the_diva_target_judgement_t judgement;
        if (window <= state->config->hit_window.cool) judgement = THE_DIVA_TARGET_JUDGEMENT_COOL;
        else if (window <= state->config->hit_window.fine) judgement = THE_DIVA_TARGET_JUDGEMENT_FINE;
        else if (window <= state->config->hit_window.safe) judgement = THE_DIVA_TARGET_JUDGEMENT_SAFE;
        else if (window <= state->config->hit_window.sad) judgement = THE_DIVA_TARGET_JUDGEMENT_SAD;

        size_t chord_start = target->chord_start;
        size_t chord_size = target->chord_size;

        the_diva_bool_t button_not_in_chord = THE_DIVA_TRUE;
        for (size_t j = chord_start; j < chord_start + chord_size; ++j)
        {
            if (state->judgements[j] != THE_DIVA_TARGET_JUDGEMENT_NONE) continue;
            if (state->chart->targets[j].button_type != button) continue;

            button_not_in_chord = THE_DIVA_FALSE;

            state->judgements[j] = judgement;
            queue_judgement_event(state, input_time, j, judgement, THE_DIVA_FALSE);

            if (state->chart->targets[j].hold == THE_DIVA_TRUE && state->chart->targets[j].button_type <= THE_DIVA_BUTTON_TYPE_SLIDE_RIGHT && state->holds[state->chart->targets[j].button_type] == 0)
            {
                state->holds[state->chart->targets[j].button_type] += 1;
            }
            return;
        }

        // fail entire chord in this case
        // right now i am judging all notes in the chord with the same judgement. Not sure how it works in the original game
        if (button_not_in_chord == THE_DIVA_TRUE)
        {
            for (size_t j = chord_start; j < chord_start + chord_size; ++j)
            {
                if (state->judgements[j] != THE_DIVA_TARGET_JUDGEMENT_NONE) continue;
                state->judgements[j] = judgement;
                queue_judgement_event(state, input_time, j, judgement, THE_DIVA_TRUE);
            }
            return;
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

the_diva_target_judgement_t the_diva_state_target_judgement(const the_diva_state_t* state, size_t target_index)
{
    if (target_index >= state->chart->targets_count) return THE_DIVA_TARGET_JUDGEMENT_NONE;
    return state->judgements[target_index];
}
 
the_diva_time_t the_diva_state_target_hit_time(const the_diva_state_t* state, size_t target_index)
{
    if (target_index >= state->chart->targets_count) return 0;
    return state->hit_times[target_index];
}

the_diva_result_t the_diva_state_event_poll(the_diva_state_t* state, the_diva_state_event_t* out_event)
{
    if (state == NULL || out_event == NULL) return THE_DIVA_RESULT_INVALID_ARGUMENT;
    
    if (state->events_read == state->events_count)
    {
        state->events_read = 0;
        state->events_count = 0;
        return THE_DIVA_RESULT_NO_EVENT;
    }

    *out_event = state->events[state->events_read++];
    return THE_DIVA_RESULT_OK;
}