#include <array>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>

def note_to_midi(n):
    m = note_re.match(n)
    if not m: raise ValueError(f"Bad note '{n}'")
    nm, octv = m.groups()
    return (int(octv)+1)*12 + notes[nm]

def midi_to_hz(m):
    return 440.0 * 2**((m-69)/12)

int note_to_midi(std::string note) {
    static std::unordered_map<std::string, int> map; 
}

struct Config {
    /* The cent difference between each note from C-1 to B9 and its detuned version. */
    std::array<float, 132> mapping;

    /* The scale itself in kind of scl-style, where it does not include the first note, just all */
    /* the cent differences between each note and the first one. Does not necessarily have 12 notes. */
    std::vector<float> Scale;

    /* How much the `Z` integer flag changes the note. */
    float StepSize;

    /* Value of A4. */
    float ReferencePitch;


    std::vector<std::string> exec_path;

    Config(std::filesystem::path config_path) {

        if (!std::filesystem::exists(config_path)) {
            std::ofstream config_file(config_path);
            if (!config_file) {
                std::cerr << "[31TETo] Failed to create config file. defaulting to 31edo.\n";
            } else {
                std::cout << "[31TETo] Created config file at " << config_path << "\n";
                config_file << "ReferencePitch=450.0\n";
                config_file << "Scale=[31ED2]\n";
                config_file << "StepSize=[1\\31]\n";
                config_file << "Resampler=wine,moresampler.exe\n";

                config_file.close();
            }
        } else {

        }
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
                    out.push_back(out[out.size() - 1]);
                
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

static void load_config(Config &cfg) {

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