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
/*
* Constants no longer needed since we don't store partial pages anymore
#define ROWS_PER_PAGE  (PAGE_SIZE / ROW_SIZE)
#define TABLE_MAX_ROWS  (ROWS_PER_PAGE * TABLE_MAX_PAGES)
*/
//Create a struct Pager which the table can call to make requests
typedef struct {
	int file_descriptor;
	uint32_t file_length;
	uint32_t num_pages;
	void* pages[TABLE_MAX_PAGES];
} Pager;
//Initializes pager and opens file.
Pager* pager_open(const char* filename);
//Retrieves a page from itself/file (file if cache miss)
void* get_page(Pager* pager, uint32_t page_num);
//Get an unused page for node splitting
uint32_t get_unused_page_num(Pager* pager);
//Flushes page to disk
void pager_flush(Pager* pager, uint32_t page_num);

//I would really like nodes to be in a seperate file, but the way they interact with table and pager force me to place them here
typedef enum { NODE_INTERNAL, NODE_LEAF } NodeType;
//We will have to come back and rework this for non int types of data, what if we want to set this up with strings?
//The below are constants for internal nodes and some for leaf nodes
#define NODE_TYPE_SIZE sizeof(uint8_t)
#define NODE_TYPE_OFFSET 0
#define IS_ROOT_SIZE sizeof(uint8_t)
#define IS_ROOT_OFFSET NODE_TYPE_SIZE
#define PARENT_POINTER_SIZE sizeof(uint32_t)
#define PARENT_POINTER_OFFSET (IS_ROOT_OFFSET + IS_ROOT_SIZE)
#define COMMON_NODE_HEADER_SIZE (NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE)

//And now the constants for leaf ndoes
#define LEAF_NODE_NUM_CELLS_SIZE sizeof(uint32_t)
#define LEAF_NODE_NUM_CELLS_OFFSET COMMON_NODE_HEADER_SIZE
#define LEAF_NODE_HEADER_SIZE (COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE)
#define LEAF_NODE_KEY_SIZE sizeof(uint32_t)
#define LEAF_NODE_KEY_OFFSET 0
#define LEAF_NODE_VALUE_SIZE ROW_SIZE
#define LEAF_NODE_VALUE_OFFSET (LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE)
#define LEAF_NODE_CELL_SIZE (LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE)
#define LEAF_NODE_SPACE_FOR_CELLS (PAGE_SIZE - LEAF_NODE_HEADER_SIZE)
#define LEAF_NODE_MAX_CELLS (LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE)

/*From the above constants here's what our leaf node layout will look like
byte 0: node_type
1: is_root
2-6: parent_pointer
6-9: num_cells
10-13: key 0
14-306: value 0
307-310 key 1
311-603: value 1
604-607: key 2
.....
3578-3871: value 12
3871-4095: wasted space
We leave the space empty to avoid splitting cells between nodes.*/

//Constants for splitting and inserting leaf nodes
#define LEAF_NODE_RIGHT_SPLIT_COUNT ((LEAF_NODE_MAX_CELLS + 1) / 2)
//If right gets an even number of cells, this will get that number + 1
#define LEAF_NODE_LEFT_SPLIT_COUNT ((LEAF_NODE_MAX_CELLS + 1) - LEAF_NODE_RIGHT_SPLIT_COUNT)

//Most of the below functions use pointer arithmatic to access the node's keys, values, and metadata.

//Returns the number of cells in the node
uint32_t* leaf_node_num_cells(void* node);
//Returns a cell in the node
void* leaf_node_cell(void* node, uint32_t cell_num);
//Returns the key for the relevant cell
uint32_t* leaf_node_key(void* node, uint32_t cell_num);
//Accesses the relevant cell's value
void* leaf_node_value(void* node, uint32_t cell_num);
//Returns if the node is a leaf or internal
NodeType get_node_type(void* node);
//Sets whether the node is a leaf or internal
void set_node_type(void* node, NodeType type);
//Creates the leaf node
void initialize_leaf_node(void* node);

