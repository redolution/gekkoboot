stub_linker_script = meson.current_source_dir() / 'link.ld'

stub_exe = executable(
    'stub',
    'stub.c',
    c_args: [
        '-fPIC',
        '-ffreestanding',
        '-mno-eabi',
        '-Wno-uninitialized',
    ],
    link_args: [
        '-nostdlib',
        '-T' + stub_linker_script,
    ],
    link_depends: stub_linker_script,
    dependencies: [
        # For the headers
        libogc_deps['ogc'].partial_dependency(
            compile_args: true,
            sources: true,
        ),
    ],
)

stub_bin = custom_target(
    'stub_bin',
    input: stub_exe,
    output: 'stub.bin',
    command: [objcopy, '-O', 'binary', '@INPUT@', '@OUTPUT@'],
)

stub_o = custom_target(
    'stub_o',
    input: stub_bin,
    output: 'stub.o',
    command: [
        objcopy,
        '-I', 'binary',
        '-O', 'default',
        '@INPUT@',
        '@OUTPUT@',
        '--prefix-sections=reloc_stub',
    ],
)

stub_dep = declare_dependency(
    sources: stub_o,
    include_directories: '.',
)
