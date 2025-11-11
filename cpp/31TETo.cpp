#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <iterator>
#include <math.h>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <limits.h>
#include <ctype.h>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

using Path = std::filesystem::path;
using std::string;
using std::string_view;
using std::unordered_map;

static bool ends_with(string_view str, string_view suffix) {
    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

static bool contains(string_view str, char c) {
    return str.find(c) != std::string_view::npos;
}

static bool contains(unordered_map<char, int> flags, char flag) {
    if (flags.find(flag) != flags.end()) {
        return 
    }
}

static int note_to_midi(string note) {
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

static string midi_to_note(int midi) {
    string out;
    std::array<const char*, 12> names = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

    out += names[midi % 12];
    
    out += std::to_string(midi / 12);
    return out;
}

static float midi_to_cents(int x, std::vector<float> scl={}) {
    if (scl.empty()) {
        scl = std::vector<float>({100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100, 1200});
    }

    return scl[(x-69) % scl.size()] + scl.back() * floor((x - 69) / scl.size());
}

static float catmull_rom(int p0, int p1, int p2, int p3, float t) {
    auto t2 = t * t;
    auto t3 = t2 * t;

    return 0.5 * (
        (2 * p1) +
        (-p0 + p2) * t +
        (2*p0 - 5*p1 + 4*p2 - p3) * t2 +
        (-p0 + 3*p1 - 3*p2 + p3) * t3
    );
}

static float midi_to_cents_catmull(float x, std::vector<float> scl={}, int offset=69) {
    if (scl.empty()) {
        scl = std::vector<float>({100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100, 1200});
    }

    int N = scl.size();
    int octaves = ((int)x - offset) / N;
    float pos = fmod(x - offset, N);
    int i1 = pos;

    int i0 = (i1 - 1) % N;
    int i2 = (i1 + 1) % N;
    int i3 = (i1 + 2) % N;
    float t = pos - i1;

    float cents = catmull_rom(scl[i0], scl[i1], scl[i2], scl[i3], t);

    cents += scl.back() * octaves;
    return cents;
}

struct Scale {
private:
    bool using_tun = false;
    bool using_scl = false;

    std::vector<float> tun_notes;
    std::vector<float> scl_notes;

    void parse_tun(Path fname) {
        tun_notes.resize(128);
        std::ifstream f(fname);
        if (!f) {
            std::cout << "Failed to open tun file " << fname << std::endl;
            return;
        }

        std::string s;
        using_tun = true;

        while (std::getline(f, s), s != "[Exact tuning]");
        
        while (std::getline(f, s)) {
            std::istringstream iss(s);

            std::string label;
            int index;
            char equals;
            float val;

            if (iss >> label >> index >> equals >> val) {
                tun_notes[index] = val;
            } else {
                std::cout << "Could not parse this line of tun file: " << s << std::endl;
            }
        }
    }

    void parse_scl(Path fname) {
        std::ifstream f(fname);
        if (!f) {
            std::cout << "Failed to open scl file " << fname << std::endl;
            return;
        }

        using_scl = true;

        string line;
        std::vector<string> lines;

        while (std::getline(f, line)) {
            if (line[0] == '!')
                continue;

            std::string word;
            std::istringstream (line) >> word;
            if (word.empty())
                continue;

            lines.push_back(word);
        }

        // lines[0] is ignored.
        int scale_size = std::stoi(lines[1]);
        
        for (auto p = lines.begin() + 2; p < lines.end(); ++p) {
            bool has_slash = contains(*p, '/');
            bool has_dot = contains(*p, '.');

            if (has_slash && has_dot) {
                std::cout << "Failed to parse scl value " << *p << std::endl;
                continue;
            }
            
            else if (has_slash) {
                auto slash_pos = (*p).find_first_of('/');
                auto num = (float) std::stoi((*p).substr(0, slash_pos));
                auto den = std::stoi((*p).substr(slash_pos + 1));

                scl_notes.push_back(log2(num / den) * 1200);
            }

            else if (has_dot) {
                scl_notes.push_back(std::stof(*p));
            }

            else {
                scl_notes.push_back(log2((float)std::stoi(*p)) * 1200);
            }
        }
    }

public:

    const float operator[](float i) {
        if (using_tun) {
            return midi_to_cents_catmull(i, tun_notes, 0);
        }

        if (using_scl) {
            return midi_to_cents_catmull(i, scl_notes);
        }

        throw std::logic_error("Didn't load a valid config file before using Scale!");
    }

    Scale(Path scale_path) {
        if (ends_with(scale_path.string(), ".tun")) {
            parse_tun(scale_path);
        }

        else if (ends_with(scale_path.string(), ".scl")) {
            parse_scl(scale_path);
        }

        else {
            std::cout << "Unsupported tuning file type, given: " << scale_path << std::endl; 
        }
    }
};

class Config {
public:
    unordered_map<int, string> executables;
    unordered_map<int, string> tunings;

    Config(Path config_path) {
        std::ifstream f(config_path);
        if (!f) {
            std::cout << "Failed to open file " << config_path;
            return;
        }
        string s {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
        f.close();

        int state = 1;
        int idx = 0;
        bool exclaimed = false;
        string fname;
        
        for (char c: s) switch (state) {
            case 1:
                if (c == '#')
                    state = 2;
                if (c == '!') {
                    exclaimed = true;
                    state = 3;
                }
                if ('0' <= c && c <= '9') {
                    idx = c - '0';
                    state = 3;
                }
            
            case 2:
                if (c == '\n')
                    state = 1;
            
            case 3:
                if ('0' <= c && c <= '9')
                    idx = idx * 10 + c - '0';
                if (c == ' ')
                    state = 4;
            
            case 4:
                if (c == '"') {
                    state = 8;
                }
                else if (c == '\n') {
                    if (exclaimed)
                        executables[idx] = fname;
                    else
                        tunings[idx] = fname;

                    exclaimed = false;
                    idx = 0;
                    fname = "";
                    state = 1;
                }
                else {
                    fname += c;
                }
            
            case 8:
                if (c == '"')
                    state = 4;
                else
                    fname += c;
        }
    }
};

static void うさげ(const char *name, const char *config_path) {
    printf(R"(%s in_file out_file pitch velocity flags offset length consonant cutoff volume modulation tempo pitchbend

Config file (%s):
    # config file example file
    # Format: <index/number> <path>
    # Paths can be absolute or relative to the config file
    # If you place a ! in front of a line, it will be used as a executable/resampler path

    !1 "C:\Program Files (x86)\UTAU\resampler.exe"
    1 "C:\5 equal divisions of 2_1 (1).tun"

Flags:
    # - edo
    $ - center note (MIDI note number)

    ^ - .tun file index (according to config, cannot be used with # or $)
    ! - executable/resampler index (according to config)

    ^ cannot be used in conjunction with # and $
    this only supports A=440hz
)"
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

static std::vector<int16_t> pitch_string_to_cents(const string& in) {

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

static string cents_to_pitch_string(const std::vector<int16_t>& in) {

    string out_str;

    int cnt = 1;

    std::vector<string> pairs;
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

static unordered_map<char, int> string_to_flags(const string& in) {
    unordered_map<char, int> flags;
    int sgn = 1;
    char last_c = -1;

    for (char c: in) {
        if (isalpha(c)) {
            flags[c] = 0;
            sgn = 1;
            last_c = c;
        } else if (c == '-') {
            sgn = -1;
        } else {
            if (last_c == -1)
                std::cout << "Flag string seems to be invalid: " << in << std::endl;
            else
                flags[last_c] = flags[last_c] * 10 + (c - '0');
        }
    }

    return flags;
}

int main(int argc, char *argv[]) {
  
    if (argc == 2) {
        Path config_path = argv[0];
        config_path = config_path.parent_path() / "config";

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

    // prog in_file out_file pitch velocity flags offset length consonant cutoff volume modulation tempo pitchbend

    Path config_path = argv[0];
    config_path = config_path.parent_path() / "config";
    Config config(config_path);

    // ignore argv[1]: in_file, argv[2]: out_file

    int pitch = note_to_midi(argv[3]);

    // ignore argv[4]: velocity

    unordered_map<char, int> flags = string_to_flags(argv[5]);
    
    // ignore argv[6] - argv[12]

    std::vector<int16_t> pitchbend_in = pitch_string_to_cents(argv[13]);
    std::vector<float> pitchbend_middle;
    std::vector<int16_t> pitchbend_out;

    for (const auto i: pitchbend_in)
        pitchbend_middle.push_back(detune(cfg, argv[3], static_cast<float>(i)));

    for (const auto f: pitchbend_middle) {
        if (f > SHRT_MAX)
            std::cout << "Frequency went above maximum!\n";
        if (f < SHRT_MIN)
            std::cout << "Frequency went below minimum!\n";

        pitchbend_out.push_back(static_cast<int16_t>(f));
    }
    
    std::vector<char *> cmd;

    if (config.executables.empty()) {
        std::cout << "Fatal error, no executables in config file " << config_path << std::endl;
        return 1;
    }
    
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
