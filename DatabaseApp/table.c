#include "Table.h"
//needed for ssize_t
#include "posix_comp.h"
//remember, needed for malloc and free
#include <stdlib.h>
//needed for memcpy
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

//Opens database file, initializing table and pager.
Table* db_open(const char* filename) {
	Pager* pager = pager_open(filename);

	Table* table = malloc(sizeof(Table));
	table->pager = pager;
	table->root_page_num = 0;

	if (pager->num_pages == 0) {
		//This means our database file is new. Initialize page 0 as leaf node.
		void* root_node = get_page(pager, 0);
		initialize_leaf_node(root_node);
		set_node_root(root_node, true);
	}
	return table;
}

//Initializes pager
Pager* pager_open(const char* filename) {
	/*O_RDWR = read/write
	O_CREAT = create if file doesn't exist
	S_IWRITE = User write permission
	S_IREAD = user read permission*/
	int fd = _open(filename, _O_RDWR | _O_CREAT, _S_IWRITE | _S_IREAD);

	if (fd == -1) {
		printf("Unable to open file\n");
		exit(EXIT_FAILURE);
	}

	off_t file_length = _lseek(fd, 0, SEEK_END);
	Pager* pager = malloc(sizeof(Pager));
	pager->file_descriptor = fd;
	pager->file_length = file_length;
	pager->num_pages = (file_length / PAGE_SIZE);

	if (file_length % PAGE_SIZE != 0) {
		printf("Db file is not a whole number of pages. Corrupt file.\n");
		exit(EXIT_FAILURE);
	}

	for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
		pager->pages[i] = NULL;
	}
	return pager;
}
//Fetches page
void* get_page(Pager* pager, uint32_t page_num) {
	if (page_num > TABLE_MAX_PAGES) {
		printf("Tried to fetch page number out of bounds. %d > %d", page_num, TABLE_MAX_PAGES);
		exit(EXIT_FAILURE);
	}
	if (pager->pages[page_num] == NULL) {
		//Cache miss. allocate memory and load from file.
		void* page = malloc(PAGE_SIZE);
		uint32_t num_pages = pager->file_length / PAGE_SIZE;
		//In case we want to save a partial page at the end of the file
		if (pager->file_length % PAGE_SIZE) {
			num_pages += 1;
		}

		if (page_num <= num_pages) {
			_lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
			ssize_t bytes_read = _read(pager->file_descriptor, page, PAGE_SIZE);
			if (bytes_read == -1) {
				printf("Error reading file: %d\n", errno);
				exit(EXIT_FAILURE);
			}
		}
		pager->pages[page_num] = page;
		if (page_num >= pager->num_pages) {
			pager->num_pages = page_num + 1;
		}
	}
	return pager->pages[page_num];
}

uint32_t get_unused_page_num(Pager* pager) {
	//We haven't implemented recycling free'd pages, so new pages will always go at the end
	return pager->num_pages;
}

// Flushes page cache to disk, closes db file, frees memory for pager and table
void db_close(Table* table) {
	Pager* pager = table->pager;

	for (uint32_t i = 0; i < pager->num_pages; i++) {
		if (pager->pages[i] == NULL) {
			continue;
		}
		pager_flush(pager, i);
		free(pager->pages[i]);
		pager->pages[i] = NULL;
	}

	int result = _close(pager->file_descriptor);
	if (result == -1) {
		printf("Error closing db file.\n");
		exit(EXIT_FAILURE);
	}
	for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
		void* page = pager->pages[i];
		if (page) {
			free(page);
			pager->pages[i] = NULL;
		}
	}
	free(pager);
	free(table);
}

void pager_flush(Pager* pager, uint32_t page_num) {
	if (pager->pages[page_num] == NULL) {
		printf("Tried to flush null page\n");
		exit(EXIT_FAILURE);
	}
	off_t offset = _lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
	if (offset == -1) {
		printf("Error seeking: %d\n", errno);
		exit(EXIT_FAILURE);
	}
	//Now that we've introduced cells, a cell takes up one page, so we don't need to worry about partial pages
	ssize_t bytes_written = _write(pager->file_descriptor, pager->pages[page_num], PAGE_SIZE);
	if (bytes_written == -1) {
		printf("Error writing:%d\n", errno);
		exit(EXIT_FAILURE);
	}
}

