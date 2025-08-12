#include "InputBuffer.h"
//this include is needed for malloc and free
#include <stdlib.h>
//function for inputbuffer creation
InputBuffer* new_input_buffer() {
	//what's this goofy ol' malloc() function mean?
	//Just kidding, I've come across malloc when doing networking (serialization and the sort)
	//This is just C's analogue for the new keyword (it definitely isn't the same, but that's kind of our usecase here)
	//Since we defined the types and variables within the header file for InputBuffer
	//We can just say "allocate memory for... the size of an InputBuffer", but remember when we did serialization
	//We had to convert our message to a char array, calculate the size of that manually, and then pass that into malloc (not literally manually but like sizeof(char) * 4 type stuff..)
	InputBuffer* input_buffer = (InputBuffer*)malloc(sizeof(InputBuffer));
	input_buffer->buffer = NULL;
	input_buffer->buffer_length = 0;
	input_buffer->input_length = 0;

	return input_buffer;
}

//how I yearn for the java garbage collector whenever i write C/C++...
void close_input_buffer(InputBuffer* input_buffer){
	free(input_buffer->buffer);
	free(input_buffer);
}