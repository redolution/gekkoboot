MEMORY
{
	STUB(RWX) : ORIGIN = 0x80001000, LENGTH = 0x2000
	MRAM(RWX) : ORIGIN = etext, LENGTH = 0x2000
}

SECTIONS
{
	.stub :
	{
		stub = .;
		*(.stub) *(.stub.*)

		/*
		KEEP(*(reloc_stub.data))
		*/
		__stub_end = .;
	} > STUB AT> MRAM
}
/*INSERT AFTER .text;*/

stub_size = __stub_end - stub;
ASSERT(stub_size <= 0x300, "Relocation stub too big!")
