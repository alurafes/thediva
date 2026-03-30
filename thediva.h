#ifndef THE_DIVA_H_
#define THE_DIVA_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
- THE DIVA core
A core library for Project DIVA (Arcade/FT) clone

The library only does the main gameplay part. No visuals, no parsers, no anything. Just time keeping and input processing.
*/

typedef uint32_t the_diva_bool_t;
#define THE_DIVA_TRUE 1
#define THE_DIVA_FALSE 0

typedef enum {
    THE_DIVA_RESULT_OK,
    THE_DIVA_RESULT_INVALID_ARGUMENT,
    THE_DIVA_RESULT_ALLOCATION_FAILED,
    THE_DIVA_RESULT_NO_EVENT,
} the_diva_result_t;

// time is tracked in microseconds. Can be negative due to the calibration offset
typedef int64_t the_diva_time_t;

#define THE_DIVA_US(us)   ((the_diva_time_t)(us))
#define THE_DIVA_MS(ms)   ((the_diva_time_t)((ms) * 1000LL))
#define THE_DIVA_SEC(s)   ((the_diva_time_t)((s) * 1000000LL))

typedef enum {
    THE_DIVA_BUTTON_TYPE_TRIANGLE,
    THE_DIVA_BUTTON_TYPE_CIRCLE,
    THE_DIVA_BUTTON_TYPE_CROSS,
    THE_DIVA_BUTTON_TYPE_SQUARE,
    THE_DIVA_BUTTON_TYPE_SLIDE_LEFT,
    THE_DIVA_BUTTON_TYPE_SLIDE_RIGHT,
    THE_DIVA_BUTTON_TYPE_SLIDE_CHAIN_LEFT,
    THE_DIVA_BUTTON_TYPE_SLIDE_CHAIN_RIGHT
} the_diva_button_type_t;

typedef enum {
    THE_DIVA_TARGET_JUDGEMENT_NONE,
    THE_DIVA_TARGET_JUDGEMENT_COOL,
    THE_DIVA_TARGET_JUDGEMENT_FINE,
    THE_DIVA_TARGET_JUDGEMENT_SAFE,
    THE_DIVA_TARGET_JUDGEMENT_SAD,
    THE_DIVA_TARGET_JUDGEMENT_MISS,
} the_diva_target_judgement_t;

typedef struct {
    uint32_t id;
    the_diva_button_type_t button_type;
    the_diva_time_t time;
    the_diva_target_judgement_t judgement;
    size_t chord_start;
    size_t chord_size;
    float x;
    float y;
    float angle;
    float distance;
    uint32_t amplitude;
    uint32_t frequency;
} the_diva_target_t;

typedef struct {
    the_diva_target_t* target;
    the_diva_bool_t wrong;
} the_diva_target_judgement_event_t;

typedef struct {
    the_diva_time_t time;
    the_diva_time_t flying_time;
} the_diva_flying_time_change_t;

typedef struct {
    the_diva_target_t* targets;
    size_t targets_count;
    the_diva_flying_time_change_t* flying_time_changes;
    size_t flying_time_changes_count;
    the_diva_time_t duration;
} the_diva_chart_t;

typedef struct {
    the_diva_time_t cool;
    the_diva_time_t fine;
    the_diva_time_t safe;
    the_diva_time_t sad;
} the_diva_state_config_hit_window;

typedef struct {
    the_diva_time_t calibration_offset;
    the_diva_time_t lookahead_time;
    the_diva_state_config_hit_window hit_window;
} the_diva_state_config_t;

typedef struct the_diva_state_t the_diva_state_t;

the_diva_result_t the_diva_state_config_fill_default(the_diva_state_config_t* out_state_config);

// the library owns the_diva_state_t
the_diva_result_t the_diva_state_create(the_diva_chart_t* chart, the_diva_state_config_t* state_config, the_diva_state_t** out_state);
void the_diva_state_destroy(the_diva_state_t** state);

void the_diva_state_tick(the_diva_state_t* state, the_diva_time_t time);
void the_diva_state_press(the_diva_state_t* state, the_diva_button_type_t button, the_diva_time_t time);
// todo: add the_diva_state_release for holds/slides
the_diva_time_t the_diva_state_current_flying_time(the_diva_state_t* state);
the_diva_result_t the_diva_state_judgement_event_poll(the_diva_state_t* state, the_diva_target_judgement_event_t* out_target_judgement_event);

#ifdef __cplusplus
}
#endif


#endif // THE_DIVA_H_