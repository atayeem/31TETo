# 31TETo
A hacky way to add support for microtones to UTAU/OpenUTAU.

Config file (it's in the resamplers folder):
    # config file example file
    # Format: <index/number> <path>
    # Paths can be absolute or relative to the config file
    # If you place a ! in front of a line, it will be used as a executable/resampler path

    !1 "C:\Program Files (x86)\UTAU\resampler.exe"
    1 "C:\5 equal divisions of 2_1 (1).tun"

Flags:
    REQUIRED: ! flag is required, and either # or ^ flag is required. Other flags are optional.

    # - edo
    $ - center note (MIDI note number, default=60)

    ^ - .tun file index (according to config, cannot be used with # or $)
    ! - executable/resampler index (according to config)

    Z - the size of one step in cents
    z - how many steps to detune

    Example: Z = 39 (1200/31), and z = -2, it will be pushed down by 2 steps of 31edo.
    By default, Z is determined by the # flag, otherwise it is 0.

    NOTE: ^ cannot be used in conjunction with # and $
    NOTE: this only supports A=440hz
