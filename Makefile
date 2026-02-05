# Nom du fichier exécutable qui sera créé sur le disque
NAME	= ft_ping

# Fichier(s) .c présents dans le projet
SRCS	= src/main.c src/net.c src/stat.c

# Programme utilisé pour compiler (gcc)
CC		= gcc

INC_DIR	= src

# Options passées à gcc lors de chaque compilation
CFLAGS 	= -Wall -Wextra -Werror -I$(INC_DIR) -MMD -MP

# Liste des fichiers .o générés à partir des .c
OBJS	= $(SRCS:.c=.o)

DEPS	= $(OBJS:.o=.d)
-include $(DEPS)


# Make vérifie si le fichier ft_ping existe et est à jour sinon le creer
all: $(NAME)

# Commande exécutée pour créer le fichier ft_ping
# Elle assemble tous les fichiers .o en un exécutable
$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -lm -o $(NAME)

# Commande exécutée pour créer un fichier .o à partir d’un fichier .c
# gcc compile le .c sans créer d’exécutable (-c)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Supprime uniquement les fichiers .o du disque
clean:
	rm -f $(OBJS)

# Supprime les fichiers .o et l’exécutable ft_ping
fclean: clean
	rm -f $(NAME)
	rm -f $(DEPS)

# Supprime tout puis relance la compilation complète
re: fclean all
