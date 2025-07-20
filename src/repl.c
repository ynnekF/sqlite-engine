/**
 * Read Execute Print Loop (REPL) for a C17 project.
 */

#include "repl.h"

/**
 * @brief Compares the first 'n' characters of two strings.
 * @return 1 if they are equal, 0 otherwise.
 */
#define IS_SAME(s1, s2, n) (strncmp((s1), (s2), (n)) == 0)

/**
 * @brief Compares a string with a string literal. (length determined automatically)
 * @return 1 if they are equal, 0 otherwise.
 */
#define IS_SAME_LIT(s, literal) (strncmp((s), (literal), sizeof(literal) - 1) == 0)

/** @brief Kills the REPL with an error message and cleans up resources. */
void repl_kill(char* msg, InputBuffer* buf);

/** @brief Gracefully exits the REPL, cleaning up resources. */
void repl_graceful_exit(InputBuffer* buf, Table* table);

int
repl_usage() {
        printf("not implemented\n");

        return METACMD_OK;
}

void
repl_kill(char* msg, InputBuffer* buf) {
        if (msg)
                error("%s", msg);

        if (buf)
                inbuf_free(buf);

        exit(EXIT_FAILURE);
}

void
repl_graceful_exit(InputBuffer* buf, Table* table) {
        if (buf)
                inbuf_free(buf);
        if (table)
                free_table(table);

        replog("goodbye.");
        exit(EXIT_SUCCESS);
}

const char*
repl_err_lookup(CommandType type) {
        switch (type) {
                case COMMAND_OK: return "COMMAND_OK";
                case COMMAND_SELECT: return "COMMAND_SELECT";
                case COMMAND_INSERT: return "COMMAND_INSERT";
                case COMMAND_UPDATE: return "COMMAND_UPDATE";
                case COMMAND_DELETE: return "COMMAND_DELETE";
                case COMMAND_UNKNOWN: return "COMMAND_UNKNOWN";
                case COMMAND_SYNTAX_ERR: return "COMMAND_SYNTAX_ERR";
                case COMMAND_SIZING_ERR: return "COMMAND_SIZING_ERR";
                default: return "COMMAND_UNKNOWN_TYPE";
        }
}

int
metacmd(InputBuffer* buffer, Table* table) {
        if (!buffer || !buffer->data)
                return METACMD_ERR;

        const char* command = buffer->data;
        replog("processing meta command: '%s'", command);

        if (IS_SAME_LIT(buffer->data, ".exit"))
                repl_graceful_exit(buffer, table);

        if (IS_SAME_LIT(command, ".help"))
                return repl_usage();

        if (IS_SAME_LIT(command, ".btree")) {
                print_tree(table->pager, table->root_page, 0);
                return METACMD_OK;
        }

        return METACMD_UNKNOWN;
}

void
repl_parse_insert(InputBuffer* buffer, Command* cmd) {
        replog("START: '%s'", buffer->data);
        cmd->type = COMMAND_INSERT;

        char* kwarg = strtok(buffer->data, " ");
        char* rowid = strtok(NULL, " ");
        char* username = strtok(NULL, " ");
        char* email = strtok(NULL, " ");

        if (rowid == NULL || username == NULL || email == NULL) {
                cmd->type = COMMAND_SIZING_ERR;
                replog("insert command requires 3 arguments: rowid, username, email");
                return;
        }

        int id = atoi(rowid);
        if (id < 0) {
                cmd->type = COMMAND_SYNTAX_ERR;
                replog("insert command requires a positive integer for id");
                return;
        }

        if (strlen(username) > COL_SIZE_USERNAME || strlen(email) > COL_SIZE_EMAIL) {
                cmd->type = COMMAND_SIZING_ERR;
                replog("username or email exceeds maximum length");
                return;
        }

        cmd->row.id = id;
        strcpy(cmd->row.username, username);
        strcpy(cmd->row.email, email);
}

Command
repl_parse_command(InputBuffer* buffer) {
        Command cmd;
        cmd.row.id = 0;
        cmd.type = COMMAND_UNKNOWN;

        if (IS_SAME(buffer->data, "insert", 6))
                repl_parse_insert(buffer, &cmd);
        else if (IS_SAME(buffer->data, "select", 6))
                cmd.type = COMMAND_SELECT;
        else if (IS_SAME(buffer->data, "update", 6))
                cmd.type = COMMAND_UPDATE;
        else if (IS_SAME(buffer->data, "delete", 6))
                cmd.type = COMMAND_DELETE;
        return cmd;
}

void
repl_forward_command(Command* cmd) {
        if (!cmd) {
                replog("no command to forward");
                return;
        }

        switch (cmd->type) {
                case COMMAND_OK: replog("command OK"); break;
                case COMMAND_SELECT: replog("command SELECT"); break;
                case COMMAND_INSERT: replog("command INSERT"); break;
                case COMMAND_UPDATE: replog("command UPDATE"); break;
                case COMMAND_DELETE: replog("command DELETE"); break;
                case COMMAND_UNKNOWN: replog("command UNKNOWN"); break;
                case COMMAND_SYNTAX_ERR: replog("command SYNTAX ERROR"); break;
                case COMMAND_SIZING_ERR: replog("command SIZING ERROR"); break;
                default: replog("unknown command type: %d", cmd->type);
        }
}

