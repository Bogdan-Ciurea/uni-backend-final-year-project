/**
 * @file files_cql_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the file object
 * @version 0.1
 * @date 2023-01-03
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef FILES_CQL_MANAGER_H
#define FILES_CQL_MANAGER_H

#include "../database_objects/file_object.hpp"
#include "cql_client.hpp"

class FilesCqlManager {
 public:
  /**
   * @brief Construct a new Files Cql Manager object
   *
   * @param cql_client - The cassandra cql client
   */
  FilesCqlManager(CqlClient* cql_client) : _cql_client(cql_client) {}

  /**
   * @brief Destroy the Files Cql Manager object
   *
   */
  ~FilesCqlManager() {}

  /**
   * @brief Will prepare the database to be operated on
   *
   * @param init_db_schema - If you want to also create the keyspace and table
   *
   * @return CqlResult     - The result of the operation
   */
  CqlResult configure(bool init_db_schema);

  /**
   * @brief Adds a file entry on the database
   *
   * @param file_data - The file object we want to add
   *
   * @return CqlResult   - The result of the operation
   */
  CqlResult create_file(const FileObject& file_data);

  /**
   * @brief Get the file object
   *
   * @param school_id  - The id of the school
   * @param id         - The uuid of the file
   *
   * @return std::pair<CqlResult, FileObject>
   */
  std::pair<CqlResult, FileObject> get_file_by_id(const int& school_id,
                                                  const CassUuid& id);

  /**
   * @brief Updates a file entry on the database
   *
   * @param school_id    - The id of the school
   * @param id           - The uuid of the file
   * @param type         - The type of the file
   * @param name         - The name of the file
   * @param files        - The files related to this files (if folder)
   * @param path_to_file - The path to the file
   * @param size         - The size of the file
   * @param added_by_user       - The user that added the file
   * @param visible_to_students - If the file is visible to students
   * @param students_can_add    - If the students can add files
   *
   * @return CqlResult  - The result of the operation
   */
  CqlResult update_file(const int& school_id, const CassUuid& id,
                        const CustomFileType& type, const std::string& name,
                        const std::vector<CassUuid>& files,
                        const std::string& path_to_file, const int& size,
                        const CassUuid& added_by_user,
                        const bool& visible_to_students,
                        const bool& students_can_add);

  /**
   * @brief Deletes a file entry on the database
   *
   * @param school_id  - The id of the school
   * @param id         - The uuid of the file
   *
   * @return CqlResult  - The result of the operation
   */
  CqlResult delete_file(const int& school_id, const CassUuid& id);

 private:
  CqlClient* _cql_client;

  CassPreparedPtr _prepared_insert_file;

  CassPreparedPtr _prepared_get_file;

  CassPreparedPtr _prepared_update_file;

  CassPreparedPtr _prepared_delete_file;

  /**
   * @brief Initialize the prepared statements used by this class
   *
   * @return CqlResult The result of the operation
   */
  CqlResult init_prepare_statements();

  /**
   * @brief Creates the keyspace and the table
   *
   * @return CqlResult The result of the operation
   */
  CqlResult init_schema();

  const char* CREATE_SCHOOL_KEYSPACE =
      "CREATE KEYSPACE IF NOT EXISTS schools WITH REPLICATION = { 'class' : "
      "'SimpleStrategy', 'replication_factor' : 3 };";

  const char* CREATE_FILES_TABLE =
      "CREATE TABLE IF NOT EXISTS schools.files ( "
      "school int, "
      "id uuid, "
      "type int, "
      "files list <uuid>, "
      "name varchar, "
      "path_to_file varchar, "
      "size int, "
      "added_by_user uuid, "
      "visible_to_students boolean, "
      "students_can_add boolean, "
      "PRIMARY KEY ((school, id)));";

  const char* INSERT_FILE =
      "INSERT INTO schools.files (school, id, type, files, name, path_to_file, "
      "size, added_by_user, visible_to_students, students_can_add) VALUES (?, "
      "?, ?, ?, ?, ?, ?, ?, ?, ?) IF NOT EXISTS;";

  const char* SELECT_FILE =
      "SELECT school, id, type, files, name, path_to_file, size, "
      "added_by_user, "
      "visible_to_students, students_can_add FROM schools.files WHERE school = "
      "? AND id = ?;";

  const char* UPDATE_FILE =
      "UPDATE schools.files SET type = ?, files = ?, name = ?, path_to_file = "
      "?, size = ?, added_by_user = ?, visible_to_students = ?, "
      "students_can_add = ? WHERE school = ? AND id = ? IF EXISTS;";

  const char* DELETE_FILE =
      "DELETE FROM schools.files WHERE school = ? AND id = ? IF EXISTS;";
};

CqlResult map_row_to_file(const CassRow* row, FileObject& file);

#endif