DESTDIR	=	/usr
PREFIX	=	/local
CC	=	gcc
SRC	=	move_cam.c
OBJ	=	move_cam feed
LIBS	=	-lwiringPi

all:		$(OBJ)
move_cam:	move_cam.c
		@echo [make move_cam]
		@$(CC) -o move_cam move_cam.c $(LIBS)

feed:		feed.c
		@echo [make feed]
		@$(CC) -o feed feed.c $(LIBS)
clean:
	@echo "[Clean]"
	@rm -f $(OBJ) *~ core *.bak

install:
	@echo "[Install]"
	@sudo cp $(OBJ)		$(DESTDIR)$(PREFIX)/bin
	@sudo chmod 4755 $(DESTDIR)$(PREFIX)/bin/move_cam
	@sudo chmod 4755 $(DESTDIR)$(PREFIX)/bin/feed
