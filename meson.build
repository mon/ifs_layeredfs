project('layeredfs', 'c', 'cpp', version: '3.4',
    default_options: [
        'cpp_std=c++17',
        'buildtype=release',
        'strip=true',
        'werror=true',
    ]
)

add_project_link_arguments('-static', language: 'cpp')
add_project_arguments('-DVER_STRING="' + meson.project_version() + '"', language: 'cpp')

third_party = static_library('3rd_party',
    sources: [
        'src/3rd_party/GuillotineBinPack.cpp',
        'src/3rd_party/Rect.cpp',
        'src/3rd_party/lodepng.cpp',
        'src/3rd_party/stb_dxt.cpp',
        'src/3rd_party/md5.cpp',

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

texbin_lib = static_library('texbin', 'src/texbin.cpp')
texbin_verbose_lib = static_library('texbin_verbose',
    'src/texbin.cpp',
    cpp_args: '-DTEXBIN_VERBOSE'
)

layeredfs_lib = static_library('layeredfs',
    sources: [
        'src/avs.cpp',
        'src/dllmain.cpp',
        'src/imagefs.cpp',
        'src/log.cpp',
        'src/modpath_handler.cpp',
        'src/ramfs_demangler.cpp',
        'src/texture_packer.cpp',
        'src/utils.cpp',
    ],
    link_with: third_party,
)

# has to be bare source so each special version gets a different copy
layeredfs_cfg_dep = declare_dependency(
    sources: [
        'src/config.cpp',
        'src/hook.cpp',
    ]
)

executable('playpen',
    sources: 'src/playpen.cpp',
    build_by_default: false,
    link_with: [layeredfs_lib, texbin_verbose_lib],
    dependencies: layeredfs_cfg_dep,
)

executable('texbin_debug',
    sources: 'src/texbin_debug.cpp',
    build_by_default: false,
    link_with: [layeredfs_lib, texbin_verbose_lib],
    dependencies: layeredfs_cfg_dep,
)

# "normal" hook
shared_library('ifs_hook',
    link_with: [layeredfs_lib, texbin_lib],
    dependencies: layeredfs_cfg_dep,
    name_prefix: '',
    install_dir: '/',
    install: true,
)

# pre-configured DLLs if you don't know how to (or can't) add cmdline args
special_cfgs = [
    ['always_verbose', ['-DCFG_VERBOSE']],
    ['always_logs_to_file', ['-DCFG_LOGFILE']],
    ['always_verbose_and_logs_to_file', ['-DCFG_VERBOSE','-DCFG_LOGFILE']],
    # useful because it flushes the logfile
    ['always_devmode_and_logs_to_file', ['-DCFG_DEVMODE', '-DCFG_LOGFILE']],
    # "Why isn't it working???"
    ['always_devmode_verbose_and_logs_to_file', ['-DCFG_DEVMODE', '-DCFG_VERBOSE', '-DCFG_LOGFILE']],
    # dump every file accessed via pkfs APIs to data_unpak
    ['pkfs_unpack', ['-DUNPAK']],
    # debug pkfs
    ['pkfs_unpack_always_devmode_verbose_and_logs_to_file', ['-DCFG_DEVMODE', '-DCFG_VERBOSE', '-DCFG_LOGFILE', '-DUNPAK']],
]

# documentation for pkfs_unpak
if host_machine.cpu_family() == 'x86'
    install_data(
        'pkfs_unpack_readme.txt',
        install_dir: '/special_builds/pkfs_unpack',
        install_tag: 'doc',
    )
endif

foreach cfg : special_cfgs
    folder_name = cfg[0]
    defines = cfg[1]

    if folder_name.contains('pkfs_unpack') and host_machine.cpu_family() != 'x86'
        continue
    endif

    shared_library('ifs_hook_' + folder_name,
        link_with: [layeredfs_lib, texbin_lib],
        dependencies: layeredfs_cfg_dep,
        cpp_args: [defines, f'-DSPECIAL_VER="@folder_name@"'],
        name_prefix: '',
        install_dir: '/special_builds' / folder_name,
        install: true,
    )
endforeach

injector_cfgs = [
    ['d3d9', '-DUSE_D3D9'],
    ['dxgi', '-DUSE_DXGI'],
    ['dxgi_for_bombergirl', ['-DUSE_DXGI', '-DBOMBERGIRL_BULLSHIT']],
    ['opengl32', '-DUSE_OGL'],
]

foreach cfg : injector_cfgs
    dll_name = cfg[0]
    define = cfg[1]
    def_file = 'src_injector' / dll_name + '.def'

    if dll_name == 'dxgi_for_bombergirl'
        if host_machine.cpu_family() != 'x86_64'
            continue
        endif
        def_file = 'src_injector/dxgi.def'
    endif

    shared_library(dll_name,
        sources: 'src_injector/dllmain.cpp',
        vs_module_defs: def_file,
        cpp_args: define,
        link_args: [
            # don't auto-export everything
            '-Wl,--exclude-all-symbols',
            # fix stdcall mangling even with `extern "C"`
            '-Wl,--enable-stdcall-fixup'
        ],
        name_prefix: '',
        install_dir: '/automatic_injector_dlls',
        install: true,
    )
endforeach
