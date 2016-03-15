#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

struct Address {
  int id;
  int set;
  char *name;
  char *email;
};

struct Database {
  int max_data;
  int max_rows;
  struct Address rows[];
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
  // TODO Pull out max_data and max_rows from file, if not create, else use params
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

void Database_create(struct Connection *conn, int max_data, int max_rows)
{
  char name[max_data];
  char email[max_data];
  // Give a conn a DB with empty addresses for each row
  int i = 0;
  for (i = 0; i < max_rows; i++) {
    // Ensures all members 'cept id are initialized to 0
    struct Address addr = {.id = i, .name = name, .email = email};
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
  char *res = strncpy(addr->name, name, conn->db->max_data);
  res[conn->db->max_data - 1] = '\0';  // Ensure termination
  if (!res) die("Name copy failed", conn);
  res = strncpy(addr->email, email, conn->db->max_data);
  res[conn->db->max_data - 1] = '\0';
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
  for (i = 0; i < conn->db->max_rows; i++) {
    struct Address *cur = &conn->db->rows[i];
    if (cur->set) {
      Address_print(cur);
    }
  }
}

int main (int argc, char *argv[])
{
  int id;

  if (argc < 3) die("USAGE: db <dbfile> <action> [action params]", NULL);

  // Get first arg
  char *filename = argv[1];

  // Get first char of second arg
  char action = argv[2][0];

  // Open file and wrap its stream in a connection
  struct Connection *conn = Database_open(filename, action);




  switch (action) {
  case 'c': {
    // CREATE
    if (argc != 5) die("Please provide max_data and max_rows", conn);
    Database_create(conn, atoi(argv[3]), atoi(argv[4]));
    Database_write(conn);
    break;
  }

  case 'g':
    // READ
    if (argc != 4) die("Please provide an id to retrieve", conn);
    id = atoi(argv[3]);  // ASCII num to int
    Database_get(conn, id);
    break;

  case 's':
    // UPDATE
    if (argc != 6) die("Need id, name, and email to set", conn);
    id = atoi(argv[3]);
    Database_set(conn, id, argv[4], argv[5]);
    Database_write(conn);
    break;

  case 'd':
    // DELETE
    if (argc != 4) die("Need id to delete", conn);
    id = atoi(argv[3]);
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
