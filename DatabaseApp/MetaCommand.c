#include "MetaCommand.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
//Function for handling meta commands, right now this simple string comparison will work since we only have one command
MetaCommandResult do_meta_command(InputBuffer* input_buffer, Table* table) {
	//In C, switching only works on stuff like int, enum, and chars, not strings, so we have to keep this as if else
	if (strcmp(input_buffer->buffer, ".exit") == 0) {
		//we're exiting, so free the buffer
		close_input_buffer(input_buffer);
		if (table != NULL) {
			db_close(table);
		}
		//You might want to make this a successful meta command
		//Don't, because then the program will keep running
		exit(EXIT_SUCCESS);
	}
	else if (strncmp(input_buffer->buffer, ".open ", 6) == 0) {
		if (table != NULL) {
			printf("Closing currently open database file...\n");
			db_close(table);
			table = NULL;
		}
		//HAve to handle opening in main 
		return META_COMMAND_OPEN_SUCCESS;
	}
	else if (strcmp(input_buffer->buffer, ".close") == 0) {
		if (table == NULL) {
			printf("No database file currently open.\n");
		}
		else {
			db_close(table);
			table = NULL;
			printf("Closed database.\n");
		}
		return META_COMMAND_CLOSE_SUCCESS;
	}
	else if (strcmp(input_buffer->buffer, ".constants") == 0) {
		printf("Constants:\n");
		print_constants();
		return META_COMMAND_SUCCESS;
	}
	else if (strcmp(input_buffer->buffer, ".btree") == 0) {
		printf("Tree:\n");
		print_tree(table->pager, 0, 0);
		return META_COMMAND_SUCCESS;
	}
	else {
		return META_COMMAND_UNRECOGNIZED_COMMAND;
	}
}