#include "Statement.h"

//Parse statement for execution
PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement) {
	//We haven't seen this string function yet, what does it do?
	//strncmp compares two strings and an n number of characters, it will return 0 if the characters exactly match
	if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
		statement->type = STATEMENT_INSERT;
		return PREPARE_SUCCESS;
	}
	if (strcmp(input_buffer->buffer, "select", 6) == 0) {
		return PREPARE_SUCCESS;
	}
	return PREPARE_UNRECOGNIZED_STATEMENT;
}
void execute_statement(Statement* statement) {
	switch (statement->type) {
	case(STATEMENT_INSERT):
		printf("This is where we'd handle an insert.\n");
		break;
	case(STATEMENT_SELECT):
		printf("This is where we'd handle a select.\n");
	}

}