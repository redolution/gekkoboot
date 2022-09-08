project(
    'libfat',
    ['c'],
    meson_version: '>=1.1',
)

external_project = import('unstable-external_project')

p = external_project.add_project(
    'configure',
    configure_options: [
        '--prefix=@PREFIX@',
        '--libdir=@LIBDIR@',
        '--incdir=@INCLUDEDIR@',
    ],
    cross_configure_options: [],
)

dep = p.dependency(':wii/libfat.a')
