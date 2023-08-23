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

# Installation
The DLLs go next to the other game DLLs. Pick the right DLL for your game - if
32 bit fails to load, try 64 bit. The `data_mods` folder goes next to the data
folder.

For most games, you do not need to modify your gamestart.bat - the hook is
loaded automatically by copying D3d9.dll to the install directory. This
*doesn't* work for Jubeat though, you need to use `-k ifs_hook.dll` in your
commandline.

For more control (or if playing Jubeat) -
Load ifs_hook.dll using your launcher/injector of choice (eg: `-k ifs_hook.dll`)

# Flags
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
```

# Where to put mod files / how textures work / edge cases
See [MOD_README.txt](data_mods/MOD_README.txt) in the `data_mods` folder.

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