//Returns the number of cells in the node
//take the pointer to node in memory, and skip most of the header info, leaving you at the start of num_cells.
uint32_t* leaf_node_num_cells(void* node) {
	return (char*)node + LEAF_NODE_NUM_CELLS_OFFSET;
}
//Returns a cell in the node
//take the pointer to node in memory, skip the header entirely, then to access the cell of a number, multiply that number by cell size
void* leaf_node_cell(void* node, uint32_t cell_num) {
	return (char*)node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE;
}
//Returns the key for the relevant cell
//Same as the above function, since it returns the starting point of the cell (i.e the key)
uint32_t* leaf_node_key(void* node, uint32_t cell_num) {
	return leaf_node_cell(node, cell_num);
}
//Accesses the relevant cell's value
//Access the cell, skip the key, you have the value
void* leaf_node_value(void* node, uint32_t cell_num) {
	return (char*)leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE;
}

uint32_t* internal_node_num_keys(void* node) {
	return (char*)node + INTERNAL_NODE_NUM_KEYS_OFFSET;
}
uint32_t* internal_node_right_child(void* node) {
	return (char*)node + INTERNAL_NODE_RIGHT_CHILD_OFFSET;
}
uint32_t* internal_node_cell(void* node, uint32_t cell_num) {
	return (char*)node + INTERNAL_NODE_HEADER_SIZE + cell_num * INTERNAL_NODE_CELL_SIZE;
}
uint32_t* internal_node_child(void* node, uint32_t child_num) {
	uint32_t num_keys = *internal_node_num_keys(node);
	if (child_num > num_keys) {
		printf("tried to access child_num %d > num_keys %d\n", child_num, num_keys);
		exit(EXIT_FAILURE);
	}
	else if (child_num == num_keys) {
		return internal_node_right_child(node);
	}
	else {
		return internal_node_cell(node, child_num);
	}
}
//Remember the child pointer comes before the key pointer, so we want to skip that.
uint32_t* internal_node_key(void* node, uint32_t key_num) {
	return internal_node_cell(node, key_num) + INTERNAL_NODE_CHILD_SIZE;
}

initialize_internal_node(void* node) {
	set_node_type(node, NODE_INTERNAL);
	set_node_root(node, false);
	*internal_node_num_keys(node) = 0;
}

uint32_t get_node_max_key(void* node) {
	switch (get_node_type(node)) {
	case NODE_INTERNAL:
		return *internal_node_key(node, *internal_node_num_keys(node) - 1);
	case NODE_LEAF:
		return *leaf_node_key(node, *leaf_node_num_cells(node) - 1);
	}
}

NodeType get_node_type(void* node) {
	//Get the address of node type, cast to uint8_t* and dereference it for our value
	uint8_t value = *((uint8_t*)((char*)node + NODE_TYPE_OFFSET));
	return (NodeType)value;
}
void set_node_type(void* node, NodeType type) {
	uint8_t value = type;
	//Go to the node type in memory, store the value there as a uint8_t type.
	*((uint8_t*)((char*)node + NODE_TYPE_OFFSET)) = value;
}

//initialize by setting the number of cells of the node to 0
void initialize_leaf_node(void* node) { 
	set_node_type(node, NODE_LEAF);
	set_node_root(node, false);
	*leaf_node_num_cells(node) = 0; 
}

//determines if a node is the root
bool is_node_root(void* node) {
	uint8_t value = *((uint8_t*)((char*)node + IS_ROOT_OFFSET));
	return (bool)value;
}
//sets the node as the root or not
void set_node_root(void* node, bool is_root) {
	uint8_t value = is_root;
	*((uint8_t*)((char*)node + IS_ROOT_OFFSET)) = value;
}

/*let N be the root node, allocate L and R as children, move the lower half of N to L and the upper half into R
NOW N is empty, add (L, K, R) in N where K is the max key in L, N remains the root*/
void create_new_root(Table* table, uint32_t right_child_page_num) {
	void* root = get_page(table->pager, table->root_page_num);
	void* right_child = get_page(table->pager, right_child_page_num);
	uint32_t left_child_page_num = get_unused_page_num(table->pager);
	void* left_child = get_page(table->pager, right_child_page_num);
	memcpy(left_child, root, PAGE_SIZE);
	set_node_root(left_child, false);

	//Root node is new internal node w one key and 2 children
	initialize_internal_node(root);
	set_node_root(root, true);
	*internal_node_num_keys(root) = 1;
	*internal_node_child(root, 0) = left_child_page_num;
	uint32_t left_child_max_key = get_node_max_key(left_child);
	*internal_node_key(root, 0) = left_child_max_key;
	*internal_node_right_child(root) = right_child_page_num;
}



