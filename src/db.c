#include "db.h"

/** B-Tree Node Constants */
const uint32_t BTREE_ORDER = 3; // Max children per node

const uint32_t BTREE_NODE_TYPE_SIZE = sizeof(uint8_t); // Type of the node (leaf or internal)
const uint32_t BTREE_NODE_TYPE_OFFSET = 0;

const uint32_t BTREE_IS_ROOT_SIZE = sizeof(uint8_t);
const uint32_t BTREE_IS_ROOT_OFFSET = BTREE_NODE_TYPE_SIZE;
const uint32_t BTREE_PARENT_POINTER_SIZE = sizeof(uint32_t);

const uint32_t BTREE_PARENT_POINTER_OFFSET = BTREE_IS_ROOT_OFFSET + BTREE_IS_ROOT_SIZE;

const uint8_t BTREE_COMMON_NODE_HEADER_SIZE = BTREE_NODE_TYPE_SIZE + BTREE_IS_ROOT_SIZE + BTREE_PARENT_POINTER_SIZE;

/** Leaf Node Constants  */
const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = BTREE_COMMON_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_NEXT_LEAF_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NEXT_LEAF_OFFSET = LEAF_NODE_NUM_CELLS_OFFSET + LEAF_NODE_NUM_CELLS_SIZE;
const uint32_t LEAF_NODE_HEADER_SIZE = BTREE_COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE +
                                       LEAF_NODE_NEXT_LEAF_SIZE;

/*
 * Leaf Node Body Layout
 */
const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_KEY_OFFSET = 0;
const uint32_t LEAF_NODE_VALUE_SIZE = SIZE_ROW;
const uint32_t LEAF_NODE_VALUE_OFFSET = LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE;
const uint32_t LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;
const uint32_t LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_MAX_CELLS = LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;

const uint32_t LEAF_NODE_RIGHT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) / 2;
const uint32_t LEAF_NODE_LEFT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) - LEAF_NODE_RIGHT_SPLIT_COUNT;

/*
 * Internal Node Header Layout
 */
const uint32_t INTERNAL_NODE_NUM_KEYS_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET = BTREE_COMMON_NODE_HEADER_SIZE;
const uint32_t INTERNAL_NODE_RIGHT_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_RIGHT_CHILD_OFFSET = INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE;
const uint32_t INTERNAL_NODE_HEADER_SIZE = BTREE_COMMON_NODE_HEADER_SIZE + INTERNAL_NODE_NUM_KEYS_SIZE +
                                           INTERNAL_NODE_RIGHT_CHILD_SIZE;

/*
 * Internal Node Body Layout
 */
const uint32_t INTERNAL_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CELL_SIZE = INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE;
const uint32_t INTERNAL_NODE_MAX_CELLS = 3;

#define INVALID_PAGE_NUM UINT32_MAX

void* get_page(Pager* pager, uint32_t page_num);

void intnode_split_and_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num);

uint32_t*
node_parent(void* node) {
        return node + BTREE_PARENT_POINTER_OFFSET;
}

uint32_t
get_unused_page_num(Pager* pager) {
        return pager->num_pages;
}

NodeType
get_node_type(void* node) {
        uint8_t value = *((uint8_t*)(node + BTREE_NODE_TYPE_OFFSET));
        return (NodeType)value;
}

void
set_node_type(void* node, NodeType type) {
        uint8_t value = type;
        *((uint8_t*)(node + BTREE_NODE_TYPE_OFFSET)) = value;
}

void
serialize_row(Row* row, char* buffer) {
        memcpy(buffer + OFS_ID, &row->id, SIZE_ID);
        memcpy(buffer + OFS_UN, row->username, SIZE_UN);
        memcpy(buffer + OFS_EM, row->email, SIZE_EM);
}

void
deserialize_row(const char* buffer, Row* row) {
        memcpy(&row->id, buffer + OFS_ID, SIZE_ID);
        memcpy(row->username, buffer + OFS_UN, SIZE_UN);
        memcpy(row->email, buffer + OFS_EM, SIZE_EM);
}

bool
is_node_root(void* node) {
        uint8_t value = *((uint8_t*)(node + BTREE_IS_ROOT_OFFSET));
        return (bool)value;
}

void
set_node_root(void* node, bool is_root) {
        uint8_t value = is_root;
        *((uint8_t*)(node + BTREE_IS_ROOT_OFFSET)) = value;
}

