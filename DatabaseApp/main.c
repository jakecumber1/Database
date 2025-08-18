#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "InputBuffer.h"
#include "MetaCommand.h"
#include "Statement.h"
//Quick function for handling our input prompt.
void print_prompt() { printf("db > "); }

//Our starting and ending buffer size for inputs
#define INITIAL_BUFFER_SIZE 256
#define MAX_BUFFER_SIZE 16384
//getline() equivalent for windows, we need an input reader which is resizable, hence the definitions above.
ssize_t getline(char** lineptr, size_t* n, FILE* stream) {
	if (!lineptr || !n || !stream) return -1;

	//POS is the number of characters read, C is our variable for holding each character from the stream
	size_t pos = 0;
	int c;

	//allocate initial buffer if needed, if malloc fails, error out
	if (*lineptr == NULL || *n == 0) {
		*n = INITIAL_BUFFER_SIZE;
		*lineptr = malloc(*n);
		if (!*lineptr) return -1;
	}
	//read a character from the stream, loop until the end of file is reached
	while ((c = fgetc(stream)) != EOF) {
		//if we need to, grow the buffer
		if (pos + 1 >= *n) {
			if (*n >= MAX_BUFFER_SIZE) {
				//buffer limit reached, throw an error
				return -1;
			}
			size_t new_size = *n * 2;
			//if our current size is bigger than the max buffer, just set our size to the max buffer
			if (new_size > MAX_BUFFER_SIZE) new_size = MAX_BUFFER_SIZE;
			//realloc resizes an existing memory block, basically we're saying "the size of lineptr is now new_size"
			//in the case *lineptr was null (it better not be we checked it twice above), then realloc works like malloc
			char* new_ptr = realloc(*lineptr, new_size);
			if (!new_ptr) return -1;
			*lineptr = new_ptr;
			*n = new_size;
		}
		(*lineptr)[pos++] = (char)c;
		//Stop reading once we've hit a new line, this means the new line IS included in the line read.
		if (c == '\n') break;
	}
	if (pos == 0 && c == EOF) return -1; //end of file with no data
	//Yes, I know what a null character is believe it or not
	//adding this at the end makes the buffer a proper c string.
	(*lineptr)[pos] = '\0';
	return (ssize_t)pos;
}

//Finally, with that monstrosity declared above, we can handle the read input function
//Reads input into an InputBuffer
void read_input(InputBuffer* input_buffer) {
	//time to remember references, what's happening here?
	//so basically, we're passing in a reference to our buffer, and a reference to our buffer length
	//this will let us put characters into the buffer and modify the buffer length in the above function
	//so that when we exit the function, what we put into the buffer and how we changed the length will stay
	//oh that that stdin, that's just standard input, remember std::cin?
	ssize_t bytes_read = getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);

	//Check if we read 0 bytes
	if (bytes_read <= 0) {
		printf("Error reading input\n");
		exit(EXIT_FAILURE);
	}
	//we're ignoring the new line character we added above
	//"BUT WHY PUT IT IN IN THE FIRST PLACE?"
	//I want to replicate the original code of this tutorial as much as possible, and that means replicating behaviors present in linux/posix functions like getline() and types like ssize_t
	input_buffer->input_length = bytes_read - 1;
	input_buffer->buffer[bytes_read - 1] = 0;
}

int main(int argc, char* argv[]) {
	//main.c will serve as our entry point into the application.
	//Similar to my MTG Deck builder project, this is my first real C project (first C project ever in fact)
	//So expect a significant amount of comments (I'd argue too many for anyone familiar with the langauge)
	Table* table = NULL;
	if (argc >= 2) {
		char* filename = argv[1];
		table = db_open(filename);
	}
	//Putting the input buffer into it's own header and c file is overkill, this is just to get me comfy with the conventions
	InputBuffer* input_buffer = new_input_buffer();
	//while loop for handling user inputs.
	while (true) {
		print_prompt();
		read_input(input_buffer);
		//Seperate out meta commands (.help, .tables, etc.) by checking if the first element in the buffer is a dot
		if (input_buffer->buffer[0] == '.') {
			switch (do_meta_command(input_buffer, table)) {
			case (META_COMMAND_SUCCESS):
				continue;
			case (META_COMMAND_UNRECOGNIZED_COMMAND):
				printf("Unrecognized command '%s' .\n", input_buffer->buffer);
				continue;
			case (META_COMMAND_CLOSE_SUCCESS):
				table = NULL;
				continue;
			case (META_COMMAND_OPEN_SUCCESS):
				char* filename = input_buffer->buffer + 6;
				filename[strcspn(filename, "\n")] = 0;
				table = db_open(filename);
				printf("Opened database file %s\n", filename);
				continue;
			}
		}
		//Check if input is a non-meta valid statement, if not restart the loop and print the error.
		Statement statement;
		switch (prepare_statement(input_buffer, &statement)) {
		case(PREPARE_SUCCESS):
			break;
		case(PREPARE_SYNTAX_ERROR):
			printf("Syntax error. Could not parse statement.");
			continue;
		case(PREPARE_UNRECOGNIZED_STATEMENT):
			printf("Unrecognized keyword at start of '%s'.\n", input_buffer->buffer);
			continue;
		case(PREPARE_STRING_TOO_LONG):
			printf("String input is too long.\n");
			continue;
		case(PREPARE_NEGATIVE_ID):
			printf("ID must be positive.\n");
			continue;
		}
		//If we reached here we have a valid non-meta statement, so execute!
		switch (execute_statement(&statement, input_buffer, table)) {
		case(EXECUTE_SUCCESS):
			printf("Executed.\n");
			break;
		case(EXECUTE_TABLE_FULL):
			printf("Error: Table full.\n");
			break;
		case(EXECUTE_DUPLICATE_KEY):
			printf("Error: Duplicate key.\n");
			break;
		case(EXECUTE_NO_TABLE):
			printf("No table to perform statement!\n");
			break;
		case(EXECUTE_NEGATIVE_ID):
			printf("Negative id was given for selection\n");
			break;
		}
	}
	return 0;
}