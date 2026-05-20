CC = gcc
CFLAGS = -Wall -Wextra -g -I../CityManager/include

all: city_hub scorer

city_hub: src/city_hub.c
	$(CC) $(CFLAGS) -o city_hub src/city_hub.c

scorer: src/scorer.c
	$(CC) $(CFLAGS) -o scorer src/scorer.c

clean:
	rm -f city_hub scorer
