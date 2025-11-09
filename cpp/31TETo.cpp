#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <math.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <filesystem>
#include <limits.h>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

static int note_to_midi(std::string note) {
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

std::string midi_to_note(int midi) {
    std::string out;
    std::array<const char*, 12> names = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

    out += names[midi % 12];
    
    out += std::to_string(midi / 12);
    return out;
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
        int scale_steps = Scale.size();
        int octave = n / scale_steps;
        int degree = n % scale_steps;
        float equave = Scale.back();

        return Scale[degree] + (octave + 1) * equave;
    }

    void generate_mapping() {
        int ref = note_to_midi(ReferencePitchName);
        for (int i = 0; i < 132; i++) {
            mapping[i] =
                1200 * log2(ReferencePitch / 440.0)   // reference pitch shift
              + scl(i - ref)                          // scale offset from reference note
              - 100 * (i - 69);                       // remove equal temperament baseline
        }
    }

    void defconfig() {
        ReferencePitch = 440.0;
        StepSize = 1200.0 / 31.0;
        
        // 31-edo chromatic with # meaning +2 (meantone)
        float steps[11] = {2, 5, 7, 10, 13, 15, 18, 20, 23, 25, 28};

        std::for_each_n(steps, sizeof(steps) / sizeof(float), [&](float n){Scale.push_back(1200.0/31 * n);});
        generate_mapping();
    }
public:
    Config(std::filesystem::path config_path) {
    }
};

static void うさげ(const char *name, const char *config_path) {
    printf(
        "%s in_file out_file pitch velocity flags offset length consonant"
        " cutoff volume modulation tempo pitchbend\n\n"
        "Config file (%s) :\n"
        "    StepSize: float: the size of each step of Z in cents (default 1200/31)\n"
        "    ReferencePitch: float: The reference pitch (default 440.0hz)\n"
        "    ReferencePitchName: string: The specific note that is tuned to the reference pitch (default \"A4\")\n"
        "    Scale: array[float] or string: The list of intervals in the scale relative to the root in cents, excluding the root.\n"
        "           or the filename of the tuning file used (not supported yet). (default 12-note subset of 31edo)\n"
        "    ExecPath: array[string]: Command line to execute resampler (required) (example: [\"wine\", \"~/bin/resampler.exe\"])\n\n"
        "Flags:\n"
        "Z:  (default 0) (no limits) change pitch by integer number of steps of size StepSize.\n"
        "    Z flag will be passed to resampler. If this is a problem, please let me know.\n"
        , name, config_path
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
    int i = 0;
    while (in[i]) {
        switch (state) {
        // The 0th letter of the pair.
        case 1:
            val = b64_to_i[in[i]];
            if (in[i] == '#')
                state = 3;
            else
                state = 2;
            break;
        
        // The 1st letter of the pair.
        case 2:
            val = (val << 6) + b64_to_i[in[i]];
            out.push_back((int16_t)(val << 4) >> 4);

            state = 1;
            break;
        
        // Count up the repetitions until we hit another `#`.
        case 3:
            if (in[i] == '#') {
                state = 1;
                for (int i = 0; i < reps - 1; i++)
                    out.push_back(out.back());
                reps = 0;
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

    for (size_t i = 1; i < pairs.size(); i++) {
        auto pair = pairs[i];

        if (pair == last_pair) {
            cnt++;
        } 
        else {
            if (cnt < 2)
                for (int _=0; _<cnt;_++) out_str += last_pair;
            else
                out_str += last_pair + "#" + std::to_string(cnt) + "#";

            cnt = 1;
            last_pair = pair;
        }
    }

    if (cnt < 2)
        for (int _=0; _<cnt;_++) out_str += last_pair;
    else
        out_str += last_pair + "#" + std::to_string(cnt) + "#";

    return out_str;
}

static float detune(const Config &cfg, const std::string& note_name, float cents) {
    return cents + cfg.mapping[note_to_midi(note_name)];
}

int main(int argc, char *argv[]) {

    std::filesystem::path config_path = argv[0];
    config_path = config_path.parent_path() / "31config.txt";
  
    if (argc == 2) {
        auto s = config_path.string();
        うさげ(argv[0], s.c_str());
        return 1;
    }

    if (argc != 14) {
        std::cout << "[31TETo] Incorrect number of command line arguments given. Expected 14, got " << argc;
        return 1;
    }

    #ifdef DEBUG
    std::cout << "set +H; valgrind ";
    for (int i = 0; i < argc - 2; i++) {
        std::cout << argv[i] << " ";
    }
    std::cout << "'" << argv[12] << "' '" << argv[13] << "'" << std::endl;
    #endif

    Config cfg("/home/atayeem/Programming/atayeem/31TETo/example.json");
//  Config cfg(config_path);

    std::vector<int16_t> pitchbend_i = pitch_string_to_cents(argv[13]);

    std::vector<float> pitchbend;
    for (const auto i: pitchbend_i)
        pitchbend.push_back(detune(cfg, argv[3], static_cast<float>(i)));

    pitchbend_i.clear();

    #ifdef DEBUG
    for (int i = 0; i < 132; i++) {
        std::cout << midi_to_note(i) << ": " << cfg.mapping[i] << "\n";
    }
    #endif

    for (const auto f: pitchbend) {
        if (f > SHRT_MAX)
            std::cout << "Frequency went above maximum!\n";
        if (f < SHRT_MIN)
            std::cout << "Frequency went below minimum!\n";

        pitchbend_i.push_back(static_cast<int16_t>(f));
    }
    
    std::vector<char *> cmd;
    
    // Put the executable name in.
    for (const auto& arg: cfg.ExecPath) {
        cmd.push_back(const_cast<char *>(arg.c_str()));
    }

    // Put the rest of the arguments
    for (int i = 1; i < argc - 1; i++) {
        cmd.push_back(argv[i]);
    }

    // Finally, put the pitch bend and null terminator
    auto new_pitch_string  = cents_to_pitch_string(pitchbend_i);

    std::cout << "OLD " << argv[13] << std::endl;
    std::cout << "NEW " << new_pitch_string << std::endl;

    cmd.push_back(const_cast<char *>(new_pitch_string.c_str()));
    cmd.push_back(nullptr);

    #ifdef _WIN32
    _spawnvp(_P_OVERLAY, cfg.ExecPath[0].c_str(), cmd.data());    
    #else
    execvp(cfg.ExecPath[0].c_str(), cmd.data());
    #endif
}
