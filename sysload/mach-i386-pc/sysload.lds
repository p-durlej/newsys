
SECTIONS
{
	. = 0x8000;
	.text	: { *(.text) }
	.rodata	: { *(.rodata) }
	.data	: { *(.data) }
	_edata = .;
	.bss	: { *(.bss) }
	_end = .;
}
