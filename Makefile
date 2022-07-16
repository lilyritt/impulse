CFLAGS += -O2 -g -Wall -Wextra -Werror -I include/
LDFLAGS = -lglfw -lm -lassimp
OBJECTS = lib/glad.o

impulse: src/main.cpp ${OBJECTS}
	c++ $< -o $@ ${CFLAGS} ${LDFLAGS} ${OBJECTS}

lib/glad.o: src/glad.c
	cc $< -o $@ -c ${CFLAGS}

.PHONY: clean
clean:
	find . -maxdepth 1 -type f -executable -delete
	find lib/ -name '*.o' -delete
