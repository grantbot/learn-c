#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define MAX_DATA 5
#define MAX_ROWS 100


struct Address {
  int id;
  int set;
  char name[MAX_DATA];
  char email[MAX_DATA];
};

struct Database {
  struct Address rows[MAX_ROWS];
};

struct Connection {
  // Link between memory and disc
  FILE *file;
  struct Database *db;
};

void Database_close(struct Connection *conn)
{
  if (conn) {
    if (conn->file) fclose(conn->file);
    if (conn->db) free(conn->db);
    free(conn);
  }
}

void die(const char *message, struct Connection *conn)
{
  if (errno) {
    perror(message);
  } else {
    printf("ERROR: %s\n", message);
  }
  Database_close(conn);
  exit(1);
}

void Address_print(const struct Address *addr)
{
  printf("%d | %s | %s\n", addr->id, addr->name, addr->email);
}

void Database_load(struct Connection *conn)
{
  // Read a conn's filestream into memory as 'db'
  int rc = fread(conn->db, sizeof(struct Database), 1, conn->file);
  if (rc != 1) die("Failed to load database.", conn);
}

struct Connection *Database_open(const char *filename, char mode)
{
  // Allocate a new connection
  // This will only malloc enough for the db pointer
  struct Connection *conn = malloc(sizeof(struct Connection));
  if (!conn) die("Memory error", NULL);  // !ptr is equiv to ptr == NULL

  // Allocate the connection's db separately
  conn->db = malloc(sizeof(struct Database));
  if (!conn->db) die("Memory error", conn);

  // Open the file and associate its stream with the conn
  if (mode == 'c') {
    // If CREATE, make new file or truncate existing
    conn->file = fopen(filename, "w");
  } else {
    // Open for read and write
    conn->file = fopen(filename, "r+");
    if (conn->file) {
      Database_load(conn);
    }
  }

  if (!conn->file) die("Failed to open file", conn);

  return conn;
}


void Database_write(struct Connection *conn)
{
  // Seek to beginning of stream
  rewind(conn->file);

  // Write the entire db to stream
  int rc = fwrite(conn->db, sizeof(struct Database), 1, conn->file);
  if (rc != 1) die ("Failed to write database", conn);

  // Commit the conn's filestream to disc
  rc = fflush(conn->file);
  if (rc == -1) die ("Cannot flush db", conn);
}

void Database_create(struct Connection *conn)
{
  // Give a conn a DB with empty addresses for each row
  int i = 0;
  for (i = 0; i < MAX_ROWS; i++) {
    // Ensures all members 'cept id are initialized to 0
    struct Address addr = {.id = i};
    conn->db->rows[i] = addr;
  }
}

void Database_set(struct Connection *conn, int id, const char *name, const char *email)
{
  // Load the desired row id
  struct Address *addr = &conn->db->rows[id];
  if (addr->set) die("Already set, delete it first", conn);

  // Flip 'set' flag
  addr->set = 1;
  // Copy name and email.
  // NB: strncpy does not terminate the dest string if you pass a string that's
  // longer than the given size.
  char *res = strncpy(addr->name, name, MAX_DATA);
  res[MAX_DATA - 1] = '\0';  // Ensure termination
  if (!res) die("Name copy failed", conn);
  res = strncpy(addr->email, email, MAX_DATA);
  res[MAX_DATA - 1] = '\0';
  if (!res) die("Email copy failed", conn);
}

void Database_get(struct Connection *conn, int id)
{
  // Get a pointer to the row and print it
  struct Address *addr = &conn->db->rows[id];  // '&' is applied last

  if (!addr->set) die("ID is not set", conn);
  Address_print(addr);
}

void Database_delete(struct Connection *conn, int id)
{
  // Copy a new struct into the appropriate row, with all members init'd to
  // 0 except id
  struct Address addr = {.id = id};
  conn->db->rows[id] = addr;
}

void Database_list(struct Connection *conn)
{
  int i = 0;
  for (i = 0; i < MAX_ROWS; i++) {
    struct Address *cur = &conn->db->rows[i];
    if (cur->set) {
      Address_print(cur);
    }
  }
}

int main (int argc, char *argv[])
{
  if (argc < 3) die("USAGE: db <dbfile> <action> [action params]", NULL);

  // Get first arg
  char *filename = argv[1];
  // Get first char of second arg
  char action = argv[2][0];
  // Open a file and wrap its stream in a connection
  struct Connection *conn = Database_open(filename, action);
  // Get id if passed in, default to 0
  int id = 0;
  if (argc > 3) id = atoi(argv[3]);  // ASCII num to int
  if (id >= MAX_ROWS) die("There aren't that many records", conn);

  switch (action) {
  case 'c':
    // CREATE
    Database_create(conn);
    Database_write(conn);
    break;

  case 'g':
    // READ
    if (argc != 4) die("Please provide an id to retrieve", conn);
    Database_get(conn, id);
    break;

  case 's':
    // UPDATE
    if (argc != 6) die("Need id, name, and email to set", conn);
    Database_set(conn, id, argv[4], argv[5]);
    Database_write(conn);
    break;

  case 'd':
    // DELETE
    if (argc != 4) die("Need id to delete", conn);
    Database_delete(conn, id);
    Database_write(conn);
    break;

  case 'l':
    // LIST ALL
    Database_list(conn);
    break;

  default:
    die("Invalid action", conn);
  }

  Database_close(conn);


  return 0;
}
