
BIN := mkbfs fnc modinfo fnconv enlarge ckexe cdcks cfont wc2c psysload sysver

CC := cc

all: $(BIN)
	cd mgen && $(MAKE)

clean:
	cd mgen && $(MAKE) clean
	rm -f $(BIN)
