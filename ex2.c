/*
 * Ilan Weiss
 * 302634654
 * ex2.c
 *
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <curl/curl.h>

#define HTTP_OK 200L
#define REQUEST_TIMEOUT_SECONDS 2L

#define URL_OK 0
#define URL_ERROR 1
#define URL_UNKNOWN 2

#define MAX_PROCESSES 1024

typedef struct {
	int ok, error, unknown;
} UrlStatus;

void usage() {
	fprintf(stderr, "usage:\n\t./ex2 FILENAME NUMBER_OF_PROCESSES\n");
	exit(EXIT_FAILURE);
}

int check_url(const char *url) {
	CURL *curl;
	CURLcode res;
	long response_code = 0L;

	curl = curl_easy_init();

	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, REQUEST_TIMEOUT_SECONDS);
		curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); /* do a HEAD request */

		res = curl_easy_perform(curl);
		if(res == CURLE_OK) {
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
			if (response_code == HTTP_OK) {
				return URL_OK;
			} else {
				return URL_ERROR;
			}
		}

		curl_easy_cleanup(curl);
	}

	return URL_UNKNOWN;
}

void serial_checker(const char *filename) {
	UrlStatus results = {0};
	FILE *toplist_file;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	toplist_file = fopen(filename, "r");

	if (toplist_file == NULL) {
		exit(EXIT_FAILURE);
	}

	while ((read = getline(&line, &len, toplist_file)) != -1) {
		if (read == -1) {
			perror("unable to read line from file");
		}
		line[read-1] = '\0'; /* null-terminate the URL */
		switch (check_url(line)) {
		case URL_OK:
			results.ok += 1;
			break;
		case URL_ERROR:
			results.error += 1;
			break;
		default:
			results.unknown += 1;
		}
	}

	free(line);
	fclose(toplist_file);

	printf("%d OK, %d Error, %d Unknown\n",
			results.ok,
			results.error,
			results.unknown);
}

void worker_checker(const char *filename, int pipe_write_fd, int worker_id, int workers_number) {
	/*
	 * TODO: this checker function should operate almost like serial_checker(), except:
	 * 1. Only processing a distinct subset of the lines (hint: think Modulo)
	 * 2. Writing the results back to the parent using the pipe_write_fd (i.e. and not to the screen)
	 */
	UrlStatus results = {0};
	FILE *toplist_file;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	toplist_file = fopen(filename, "r");

	if (toplist_file == NULL) {
		perror("File can't be opened");
		exit(EXIT_FAILURE);
		}

	char c;
	int numOfLines = 0;
	while ((c = fgetc(toplist_file))!= EOF){
		if (c=='\n'){
			numOfLines++;
		}
	}

	rewind(toplist_file);
	int line_per_worker = (int)(numOfLines / workers_number);
	int currentWorkerStartLine = worker_id * line_per_worker;
	for (int i = 0; i < currentWorkerStartLine; i++){
		getline(&line, &len, toplist_file);
	}
	if (worker_id == workers_number-1){
		line_per_worker += numOfLines % workers_number;
		}
	for (int i = 0 ; i < line_per_worker; i++){
		read = getline(&line, &len, toplist_file);
		if (read < 0){
			perror("Reading problem");
			fclose(toplist_file);
			exit(EXIT_FAILURE);
		}
		line[read-1] = '\0'; /* null-terminate the URL */
		switch (check_url(line)) {
			case URL_OK:
				results.ok += 1;
				break;
			case URL_ERROR:
				results.error += 1;
				break;
			default:
				results.unknown += 1;
			}
		}

	free(line);
	fclose(toplist_file);
	write(pipe_write_fd, &results, sizeof(results));
	}


void parallel_checker(const char *filename, int number_of_processes) {
	/*
	 * TODO:
	 * 1. Start number_of_processes new workers (i.e. processes running the worker_checker function)
	 * 2. Collect the results from all the workers
	 * 3. Print the accumulated results
	 */

	int pipe_write_fd[2];
	UrlStatus res = {0};
	UrlStatus tmp = {0};
	if (pipe(pipe_write_fd)==-1){
		perror("Pipe");
		exit(EXIT_FAILURE);
	}

	for (int i = 0 ;i < number_of_processes ; i++){
		pid_t workerId = fork();
		if (workerId ==-1){
			perror("Fork");
			close(pipe_write_fd[0]);
			close(pipe_write_fd[1]);
			exit(EXIT_FAILURE);
		}
		if (workerId==0){
			worker_checker(filename, pipe_write_fd[1], i, number_of_processes);
			return;
		}
	}
	for (int i=0;i<number_of_processes; i++){
		wait(NULL);
		if (read(pipe_write_fd[0], &tmp, sizeof(res)) < 0 ){
				perror("can't read");
				exit(EXIT_FAILURE);
		}
		res.ok += tmp.ok;
		res.error += tmp.error;
		res.unknown += tmp.unknown;
	}
	close(pipe_write_fd[0]);
	close(pipe_write_fd[1]);
	printf("%d OK, %d Error, %d Unknown\n",
			res.ok,res.error,res.unknown);
}

int main(int argc, char **argv) {
	if (argc != 3) {
		usage();
	} else if (atoi(argv[2]) == 1) {
		serial_checker(argv[1]);
	} else {
		parallel_checker(argv[1], atoi(argv[2]));
	}

	return EXIT_SUCCESS;
}
