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
	uint32_t num_rows = pager->file_length / ROW_SIZE;

	Table* table = malloc(sizeof(Table));
	table->pager = pager;
	table->num_rows = num_rows;
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
	}
	return pager->pages[page_num];
}
// Flushes page cache to disk, closes db file, frees memory for pager and table
void db_close(Table* table) {
	Pager* pager = table->pager;
	uint32_t num_full_pages = table->num_rows / ROWS_PER_PAGE;

	for (uint32_t i = 0; i < num_full_pages; i++) {
		if (pager->pages[i] == NULL) {
			continue;
		}
		pager_flush(pager, i, PAGE_SIZE);
		free(pager->pages[i]);
		pager->pages[i] = NULL;
	}
	//Handle partial page write off at the end of the file
	//Once we swap to btree this won't be necessary
	uint32_t num_additional_rows = table->num_rows % ROWS_PER_PAGE;
	if (num_additional_rows > 0) {
		uint32_t page_num = num_full_pages;
		if (pager->pages[page_num] != NULL) {
			pager_flush(pager, page_num, num_additional_rows * ROW_SIZE);
			free(pager->pages[page_num]);
			pager->pages[page_num] = NULL;
		}
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

void pager_flush(Pager* pager, uint32_t page_num, uint32_t size) {
	if (pager->pages[page_num] == NULL) {
		printf("Tried to flush null page\n");
		exit(EXIT_FAILURE);
	}
	off_t offset = _lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
	if (offset == -1) {
		printf("Error seeking: %d\n", errno);
		exit(EXIT_FAILURE);
	}

	ssize_t bytes_written = _write(pager->file_descriptor, pager->pages[page_num], size);
	if (bytes_written == -1) {
		printf("Error writing:%d\n", errno);
		exit(EXIT_FAILURE);
	}
}

void serialize_row(Row* source, void* destination) {
	memcpy((uint8_t*)destination + ID_OFFSET, &(source->id), ID_SIZE);
	memcpy((uint8_t*)destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
	memcpy((uint8_t*)destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
}
void deserialize_row(void* source, Row* destination) {
	memcpy(&(destination->id), (uint8_t*)source + ID_OFFSET, ID_SIZE);
	memcpy(&(destination->username), (uint8_t*)source + USERNAME_OFFSET, USERNAME_SIZE);
	memcpy(&(destination->email), (uint8_t*)source + EMAIL_OFFSET, EMAIL_SIZE);
}

void print_row(Row* row) {
	printf("(%d, %s, %s)\n", row->id, row->username, row->email);
}

Cursor* table_start(Table* table) {
	Cursor* cursor = malloc(sizeof(Cursor));
	cursor->table = table;
	cursor->row_num = 0;
	//if there are 0 rows, we're pointing at the end of the table!
	cursor->end_of_table = (table->num_rows == 0);
	return cursor;
}

Cursor* table_end(Table* table) {
	Cursor* cursor = malloc(sizeof(Cursor));
	cursor->table = table;
	cursor->row_num = table->num_rows;
	cursor->end_of_table = true;

	return cursor;
}
void cursor_advance(Cursor* cursor) {
	if (cursor->end_of_table) {
		printf("Advance ignored, at end of table");
		return;
	}
	cursor->row_num += 1;
	if (cursor->row_num >= cursor->table->num_rows) {
		//we're at the end of the table, so set this to true
		cursor->end_of_table = true;
	}
}

void* cursor_value(Cursor* cursor) {
	uint32_t row_num = cursor->row_num;
	uint32_t page_num = row_num / ROWS_PER_PAGE;
	void* page = get_page(cursor->table->pager, page_num);
	uint32_t row_offset = row_num % ROWS_PER_PAGE;
	uint32_t byte_offset = row_offset * ROW_SIZE;
	return (uint8_t*)page + byte_offset;
}