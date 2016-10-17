
SECTIONS
{
	. = 0x100000;
	.text16 : { *(.text16) }
	.data16 : { *(.data16) }
	_text = .;
	.text	: { *(.text) }
	.rodata	: { *(.rodata) }
	_data = .;
	.data	: { *(.data) }
	_edata = .;
	.bss	: { *(.bss) }
	_end = .;
}
