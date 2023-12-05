project(
    'iplboot',
    ['c', 'cpp'],
    meson_version: '>=1.2',
    default_options: {
        'optimization': 's',
    },
)

libogc_proj = subproject(
    'libogc2',
    default_options: {
        'libraries': ['ogc'],
    },
)
libogc_deps = libogc_proj.get_variable('deps')

subdir('buildtools')
subdir('res')
objcopy = find_program('objcopy')
elf2dol = find_program('elf2dol')

# Add executables to this mapping to generate corresponding DOLs.
# The variable '@key@_dol' will be set for later use.
dols = {}

# Add executables to this mapping as {'exe':, 'link_args':, 'dol':} to compress them.
# The variable '@key@_compressed' will be set for later use.
# The optional key 'dol' is a boolean that controls whether to add the compressed
# executable to `dols`.
compressed_exes = {}

subdir('stub')

linker_script = meson.current_source_dir() / 'ogc.ld'

iplboot = executable(
    'iplboot',
    'source/main.c',
    'source/utils.c',
    'source/fatfs/ff.c',
    'source/fatfs/ffsystem.c',
    'source/fatfs/ffunicode.c',
    'source/ffshim.c',
    dependencies: [
        libogc_deps['ogc'],
        stub_dep,
    ],
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
            '-Wl,--section-start,.init=0x01500000',
            '-Wl,--nmagic',
        ],
    },
    'iplboot_lowmem': {
        'exe': iplboot,
        'dol': true,
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
    set_variable(name + '_dol', dol)
endforeach

if full_rom_opt.allowed()
    qoob_pro = custom_target(
        'qoob_pro',
        input: [iplboot_dol, ipl_rom],
        output: 'iplboot_qoob_pro.gcb',
        command: [dol2ipl, '@OUTPUT@', '@INPUT@'],
        build_by_default: true,
    )
    alias_target('qoob_pro', qoob_pro)
endif

iplboot_sx_stripped = custom_target(
    iplboot_sx_compressed.name() + '_stripped',
    input: iplboot_sx_compressed,
    output: iplboot_sx_compressed.name() + '_stripped.elf',
    command: [objcopy, '-S', '@INPUT@', '@OUTPUT@'],
)
qoob_sx = custom_target(
    'qoob_sx',
    input: [iplboot_sx_stripped, qoob_sx_updater],
    output: 'qoob_sx_iplboot_upgrade.elf',
    command: [dol2ipl, '@OUTPUT@', '@INPUT@'],
    build_by_default: true,
)
alias_target('qoob_sx', qoob_sx)

viper = custom_target(
    'viper',
    input: iplboot_dol,
    output: 'iplboot_viper.vgc',
    command: [dol2ipl, '@OUTPUT@', '@INPUT@'],
    build_by_default: true,
)
alias_target('viper', viper)

pico = custom_target(
    'pico',
    input: iplboot_dol,
    output: 'iplboot_pico.uf2',
    command: [dol2ipl, '@OUTPUT@', '@INPUT@'],
    build_by_default: true,
)
alias_target('pico', pico)

gci = custom_target(
    'gci',
    input: iplboot_lowmem_dol,
    output: 'iplboot_memcard.gci',
    command: [dol2gci, '@INPUT@', '@OUTPUT@', 'boot.dol'],
    build_by_default: true,
)
alias_target('gci', gci)
