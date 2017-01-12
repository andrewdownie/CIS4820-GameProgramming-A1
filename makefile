#$(CFLAGS)
EXE = a1
LDFLAGS = -lGL -lGLU -lglut
$(EXE) : a1.c graphics.c visible.c graphics.h
	gcc $< graphics.c visible.c -o $@ $(LDFLAGS) -lm
