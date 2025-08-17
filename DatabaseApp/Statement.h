#ifndef STATEMENT_H
#define STATEMENT_H
#include "InputBuffer.h"
#include "Table.h"
//We will also include prepare returns here as well, since they're handled in the same block
//after meta commands have already been handled
//PrepareResult is effectively our SQL compiler
/*prepare_success = parsed with no trouble
prepare_unrecognized_statement = command of the statement (insert, select, etc.) wasn't recognized
prepare_syntax_error = there was an issue with how the statement was parsed (missing an arg, invalid entry, etc.)
PREPARE_STRING_TOO_LONG = either the name or email field are too long
prepare_negative_id = a negative id was passed into insert statement*/
typedef enum {PREPARE_SUCCESS, PREPARE_UNRECOGNIZED_STATEMENT, 
PREPARE_SYNTAX_ERROR, PREPARE_STRING_TOO_LONG,
PREPARE_NEGATIVE_ID} PrepareResult;

typedef enum {STATEMENT_INSERT, STATEMENT_SELECT} StatementType;

typedef struct { StatementType type; Row row_to_insert; } Statement;

typedef enum { EXECUTE_SUCCESS, EXECUTE_TABLE_FULL, EXECUTE_DUPLICATE_KEY, EXECUTE_NO_TABLE } ExecuteResult;

PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement);
PrepareResult prepare_insert(InputBuffer* input_buffer, Statement* statement);


ExecuteResult execute_statement(Statement* statement, Table* table);
ExecuteResult execute_insert(Statement* statement, Table* table);
ExecuteResult execute_select(Statement* statement, Table* table);

#endif