
FONTS = mono system mono-large system-large

all: $(FONTS)

mono: mono.fnt
	../cross/fnc -ts=2 -bs=2 mono.fnt mono

mono-large: mono.fnt
	../cross/fnc -ts=4 -bs=4 mono-large.fnt mono-large

%: %.fnt
	../cross/fnc $< $@

clean:
	rm -f $(FONTS)