void*
leafnode_cell(void* node, uint32_t cell_num) {
        return (char*)node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE;
}

void*
leafnode_val(void* node, uint32_t cell_num) {
        return (char*)leafnode_cell(node, cell_num) + LEAF_NODE_VALUE_OFFSET;
}

uint32_t*
leafnode_num_cells(void* node) {
        return (uint32_t*)((char*)node + LEAF_NODE_NUM_CELLS_OFFSET);
}

uint32_t*
leafnode_get_key(void* node, uint32_t cell_num) {
        return leafnode_cell(node, cell_num);
}

uint32_t*
leafnode_next_leaf(void* node) {
        return node + LEAF_NODE_NEXT_LEAF_OFFSET;
}

uint32_t*
intnode_num_keys(void* node) {
        return node + INTERNAL_NODE_NUM_KEYS_OFFSET;
}

uint32_t*
intnode_right_child(void* node) {
        return node + INTERNAL_NODE_RIGHT_CHILD_OFFSET;
}

uint32_t*
intnode_cell(void* node, uint32_t cell_num) {
        return node + INTERNAL_NODE_HEADER_SIZE + cell_num * INTERNAL_NODE_CELL_SIZE;
}

void
new_leafnode(void* node) {
        set_node_type(node, NODE_LEAF);
        set_node_root(node, false);
        *leafnode_num_cells(node) = 0;
        *leafnode_next_leaf(node) = 0; // No next leaf node
}

void
new_intnode(void* node) {
        set_node_type(node, NODE_INTERNAL);
        set_node_root(node, false);
        *intnode_num_keys(node) = 0;
        *intnode_right_child(node) = INVALID_PAGE_NUM;
}

uint32_t*
intnode_key(void* node, uint32_t key_num) {
        return (void*)intnode_cell(node, key_num) + INTERNAL_NODE_CHILD_SIZE;
}

uint32_t
get_node_max_key(Pager* pager, void* node) {
        /** 
         * Internal nodes maximum key is the far right key
         * Leaf nodes maximum key is the last key 
         */
        if (get_node_type(node) == NODE_LEAF) {
                return *leafnode_get_key(node, *leafnode_num_cells(node) - 1);
        }
        void* right_child = get_page(pager, *intnode_right_child(node));
        return get_node_max_key(pager, right_child);
}

Cursor*
leafnode_find(Table* table, uint32_t page_num, uint32_t key) {
        void* node = get_page(table->pager, page_num);
        uint32_t len = *leafnode_num_cells(node);

        Cursor* cursor = malloc(sizeof(Cursor));
        cursor->table = table;
        cursor->page_num = page_num;

        uint32_t l = 0;
        uint32_t r = len;

        while (r != l) {
                uint32_t m = (l + r) / 2;
                uint32_t found = *leafnode_get_key(node, m);
                if (key == found) {
                        cursor->cell_num = m;
                        return cursor;
                } else if (key < found) {
                        r = m;
                } else {
                        l = m + 1;
                }
        }
        cursor->cell_num = l;
        return cursor;
}

uint32_t
intnode_find_child(void* node, uint32_t key) {
        /** Return the index of the child which should contain the given key. */
        uint32_t num_keys = *intnode_num_keys(node);
        uint32_t l = 0;
        uint32_t r = num_keys; /* there is one more child than key */
        while (l != r) {
                uint32_t m = (l + r) / 2;
                if (*intnode_key(node, m) >= key)
                        r = m;
                else
                        l = m + 1;
        }

        return l;
}

uint32_t*
intnode_get_child(void* node, uint32_t child_num) {
        uint32_t num_keys = *intnode_num_keys(node);

        if (child_num > num_keys) {
                printf("Tried to access child_num %d > num_keys %d\n", child_num, num_keys);
                exit(EXIT_FAILURE);

        } else if (child_num == num_keys) {
                uint32_t* right_child = intnode_right_child(node);
                if (*right_child == INVALID_PAGE_NUM) {
                        printf("Tried to access right child of node, but was invalid page\n");
                        exit(EXIT_FAILURE);
                }
                return right_child;
        } else {
                uint32_t* child = intnode_cell(node, child_num);
                if (*child == INVALID_PAGE_NUM) {
                        printf("Tried to access child %d of node, but was invalid page\n", child_num);
                        exit(EXIT_FAILURE);
                }
                return child;
        }
}

