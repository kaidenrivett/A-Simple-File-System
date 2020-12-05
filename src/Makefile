.phony all:
all:
	gcc -Wall diskhelp.c diskinfo.c -o diskinfo
	gcc -Wall diskhelp.c disklist.c -o disklist
	gcc -o diskget -std=c11 -Wall diskhelp.c -ggdb3 diskget.c


.phony clean:
clean:
	-rm diskinfo
	-rm disklist
	-rm diskget
