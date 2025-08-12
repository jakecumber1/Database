#include "Statement.h"
//strncmp, strcmp, etc.
#include <string.h>
#include <stdio.h>
//Parse statement for execution
PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement) {
	//We haven't seen this string function yet, what does it do?
	//strncmp compares two strings and an n number of characters, it will return 0 if the characters exactly match
	if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
		statement->type = STATEMENT_INSERT;
		//there's a similar function to this called 'scanf' it has similar behavior but for stdin instead of existing strings
		//%d = digit %s = sequence of non-white space characters
		//So basically, match the string to "insert digit word word" the int returned is the number of % matched.
		int args_assigned = sscanf(input_buffer->buffer, "insert %d %s %s", &(statement->row_to_insert.id), 
			statement->row_to_insert.username, statement->row_to_insert.email);
		//One of the args in the statement is missing.
		if (args_assigned < 3) {
			return PREPARE_SYNATAX_ERROR;
		}
		return PREPARE_SUCCESS;
	}
	if (strncmp(input_buffer->buffer, "select", 6) == 0) {
		statement->type = STATEMENT_SELECT;
		return PREPARE_SUCCESS;
	}
	return PREPARE_UNRECOGNIZED_STATEMENT;
}

ExecuteResult execute_insert(Statement* statement, Table* table) {
	if (table->num_rows >= TABLE_MAX_ROWS) {
		return EXECUTE_TABLE_FULL;
	}
	Row* row_to_insert = &(statement->row_to_insert);

	serialize_row(row_to_insert, row_slot(table, table->num_rows));
	table->num_rows += 1;
	return EXECUTE_SUCCESS;
}
ExecuteResult execute_select(Statement* statement, Table* table) {
	Row row;
	for (uint32_t i = 0; i < table->num_rows; i++) {
		deserialize_row(row_slot(table, i), &row);
		print_row(&row);
	}
	return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement* statement, Table* table) {
	switch (statement->type) {
	case(STATEMENT_INSERT):
		return execute_insert(statement, table);
	case(STATEMENT_SELECT):
		return execute_select(statement, table);
	}

}
