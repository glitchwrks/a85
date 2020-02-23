a85: a85.c a85util.c a85eval.c
	cc -o a85 a85.c a85util.c a85eval.c

clean:
	rm -f a85
	rm -f TEST85.HEX TEST85.PRN

test: a85
	./a85 TEST85.ASM -o TEST85.HEX -l TEST85.PRN