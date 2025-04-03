# IFS LayeredFS

Never repack textures again! IFS LayeredFS is a modding engine for Konami arcade
games that use libavs (which is basically everything in the last 15 years).

It intercepts file accesses and replaces them with modded files in the
`data_mods` folder, so mods don't have to be installed directly into the game
folder.

For IFS files (a common texture container format) and texbin files (used instead
of IFS in Gitadora, Jubeat and maybe some other games), it will take plain PNG
images and automatically repack them into the game-specific format. This allows
rapid development of texture mods, as the unergonomic texture packing process
can be completely bypassed.

# Supported games

Game support is based on AVS version, not any particular game, and LayeredFS
supports almost all in-use AVS versions. However, these are games that LayeredFS
should work on:
- Beatmania IIDX (9th Style (untested) to latest release)
- Sound Voltex (all versions)
- DDR (X (untested) to latest release)
- pop'n music (15, ADVENTURE (untested) to latest release)
- jubeat (all versions)
- GFDM (v4 to XG3)
- GITADORA (all versions)
- MÚSECA
- DANCERUSH
- REFLEC BEAT
- BeatStream
- Nostalgia
- Bombergirl

If you find a game that doesn't work, please open an issue!

# Installation

The DLLs go next to the other game DLLs. Pick the right DLL for your game - if
32 bit fails to load, try 64 bit. The `data_mods` folder goes next to the data
folder.

For most games, you do not need to modify your gamestart.bat - the hook is
loaded automatically by copying `D3d9.dll` to the install directory. For
**Jubeat** and **GFDM** (_not_ Gitadora), you'll need to use `opengl32.dll`
instead. If neither of these work and the game is very new, try `dxgi.dll`. For
**Bombergirl**, it has a special DLL loader that likes to break things. You will
need to use `dxgi_for_bombergirl.dll` (renamed to `dxgi.dll`) **AND** copy
`dxgi.dll` from `C:\Windows\System32` (renamed to `_dxgi.dll`).

If you don't really care, you can copy all the injector DLLs into the game
folder, it won't break anything.

For more control, load ifs_hook.dll using your launcher/injector of choice
(eg: add `-k ifs_hook.dll` to the spice commandline, or `-K ifs_hook.dll` to
the bemanitools commandline)

## GFDM: an extra note

As discovered in [this issue](https://github.com/mon/ifs_layeredfs/issues/5),
GFDM tools are currently very basic and don't perform some important path
remapping. While the game looks in `/data`, layeredfs can only see `/bin`. To
fix this, hex-edit `boot.dll`, and change `2E 2E 2F 00` -> `2E 2F 00 00`, then
move all the modules out of `bin` into the main game folder.

# Where to put mod files / how textures work / edge cases

Here are some examples of the correct path to use for your mod:

### IFS files

`data/graphics/ver04/logo_j.ifs`
- To replace repacked IFS: `data_mods/example/graphics/ver04/logo_j.ifs`
- To replace individual texture "warning":
  - `data_mods/example/graphics/ver04/logo_j_ifs/tex/warning.png`
  - *or*
  - `data_mods/example/graphics/ver04/logo_j_ifs/warning.png`

### Jubeat or Gitadora .bin textures

`data/d3/model/tex_l44fo_continue.bin`
- To replace repacked .bin: `data_mods/example/d3/model/tex_l44fo_continue.bin`
- To replace individual texture "CTN_T0000": `data_mods/example/d3/model/tex_l44fo_continue/CTN_T0000.png`

### XML files

`data/others/music_db.xml`
- To replace the entire file: `data_mods/example/others/music_db.xml`
- To append to the file: `data_mods/example/others/music_db.merged.xml`

### Games with a `data2` folder (Beatstream/Nostalgia)

`data/graphic/information.ifs`
- It's the same as normal! `data_mods/example/graphic/information.ifs`

`data2/graphic/information.ifs`
- It needs an extra subfolder: `data_mods/example/data2/graphic/information.ifs`

See [MOD_README.txt](data_mods/MOD_README.txt) for more details.

# Flags

If you can't work out to enable a flag, the release .zip contains DLLs that have
certain useful combos pre-enabled. They can be found in the `special_builds`
folder.
```
--layered-disable Disable layeredfs
--layered-verbose Print *tons* of info. Useful to find why your mods aren't
                  loaded
--layered-devmode Instead of caching, check the data_mods folder for every file.
                    A little slower, but lets you modify mods on-the-fly
--layered-allowlist=a,b,c,d
--layered-blocklist=a,b,c,d
                  If an allowlist is present, ONLY those folders in the
                  allowlist are included in the loaded mods list.
                  If a blocklist is present, folders in the blocklist are
                  excluded from the loaded mods list.
                  Folders cannot have "," in their name if using allow/blocklist
--layered-logfile=filename.log
                  Use a custom, separate logfile instead of the game's log.
--layered-data-mods-folder=./some_folder
                  Use a custom mods folder instead of the default ./data_mods
                  MUST start with "./" to avoid path weirdness.
```

# Logs

Info is logged to to the game's logging system. However, if something breaks
before LayeredFS gets a chance to set this up, it will create `ifs_hook.log`
with details. If something breaks, send the game log and ifs_hook.log (if it
exists) to mon.

# Building

This code has grown organically and is some of the worst C that I have ever
distributed. The code itself is more or less fine, but the organisation leaves
a lot to be desired. There are many vestigal remains of past AVS experiments.

Prior to version 3.0, it was built with Visual Studio 2017 with the 141_xp
toolchain. However, in VS 2019 16.7, they broke this (it pulls in Vista-only
APIs), so I gave up and moved to mingw-w64. I currently just have that installed
in an Ubuntu install in WSL. If it ever breaks, I'll make a docker or nix image
or something... Feel free to open an issue if you're struggling to build it.

As long as you have Meson and mingw-w64 installed, it should be as simple as
running `build.sh`.

# Contributing

I encourage pull requests, but might take a while to properly test them prior
to merge. If you can show your PR works with SDVX/IIDX < 24/IIDX >= 25, I will
probably accept it.

Please read the `LICENSE`. It is nonstandard.