void leaf_node_insert(Cursor* cursor, uint32_t key, Row* value) {
	void* node = get_page(cursor->table->pager, cursor->page_num);

	uint32_t num_cells = *leaf_node_num_cells(node);
	if (num_cells >= LEAF_NODE_MAX_CELLS) {
		//this means the node is full and needs to be split
		leaf_node_split_and_insert(cursor, key, value);
		return;
	}
	if (cursor->cell_num < num_cells) {
		//space needs to be made for the new cell, shift all nodes to the right of the cursor over 1.
		for (uint32_t i = num_cells; i > cursor->cell_num; i--) {
			memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i - 1), LEAF_NODE_CELL_SIZE);
		}
	}
	*(leaf_node_num_cells(node)) += 1;
	*(leaf_node_key(node, cursor->cell_num)) = key;
	serialize_row(value, leaf_node_value(node, cursor->cell_num));
}

void leaf_node_split_and_insert(Cursor* cursor, uint32_t key, Row* value) {
	void* old_node = get_page(cursor->table->pager, cursor->page_num);
	uint32_t new_page_num = get_unused_page_num(cursor->table->pager);
	void* new_node = get_page(cursor->table->pager, new_page_num);
	initialize_leaf_node(new_node);

	//next we need to make sure all existing and  the new key are divided evenly going from right to left
	for (int32_t i = LEAF_NODE_MAX_CELLS; i >= 0; i--) {
		void* destination_node;
		if (i >= LEAF_NODE_LEFT_SPLIT_COUNT) {
			destination_node = new_node;
		}
		else {
			destination_node = old_node;
		}
		uint32_t index_within_node = i % LEAF_NODE_LEFT_SPLIT_COUNT;
		void* destination = leaf_node_cell(destination_node, index_within_node);

		if (i == cursor->cell_num) {
			serialize_row(value, destination);
		}
		else if (i > cursor->cell_num) {
			memcpy(destination, leaf_node_cell(old_node, i - 1), LEAF_NODE_CELL_SIZE);
		}
		else {
			memcpy(destination, leaf_node_cell(old_node, i), LEAF_NODE_CELL_SIZE);
		}
	}
	//Now we need to make sure the cell count on both leafs are correct
	*(leaf_node_num_cells(old_node)) = LEAF_NODE_LEFT_SPLIT_COUNT;
	*(leaf_node_num_cells(new_node)) - LEAF_NODE_RIGHT_SPLIT_COUNT;
	//Finally we need to update the parent and make sure it points to both nodes, if it was the root we need to create a parent for it
	if (is_node_root(old_node)) {
		return create_new_root(cursor->table, new_page_num);
	}
	else {
		printf("Need to implment updating parent after splitting");
		exit(EXIT_FAILURE);
	}
}