void
intnode_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num) {
        /*
        Add a new child/key pair to parent that corresponds to child
        */
        void* parent = get_page(table->pager, parent_page_num);
        void* child = get_page(table->pager, child_page_num);

        uint32_t child_max_key = get_node_max_key(table->pager, child);
        uint32_t index = intnode_find_child(parent, child_max_key);

        uint32_t original_num_keys = *intnode_num_keys(parent);
        *intnode_num_keys(parent) = original_num_keys + 1;

        if (original_num_keys >= INTERNAL_NODE_MAX_CELLS) {
                intnode_split_and_insert(table, parent_page_num, child_page_num);
                return;
        }

        uint32_t right_child_page_num = *intnode_right_child(parent);
        void* right_child = get_page(table->pager, right_child_page_num);

        if (child_max_key > get_node_max_key(table->pager, right_child)) {
                /* Replace right child */
                *intnode_get_child(parent, original_num_keys) = right_child_page_num;
                *intnode_key(parent, original_num_keys) = get_node_max_key(table->pager, right_child);
                *intnode_right_child(parent) = child_page_num;
        } else {
                /* Make room for the new cell */
                for (uint32_t i = original_num_keys; i > index; i--) {
                        void* destination = intnode_cell(parent, i);
                        void* source = intnode_cell(parent, i - 1);
                        memcpy(destination, source, INTERNAL_NODE_CELL_SIZE);
                }
                *intnode_get_child(parent, index) = child_page_num;
                *intnode_key(parent, index) = child_max_key;
        }
}

void
update_internal_node_key(void* node, uint32_t old_key, uint32_t new_key) {
        uint32_t old_child_index = intnode_find_child(node, old_key);
        *intnode_key(node, old_child_index) = new_key;
}



void
create_new_root(Table* table, uint32_t right_child_page_num) {
        void* root = get_page(table->pager, table->root_page);
        void* right_child = get_page(table->pager, right_child_page_num);
        uint32_t left_child_page_num = get_unused_page_num(table->pager);
        void* left_child = get_page(table->pager, left_child_page_num);

        if (get_node_type(root) == NODE_INTERNAL) {
                new_intnode(right_child);
                new_intnode(left_child);
        }
        /* Left child has data copied from old root */
        memcpy(left_child, root, PAGE_SIZE);
        set_node_root(left_child, false);

        if (get_node_type(left_child) == NODE_INTERNAL) {
                void* child;
                for (int i = 0; i < *intnode_num_keys(left_child); i++) {
                        child = get_page(table->pager, *intnode_get_child(left_child, i));
                        *node_parent(child) = left_child_page_num;
                }
                child = get_page(table->pager, *intnode_right_child(left_child));
                *node_parent(child) = left_child_page_num;
        }
        /* Root node is a new internal node with one key and two children */
        new_intnode(root);
        set_node_root(root, true);
        *intnode_num_keys(root) = 1;
        *intnode_get_child(root, 0) = left_child_page_num;
        uint32_t left_child_max_key = get_node_max_key(table->pager, left_child);
        *intnode_key(root, 0) = left_child_max_key;
        *intnode_right_child(root) = right_child_page_num;
        *node_parent(left_child) = table->root_page;
        *node_parent(right_child) = table->root_page;
}

