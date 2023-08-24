project('layeredfs', 'c', 'cpp', version: '3.0_BETA2',
    default_options: [
        'cpp_std=c++17',
        'buildtype=release',
        'strip=true',
        'werror=true',
    ]
)

add_project_link_arguments('-static', language: 'cpp')

third_party = static_library('3rd_party',
    sources: [
        'src/3rd_party/GuillotineBinPack.cpp',
        'src/3rd_party/Rect.cpp',
        'src/3rd_party/lodepng.cpp',
        'src/3rd_party/stb_dxt.cpp',

        'src/3rd_party/libsquish/alpha.cpp',
        'src/3rd_party/libsquish/clusterfit.cpp',
        'src/3rd_party/libsquish/colourblock.cpp',
        'src/3rd_party/libsquish/colourfit.cpp',
        'src/3rd_party/libsquish/colourset.cpp',
        'src/3rd_party/libsquish/maths.cpp',
        'src/3rd_party/libsquish/rangefit.cpp',
        'src/3rd_party/libsquish/singlecolourfit.cpp',
        'src/3rd_party/libsquish/squish.cpp',

        'src/3rd_party/minhook/src/buffer.c',
        'src/3rd_party/minhook/src/hde/hde32.c',
        'src/3rd_party/minhook/src/hde/hde64.c',
        'src/3rd_party/minhook/src/hook.c',
        'src/3rd_party/minhook/src/trampoline.c',
    ],
    # ignore warnings in 3rd party libs
    cpp_args: '-w',
)

layeredfs_lib = static_library('layeredfs',
    sources: [
        'src/avs.cpp',
        'src/config.cpp',
        'src/dllmain.cpp',
        'src/hook.cpp',
        'src/log.cpp',
        'src/modpath_handler.cpp',
        'src/playpen.cpp',
        'src/ramfs_demangler.cpp',
        'src/texbin.cpp',
        'src/texture_packer.cpp',
        'src/utils.cpp',
    ],
    link_with: third_party,
    cpp_args: '-DVER_STRING="' + meson.project_version() + '"',
)

shared_library('ifs_hook',
    sources: 'src/dllmain.cpp',
    link_with: layeredfs_lib,
    name_prefix: '',
)

executable('playpen',
    sources: 'src/playpen.cpp',
    build_by_default: false,
    link_with: layeredfs_lib
)
