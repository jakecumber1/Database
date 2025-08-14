#include "Statement.h"
//strncmp, strcmp, etc.
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//Parse statement for execution
PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement) {
	//We haven't seen this string function yet, what does it do?
	//strncmp compares two strings and an n number of characters, it will return 0 if the characters exactly match
	if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
		return prepare_insert(input_buffer, statement);
	}
	if (strncmp(input_buffer->buffer, "select", 6) == 0) {
		statement->type = STATEMENT_SELECT;
		return PREPARE_SUCCESS;
	}
	return PREPARE_UNRECOGNIZED_STATEMENT;
}
PrepareResult prepare_insert(InputBuffer* input_buffer, Statement* statement) {
	statement->type = STATEMENT_INSERT;
	//Converts our input buffer from a string into null terminated tokens (char arrays) based on a delimiter (space in this case).
	char* keyword = strtok(input_buffer->buffer, " ");
	char* id_string = strtok(NULL, " ");
	char* username = strtok(NULL, " ");
	char* email = strtok(NULL, " ");

	if (id_string == NULL || username == NULL || email == NULL) {
		return PREPARE_SYNTAX_ERROR;
	}
	//atoi = ascii to int, convert our null terminated char array to a int value for our id.
	int id = atoi(id_string);
	if (strlen(username) > COLUMN_USERNAME_SIZE) {
		return PREPARE_STRING_TOO_LONG;
	}
	if (strlen(email) > COLUMN_EMAIL_SIZE) {
		return PREPARE_STRING_TOO_LONG;
	}
	statement->row_to_insert.id = id;
	if (id < 0) {
		return PREPARE_NEGATIVE_ID;
	}

	strcpy_s(statement->row_to_insert.username, COLUMN_USERNAME_SIZE, username);
	strcpy_s(statement->row_to_insert.email, COLUMN_EMAIL_SIZE, email);

	return PREPARE_SUCCESS;
}
ExecuteResult execute_insert(Statement* statement, Table* table) {
	void* node = get_page(table->pager, table->root_page_num);
	if (*leaf_node_num_cells(node) >= LEAF_NODE_MAX_CELLS) {
		return EXECUTE_TABLE_FULL;
	}
	Row* row_to_insert = &(statement->row_to_insert);
	Cursor* cursor = table_end(table);
	leaf_node_insert(cursor, row_to_insert->id, row_to_insert);

	//We created a cursor so now we must free it
	free(cursor);

	return EXECUTE_SUCCESS;
}
ExecuteResult execute_select(Statement* statement, Table* table) {
	Cursor* cursor = table_start(table);
	Row row;
	while(!(cursor->end_of_table)) {
		deserialize_row(cursor_value(cursor), &row);
		print_row(&row);
		cursor_advance(cursor);
	}
	//remember, created cursor, we must free it.
	free(cursor);
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
