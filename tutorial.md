# How to use 31TETo in OpenUTAU

## First, an example


## What does 31TETo do?
It remaps the scale in the editor to the notes of whatever scale you specify, in a direct way that can seem weird at first. For example, if I set the # flag to 24, it will have this behavior:
```
C4  -> C4
C#4 -> Ct4
D4  -> C#4
D#4 -> Dd4
E4  -> D4
```
For big EDOs, this is definitely unwieldy, so it also has support for both .scl and .tun file loading. But you might not like this, because it removes a bunch of notes that you could use! So it also provides a Z and z flag, Z controls how big a step is in cents, z controls how many of these steps to offset by.

## What are resampler flags

Go into one of these.  \
<img width="449" height="179" alt="image" src="https://github.com/user-attachments/assets/b90e93c1-4049-49c0-8650-ee396a9090b9" />\
At the bottom you will see this section. Click on the gear icon.  \
<img width="1031" height="292" alt="image" src="https://github.com/user-attachments/assets/0395b29d-1cbe-419a-b3c9-f7d30c02d8af" />\
Click on any of these. In OpenUTAU these are called expressions. Some of them are also resampler flags.  \
<img width="595" height="413" alt="image" src="https://github.com/user-attachments/assets/4901c08a-a861-4c51-9767-baa422c9d826" />\
It is a resampler flag if there is a letter where it says "Resampler flag", in this case the letter is 'B'.  

What this means is that this value will be given to the resampler.  \
<img width="374" height="416" alt="image" src="https://github.com/user-attachments/assets/58da762a-e509-4166-8fe3-e227c0012b2b" />\
As you can see, the bottom area can be used to set this value for each note.  

Usually, these will be used for normal singing things, like voicing and gender, but 31TETo uses them to change its microtonal retuning.

### Add a flag
Click the plus button here:  \
<img width="187" height="201" alt="image" src="https://github.com/user-attachments/assets/24257e16-3bf5-4b39-84fd-884cf12d9e6c" />

This will open a menu like this:  \
<img width="441" height="179" alt="Executable" src="https://github.com/user-attachments/assets/995343c6-9af9-4632-aa99-fe5fd7517796" />\
That's how you specify options in this program. Using the default values. Here's how to set up the other ones as well.  \
<img width="436" height="170" alt="Resampler" src="https://github.com/user-attachments/assets/c5cdec37-24fb-42c7-b81e-ecf4a666ebca" />\
<img width="433" height="171" alt="Tuning file" src="https://github.com/user-attachments/assets/06e302c5-ac2c-42f8-a7bc-5e7eb346ad8b" />\
<img width="432" height="173" alt="Size of EDOstep" src="https://github.com/user-attachments/assets/6683bef9-e4df-4afc-9a1c-d9fd05c56467" />\
Hey! notice how this one has only a range of -2 to 2? This makes it much easier to use. You never need to detune by more than 3 EDOsteps, so it's easier to select this way.  \
<img width="431" height="178" alt="EDOsteps shift" src="https://github.com/user-attachments/assets/2b205eb8-9e79-48ba-94f5-5541682e2b50" />\
You never need to use this. Unless you know why you need it.  \
<img width="435" height="179" alt="Center note" src="https://github.com/user-attachments/assets/aff1f5d2-5a9d-494c-9873-fee64d5f609b" />


## Instructions

1. Find the Resamplers folder. Here's one way to do it,  
<img width="382" height="138" alt="image" src="https://github.com/user-attachments/assets/92024120-2eb3-452c-979a-1f2107895f9d" /> \
Click on 'Open Logs Location', and go back one folder, then open the Resamplers folder.

2. Start by writing your config file. The config file should be called 'config' and it is in the Resamplers folder. You'll have to make a new file called 'config', not 'config.txt'.
3. Here's an example of a valid config file:
```
# this is a comment
!1 moresampler.exe
!2 SillySampler.exe
1 C:\Users\silly\Documents\my_scale.scl
2 augdimhextrug.tun
```
What does this mean? Any line that starts with `#` will be ignored. The program won't read them. A line that starts with an exclamation mark and a number will have to name an exe file. If you only write the name of the file, it will try to find it in the Resamplers folder. So in this case, I have an exe file called `moresampler.exe` in my Resamplers folder. I have a tun file called `augdimhextrug.tun`.

Every line should have a number before it. It's up to you to decide what to put. Tuning files are specified on lines that don't have exclamation marks, and these are optional, as you will soon see. You need at least one exe file, though.

4. Set up OpenUTAU.
You'll need flags like these:
```
Flags:
    REQUIRED: ! flag is required, and either # or ^ flag is required. Other flags are optional.

    # - edo
    $ - center note (MIDI note number, default=60)

    ^ - .tun/.scl file index (according to config, cannot be used with # or $)
    ! - executable/resampler index (according to config)

    Z - the size of one step in cents
    z - how many steps to detune

    Example: Z = 39 (1200/31), and z = -2, it will be pushed down by 2 steps of 31edo.
    By default, Z is determined by the # flag, otherwise it is 0.

    NOTE: ^ cannot be used in conjunction with # and $
    NOTE: this only supports A=440hz
```
Go to the previous section for information on how to do this. Remember that the name and short name for them doesn't matter. Choose anything. The only thing that matters is the default value and the resampler flag. \

The key point is that ^ has to be set to a valid tuning file in your config file, based on the number you wrote. Same thing with !, except for numbers with exclamation marks before them.

5. Make sure to choose the right resampler  \
   Click the gear icon here: \
   <img width="203" height="96" alt="image" src="https://github.com/user-attachments/assets/4473534d-1ee6-46e9-ae47-73f61edaeae1" />\
   And make sure it is set to 31TETo (Linux) or 31TETo.exe (Windows): \
   <img width="320" height="220" alt="image" src="https://github.com/user-attachments/assets/0060011f-517b-47e6-8099-af801ebfc83f" />

6. Just render! It should generate some nice microtonal harmony!

## Z flag
When you set the EDO flag `#`, it will be taken care of automatically, and you can immediately use `z`. If you are instead using `^` to choose a tuning file, set your `Z` flag to the amount you want in cents, which can be calculated from an EDO using `cents = 1200/EDO`. \

With the `z` flag, you might do something like this:  \
<img width="295" height="396" alt="image" src="https://github.com/user-attachments/assets/ee600025-1470-45f4-be11-c2f50a3acac3" />\

# Other
You can ask me (@retail_trash) a question about this on my [Discord server](https://discord.gg/k3Cp7kW6Jv).

## microtau
microtau's functionality is exactly a subset of 31TETo's functionality. It only lacks .scl support, and the z and Z flags. Projects in both will work the same.
