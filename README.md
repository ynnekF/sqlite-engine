# SQLite Engine

SQLite clone written in C (Inspiration/Tutorial: https://cstack.github.io/db_tutorial/)

Contents
- Establish Read-Execute-Print Loop (REPL)
- Handle meta commands (.[cmd])
- Initially create an in-memory, append-only data source
- Add data persistence
- Implement cursors within the backend
- Implement B+ Trees within the backend

## Commands
Below is an example of the `.help` meta-command..
```
[INFO] Entering REPL loop. Type '.exit' to quit.

db> .help
(src/repl.c 213)         received command: '.help' of size 5/1024
(src/repl.c 75)          processing meta command: '.help'
not implemented
```

Other meta-commands include
- `.btree` to visualize the tree which houses the table data
- `.exit` to gracefully exit the REPL loop

Beyond that, the REPL loop serves simple inserts and selects.

```
db> insert 1 foo bar
(src/repl.c 213)         received command: 'insert 1 foo bar' of size 16/1024
(src/repl.c 93)          START: 'insert 1 foo bar'
(src/repl.c 234)         handling command: 2
(src/db.c 647)           Executing insert command
(src/db.c 653)           username size: 3, email size: 3

db> select
(src/repl.c 213)         received command: 'select' of size 6/1024
(src/repl.c 234)         handling command: 1
(1, foo, bar)
```
Data is written and persisted within a file whose path is provided to the executable. Defaults to `mydb.db`


## B-Trees
Balanced tree data structure used for logarithmic time operations. Each node is capable of having more than two children, having up to `m` instead. `m` is known as the tree's order. B-trees are the most common type of database index. 

B+ Trees are similar but used by SQLite to store tables as opposed ot acting as an index. While an unsorted array of rows is space efficient and allows for quick inserts, searching requires a full table scan. Deletions also require moving all the rows that come after the deleted one.

Using a tree structure, each node can store some number of rows and allow users quick insertions, deletions and lookups.

**Installing `rspec`**
```
$ sudo apt install autoconf patch build-essential rustc libssl-dev libyaml-dev libreadline6-dev zlib1g-dev libgmp-dev libncurses5-dev libffi-dev libgdbm6 libgdbm-dev libdb-dev uuid-dev
$ sudo apt install ruby-full
$ ruby --version 
$ gem --version
$ sudo gem install rspec
```

## Running Tests

To run the test suite one can run `make tests` or `rspec --format documentation`
```
$ rspec --format documentation

Simple sanity tests
  prints usage information
  prints error message for unknown command
  prints goodbye message on exit
  prints command sizing error for invalid insert
  prints command syntax error for invalid insert
  prints command unknown error for invalid command
  prints ok for select
  prints ok for insert
  prints ok for update
  prints ok for delete

Simple DB operations
  prints adds table row on insert
  prints adds table row on multiple inserts
  prints row with max column size values
  prints error message if strings are too long
  prints error message for duplicate insert

Btree operations
  allows printing out the structure of a 4-leaf-node btree

Complex operation
  allows printing out the structure of a 4-leaf-node btree2

Finished in 0.90201 seconds (files took 0.08365 seconds to load)
17 examples, 0 failures
```