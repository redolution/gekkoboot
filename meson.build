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

subdir('buildtools')
subdir('res')
objcopy = find_program('objcopy')
elf2dol = find_program('elf2dol')
dols = {}
compressed_exes = {}

linker_script = meson.current_source_dir() / 'ogc.ld'

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
    link_args: ['-T' + linker_script],
    link_depends: [linker_script],
    name_suffix: 'elf',
)
compressed_exes += {
    'iplboot': {
        'exe': iplboot,
        'link_args': [
            '-Wl,--section-start,.init=0x01300000'
        ],
        'dol': true,
    },
    'iplboot_sx': {
        'exe': iplboot,
        'link_args': [
            '-Wl,--section-start,.init=0x01500000'
        ],
    },
}

subdir('packer')

foreach name, exe: dols
    dol = custom_target(
        name + '_dol',
        input: exe,
        output: name + '.dol',
        command: [elf2dol, '@INPUT@', '@OUTPUT@'],
        build_by_default: true,
    )
    dols += {name: dol}
endforeach

if ipl_rom.length() != 0
    qoob_pro = custom_target(
        'qoob_pro',
        input: [dols['iplboot'], ipl_rom],
        output: 'iplboot_qoob_pro.gcb',
        command: [dol2ipl, '@OUTPUT@', '@INPUT@'],
        build_by_default: true,
    )
    alias_target('qoob_pro', qoob_pro)
else
    warning('IPL ROM not found, building for Qoob Pro will not be possible!')
endif

sx_exe = compressed_exes['iplboot_sx']['compressed']
qoob_sx_stripped = custom_target(
    sx_exe.name() + '_stripped',
    input: sx_exe,
    output: sx_exe.name() + '_stripped.elf',
    command: [objcopy, '-S', '@INPUT@', '@OUTPUT@'],
)
qoob_sx = custom_target(
    'qoob_sx',
    input: [qoob_sx_stripped, qoob_sx_updater],
    output: 'qoob_sx_iplboot_upgrade.elf',
    command: [dol2ipl, '@OUTPUT@', '@INPUT@'],
    build_by_default: true,
)
alias_target('qoob_sx', qoob_sx)

viper = custom_target(
    'viper',
    input: dols['iplboot'],
    output: 'iplboot_viper.vgc',
    command: [dol2ipl, '@OUTPUT@', '@INPUT@'],
    build_by_default: true,
)
alias_target('viper', viper)

pico = custom_target(
    'pico',
    input: dols['iplboot'],
    output: 'iplboot_pico.uf2',
    command: [dol2ipl, '@OUTPUT@', '@INPUT@'],
    build_by_default: true,
)
alias_target('pico', pico)
