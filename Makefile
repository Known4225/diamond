all:
	gcc diamond.c -L./Linux -lturtletextures -lglfw3 -ldl -lm -lX11 -lglad -lGL -lGLU -lpthread -DOS_LINUX -DDEBUGGING_FLAG -Wall -o diamond.o
rel:
	gcc diamond.c -L./Linux -lturtletextures -lglfw3 -ldl -lm -lX11 -lglad -lGL -lGLU -lpthread -DOS_LINUX -O3 -o diamond.o
lib:
	cp turtle.h turtlelib.c
	gcc turtlelib.c -c -L./Linux -lglfw3 -ldl -lm -lX11 -lglad -lGL -lGLU -lpthread -DTURTLE_IMPLEMENTATION -DTURTLE_ENABLE_TEXTURES -DOS_LINUX -O3 -o Linux/libturtletextures.a
	rm turtlelib.c
win:
	gcc diamond.c -L./Windows -lturtletextures -lstbiwrite -lglfw3 -lopengl32 -lgdi32 -lglad -lole32 -luuid -lwsock32 -lWs2_32 -lMf -lMfplat -lmfreadwrite -lmfuuid -DOS_WINDOWS -DDEBUGGING_FLAG -Wall -o diamond.exe
winrel:
	gcc diamond.c -L./Windows -lturtletextures -lstbiwrite -lglfw3 -lopengl32 -lgdi32 -lglad -lole32 -luuid -lwsock32 -lWs2_32 -lMf -lMfplat -lmfreadwrite -lmfuuid -DOS_WINDOWS -O3 -o diamond.exe
winlib:
	cp turtle.h turtlelib.c
	gcc turtlelib.c -c -DTURTLE_IMPLEMENTATION -DTURTLE_ENABLE_TEXTURES -DOS_WINDOWS -O3 -o Windows/turtletextures.lib
	rm turtlelib.c