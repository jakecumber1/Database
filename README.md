# Database Application in C (SQLite inspired)

This is a single table database application meant to mimic the components of SQLite under the hood (virtual machine, input parser, BTree, pager, etc.). This application is currently Windows only.

# Running the application:

## Visual Studio 2022

Pull the GitHub repo and open in Visual Studio 2022. The program should run when pressing start with/without debugging, no extra dependencies are needed.

## From the exe

The exe can be found in the root folder of the repo. Either pull the repo or download the exe, starting the exe should bring up the command line prompt for the application.

# Inputs

Inputs are meta commands and statements given by the user. Currently, commands and statements are case sensitive (.open will run, but .OPEN will not).

## Meta Commands

Meta commands are used to display information about the application or modify the environment itself rather than modifying a table (opening/closing databases, exiting the application). Meta commands always begin with a '.'. Here are the meta commands available in this current build:

### .exit

Closes the application, flushes current database (if one is open) to disk.

### .open filename.db

Opens a database with the given filename, if that file doesn't exist, the application creates it. Note, the database will be created in the same folder as the exe, and custom paths for storage have yet to be implemented. If a database is currently open, .open will close the current database before opening the new one.

### .close

Closes the current database and flushes to disk.

### .constants

Prints out the constants used for creating the leaf nodes, will allow you to get an idea for how the leaves are structured (and roughly how much space they take up).

### .btree

Prints out a representation of the B-Tree used to store the database keys.

## Statements

Statements are commands given by the user which access or modify the database file itself, there are currently only two statements in this build.

### insert int string strin>

Inserts a row of data into the database matching the pattern <int> <string> <string>. This is meant to represent an employee entry in a database with the format (id, name, email). The name must be under 33 characters long (remember, no spaces) and the email must be under 256 characters. If there is no database open or an invalid set of arguments is given, insert will abort. Negative or duplicate ids cannot be inserted.

### select optional: int optional int-int

Prints the database when no arguments are given. If one integer is given, it will return a row with the id matching the given integer (if it exists). If a dash and second integer are given, for example: select 1-10, the application will print all rows it can find in between (and including) 1-10. If the database is empty, isn't open, or an invalid range is given, select will abort.