void
repl_loop(int argc, char const** argv) {
        /**
         * Check if the database file is provided as an argument, if not, exit.
         */
        if (argc < 2 || !argv[1]) {
                repl_kill("No database file specified", NULL);
        }

        Table* table = new_table(argv[1]);
        if (!table) {
                perror("Failed to allocate memory for table");
                exit(EXIT_FAILURE);
        }

        /**
         * Create a buffer used to read commands from the user. Exit on failure.
         * buffer.data -> stdin data
         * buffer.size -> size of the data read
         * buffer.capacity -> initial capacity of the buffer
         */
        InputBuffer* buffer = inbuf_new(1024);
        if (!buffer)
                repl_kill("Failed to create buffer", buffer);

        info("Entering REPL loop. Type '.exit' to quit.\n");

        while (1) {
                printf("db> ");
                fflush(stdout);

                /**
                 * Read a line of input from stdin.
                 * getline() allocates memory for the buffer, which we manage with InputBuffer.
                 * 
                 * Note: getline() will read until a newline character is encountered.
                 */
                ssize_t bytes_in = getline(&buffer->data, &buffer->capacity, stdin);
                if (bytes_in == -1) {
                        repl_kill("Error reading input", buffer);
                }

                // Exclude the newline character
                ssize_t size_in = bytes_in - 1;

                if (size_in == 0)
                        continue; // Skip empty input

                buffer->size = size_in;
                buffer->data[size_in] = 0;

                replog("received command: '%s' of size %zu/%zu", buffer->data, buffer->size, buffer->capacity);

                /**
                 * Handle meta commands that start with a dot (e.g., .exit, .help, etc.)
                 * Then continue to next iter., nothing to be done after a meta command.
                 */
                if (buffer->data[0] == '.') {
                        if (metacmd(buffer, table) != METACMD_OK)
                                replog("unrecognized meta command: '%s'", buffer->data);
                        continue;
                }

                /**
                 * Parse the command from the input buffer.
                 */
                Command cmd = repl_parse_command(buffer);
                if (cmd.type >= COMMAND_UNKNOWN) {
                        replog("command parse error [%s]", repl_err_lookup(cmd.type));
                        continue;
                }

                replog("handling command: %d", cmd.type);
                exec_command(&cmd, table);
        }
}

// Statement statement;
// PrepareResult prepare_result = prepare_statement(buffer, &statement);
// if (prepare_result == PREPARE_SUCCESS) {
//         switch (statement.type) {
//                 case STATEMENT_INSERT: printf("Prepared INSERT statement.\n"); break;
//                 case STATEMENT_SELECT: printf("Prepared SELECT statement.\n"); break;
//                 default: repl_kill("Unknown statement type", buffer); break;
//         }
// } else {
//         if (prepare_result == PREPARE_SYNTAX_ERROR) {
//                 printf("Syntax error in statement\n");
//         } else if (prepare_result == PREPARE_NEGATIVE_ID) {
//                 printf("Negative ID is not allowed\n");
//         } else if (prepare_result == PREPARE_STRING_TOO_LONG) {
//                 printf("Username or email exceeds maximum length\n");
//         } else {
//                 printf("Unrecognized statement\n");
//         }
//         continue;
// }
// int statement_result = execute_statement(&statement, table);
// if (statement_result == EXECUTE_SUCCESS) {
//         printf("Executed statement successfully.\n");
// } else if (statement_result == EXECUTE_DUPLICATE_KEY) {
//         printf("Duplicate key error during execution\n");
// } else if (statement_result == EXECUTE_TABLE_FULL) {
//         printf("Table is full, cannot insert new row\n");
// } else {
//         printf("Unknown execution error\n");
// }
// int
// prepare_insert(InputBuffer* buffer, Statement* statement) {
//         if (!buffer || !buffer->data) {
//                 error("Buffer is NULL or empty");
//                 return PREPARE_SYNTAX_ERROR;
//         }
//         if (!statement) {
//                 error("Statement is NULL");
//                 return PREPARE_SYNTAX_ERROR;
//         }
//         char* keyword = strtok(buffer->data, " ");
//         char* id_str = strtok(NULL, " ");
//         char* username = strtok(NULL, " ");
//         char* email = strtok(NULL, " ");
//         if (!keyword || !id_str || !username || !email) {
//                 error("Invalid INSERT statement format. Expected: insert <id> <username> <email>");
//                 return PREPARE_SYNTAX_ERROR;
//         }
//         int id = atoi(id_str);
//         if (id < 0) {
//                 error("Invalid ID. ID must be a positive integer.");
//                 return PREPARE_NEGATIVE_ID;
//         }
//         if (strlen(username) > COLUMN_USERNAME_SIZE || strlen(email) > COLUMN_EMAIL_SIZE) {
//                 error("Username or email exceeds maximum length.");
//                 return PREPARE_STRING_TOO_LONG;
//         }
// statement->type = STATEMENT_INSERT;
// statement->row_to_insert.id = id;
// strncpy(statement->row_to_insert.username, username, COLUMN_USERNAME_SIZE);
// statement->row_to_insert.username[COLUMN_USERNAME_SIZE] = '\0'; // Ensure null-termination
// strncpy(statement->row_to_insert.email, email, COLUMN_EMAIL_SIZE);
// statement->row_to_insert.email[COLUMN_EMAIL_SIZE] = '\0'; // Ensure null
//         return PREPARE_SUCCESS;
// }
// int
// prepare_statement(InputBuffer* buffer, Statement* statement) {
// if (IS_SAME(buffer->data, "insert", 6)) {
//         return prepare_insert(buffer, statement);
// }
// if (IS_SAME(buffer->data, "select", 6)) {
//         statement->type = STATEMENT_SELECT;
//         return PREPARE_SUCCESS;
// }
// return PREPARE_UNRECOGNIZED_STATEMENT;
// }