stub_linker_script = meson.current_source_dir() / 'link.ld'

stub_lib = static_library(
    'stub',
    'stub.c',
    c_args: [
        '-ffreestanding',
        '-mno-eabi',
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

stub_dep = declare_dependency(
    link_with: stub_lib,
    link_args: [
        '-T' + stub_linker_script,
    ],
    include_directories: '.',
)
