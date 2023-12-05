OUTPUT_FORMAT("elf32-powerpc")
OUTPUT_ARCH(powerpc:common)
ENTRY(not_main)

PHDRS
{
	stub PT_LOAD FLAGS(5);
	text PT_LOAD FLAGS(5);
	data PT_LOAD FLAGS(6);
	bss  PT_LOAD FLAGS(6);
}

SECTIONS
{
	. = ALIGN(32);
	.text :
	{
		*(.text.not_main)
		*(.text .text.*)
	} : text
}
