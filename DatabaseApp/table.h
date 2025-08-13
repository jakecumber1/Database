#ifndef TABLE_H
#define TABLE_H
//Include for uint32_t
#include <stdint.h>
#include <stdbool.h>
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

//Create a struct Pager which the table can call to make requests
typedef struct {
	int file_descriptor;
	uint32_t file_length;
	void* pages[TABLE_MAX_PAGES];
} Pager;
//Initializes pager and opens file.
Pager* pager_open(const char* filename);
//Retrieves a page from itself/file (file if cache miss)
void* get_page(Pager* pager, uint32_t page_num);
//Flushes page to disk
void pager_flush(Pager* pager, uint32_t page_num, uint32_t size);


typedef struct {
	uint32_t num_rows;
	Pager* pager;
} Table;


//Prints a row to console
void print_row(Row* row);
//Initializes table and pager, opens/creates database file
Table* db_open(const char* filename);
//Flushes memory to disk, closes db file, and frees table and pager on ".exit".
void db_close(Table* table);


//Represnts a location on the table
typedef struct {
	Table* table;
	uint32_t row_num;
	bool end_of_table;
} Cursor;
//Once we split our implementation into a BTree, this will make inserts, modifications, and deletes much easier.

//Return a cursor pointing at the start of a table
Cursor* table_start(Table* table);

//Return a cursor at the end of a table
Cursor* table_end(Table* table);
//returns pointer to position in table described by the cursor
void* cursor_value(Cursor* cursor);
//Advances cursor to the next row
void cursor_advance(Cursor* cursor);
#endif