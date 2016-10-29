
FONTS = mono system mono-large system-large mono-narrow mono-large-narrow

all: $(FONTS)

mono: mono.fnt
	../cross/fnc -ts=2 -bs=2 mono.fnt mono

mono-large: mono.fnt
	../cross/fnc -ts=4 -bs=4 mono-large.fnt mono-large

mono-narrow: mono-narrow.fnt
	../cross/fnc -ts=1 -bs=1 mono-narrow.fnt mono-narrow

mono-large-narrow: mono-large-narrow.fnt
	../cross/fnc -ts=1 -bs=1 mono-large-narrow.fnt mono-large-narrow

%: %.fnt
	../cross/fnc $< $@

clean:
	rm -f $(FONTS)
