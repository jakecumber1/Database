#ifndef META_COMMAND_H
#define META_COMMAND_H
#include "InputBuffer.h"
#include "table.h"
typedef enum {
	META_COMMAND_SUCCESS,
	META_COMMAND_UNRECOGNIZED_COMMAND,
	META_COMMAND_CLOSE_SUCCESS,
	META_COMMAND_OPEN_SUCCESS
} MetaCommandResult;

MetaCommandResult do_meta_command(InputBuffer* input_buffer, Table* table);

#endif