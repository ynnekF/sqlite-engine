#ifndef REPL_H
#define REPL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buf.h"
#include "db.h"
#include "log.h"
#include "repl.h"

typedef enum {
        METACMD_OK,
        METACMD_UNKNOWN,
        METACMD_ERR
} MetaCmdResult;

typedef enum {
        PREPARE_SUCCESS,
        PREPARE_SYNTAX_ERROR,
        PREPARE_UNRECOGNIZED_STATEMENT,
        PREPARE_NEGATIVE_ID,
        PREPARE_STRING_TOO_LONG
} PrepareResult;

void repl_prompt();
void repl_loop();

#endif // REPL_H