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
--layered-verbose Print *tons* of info. Useful to find why your mods aren't loaded
--layered-devmode Instead of caching, check the data_mods folder for every file.
                   A little slower, but lets you modify mods on-the-fly

Info is logged to ifs_hook.log
If something breaks, send the log to mon

Usage:
Please see MOD_README.txt under "data_mods"