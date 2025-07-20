#ifndef DB_H
#define DB_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
// include fcntl for open() function and O_RDWR, O_CREAT flags
#include <fcntl.h>
// include fcntl for open() function
#include <unistd.h>
// include s_iwusr and s_irusr for file permissions
#include <errno.h>
#include <stdbool.h>
#include <sys/stat.h>

/** Column attributes sizes. */
#define COL_SIZE_USERNAME 32
#define COL_SIZE_EMAIL    255

/**
 * @brief Represents a single row in the database.
 * Implemented within repl.Command object
 */
typedef struct {
        uint32_t id;
        char username[COL_SIZE_USERNAME + 1];
        char email[COL_SIZE_EMAIL + 1];
} Row;

typedef enum {
        COMMAND_OK,
        COMMAND_SELECT,
        COMMAND_INSERT,
        COMMAND_UPDATE,
        COMMAND_DELETE,
        COMMAND_UNKNOWN,
        COMMAND_SYNTAX_ERR,
        COMMAND_SIZING_ERR,
} CommandType;

typedef struct {
        CommandType type;
        Row row;
} Command;

#define ATTR_SIZE(Struct, Attribute) sizeof(((Struct*)0)->Attribute)

/** Sizes of the Row attributes. */
#define SIZE_ID ATTR_SIZE(Row, id)
#define SIZE_UN ATTR_SIZE(Row, username)
#define SIZE_EM ATTR_SIZE(Row, email)

/** Total size of a Row. */
#define SIZE_ROW (SIZE_ID + SIZE_UN + SIZE_EM)

/** Offsets of the Row attributes. */
#define OFS_ID 0
#define OFS_UN (OFS_ID + SIZE_ID)
#define OFS_EM (OFS_UN + SIZE_UN)

/** Table attributes */
#define PAGE_SIZE       4096
#define TABLE_MAX_PAGES 100
#define ROWS_PER_PAGE   (PAGE_SIZE / SIZE_ROW)
#define TABLE_MAX_ROWS  (TABLE_MAX_PAGES * ROWS_PER_PAGE)

typedef struct {
        int fd;
        uint32_t file_len;
        uint32_t num_pages;
        void* pages[TABLE_MAX_PAGES];
} Pager;

typedef struct {
        uint32_t num_rows;
        uint32_t root_page;
        Pager* pager;
} Table;

typedef struct {
        Table* table;
        uint32_t page_num;
        uint32_t cell_num;
        bool table_end;
} Cursor;

typedef enum {
        NODE_INTERNAL,
        NODE_LEAF
} NodeType;

/**
 * Functions
 */
void exec_command(Command* cmd, Table* table);
Table* new_table(const char* filename);
void free_table(Table* table);
void print_tree(Pager* pager, uint32_t page_num, uint32_t indentation_level);
#endif // DB_H