//Constants for internal nodes
#define INTERNAL_NODE_NUM_KEYS_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_NUM_KEYS_OFFSET COMMON_NODE_HEADER_SIZE
#define INTERNAL_NODE_RIGHT_CHILD_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_RIGHT_CHILD_OFFSET (INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE)
#define INTERNAL_NODE_HEADER_SIZE (COMMON_NODE_HEADER_SIZE + INTERNAL_NODE_NUM_KEYS_SIZE + INTERNAL_NODE_RIGHT_CHILD_SIZE)
//Internal node body layout
#define INTERNAL_NODE_KEY_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_CHILD_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_CELL_SIZE (INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE)

/*So from the above constants this is what our internal layout looks like
byte 0: node_type
byte 1: is_root
2-6 parent_pointer
6-9 num_keys
10-13 right child pointer
14-17 child pointer 0
18-21 key 0
22-25 child pointer 1
26-29 key 1
....
4086-4089 child pointer 509
4090-4093 key 509
4094-4095 wasted space

this is a *ridiculous* growth rate, 3 layers deep is about 550 gb of data total,
but since we only need to access 4 disk pages total (root + 2 internals + 1 leaf) we only have to load
16kb of memory to find our key (4 kb per node).
*/
//Gets num of keys in internal node
uint32_t* internal_node_num_keys(void* node);
//Gets the right child of the internal node
uint32_t* internal_node_right_child(void* node);
//Gets a cell within the internal node
uint32_t* internal_node_cell(void* node, uint32_t cell_num);
//Gets a numbered child within the internal node
uint32_t* internal_node_child(void* node, uint32_t child_num);
//Gets a key within the internal node.
uint32_t* internal_node_key(void* node, uint32_t key_num);
//Initializes internal node
initialize_internal_node(void* node);


//Returns the maximum key within a node
uint32_t get_node_max_key(void* node);

//determines if a node is the root
bool is_node_root(void* node);
//sets the node as the root or not
void set_node_root(void* node, bool is_root);

typedef struct {
	Pager* pager;
	uint32_t root_page_num;
} Table;

//Function for creating a new root in our btree
void create_new_root(Table* table, uint32_t right_child_page_num);

//Prints a row to console
void print_row(Row* row);
//Initializes table and pager, opens/creates database file
Table* db_open(const char* filename);
//Flushes memory to disk, closes db file, and frees table and pager on ".exit".
void db_close(Table* table);


//Represnts a location on the table
typedef struct {
	Table* table;
	uint32_t page_num;
	uint32_t cell_num;
	bool end_of_table;
} Cursor;
//Once we split our implementation into a BTree, this will make inserts, modifications, and deletes much easier.

//Return a cursor pointing at the start of a table
Cursor* table_start(Table* table);

/*
* Depreciated, functionality replaced with table_find
//Return a cursor at the end of a table
Cursor* table_end(Table* table);
*/

//Returns position of a given key or where the key should be inserted.
Cursor* table_find(Table* table, uint32_t key);
//returns pointer to position in table described by the cursor
void* cursor_value(Cursor* cursor);
//Advances cursor to the next row
void cursor_advance(Cursor* cursor);
//Inserts a leaf node into the tree
void leaf_node_insert(Cursor * cursor, uint32_t key, Row* value);
//Splits a full node into two and inserts a key
void leaf_node_split_and_insert(Cursor* cursor, uint32_t key, Row* value);
//Finds leaf node on a page with binary search.
Cursor* leaf_node_find(Table* table, uint32_t page_num, uint32_t key);

//Prints constant values relevant to leaf nodes
void print_constants();

/*
* depreciated, replaced by print_tree
//Prints out a leaf node.
void print_leaf_node(void* node);
*/

//adds an number of indents equal to level
void indent(uint32_t level);

void print_tree(Pager* pager, uint32_t page_num, uint32_t indentation_level);

#endif