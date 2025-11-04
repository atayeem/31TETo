#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <filesystem>
#include "cJSON.h"

/*
TODO (in order of importance):
- Add subprocess execution, probably using std::system()
- Finish configuration logic
- Figure out how to do clean pitch shifting (possibly by changing the fundamental note)
  (this would be based on the average of the whole sample, the note can not change during)
  (a sample.)
- Streamline error printing
- Add support for .scl files
- Add support for .tun files

Part 2:
- Parse .ustx format.
- Figure out how to smoothly interpolate notes.
*/

float midi_to_cents_12edo(int midi_note) {
    return 100 * (midi_note - 69);
}

// This the kind of code they would write for the PDP-11
int note_to_midi(std::string note) {
    const char *s = note.c_str();
    int n, octv, sgn;
    switch (s[0]) {
        case 'C': n = 0; break;
        case 'D': n = 2; break;
        case 'E': n = 4; break;
        case 'F': n = 5; break;
        case 'G': n = 7; break;
        case 'A': n = 9; break;
        case 'B': n = 11; break;
    }

    // A-1
    if (s[1] == '-') {
        sgn = -1;
        octv = s[3] - '0';
    }

    else if (s[1] == '#') {
        
        // A#-1
        if (s[2] == '-') {
            sgn = -1;
            octv = s[3] - '0';
        }
        // A#1
        else {
            sgn = 1;
            octv = s[2] - '0';
        }
    }

    // A1
    else {
        sgn = 1;
        octv = s[1] - '0';
    }

    return (sgn * octv + 1) * 12 + n;
}

struct Config {
    /* The cent difference between each note from C-1 to B9 and its detuned version. */
    std::array<float, 132> mapping;

    /* The scale itself in kind of scl-style, where it does not include the first note, just all */
    /* the cent differences between each note and the first one. Does not necessarily have 12 notes. */
    std::vector<float> Scale;

    /* How much the `Z` integer flag changes the note. */
    float StepSize;

    /* For example, standard tuning is ReferencePitch=440.0 */
    float ReferencePitch;
    std::string ReferencePitchName;

    std::vector<std::string> ExecPath;

private:
    /* Calculate scale degree in cents */
    float scl(int n) {
        int scale_steps = Scale.size() - 1;
        int octave = n / scale_steps;
        int degree = n % scale_steps;
        float equave = Scale.back();

        return Scale[degree] + octave * equave;
    }

    /* I don't know how this works. */
    void generate_mapping() {
        int ref = note_to_midi(ReferencePitchName);
        for (int i = 0; i < 132; i++) {
            mapping[i] = 1200 * log2(ReferencePitch/440.0) + 100 * (69 - ref) + scl(i) - midi_to_cents_12edo(i);
        }
    }

    void defconfig() {
        ReferencePitch = 440.0;
        StepSize = 1200.0 / 31.0;
        
        // 31-edo chromatic with # meaning +2 (meantone)
        float steps[11] = {2, 5, 7, 10, 13, 15, 18, 20, 23, 25, 28};

        std::for_each_n(steps, sizeof(steps) / sizeof(float), [](float &n){n *= 1200.0/31;});
        std::copy_n(steps, 12, Scale);
    }
public:
    Config(std::filesystem::path config_path) {

        if (!std::filesystem::exists(config_path)) {
            std::ofstream config_file(config_path);
            if (!config_file) {
                std::cout << "[31TETo] JSON config could not be created at " << config_path << "\n";
                
            } else {
                std::cout << "[31TETo] Created new config file at " << config_path << "\n";
                config_file.close();
            }
            std::cout << "[31TETo] Continuing with defconfig\n";
            defconfig();

        } else {
            std::ifstream config_file(config_path);
            std::string contents(std::istreambuf_iterator<char>(config_file), {});

            cJSON* data = cJSON_Parse(contents.c_str());

            if (!data) {
                std::cout << "[31TETo] JSON config at " << config_path << " is malformed: " << cJSON_GetErrorPtr() << "\n";
                std::cout << "[31TETo] Continuing with defconfig.\n";
                defconfig();
            }

            else {
                const cJSON* focus = cJSON_GetObjectItemCaseSensitive(data, "StepSize");
                if (cJSON_IsNumber(focus)) {
                    StepSize = focus->valuedouble;
                } else {
                    std::cout << "[31TETo] StepSize isn't a number.\n";
                    StepSize = pow(2, 1/31);
                }

                focus = cJSON_GetObjectItemCaseSensitive(data, "ReferencePitch");
                if (cJSON_IsNumber(focus)) {
                    ReferencePitch = focus->valuedouble;
                } else {
                    std::cout << "[31TETo] ReferencePitch isn't a number.\n";
                    ReferencePitch = 440.0;
                }

                focus = cJSON_GetObjectItemCaseSensitive(data, "ReferencePitchName");
                if (cJSON_IsString(focus) && (focus->valuestring != nullptr) ) {
                    ReferencePitchName = cJSON_GetStringValue(focus);
                } else {
                    std::cout << "[31TETo] ReferencePitchName isn't a string.\n";
                    ReferencePitchName = "A";
                }

                focus = cJSON_GetObjectItemCaseSensitive(data, "Scale");
                if (cJSON_IsArray(focus)) {
                    const cJSON* degree;
                    cJSON_ArrayForEach(degree, focus) {
                        if (cJSON_IsNumber(degree)) {
                            Scale.push_back(degree->valuedouble);
                        } else {
                            std::cout << "[31TETo] element of Scale array isn't a number.\n";
                        }
                    }
                } else if (cJSON_IsString(focus) && (focus->valuestring != nullptr)) {
                    std::cout << "[31TETo] Scale is a string: Opening file to get scale not yet supported.\n";
                } else {
                    std::cout << "[31TETo] Scale is not a string or array. Continuing with defconfig.\n";
                    defconfig();
                }

                cJSON_Delete(data);
            }
            config_file.close();
        }

        generate_mapping();
    }
};

