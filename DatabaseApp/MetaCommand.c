#include "MetaCommand.h"
#include <stdlib.h>
#include <string.h>
//Function for handling meta commands, right now this simple string comparison will work since we only have one command
MetaCommandResult do_meta_command(InputBuffer* input_buffer, Table* table) {
	if (strcmp(input_buffer->buffer, ".exit") == 0) {
		//we're exiting, so free the buffer
		close_input_buffer(input_buffer);
		//You might want to make this a successful meta command
		//Don't, because then the program will keep running
		exit(EXIT_SUCCESS);
	}
	else {
		return META_COMMAND_UNRECOGNIZED_COMMAND;
	}
}