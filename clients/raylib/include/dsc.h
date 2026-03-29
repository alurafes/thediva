#ifndef THE_DIVA_DSC_H_
#define THE_DIVA_DSC_H_

// todo: move this out of this particular client

#include <stdlib.h>
#include <stdint.h>

typedef enum {
    THE_DIVA_DSC_RESULT_OK,
    THE_DIVA_DSC_RESULT_INVALID_OPCODE
} the_diva_dsc_result_t;

typedef enum {
    THE_DIVA_DSC_OPCODE_TIME = 0x01,
    THE_DIVA_DSC_OPCODE_TARGET = 0x06,
    THE_DIVA_DSC_OPCODE_LYRIC = 0x18,
    THE_DIVA_DSC_OPCODE_MUSIC_PLAY = 0x19,
    THE_DIVA_DSC_OPCODE_BAR_TIME_SET = 0x1C,
    THE_DIVA_DSC_OPCODE_TARGET_FLYING_TIME = 0x3A,
    THE_DIVA_DSC_OPCODE_ENUM_END = 0x6B
} the_diva_dsc_opcode_t;

typedef struct {
    uint32_t opcode;
    size_t parameters_count;
    const uint32_t *parameters;
} the_diva_dsc_command_t;

the_diva_dsc_result_t the_diva_dsc_read_from_buffer(const char* dsc_file_buffer, size_t dsc_file_buffer_size, the_diva_dsc_command_t *commands, size_t *commands_length);

#endif // THE_DIVA_DSC_H_