static void usage(const char *name) {
    printf(
        "%s in_file out_file pitch velocity flags offset length consonant"
        " cutoff volume modulation tempo pitchbend\n\n"
        "Config file:\n"
        "   "
        "Flags:\n"
        "Z:  (default 0) (no limits) change pitch by integer number of steps of size StepSize.\n"
        "     Z flag will not be passed to resampler. If this is a problem, please let me know.\n"
        
        , name
    );
}

static constexpr auto make_b64_table() {
    std::array<unsigned char, 256> table{};
    for (unsigned i = 0; i < 256; ++i) {
        table[i] =
            (i >= 'A' && i <= 'Z') ? i - 'A' :
            (i >= 'a' && i <= 'z') ? i - 'a' + 26 :
            (i >= '0' && i <= '9') ? i - '0' + 52 :
            (i == '+') ? 62 :
            (i == '/') ? 63 :
            255;
    }
    return table;
}

static constexpr auto b64_to_i = make_b64_table();

static const unsigned char i_to_b64[256] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::vector<int16_t> pitch_string_to_cents(const std::string& in) {

    std::vector<int16_t> out;

    /* 3 state DFA */

    int reps = 0;
    int state = 1;
    uint16_t val;
    int16_t nval;
    int i = 0;
    while (in[i]) {
        switch (state) {
        case 1:
            val = b64_to_i[in[i]];

            if (in[i] == '#')
                state = 3;
            else
                state = 2;
            break;
            
        case 2:
            val = (val << 6) + b64_to_i[in[i]];
            out.push_back((int16_t)(val << 4) >> 4);

            state = 1;
            break;
        
        case 3:
            if (in[i] == '#') {
                state = 1;
                for (int i = 0; i < reps - 1; i++)
                    out.push_back(out.back());
                
            }
            else {
                reps = reps * 10 + in[i] - '0';
            }
            break;
        }

        i++;
    }

    return out;
}

static std::string cents_to_pitch_string(const std::vector<int16_t>& in) {

    std::string out_str;

    int cnt = 1;

    std::vector<std::string> pairs;
    for (int16_t i: in) {
        uint16_t n = i & 0xFFF;
        char app[3];
        app[0] = i_to_b64[n >> 6];
        app[1] = i_to_b64[n & 0x3F];
        app[2] = 0;

        pairs.push_back(app);
    }

    auto last_pair = pairs[0];

    for (int i = 1; i < pairs.size(); i++) {
        auto pair = pairs[i];

        if (pair == last_pair) {
            cnt++;
        } 
        else {
            if (cnt < 6)
                for (int _=0; _<cnt;_++) out_str += last_pair;
            else
                out_str += last_pair + "#" + std::to_string(cnt) + "#";

            cnt = 1;
            last_pair = pair;
        }
    }

    if (cnt < 6)
        for (int _=0; _<cnt;_++) out_str += last_pair;
    else
        out_str += last_pair + "#" + std::to_string(cnt) + "#";

    return out_str;
}

int main(int argc, char *argv[]) {

    if (argc == 2) {
        usage(argv[0]);
        return 1;
    }

    if (argc != 14) {
        fprintf(stderr, "[31TETo] Incorrect number of command line arguments given. Expected 14, got %d", argc);
        return 1;
    }

    std::filesystem::path config_path = argv[0];
    config_path = config_path.parent_path() / "31config.txt";

    Config cfg(config_path);

    std::vector<int16_t> pitchbend = pitch_string_to_cents(argv[13]);


    return 0;
}