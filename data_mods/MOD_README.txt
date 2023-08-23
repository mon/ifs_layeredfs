IFS LayeredFS will search every directory in this folder for modded files to use.

Search order
  Alphabetical via mod folder name
  The first folder with a matching mod will be used for that file
  This is done on a per-file basis
  If two mods change the same IFS but different images, there are no conflicts
  EXAMPLE:
    data_mods/Boltex/test_ifs/logo.png
    data_mods/Loltex/test_ifs/logo.png
    data_mods/Loltex/test_ifs/title.png
    Game asks for logo image : Boltex used
    Game asks for title image: Loltex used


Folder structure
  Mods get their own folder, eg "english" or "custom_songs"
  The structure inside that folder is identical to the "data" folder
  Any files that exist here will be used instead

Special case: IFS files and their textures
  IFS file contents can be modded by replacing ".ifs" with "_ifs" and creating a folder.
  (This is the folder name you get by using ifstools to extract a .ifs file)
  Most textures live inside the "tex" folder eg "somefile_ifs/tex/image.png"
    You can just use "somefile_ifs/image.png" for neater folders
  Replacement images must match the dimensions of the image they are replacing
    If not, the error is logged and the image is not used
  Images that do not exist in the original IFS will be added to its texture list
    This is especially useful for preview jackets

Special case: IFS files that do not exist
  IFS LayeredFS cannot create IFS files out of nothing
  Either use a packed ".ifs", or modify an existing file with "_ifs"

Special case: "texbin" files (.bin textures from Jubeat and Gitadora)
  Texbin file contents can be modded by deleting ".bin" and creating a folder.
  (This is the folder name you get by using gitadora-texbintool to extract a .bin file)
  Replacement images must match the dimensions of the image they are replacing
    If not, the error is logged and the image is not used
  Images that do not exist in the original .bin will be added to its texture list
  Gitadora's fancy texture packing/arrangement is handled automatically

Special case: "texbin" files that do not exist
  Unlike IFS files, LayeredFS will create a .bin file for you even if the original
  does not exist!

Special case: XML files
  XML files with the same name as the default will be used normally
  XML files that end in ".merged.xml" instead of ".xml" will be merged into the default
    eg: music_db.merged.xml
  Merged files must have valid structure (<?xml ... ?> header and single root)
    eg: music_db.merged.xml must have the "<mdb>" root element

The _cache folder
  Any files that are adjusted are placed into the _cache folder for redirection
  It can be deleted at any time
  Textures are compressed as they are loaded for the first time
    First launch compression of textures may slow down first launch a little

Mods that don't live in data/
  Some games (Beatstream, Nostalgia) have extra data folders (data2, data_op2)
  These subfolders must exist in the mod folder if you want to mod them.
  For example, for a Beatstream mod using data2:
    data_mods/
      my_cool_mod/
        graphic/
          some_mod_for_data
        data2/
          graphic/
            some_mod_for_data2
