For games that use pkfs functionality (gfdm/jubeat 2008/jubeat ripples), this
will export every accessed file to a `data_unpak` folder so you can easily
extract it using gitadora-textool and make mods.

You can tell if your game uses pkfs if the `data` folder contains
`finfolist.bin`, a `pack` folder, and a bunch of `.pak` files.

Because .pak files do not contain a table of filenames, you have to brute-force
filenames to fully extract them. This is why there isn't an easy one-stop tool
to extract everything for editing. The "pakdump" github project comes close, but
misses a few files.