void serialize_row(Row* source, void* destination) {
	memcpy((char*)destination + ID_OFFSET, &(source->id), ID_SIZE);
	memcpy((char*)destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
	memcpy((char*)destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
}
void deserialize_row(void* source, Row* destination) {
	memcpy(&(destination->id), (char*)source + ID_OFFSET, ID_SIZE);
	memcpy(&(destination->username), (char*)source + USERNAME_OFFSET, USERNAME_SIZE);
	memcpy(&(destination->email), (char*)source + EMAIL_OFFSET, EMAIL_SIZE);
}

void print_row(Row* row) {
	printf("(%d, %s, %s)\n", row->id, row->username, row->email);
}

Cursor* table_start(Table* table) {
	Cursor* cursor = malloc(sizeof(Cursor));
	cursor->table = table;
	cursor->page_num = table->root_page_num;
	cursor->cell_num = 0;

	void* root_node = get_page(table->pager, table->root_page_num);
	uint32_t num_cells = *leaf_node_num_cells(root_node);
	cursor->end_of_table = (num_cells == 0);
	return cursor;
}

/*
* Depreciated, replaced by table_find
Cursor* table_end(Table* table) {
	Cursor* cursor = malloc(sizeof(Cursor));
	cursor->table = table;
	cursor->page_num = table->root_page_num;
	void* root_node = get_page(table->pager, table->root_page_num);
	uint32_t num_cells = *leaf_node_num_cells(root_node);
	cursor->cell_num = num_cells;
	cursor->end_of_table = true;

	return cursor;
}
*/


//Returns the position of the key, the position of the key we'll need to move, or one position past the last key
Cursor* leaf_node_find(Table* table, uint32_t page_num, uint32_t key) {
	void* node = get_page(table->pager, page_num);
	uint32_t num_cells = *leaf_node_num_cells(node);
	Cursor* cursor = malloc(sizeof(Cursor));
	cursor->table = table;
	cursor->page_num = page_num;

	//binary search
	uint32_t min_index = 0;
	uint32_t one_past_max_index = num_cells;
	while (one_past_max_index != min_index) {
		uint32_t index = (min_index + one_past_max_index) / 2;
		uint32_t key_at_index = *leaf_node_key(node, index);

		if (key == key_at_index) {
			cursor->cell_num = index;
			return cursor;
		}
		if (key < key_at_index) {
			one_past_max_index = index;
		}
		else {
			min_index = index + 1;
		}
	}
	cursor->cell_num = min_index;
	return cursor;
}

Cursor* internal_node_find(Table* table, uint32_t page_num, uint32_t key) {
	void* node = get_page(table->pager, page_num);
	uint32_t num_keys = *internal_node_num_keys(node);
	//Binary search to find index to search

	uint32_t min_index = 0;
	uint32_t max_index = num_keys; //one more child than keys
	while (min_index != max_index) {
		uint32_t index = (min_index + max_index) / 2;
		uint32_t key_to_right = *internal_node_key(node, index);
		if (key_to_right >= key) {
			//our key must be to the left of this one (or is this one)
			max_index = index;
		}
		else {
			//our key must be to the right of this one
			min_index = index + 1;
		}
	}
	uint32_t child_num = (*internal_node_child(node, min_index));
	void* child = get_page(table->pager, child_num);
	switch (get_node_type(child)) {
	case NODE_LEAF:
		return leaf_node_find(table, child_num, key);
	case NODE_INTERNAL:
		return internal_node_find(table, child_num, key);
	}
}

Cursor* table_find(Table* table, uint32_t key) {
	uint32_t root_page_num = table->root_page_num;
	void* root_node = get_page(table->pager, root_page_num);

	if (get_node_type(root_node) == NODE_LEAF) {
		return leaf_node_find(table, root_page_num, key);
	}
	else {
		return internal_node_find(table, root_page_num, key);
	}
}

void cursor_advance(Cursor* cursor) {
	uint32_t page_num = cursor->page_num;
	void* node = get_page(cursor->table->pager, page_num);
	cursor->cell_num += 1;
	if (cursor->cell_num >= (*leaf_node_num_cells(node))) {
		cursor->end_of_table = true;
	}
}

void* cursor_value(Cursor* cursor) {
	uint32_t page_num = cursor->page_num;
	void* page = get_page(cursor->table->pager, page_num);
	return leaf_node_value(page, cursor->cell_num);
}
void print_constants() {
	printf("ROW_SIZE: %d\n", ROW_SIZE);
	printf("COMMON_NODE_HEADER_SIZE: %d\n", COMMON_NODE_HEADER_SIZE);
	printf("LEAF_NODE_HEADER_SIZE: %d\n", LEAF_NODE_HEADER_SIZE);
	printf("LEAF_NODE_CELL_SIZE: %d\n", LEAF_NODE_CELL_SIZE);
	printf("LEAF_NODE_SPACE_FOR_CELLS: %d\n", LEAF_NODE_SPACE_FOR_CELLS);
	printf("LEAF_NODE_MAX_CELLS: %d\n", LEAF_NODE_MAX_CELLS);
}
/*
* depreciated, replaced by print_tree
void print_leaf_node(void* node) {
	uint32_t num_cells = *leaf_node_num_cells(node);
	printf("leaf (size %d)\n", num_cells);
	for (uint32_t i = 0; i < num_cells; i++) {
		uint32_t key = *leaf_node_key(node, i);
		printf(" - %d : %d\n", i, key);
	}
}
*/
void indent(uint32_t level) {
	for (uint32_t i = 0; i < level; i++) {
		printf("  ");
	}
}

//Prints out our BTree
void print_tree(Pager* pager, uint32_t page_num, uint32_t indentation_level) {
	void* node = get_page(pager, page_num);
	uint32_t num_keys, child;

	switch (get_node_type(node)) {
	case (NODE_LEAF):
		num_keys = *leaf_node_num_cells(node);
		indent(indentation_level);
		printf("- leaf (size %d)\n", num_keys);
		for (uint32_t i = 0; i < num_keys; i++) {
			indent(indentation_level + 1);
			printf("- %d\n", *leaf_node_key(node, i));
		}
		break;
	case (NODE_INTERNAL):
		num_keys = *internal_node_num_keys(node);
		indent(indentation_level);
		printf("- internal (size %d)\n", num_keys);
		for (uint32_t i = 0; i < num_keys; i++) {
			child = *internal_node_child(node, i);
			print_tree(pager, child, indentation_level + 1);

			indent(indentation_level + 1);
			printf("- key %d\n", *internal_node_key(node, i));
		}
		child = *internal_node_right_child(node);
		print_tree(pager, child, indentation_level + 1);
		break;
	}
}