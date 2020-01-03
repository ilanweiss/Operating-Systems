/*
 * ex1.c
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 65536
#define DESTINATION_FILE_MODE S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH

extern int opterr, optind;

void exit_with_usage(const char *message) {
	fprintf (stderr, "%s\n", message);
	fprintf (stderr, "Usage:\n\tex1 [-f] SOURCE DEST BUFFER_SIZE\n");
	exit(EXIT_FAILURE);
}

void copy_file(const char *source_file, const char *dest_file, int buffer_size, int force_flag) {
	/*
	 * Copy source_file content to dest_file, buffer_size bytes at a time.
	 * If force_flag is true, then also overwrite dest_file. Otherwise print error ("File exists"), and exit.
	 *
	 * TODO:
	 * 	1. Open source_file for reading
	 * 	2. Open dest_file for writing (Hint: is force_flag true?)
	 * 	3. Loop reading from source and writing to the destination buffer_size bytes each time
	 * 	4. Close source_file and dest_file
	 *
	 *  ALWAYS check the return values of syscalls for errors!
	 *  If an error was found, use perror(3) to print it with a helpful message, and then exit(EXIT_FAILURE)
	 */
	int fileSource = open(source_file,O_RDONLY);
	if (fileSource ==-1){
		perror("Unable to open source file for reading");
		close(fileSource);
		exit(EXIT_FAILURE);
		}
	
	int fileDestenation = open(dest_file,O_CREAT|O_WRONLY|(force_flag)?O_TRUNC:O_EXCL,DESTINATION_FILE_MODE);
	if (fileDestenation == -1){
		perror("Unable to open destination file for writing");
		close(fileSource);
		exit(EXIT_FAILURE);
		}
	int reader,writer;
	void *buff[MAX_BUFFER_SIZE];
	while ((reader = read(fileSource,buff,buffer_size)) != 0){
		if (reader == -1){
			perror("Reading source file failur");
			close(fileSource);
			close(fileDestenation);
			exit(EXIT_FAILURE);
			}
		writer = write(fileDestenation,buff,reader);
		if (writer == -1){
			perror("writing Destenation file failur");
			close(fileSource);
			close(fileDestenation);
			exit(EXIT_FAILURE);
			}
		}
	close(fileSource);
	close(fileDestenation);
	printf("File %s was copied to %s\n",source_file ,dest_file);
	exit(EXIT_SUCCESS);
		
	

}

void parse_arguments(
		int argc, char **argv,
		char **source_file, char **dest_file, int *buffer_size, int *force_flag) {
	/*
	 * parses command line arguments and set the arguments required for copy_file
	 */
	int option_character;

	opterr = 0; /* Prevent getopt() from printing an error message to stderr */

	while ((option_character = getopt(argc, argv, "f")) != -1) {
		switch (option_character) {
		case 'f':
			*force_flag = 1;
			break;
		default:  /* '?' */
			exit_with_usage("Unknown option specified");
		}
	}

	if (argc - optind != 3) {
		exit_with_usage("Invalid number of arguments");
	} else {
		*source_file = argv[argc-3];
		*dest_file = argv[argc-2];
		*buffer_size = atoi(argv[argc-1]);

		if (strlen(*source_file) == 0 || strlen(*dest_file) == 0) {
			exit_with_usage("Invalid source / destination file name");
		} else if (*buffer_size < 1 || *buffer_size > MAX_BUFFER_SIZE) {
			exit_with_usage("Invalid buffer size");
		}
	}
}

int main(int argc, char **argv) {
	int force_flag = 0; /* force flag default: false */
	char *source_file = NULL;
	char *dest_file = NULL;
	int buffer_size = MAX_BUFFER_SIZE;

	parse_arguments(argc, argv, &source_file, &dest_file, &buffer_size, &force_flag);

	copy_file(source_file, dest_file, buffer_size, force_flag);

	return EXIT_SUCCESS;
}
