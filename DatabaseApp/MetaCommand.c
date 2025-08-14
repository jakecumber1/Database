#include "MetaCommand.h"
#include <stdlib.h>
#include <string.h>
//Function for handling meta commands, right now this simple string comparison will work since we only have one command
MetaCommandResult do_meta_command(InputBuffer* input_buffer, Table* table) {
	if (strcmp(input_buffer->buffer, ".exit") == 0) {
		//we're exiting, so free the buffer
		close_input_buffer(input_buffer);
		db_close(table);
		//You might want to make this a successful meta command
		//Don't, because then the program will keep running
		exit(EXIT_SUCCESS);
	}
	else if (strcmp(input_buffer->buffer, ".constants") == 0) {
		printf("Constants:\n");
		print_constants();
		return META_COMMAND_SUCCESS;
	}
	else if (strcmp(input_buffer->buffer, ".btree") == 0) {
		printf("Tree:");
		print_leaf_node(get_page(table->pager, 0));
		return META_COMMAND_SUCCESS;
	}
	else {
		return META_COMMAND_UNRECOGNIZED_COMMAND;
	}
}