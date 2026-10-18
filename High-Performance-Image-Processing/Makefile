CC = gcc
CFLAGS = -Wall -Wextra -Isrc

# Default target: running 'make' will compile both 'server' and 'client'
all: deps server client

# Rule to download dependencies (stb headers) if they don't exist
deps:
	@if [ ! -f src/stb_image.h ]; then \
		echo "Downloading stb_image.h..."; \
		wget -q https://raw.githubusercontent.com/nothings/stb/master/stb_image.h -P src; \
	fi
	@if [ ! -f src/stb_image_write.h ]; then \
		echo "Downloading stb_image_write.h..."; \
		wget -q https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h -P src; \
	fi
	@if [ ! -f src/stb_image_resize2.h ]; then \
		echo "Downloading stb_image_resize2.h..."; \
		wget -q https://raw.githubusercontent.com/nothings/stb/master/stb_image_resize2.h -P src; \
	fi

# Rule to compile the server
# It depends on 'src/server.c' and 'src/logging.c'
server: src/server.c src/logging.c
	$(CC) $(CFLAGS) src/server.c src/logging.c -o server -pthread

# Rule to compile the client
# It depends on 'src/client.c' and 'src/logging.c'
client: src/client.c src/logging.c
	$(CC) $(CFLAGS) src/client.c src/logging.c -o client -lm

# New rule to clear just the log file
clean-logs:
	rm -f application.log

# Helper rule to clean up generated files
clean: clean-logs
	rm -f server client *.o output/*.png debug_resized.png src/stb_*.h
	rmdir output 2>/dev/null || true
