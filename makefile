LDFLAGS = -lGL -lGLU -lglut


a1 : a1.c graphics.c visible.c graphics.h
	gcc a1.c graphics.c visible.c -o a1 $(LDFLAGS) -lm

play: a1
	./a1