void
intnode_split_and_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num) {
        uint32_t old_page_num = parent_page_num;
        void* old_node = get_page(table->pager, parent_page_num);
        uint32_t old_max = get_node_max_key(table->pager, old_node);
        void* child = get_page(table->pager, child_page_num);
        uint32_t child_max = get_node_max_key(table->pager, child);
        uint32_t new_page_num = get_unused_page_num(table->pager);
        uint32_t splitting_root = is_node_root(old_node);

        void* parent;
        void* new_node;
        if (splitting_root) {
                create_new_root(table, new_page_num);
                parent = get_page(table->pager, table->root_page);
                old_page_num = *intnode_get_child(parent, 0);
                old_node = get_page(table->pager, old_page_num);
        } else {
                parent = get_page(table->pager, *node_parent(old_node));
                new_node = get_page(table->pager, new_page_num);
                new_intnode(new_node);
        }

        uint32_t* old_num_keys = intnode_num_keys(old_node);
        uint32_t cur_page_num = *intnode_right_child(old_node);
        void* cur = get_page(table->pager, cur_page_num);

        intnode_insert(table, new_page_num, cur_page_num);
        *node_parent(cur) = new_page_num;
        *intnode_right_child(old_node) = INVALID_PAGE_NUM;
        for (int i = INTERNAL_NODE_MAX_CELLS - 1; i > INTERNAL_NODE_MAX_CELLS / 2; i--) {
                cur_page_num = *intnode_get_child(old_node, i);
                cur = get_page(table->pager, cur_page_num);
                intnode_insert(table, new_page_num, cur_page_num);
                *node_parent(cur) = new_page_num;
                (*old_num_keys)--;
        }

        *intnode_right_child(old_node) = *intnode_get_child(old_node, *old_num_keys - 1);
        (*old_num_keys)--;

        uint32_t max_after_split = get_node_max_key(table->pager, old_node);

        uint32_t destination_page_num = child_max < max_after_split ? old_page_num : new_page_num;

        intnode_insert(table, destination_page_num, child_page_num);
        *node_parent(child) = destination_page_num;

        update_internal_node_key(parent, old_max, get_node_max_key(table->pager, old_node));

        if (!splitting_root) {
                intnode_insert(table, *node_parent(old_node), new_page_num);
                *node_parent(new_node) = *node_parent(old_node);
        }
}

Cursor*
intnode_find(Table* table, uint32_t page_num, uint32_t key) {
        void* node = get_page(table->pager, page_num);
        uint32_t child_index = intnode_find_child(node, key);
        uint32_t child_num = *intnode_get_child(node, child_index);
        void* child = get_page(table->pager, child_num);
        switch (get_node_type(child)) {
                case NODE_LEAF: return leafnode_find(table, child_num, key);
                case NODE_INTERNAL: return intnode_find(table, child_num, key);
        }
}



void
leaf_node_split_and_insert(Cursor* cursor, uint32_t key, Row* value) {
        dblog("leaf_node_split_and_insert()");

        void* old_node = get_page(cursor->table->pager, cursor->page_num);
        uint32_t old_max = get_node_max_key(cursor->table->pager, old_node);
        uint32_t new_page_num = get_unused_page_num(cursor->table->pager);
        void* new_node = get_page(cursor->table->pager, new_page_num);
        new_leafnode(new_node);
        *node_parent(new_node) = *node_parent(old_node);
        *leafnode_next_leaf(new_node) = *leafnode_next_leaf(old_node);
        *leafnode_next_leaf(old_node) = new_page_num;

        for (int32_t i = LEAF_NODE_MAX_CELLS; i >= 0; i--) {
                void* destination_node;
                if (i >= LEAF_NODE_LEFT_SPLIT_COUNT) {
                        destination_node = new_node;
                } else {
                        destination_node = old_node;
                }
                uint32_t index_within_node = i % LEAF_NODE_LEFT_SPLIT_COUNT;
                void* destination = leafnode_cell(destination_node, index_within_node);

                if (i == cursor->cell_num) {
                        serialize_row(value, leafnode_val(destination_node, index_within_node));
                        *leafnode_get_key(destination_node, index_within_node) = key;
                } else if (i > cursor->cell_num) {
                        memcpy(destination, leafnode_cell(old_node, i - 1), LEAF_NODE_CELL_SIZE);
                } else {
                        memcpy(destination, leafnode_cell(old_node, i), LEAF_NODE_CELL_SIZE);
                }
        }
        /* Update cell count on both leaf nodes */
        *(leafnode_num_cells(old_node)) = LEAF_NODE_LEFT_SPLIT_COUNT;
        *(leafnode_num_cells(new_node)) = LEAF_NODE_RIGHT_SPLIT_COUNT;
        if (is_node_root(old_node)) {
                return create_new_root(cursor->table, new_page_num);
        } else {
                uint32_t parent_page_num = *node_parent(old_node);
                uint32_t new_max = get_node_max_key(cursor->table->pager, new_node);
                void* parent = get_page(cursor->table->pager, parent_page_num);
                update_internal_node_key(parent, old_max, new_max);
                intnode_insert(cursor->table, parent_page_num, new_page_num);
                return;
        }
}

