packer_crt = static_library(
    'packer_crt',
    'crt0.S',
    c_args: ['-mregnames'],
    pic: false,
)

packer_stub = static_library(
    'packer',
    'main.c',
    c_args: [
        '-msdata',
        '-G', '32768',
        '-ffreestanding',
    ],
    override_options: {
        'c_std': 'gnu17',
	'b_lto': true,
    },
    link_with: packer_crt,
    dependencies: [
        # For the headers
        libogc_deps['ogc'].partial_dependency(
            compile_args: true,
            sources: true,
        ),
    ],
    pic: false,
)

packer_linker_script = meson.current_source_dir() / 'link.ld'

stat = find_program('stat')
seven_zip = find_program('7z')

foreach name, exe: compressed_exes
    bin = custom_target(
        name + '_bin',
        input: exe['exe'],
        output: name + '.bin',
        command: [objcopy, '-O', 'binary', '@INPUT@', '@OUTPUT@'],
    )

    bin_size = custom_target(
        name + '_bin_size',
        input: bin,
        output: name + '.bin_size.ld',
        capture: true,
        command: [stat, '--format=executable_bin_size = %s;', '@INPUT@'],
    )

    bin_xz = custom_target(
        name + '_bin_xz',
        input: bin,
        output: name + '.bin.xz',
        capture: true,
        command: [seven_zip, 'a', 'dummy.xz', '-so', '@INPUT@', '-mf=ppc', '-mx=9'],
    )

    bin_xz_o = custom_target(
        name + '_bin_xz_o',
        input: bin_xz,
        output: name + '.bin.xz.o',
        command: [
            objcopy,
            '-I', 'binary',
            '-O', 'default',
            '@INPUT@',
            '@OUTPUT@',
            '--prefix-sections=exe_xz',
        ],
    )

    out = executable(
        name + '_compressed',
        bin_xz_o,
        link_with: packer_stub,

        link_args: [
            '-T' + bin_size.full_path(),
            '-T' + packer_linker_script,
            exe.get('link_args', []),
        ],
        link_depends: [
            bin_size,
            packer_linker_script,
        ],

        name_suffix: 'elf',
    )

    set_variable(name + '_compressed', out)
    if exe.get('dol', false)
        dols += {name: out}
    endif
endforeach
