#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <libgen.h>
#include <string.h>
#include <fcntl.h>

#define PATHMAX 100

DIR *open_this_directory();
DIR *open_directory(int dir_fd);
void verify_insensitive_flag(char *given_flag);
void list_file_names_from_directory(int dir_fd,
                                    const char relative_path[PATHMAX],
                                    char *word,
                                    char *(*case_sensitivity_mod)(char *, char *) );
void to_lowercase(char word[]);
int is_uppercase(int ascii_code);
char *case_sensitive(char *filename, char *given_word);
char *case_insensitive(char *filename, char *given_word);


int
is_uppercase(int ascii_code)
{
	return ((0x40 < ascii_code) && (ascii_code < 0x5B));
}

void
to_lowercase(char word[])
{
	size_t size = strlen(word);
	int ascii_code;
	for (size_t i = 0; i < size; i++) {
		ascii_code = (int) word[i];
		if (is_uppercase(ascii_code)) {
			word[i] += 0x20;
		}
	}
}

char *
case_sensitive(char *filename, char *given_word)
{
	return strstr(filename, given_word);
}

char *
case_insensitive(char *filename, char *given_word)
{
	to_lowercase(filename);
	return case_sensitive(filename, given_word);
}

DIR *
open_this_directory()
{
	DIR *dirp = opendir(".");
	if (dirp == NULL) {
		perror("No se pudo abrir .\n");
		exit(-1);
	}
	return dirp;
}

DIR *
open_directory(int dir_fd)
{
	DIR *dirp = fdopendir(dir_fd);
	if (dirp == NULL) {
		perror("No se pudo abrir\n");
		exit(-1);
	}
	return dirp;
}

void
verify_insensitive_flag(char *given_flag)
{
	char true_flag[] = "-i";
	if (strcmp(true_flag, given_flag) != 0) {
		printf("No se ha reconocido el flag: %s\n", given_flag);
		exit(-1);
	}
}

void
list_file_names_from_directory(int dir_fd,
                               const char relative_path[PATHMAX],
                               char *word,
                               char *(*case_sensitivity_mod)(char *, char *) )
{
	DIR *dirp = open_directory(dir_fd);
	struct dirent *sd;
	char filepath[PATHMAX];
	char filename[PATHMAX];
	char slash[] = "/";
	int subdir_fd;

	while ((sd = readdir(dirp)) != NULL) {
		if (sd->d_name[0] != '.') {
			strcpy(filepath, relative_path);
			strcat(filepath, sd->d_name);

			strcpy(filename, sd->d_name);
			if (case_sensitivity_mod(filename, word) != NULL) {
				printf("%s\n", filepath);
			}

			if (sd->d_type == DT_DIR) {
				subdir_fd =
				        openat(dir_fd, sd->d_name, O_DIRECTORY);
				strcat(filepath, slash);
				list_file_names_from_directory(subdir_fd,
				                               filepath,
				                               word,
				                               case_sensitivity_mod);
			}
		}
	}
}

int
main(int argc, char *argv[])
{
	DIR *dirp = open_this_directory();
	char *word;
	char *(*case_sensitivity_mod)(char *, char *);

	switch (argc) {
	case 2:
		word = argv[1];
		case_sensitivity_mod = case_sensitive;
		break;
	case 3:
		verify_insensitive_flag(argv[1]);
		word = argv[2];
		to_lowercase(word);
		case_sensitivity_mod = case_insensitive;
		break;
	default:
		printf("La cantidad de parametros %d es incorrecta.\n", argc);
		exit(-1);
	}

	int dir_fd = dirfd(dirp);
	char filepath[PATHMAX] = "";

	list_file_names_from_directory(dir_fd, filepath, word, case_sensitivity_mod);

	return 0;
}