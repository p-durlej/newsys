
SECTIONS
{
	. = 0x7fc00000;
	.text	: { *(.text)	}
	.rodata	: { *(.rodata)	}
	.rodata : { *(.gtab)	}
	.data	: { *(.data)	}
	_edata		= .;
	.bss	: { *(.bss)	}
	_end		= .;
}
