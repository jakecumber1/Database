#ifndef TABLE_H
#define TABLE_H
//Include for uint32_t
#include <stdint.h>
#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255
//A row in the table
typedef struct {
	//Oh boy, unrecognized type!
	//Unsigned int 32 type
	//Why would we use this?
	//It guarentees a size of 32 bits, making it helpful for use across platforms
	uint32_t id;
	//include +1 for null character
	char username[COLUMN_USERNAME_SIZE + 1];
	char email[COLUMN_EMAIL_SIZE + 1];
} Row;

//Macro for computing the bytes of an attribute (id, username, etc.) from a struct (Row) at compile time.
#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)

/*(Struct*)0 casts the integer 0 to a pointer of type Struct*
this makes a pointer to memory address zero
How is this safe? doesn't ->Attribute dereference the pointer of this nonexistent object?
Not quite, sizeof only inspects the type of the expression here, so the pointer never actually gets dereferenced
it just returns the size of the attribute type in the struct */

#define ID_SIZE  size_of_attribute(Row, id)
#define USERNAME_SIZE size_of_attribute(Row, username)
#define EMAIL_SIZE size_of_attribute(Row, email)
#define ID_OFFSET 0
//uh.... I guess we're defining these as compiler time constants.
//Bug occured here originally, if you declare macros like this, don't forget the () around the arithmatic
//Or you may get the incorrect calculation
#define USERNAME_OFFSET  (ID_OFFSET + ID_SIZE)
#define EMAIL_OFFSET  (USERNAME_OFFSET + USERNAME_SIZE)
#define ROW_SIZE  (ID_SIZE + USERNAME_SIZE + EMAIL_SIZE)


void serialize_row(Row* source, void* destination);
void deserialize_row(void* source, Row* destination);

#define PAGE_SIZE 4096
#define TABLE_MAX_PAGES  100
#define ROWS_PER_PAGE  (PAGE_SIZE / ROW_SIZE)
#define TABLE_MAX_ROWS  (ROWS_PER_PAGE * TABLE_MAX_PAGES)

typedef struct {
	uint32_t num_rows;
	void* pages[TABLE_MAX_PAGES];
} Table;

void* row_slot(Table* table, uint32_t row_num);
void print_row(Row* row);
Table* new_table();
void free_table(Table* table);
#endif