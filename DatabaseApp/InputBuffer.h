#ifndef INPUTBUFFER_H
#define INPUTBUFFER_H
//This include is needed for NULL in the c file and size_t now.
//Yes, you read that right, NULL and size_t need to be included.
#include <stddef.h>
#include "posix_comp.h"
//okay this syntax is a little unfamiliar, so I'll break it down.
/*
So what i've always seen are structs defined like this:

struct name_of_struct {struct definition};

so what's typedef doing here?
Well, if we didn't include it we'd have to type struct out anytime we declared an object like:
struct InputBuffer ib;

By adding typedef we can just do:
InputBuffer ib;
*/
//Struct for Input Buffer for user input.
typedef struct {
	char* buffer;
	size_t buffer_length;
	//The original tutorial uses ssize_t, a POSIX only type unavailable to us.
	//We're just gonna say this is a long and come back if we have any issues in the future, same with the rest of the ssize_t's in this project
	ssize_t input_length;
} InputBuffer;

InputBuffer* new_input_buffer();
void close_input_buffer(InputBuffer* input_buffer);

#endif