#include "dsc.h"

#include <stdio.h>

the_diva_dsc_result_t get_opcode_parameters_count(uint32_t opcode, size_t* count)
{
    if (opcode >= THE_DIVA_DSC_OPCODE_ENUM_END || opcode < 0) return THE_DIVA_DSC_RESULT_INVALID_OPCODE;
    switch (opcode)
    {
        case 0x00: *count = 0; break;
        case THE_DIVA_DSC_OPCODE_TIME: *count = 1; break;
        case 0x02: *count = 4; break;
        case 0x03: *count = 2; break;
        case 0x04: *count = 2; break;
        case 0x05: *count = 2; break;
        case THE_DIVA_DSC_OPCODE_TARGET: *count = 7; break;
        case 0x07: *count = 4; break;
        case 0x08: *count = 2; break;
        case 0x09: *count = 6; break;
        case 0x0A: *count = 2; break;
        case 0x0B: *count = 1; break;
        case 0x0C: *count = 6; break;
        case 0x0D: *count = 2; break;
        case 0x0E: *count = 1; break;
        case 0x0F: *count = 1; break;
        case 0x10: *count = 3; break;
        case 0x11: *count = 2; break;
        case 0x12: *count = 3; break;
        case 0x13: *count = 5; break;
        case 0x14: *count = 5; break;
        case 0x15: *count = 4; break;
        case 0x16: *count = 4; break;
        case 0x17: *count = 5; break;
        case THE_DIVA_DSC_OPCODE_LYRIC: *count = 2; break;
        case THE_DIVA_DSC_OPCODE_MUSIC_PLAY: *count = 0; break;
        case 0x1A: *count = 2; break;
        case 0x1B: *count = 4; break;
        case THE_DIVA_DSC_OPCODE_BAR_TIME_SET: *count = 2; break;
        case 0x1D: *count = 2; break;
        case 0x1E: *count = 1; break;
        case 0x1F: *count = 21; break;
        case 0x20: *count = 0; break;
        case 0x21: *count = 3; break;
        case 0x22: *count = 2; break;
        case 0x23: *count = 5; break;
        case 0x24: *count = 1; break;
        case 0x25: *count = 1; break;
        case 0x26: *count = 7; break;
        case 0x27: *count = 1; break;
        case 0x28: *count = 1; break;
        case 0x29: *count = 2; break;
        case 0x2A: *count = 1; break;
        case 0x2B: *count = 2; break;
        case 0x2C: *count = 1; break;
        case 0x2D: *count = 2; break;
        case 0x2E: *count = 3; break;
        case 0x2F: *count = 3; break;
        case 0x30: *count = 1; break;
        case 0x31: *count = 2; break;
        case 0x32: *count = 2; break;
        case 0x33: *count = 3; break;
        case 0x34: *count = 6; break;
        case 0x35: *count = 6; break;
        case 0x36: *count = 1; break;
        case 0x37: *count = 1; break;
        case 0x38: *count = 2; break;
        case 0x39: *count = 3; break;
        case THE_DIVA_DSC_OPCODE_TARGET_FLYING_TIME: *count = 1; break;
        case 0x3B: *count = 2; break;
        case 0x3C: *count = 2; break;
        case 0x3D: *count = 4; break;
        case 0x3E: *count = 4; break;
        case 0x3F: *count = 1; break;
        case 0x40: *count = 2; break;
        case 0x41: *count = 1; break;
        case 0x42: *count = 2; break;
        case 0x43: *count = 1; break;
        case 0x44: *count = 1; break;
        case 0x45: *count = 3; break;
        case 0x46: *count = 3; break;
        case 0x47: *count = 3; break;
        case 0x48: *count = 2; break;
        case 0x49: *count = 1; break;
        case 0x4A: *count = 9; break;
        case 0x4B: *count = 3; break;
        case 0x4C: *count = 2; break;
        case 0x4D: *count = 4; break;
        case 0x4E: *count = 2; break;
        case 0x4F: *count = 3; break;
        case 0x50: *count = 2; break;
        case 0x51: *count = 24; break;
        case 0x52: *count = 1; break;
        case 0x53: *count = 2; break;
        case 0x54: *count = 1; break;
        case 0x55: *count = 3; break;
        case 0x56: *count = 1; break;
        case 0x57: *count = 3; break;
        case 0x58: *count = 4; break;
        case 0x59: *count = 1; break;
        case 0x5A: *count = 2; break;
        case 0x5B: *count = 6; break;
        case 0x5C: *count = 3; break;
        case 0x5D: *count = 2; break;
        case 0x5E: *count = 3; break;
        case 0x5F: *count = 3; break;
        case 0x60: *count = 4; break;
        case 0x61: *count = 1; break;
        case 0x62: *count = 1; break;
        case 0x63: *count = 3; break;
        case 0x64: *count = 3; break;
        case 0x65: *count = 4; break;
        case 0x66: *count = 2; break;
        case 0x67: *count = 3; break;
        case 0x68: *count = 3; break;
        case 0x69: *count = 8; break;
        case 0x6A: *count = 2; break;
    }
    return THE_DIVA_DSC_RESULT_OK;
}

the_diva_dsc_result_t the_diva_dsc_read_from_buffer(const char* dsc_file_buffer, size_t dsc_file_buffer_size, the_diva_dsc_command_t *commands, size_t *commands_length)
{
    size_t read_commands = 0;
    for (size_t i = 4; i < dsc_file_buffer_size; )
    {
        uint32_t opcode = *(uint32_t*)&dsc_file_buffer[i];
        i += 4;

        size_t parameters_count = 0;
        the_diva_dsc_result_t result = get_opcode_parameters_count(opcode, &parameters_count);
        if (result != THE_DIVA_DSC_RESULT_OK) return result;

        read_commands++;
    
        if (commands)
        {
            commands[read_commands - 1] = (the_diva_dsc_command_t){
                .opcode = opcode,
                .parameters_count = parameters_count,
                .parameters = (uint32_t*)&dsc_file_buffer[i]
            };
        }

        i += parameters_count * 4;
    }

    *commands_length = read_commands;

    return THE_DIVA_DSC_RESULT_OK;
}