#include <cstdint>
#include <fstream>
#include <ios>
#include <iostream>
#include <filesystem>
#include <ranges>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>

namespace dsc {
    namespace exception 
    {
        class parsing_failure : public std::runtime_error {
            public:
                explicit parsing_failure(const std::string& message) : std::runtime_error("DSC Parsing failed: " + message) {}
        };
        class invalid_opcode : public std::runtime_error {
            public:
                explicit invalid_opcode(uint32_t opcode) : std::runtime_error("Invalid opcode " + std::to_string(opcode)) {}
        };
    }

    enum opcode {
        DSC_OPCODE_TIME = 0x01,
        DSC_OPCODE_TARGET = 0x06,
        DSC_OPCODE_LYRIC = 0x18,
        DSC_OPCODE_MUSIC_PLAY = 0x19,
        DSC_OPCODE_BAR_TIME_SET = 0x1C,
        DSC_OPCODE_TARGET_FLYING_TIME = 0x3A,
        DSC_OPCODE_ENUM_END = 0x6B
    };

    uint32_t get_opcode_parameters_count(uint32_t opcode)
    {
        if (opcode >= DSC_OPCODE_ENUM_END || opcode < 0) throw exception::invalid_opcode(opcode);
        constexpr std::array<uint32_t, DSC_OPCODE_ENUM_END> opcode_parameters = [] {
            std::array<uint32_t, DSC_OPCODE_ENUM_END> values = {};
            values[0x00] = 0;
            values[DSC_OPCODE_TIME] = 1;
            values[0x02] = 4;
            values[0x03] = 2;
            values[0x04] = 2;
            values[0x05] = 2;
            values[DSC_OPCODE_TARGET] = 7;
            values[0x07] = 4;
            values[0x08] = 2;
            values[0x09] = 6;
            values[0x0A] = 2;
            values[0x0B] = 1;
            values[0x0C] = 6;
            values[0x0D] = 2;
            values[0x0E] = 1;
            values[0x0F] = 1;
            values[0x10] = 3;
            values[0x11] = 2;
            values[0x12] = 3;
            values[0x13] = 5;
            values[0x14] = 5;
            values[0x15] = 4;
            values[0x16] = 4;
            values[0x17] = 5;
            values[DSC_OPCODE_LYRIC] = 2;
            values[DSC_OPCODE_MUSIC_PLAY] = 0;
            values[0x1A] = 2;
            values[0x1B] = 4;
            values[DSC_OPCODE_BAR_TIME_SET] = 2;
            values[0x1D] = 2;
            values[0x1E] = 1;
            values[0x1F] = 21;
            values[0x20] = 0;
            values[0x21] = 3;
            values[0x22] = 2;
            values[0x23] = 5;
            values[0x24] = 1;
            values[0x25] = 1;
            values[0x26] = 7;
            values[0x27] = 1;
            values[0x28] = 1;
            values[0x29] = 2;
            values[0x2A] = 1;
            values[0x2B] = 2;
            values[0x2C] = 1;
            values[0x2D] = 2;
            values[0x2E] = 3;
            values[0x2F] = 3;
            values[0x30] = 1;
            values[0x31] = 2;
            values[0x32] = 2;
            values[0x33] = 3;
            values[0x34] = 6;
            values[0x35] = 6;
            values[0x36] = 1;
            values[0x37] = 1;
            values[0x38] = 2;
            values[0x39] = 3;
            values[DSC_OPCODE_TARGET_FLYING_TIME] = 1;
            values[0x3B] = 2;
            values[0x3C] = 2;
            values[0x3D] = 4;
            values[0x3E] = 4;
            values[0x3F] = 1;
            values[0x40] = 2;
            values[0x41] = 1;
            values[0x42] = 2;
            values[0x43] = 1;
            values[0x44] = 1;
            values[0x45] = 3;
            values[0x46] = 3;
            values[0x47] = 3;
            values[0x48] = 2;
            values[0x49] = 1;
            values[0x4A] = 9;
            values[0x4B] = 3;
            values[0x4C] = 2;
            values[0x4D] = 4;
            values[0x4E] = 2;
            values[0x4F] = 3;
            values[0x50] = 2;
            values[0x51] = 24;
            values[0x52] = 1;
            values[0x53] = 2;
            values[0x54] = 1;
            values[0x55] = 3;
            values[0x56] = 1;
            values[0x57] = 3;
            values[0x58] = 4;
            values[0x59] = 1;
            values[0x5A] = 2;
            values[0x5B] = 6;
            values[0x5C] = 3;
            values[0x5D] = 2;
            values[0x5E] = 3;
            values[0x5F] = 3;
            values[0x60] = 4;
            values[0x61] = 1;
            values[0x62] = 1;
            values[0x63] = 3;
            values[0x64] = 3;
            values[0x65] = 4;
            values[0x66] = 2;
            values[0x67] = 3;
            values[0x68] = 3;
            values[0x69] = 8;
            values[0x6A] = 2;
            return values;
        }();
        return opcode_parameters[opcode];
    }

    class dsc_script 
    {
        public:
            explicit dsc_script(std::string dsc_path) 
            : dsc_path(dsc_path) {
                auto dsc_file = std::ifstream{dsc_path, std::ios::binary};
                if (!dsc_file.is_open()) throw exception::parsing_failure("couldn't open file " + std::string{dsc_path});

                this->parse_dsc(dsc_file);

                dsc_file.close();
            }
        private:
            std::string dsc_path;
            std::vector<uint32_t> read_opcode_parameters(std::ifstream& file, uint32_t count) const {
                std::vector<uint32_t> parameters(count);
                for (uint32_t i = 0; i < count; ++i)
                {
                    uint32_t parameter;
                    file.read(reinterpret_cast<char*>(&parameter), sizeof(parameter));
                    parameters.push_back(parameter);
                }
                return parameters;
            }
            void parse_dsc(std::ifstream& file) {
                file.seekg(4, std::ios_base::cur);

                uint32_t opcode;
                while (file.read(reinterpret_cast<char*>(&opcode), sizeof(opcode)))
                {
                    try {
                        auto parameters_count = get_opcode_parameters_count(opcode);
                        auto parameters = read_opcode_parameters(file, parameters_count);
                        std::cout << "opcode " << std::hex << opcode << " - " << std::dec << parameters_count << " parameters\n";
                    } catch (exception::invalid_opcode error) {
                        throw exception::parsing_failure("got invalid opcode in dsc: " + std::to_string(opcode));
                    }
                }
            }

        friend std::ostream& operator<<(std::ostream& os, const dsc_script& dsc);
    };

    std::ostream& operator<<(std::ostream& os, const dsc_script& dsc) 
    {
        os << dsc.dsc_path;
        return os;
    }
}

int main()
{
    auto dscs = std::filesystem::directory_iterator{"./build/dsc/"} 
        | std::views::filter([&](const auto& p){
            return p.is_regular_file() && p.path().extension() == ".dsc";
        })
        | std::views::transform([&](const auto& p){
            return dsc::dsc_script{p.path()};
        });
    for (const auto& dsc : dscs) {
        std::cout << dsc << '\n';
    }
    return 0;
}