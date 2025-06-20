-include */Makefile.in

all:
	@

indent:
	uncrustify -c $(srctree)/etc/uncrustify.cfg $(c_src-y) \
	 $(c_src-y:%.c=%.h) --replace --no-backup
