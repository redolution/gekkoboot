project(
    'iplboot',
    ['c'],
    meson_version: '>=1.2',
    default_options: {
        'optimization': 's',
    },
)

libogc_proj = subproject(
    'libogc2',
    default_options: {
        'opt_level': get_option('optimization'),
        'libraries': ['ogc'],
    },
)
libogc_deps = libogc_proj.get_variable('deps')

elf2dol = find_program('elf2dol')
dols = []

iplboot = executable(
    'iplboot',
    'source/main.c',
    'source/stub.c',
    'source/utils.c',
    'source/fatfs/ff.c',
    'source/fatfs/ffsystem.c',
    'source/fatfs/ffunicode.c',
    'source/ffshim.c',
    dependencies: libogc_deps['ogc'],
    name_suffix: 'elf',
)
dols += [iplboot]

foreach exe: dols
    custom_target(
        exe.name() + '_dol',
        input: exe,
        output: exe.name() + '.dol',
        command: [elf2dol, '@INPUT@', '@OUTPUT@'],
        build_by_default: true,
    )
endforeach
