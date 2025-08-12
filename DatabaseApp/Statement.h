#ifndef STATEMENT_H
#define STATEMENT_H
#include "InputBuffer.h"
//We will also include prepare returns here as well, since they're handled in the same block
//after meta commands have already been handled
//PrepareResult is effectively our SQL compiler
typedef enum {PREPARE_SUCCESS, PREPARE_UNRECOGNIZED_STATEMENT} PrepareResult;

typedef enum {STATEMENT_INSERT, STATEMENT_SELECT} StatementType;

typedef struct { StatementType type; } Statement;

PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement);
void execute_statement(Statement* statement);

#endif