void
leafnode_insert(Cursor* cursor, uint32_t key, Row* value) {
        void* node = get_page(cursor->table->pager, cursor->page_num);

        uint32_t num_cells = *leafnode_num_cells(node);
        if (num_cells >= LEAF_NODE_MAX_CELLS) {
                // Node full
                leaf_node_split_and_insert(cursor, key, value);
                return;
        }

        if (cursor->cell_num < num_cells) {
                // Make room for new cell
                for (uint32_t i = num_cells; i > cursor->cell_num; i--) {
                        memcpy(leafnode_cell(node, i), leafnode_cell(node, i - 1), LEAF_NODE_CELL_SIZE);
                }
        }

        *(leafnode_num_cells(node)) += 1;
        *(leafnode_get_key(node, cursor->cell_num)) = key;
        serialize_row(value, leafnode_val(node, cursor->cell_num));
}

Pager*
new_pager(const char* filename) {
        int fd = open(filename,
                      O_RDWR |  // Read/Write mode
                      O_CREAT,  // Create file if it does not exist
                      S_IWUSR | // User write permission
                      S_IRUSR   // User read permission
        );

        if (fd == -1) {
                printf("Unable to open file\n");
                exit(EXIT_FAILURE);
        }

        // lseek() returns the offset of the file pointer, which is the file length if
        // we seek to the end of the file.
        off_t file_length = lseek(fd, 0, SEEK_END);
        if (file_length % PAGE_SIZE != 0) {
                printf("Db file is not a whole number of pages. Corrupt file.\n");
                exit(EXIT_FAILURE);
        }

        // Create a new Pager instance and initialize it.
        Pager* pager = malloc(sizeof(Pager));
        pager->num_pages = file_length / PAGE_SIZE;
        pager->file_len = file_length;
        pager->fd = fd;

        for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) pager->pages[i] = NULL;
        return pager;
}

Table*
new_table(const char* filename) {
        Table* table = (Table*)malloc(sizeof(Table));
        if (!table) {
                perror("Failed to allocate memory for table");
                return NULL;
        }

        Pager* pager = new_pager(filename);
        if (!pager) {
                perror("Failed to create pager");
                free(table);
                return NULL;
        }

        table->pager = pager;
        table->root_page = 0; // Initialize root page to 0
        if (pager->num_pages == 0) {
                // If the file is empty, create a new root page.
                void* root_node = get_page(pager, 0);
                new_leafnode(root_node);
                set_node_root(root_node, true);
                // *leafnode_num_cells(root_node) = 0; // Initialize number of cells to 0
        }
        // table->num_rows = 0;
        return table;
}

void*
get_page(Pager* pager, uint32_t page_num) {
        if (page_num > TABLE_MAX_PAGES) {
                printf("Tried to fetch page number out of bounds. %d > %d\n", page_num, TABLE_MAX_PAGES);
                exit(EXIT_FAILURE);
        }

        if (pager->pages[page_num] != NULL)
                return pager->pages[page_num]; // Cache hit. Return the page.

        void* page = malloc(PAGE_SIZE);
        uint32_t num_pages = pager->file_len / PAGE_SIZE;

        if (pager->file_len % PAGE_SIZE) // Partial page at end of file
                num_pages += 1;

        if (page_num <= num_pages) {
                lseek(pager->fd, page_num * PAGE_SIZE, SEEK_SET);
                ssize_t bytes_read = read(pager->fd, page, PAGE_SIZE);
                if (bytes_read == -1) {
                        printf("Error reading file: %d\n", errno);
                        exit(EXIT_FAILURE);
                }
        }
        pager->pages[page_num] = page;

        if (page_num >= pager->num_pages)
                pager->num_pages = page_num + 1;

        return pager->pages[page_num];
}

void
pager_flush(Pager* pager, uint32_t page_num) {
        if (pager->pages[page_num] == NULL) {
                printf("Tried to flush null page\n");
                exit(EXIT_FAILURE);
        }

        off_t offset = lseek(pager->fd, page_num * PAGE_SIZE, SEEK_SET);

        if (offset == -1) {
                printf("Error seeking: %d\n", errno);
                exit(EXIT_FAILURE);
        }

        ssize_t bytes_written = write(pager->fd, pager->pages[page_num], PAGE_SIZE);

        if (bytes_written == -1) {
                printf("Error writing: %d\n", errno);
                exit(EXIT_FAILURE);
        }
}

