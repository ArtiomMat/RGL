mori: *.c *.h
	x86_64-w64-mingw32-gcc -Wall -Werror -o mori *.c -lopengl32 -lgdi32 -lntdll