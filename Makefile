-include */Makefile.in

all:
	@

indent:
	uncrustify -c .uncrustify $(c_src-y) \
	 $(c_src-y:%.c=%.h) --replace --no-backup
