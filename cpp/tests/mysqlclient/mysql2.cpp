#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>

// just going to input the general details and not the port numbers
struct connection_details
{
    char *server;
    char *user;
    char *password;
    char *database;
};

MYSQL* mysql_connection_setup(struct connection_details mysql_details)
{
     // first of all create a mysql instance and initialize the variables within
    MYSQL *connection = mysql_init(NULL);

    // connect to the database with the details attached.
    if (!mysql_real_connect(connection,mysql_details.server, mysql_details.user, mysql_details.password, mysql_details.database, 0, NULL, 0)) {
      printf("Conection error : %s\n", mysql_error(connection));
      exit(1);
    }
    return connection;
}

MYSQL_RES* mysql_perform_query(MYSQL *connection, char *sql_query)
{
   // send the query to the database
   if (mysql_query(connection, sql_query))
   {
      printf("MySQL query error : %s\n", mysql_error(connection));
      exit(1);
   }

   return mysql_use_result(connection);
}

int main()
{
  MYSQL *conn;		// the connection
  MYSQL_RES *res;	// the results
  MYSQL_ROW row;	// the results row (line by line)

  struct connection_details mysqlD;
  mysqlD.server = "localhost";  // where the mysql database is
  mysqlD.user = "mysqlusername";		// the root user of mysql
  mysqlD.password = "mysqlpassword"; // the password of the root user in mysql
  mysqlD.database = "mysql";	// the databse to pick

  // connect to the mysql database
  conn = mysql_connection_setup(mysqlD);

  // assign the results return to the MYSQL_RES pointer
  res = mysql_perform_query(conn, "show tables");

  printf("MySQL Tables in mysql database:\n");
  while ((row = mysql_fetch_row(res)) !=NULL)
      printf("%s\n", row[0]);

  /* clean up the database result set */
  mysql_free_result(res);
  /* clean up the database link */
  mysql_close(conn);

  return 0;
}
