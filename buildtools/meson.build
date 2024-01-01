python = find_program('python3')
dol2ipl = [python, files('dol2ipl.py')]
qoob_injector = [python, files('qoob_injector.py')]
psf2c = [python, files('psf2c.py')]

dol2gci = executable(
    'dol2gci',
    'dol2gci.cpp',
    native: true,
)

subdir('doltool')
