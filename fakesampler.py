#!/usr/bin/env python3

RESAMPLER = ["wine", "/home/atayeem/.local/share/OpenUtau/Resamplers/moresampler.exe"]

import sys, subprocess

b64 = {
    "+": 62,
    "/": 63
}

def _init_b64():
    A = ord("A")
    a = ord("a")
    zero = ord("0")

    for i in range(26):
        b64[chr(a + i)] = 26 + i
        b64[chr(A + i)] = i

    for i in range(10):
        b64[chr(zero + i)] = 52 + i
    
    global anti_b64
    anti_b64 = {value: key for key, value in b64.items()}


def pitch_string_to_cents(s: str) -> list[int]:
    def u12_to_s12(x: int) -> int:
        return x - 4096 if x & 0x800 else x
    
    def b64_pair_to_int(p: str) -> int:
        return u12_to_s12(b64[p[0]] * 64 + b64[p[1]])
    
    def b64_pairs_to_ints(pairs: str) -> list[int]:
        ret: list[int] = []
        for i in range(0, len(pairs), 2):
            ret.append(b64_pair_to_int(pairs[i:i+2]))
        
        return ret

    out = []

    # run length decode
    parts = s.split('#')
    out = []
    for i in range(0, len(parts), 2):
        chunk = parts[i:i+2]
        if len(chunk) == 2:
            ps, run = chunk
            out += b64_pairs_to_ints(ps)
            out += [out[-1]] * (int(run) - 1)
        else:
            out += b64_pairs_to_ints(chunk[0])

    return out
    
def cents_to_pitch_string(x: list[int]) -> str:

    def s12_to_u12(x: int) -> int:
        return x & 0xFFF

    def int_to_b64_pair(x: int) -> str:
        x = s12_to_u12(x)
        return anti_b64[x >> 6] + anti_b64[x & 0x3F]
    
    out_str: str = ""

    cnt = 1

    pairs = [int_to_b64_pair(i) for i in x]

    last_pair = pairs[0]

    for pair in pairs[1:]:
        if pair == last_pair:
            cnt += 1
        else:
            if cnt < 6:
                out_str += last_pair * cnt
            else:
                out_str += f"{last_pair}#{cnt}#"

            cnt = 1
            last_pair = pair
    
    if cnt < 6:
        out_str += last_pair * (cnt)
    else:
        out_str += f"{last_pair}#{cnt}#"
        
    return out_str

def ints_to_floats(x: list[int]):
    return [float(i) for i in x]

def floats_to_ints(x: list[float]):
    out: list[int] = []
    display_warning = False
    for f in x:
        i = int(f)
        if i > 2047:
            display_warning = True
            out.append(2047)
        
        elif i < -2048:
            display_warning = True
            out.append(-2048)

        else:
            out.append(i)
    
    if display_warning:
        print("[fakesampler] Warning: frequency clipped out of range!", file=sys.stderr)
    
    return out

# To preserve the meantone property that C# < Db, sharp (#) corresponds to going up by 2\32.
map_31: dict[str, float] = {
    "C": 0,
    "C#": 2,
    "D": 5,
    "D#": 7,
    "E": 10,
    "F": 13,
    "F#": 15,
    "G": 18,
    "G#": 20,
    "A": 23,
    "A#": 25,
    "B": 28
}

# assumption: 31EDO
for i, (key, value) in enumerate(map_31.items()):
    l31 = 1200 / 31
    map_31[key] = l31 * value - 100 * i

# assumption: 31EDO
def note_string_to_detune_amount(s: str) -> float:
    note = s[:-1]
    return map_31[note]

# Remove only the Z flag, get the value of it, and reconstruct the flags string without it.
# assumption: 31EDO
def flags_to_new_flags_and_detune(flags: str) -> tuple[str, float]:
    flags_dict: dict[str, int] = {}
    sgn = 1
    last_c = None
    for c in flags:
        if c.isalpha():
            flags_dict[c] = 0
            sgn = 1
            last_c = c
        elif c == '-':
            sgn = -1
        else:
            if not last_c:
                raise ValueError("Flag string seems to be invalid: " + flags)
            
            flags_dict[last_c] = flags_dict[last_c] * 10 + int(c)
    
    if "Z" in flags_dict:
        out_f = sgn * (1200 / 31) * flags_dict["Z"]
        flags_dict.pop("Z")
    else:
        out_f = 0.0
    
    out_s = ""

    for key, value in flags_dict.items():
        out_s += f"{key}{value}"
    
    return (out_s, out_f)


# resampler in_file out_file pitch velocity [flags] [offset] [length] [consonant] [cutoff] [volume] [modulation] [tempo] [pitchbend]
def main(argc: int, argv: list[str]):
    _init_b64()
    
    if argc != 14:
        raise ValueError(f"Wrong number of arguments given ({argc}, rather than 14).\nInvocation: " + " ".join(argv))
    
    pitchbend = pitch_string_to_cents(argv[13])

    detune = note_string_to_detune_amount(argv[3])
    new_flags, flag_detune = flags_to_new_flags_and_detune(argv[5])
    pitchbend = cents_to_pitch_string(floats_to_ints([p + detune + flag_detune for p in pitchbend]))

    new_argv = RESAMPLER + argv[1:5] + [new_flags] + argv[6:13] + [pitchbend]
    subprocess.run(new_argv)

if __name__ == "__main__":
    main(len(sys.argv), sys.argv)
