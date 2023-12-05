fs = import('fs')

ipl_rom_name = 'ipl.rom'
full_rom_opt = get_option('full_rom')
if full_rom_opt.allowed()
    full_rom_opt = full_rom_opt.require(
        fs.is_file(ipl_rom_name),
        error_message: 'IPL ROM file @0@ not found, can\'t build Qoob Pro image!'.format(ipl_rom_name),
    )
    if full_rom_opt.allowed()
        ipl_rom = files(ipl_rom_name)
    endif
endif
summary(
    'Qoob Pro support',
    full_rom_opt.allowed(),
    bool_yn: true,
)

qoob_sx_updater = files('qoob_sx_13c_upgrade.elf')

terminus = files('terminus-0.49.1-116b-ll2-td1.psf')
