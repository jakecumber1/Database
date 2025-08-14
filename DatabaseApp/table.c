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
//initialize by setting the number of cells of the node to 0
void initialize_leaf_node(void* node) { *leaf_node_num_cells(node) = 0; }

void leaf_node_insert(Cursor* cursor, uint32_t key, Row* value) {
	void* node = get_page(cursor->table->pager, cursor->page_num);

	uint32_t num_cells = *leaf_node_num_cells(node);
	if (num_cells >= LEAF_NODE_MAX_CELLS) {
		//this means the node is full
		printf("need to implement splitting leaf node.\n");
		exit(EXIT_FAILURE);
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

void print_leaf_node(void* node) {
	uint32_t num_cells = *leaf_node_num_cells(node);
	printf("leaf (size %d)\n", num_cells);
	for (uint32_t i = 0; i < num_cells; i++) {
		uint32_t key = *leaf_node_key(node, i);
		printf(" - %d : %d\n", i, key);
	}
}