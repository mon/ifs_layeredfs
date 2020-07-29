Installation:
The DLLs go next to the other game DLLs
Pick the right DLL for your game - if 32 bit fails to load, try 64 bit
The data_mods folder goes next to the data folder

For 32 bit ONLY -
  you do not need to modify your gamestart.bat - the hook is loaded automatically
  using D3d9.dll

For 64 bit, or more control with 32 bit -
Load ifs_hook.dll using your launcher/injector of choice (eg: -k ifs_hook.dll)

Flags:
--layered-disable Disable layeredfs
--layered-verbose Print *tons* of info. Useful to find why your mods aren't loaded
--layered-devmode Instead of caching, check the data_mods folder for every file.
                    A little slower, but lets you modify mods on-the-fly
--layered-allowlist=a,b,c,d
--layered-blocklist=a,b,c,d
                  If an allowlist is present, ONLY those folders in the allowlist
                    are included in the loaded mods list.
                  If a blocklist is present, folders in the blocklist are excluded
                    from the loaded mods list.
                  Folders cannot have "," in their name if using allow/blocklist

Info is logged to ifs_hook.log
If something breaks, send the log to mon

Usage:
Please see MOD_README.txt under "data_mods"