void
free_table(Table* table) {
        Pager* pager = table->pager;

        for (uint32_t i = 0; i < pager->num_pages; i++) {
                if (pager->pages[i] == NULL) {
                        continue;
                }
                pager_flush(pager, i);
                free(pager->pages[i]);
                pager->pages[i] = NULL;
        }

        int result = close(pager->fd);
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

void
print_row(Row* row) {
        printf("(%d, %s, %s)\n", row->id, row->username, row->email);
}

Cursor*
table_find(Table* table, uint32_t key) {
        uint32_t root_page = table->root_page;

        void* root = get_page(table->pager, root_page);

        if (get_node_type(root) == NODE_LEAF) {
                return leafnode_find(table, root_page, key);
        } else {
                return intnode_find(table, root_page, key);
        }
}

Cursor*
table_start(Table* table) {
        Cursor* cursor = table_find(table, 0);
        void* node = get_page(table->pager, cursor->page_num);
        uint32_t num_cells = *leafnode_num_cells(node);
        cursor->table_end = (num_cells == 0);
        return cursor;
}

void*
cursor_value(Cursor* cursor) {
        uint32_t page_num = cursor->page_num;
        void* page = get_page(cursor->table->pager, page_num);
        return leafnode_val(page, cursor->cell_num);
}

void
cursor_advance(Cursor* cursor) {
        uint32_t page_num = cursor->page_num;
        void* page = get_page(cursor->table->pager, page_num);

        cursor->cell_num += 1;
        if (cursor->cell_num >= *leafnode_num_cells(page)) {
                uint32_t next_page_num = *leafnode_next_leaf(page);
                if (next_page_num == 0) {
                        /* This was rightmost leaf */
                        cursor->table_end = true;
                } else {
                        cursor->page_num = next_page_num;
                        cursor->cell_num = 0;
                }
        }
}

void
indent(uint32_t level) {
        for (uint32_t i = 0; i < level; i++) {
                printf("  ");
        }
}

void
print_tree(Pager* pager, uint32_t page_num, uint32_t indentation_level) {
        void* node = get_page(pager, page_num);
        uint32_t num_keys, child;

        switch (get_node_type(node)) {
                case (NODE_LEAF):
                        num_keys = *leafnode_num_cells(node);
                        indent(indentation_level);
                        printf("- leaf (size %d)\n", num_keys);
                        for (uint32_t i = 0; i < num_keys; i++) {
                                indent(indentation_level + 1);
                                printf("- %d\n", *leafnode_get_key(node, i));
                        }
                        break;
                case (NODE_INTERNAL):
                        num_keys = *intnode_num_keys(node);
                        indent(indentation_level);
                        printf("- internal (size %d)\n", num_keys);
                        if (num_keys > 0) {
                                for (uint32_t i = 0; i < num_keys; i++) {
                                        child = *intnode_get_child(node, i);
                                        print_tree(pager, child, indentation_level + 1);
                                        indent(indentation_level + 1);
                                        printf("- key %d\n", *intnode_key(node, i));
                                }
                                child = *intnode_right_child(node);
                                print_tree(pager, child, indentation_level + 1);
                        }
                        break;
        }
}

void
exec_insert(Command* cmd, Table* table) {
        replog("Executing insert command");

        void* node = get_page(table->pager, table->root_page);
        uint32_t num_cells = *leafnode_num_cells(node);

        //log user name and email sizes
        replog("username size: %zu, email size: %zu", strlen(cmd->row.username), strlen(cmd->row.email));

        Row* row = &cmd->row;
        uint32_t key_to_insert = row->id;
        Cursor* cursor = table_find(table, key_to_insert);

        if (cursor->cell_num < num_cells) {
                uint32_t key_at_index = *leafnode_get_key(node, cursor->cell_num);
                if (key_at_index == key_to_insert) {
                        replog("Duplicate key error, row with id %d already exists", key_to_insert);
                        return;
                }
        }
        leafnode_insert(cursor, row->id, row);
        free(cursor);
}

void
exec_select(Command* cmd, Table* table) {
        Cursor* cursor = table_start(table);
        Row row;
        while (!cursor->table_end) {
                deserialize_row(cursor_value(cursor), &row);
                print_row(&row);
                cursor_advance(cursor);
        }
        free(cursor);
}

void
exec_command(Command* cmd, Table* table) {
        switch (cmd->type) {
                case COMMAND_SELECT: exec_select(cmd, table); break;
                case COMMAND_INSERT: exec_insert(cmd, table); break;
                default: replog("Unknown command"); break;
        }
}
