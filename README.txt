The DLLs go next to the other game DLLs
The data_mod folder goes next to the data folder

Any files in the data folder that exist in data_mod will be loaded, with the
exception of ifs files. Instead, images are loaded as png from filename_ifs/
folders (they do not need to be inside a /tex/ folder inside that). See the
SDVX translation for example.

You do not need to modify your gamestart.bat - the hook is loaded automatically.

However, if you want more control, you can delete D3d9.dll and then load
ifs_hook.dll using your launcher/injector of choice.

Info is logged to ifs_hook.log
If something breaks, send that to mon
