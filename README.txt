Installation:
The DLLs go next to the other game DLLs
The data_mods folder goes next to the data folder

You do not need to modify your gamestart.bat - the hook is loaded automatically.

However, if you want more control, you can delete D3d9.dll and then load
ifs_hook.dll using your launcher/injector of choice.

Info is logged to ifs_hook.log
If something breaks, send the log to mon

Usage:
Please see MOD_README.txt under "data_mods"