/**
 * @file student_references_cql_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the school object
 * @version 0.1
 * @date 2023-01-06
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef STUDENT_REFERENCES_CQL_MANAGER_H
#define STUDENT_REFERENCES_CQL_MANAGER_H

#include "../database_objects/student_reference_object.hpp"
#include "cql_client.hpp"

class StudentReferencesCqlManager {
 public:
  /**
   * @brief Construct a new Student References Cql Manager object
   *
   * @param cql_client - The cassandra cql client
   */
  StudentReferencesCqlManager(CqlClient* cql_client)
      : _cql_client(cql_client) {}

  /**
   * @brief Destroy the Student References Cql Manager object
   *
   */
  ~StudentReferencesCqlManager() {}

  /**
   * @brief Will prepare the database to be operated on
   *
   * @param init_db_schema - If you want to also create the keyspace and table
   *
   * @return CqlResult     - The result of the operation
   */
  CqlResult configure(bool init_db_schema);

  /**
   * @brief Adds a student reference entry on the database
   *
   * @param student_reference_data - The student reference object we want to add
   *
   * @return CqlResult   - The result of the operation
   */
  CqlResult create_student_reference(
      const StudentReferenceObject& student_reference_data);

  /**
   * @brief Get the student references object
   *
   * @param school_id  - The id of the school
   * @param student_id         - The uuid of the student
   *
   * @return std::pair<CqlResult, std::vector<StudentReferenceObject>>
   */
  std::pair<CqlResult, std::vector<StudentReferenceObject>>
  get_student_references(const int& school_id, const CassUuid& student_id);

  /**
   * @brief Updates a student reference entry on the database.
   * Considering that we cannot update a primary key, we will delete the old
   * entry and add a new one
   *
   * @param school_id      - The id of the school
   * @param student_id     - The id of the student
   * @param old_reference  - The old reference we want to update
   * @param new_reference  - The new reference we want to update to
   * @param type           - The type of the reference
   *
   */
  CqlResult update_student_reference(const int& school_id,
                                     const CassUuid& student_id,
                                     const std::string& old_reference,
                                     const std::string& new_reference,
                                     const ReferenceType& type);

  /**
   * @brief Deletes a student reference entry on the database
   *
   * @param school_id  - The id of the school
   * @param student_id - The id of the student
   * @param reference  - The reference
   *
   */
  CqlResult delete_student_reference(const int& school_id,
                                     const CassUuid& student_id,
                                     const std::string& reference);

  /**
   * @brief Deletes all references entries of a student on the database
   *
   * @param school_id  - The id of the school
   * @param student_id - The id of the student
   *
   * @return CqlResult   - The result of the operation
   */
  CqlResult delete_student_references(const int& school_id,
                                      const CassUuid& student_id);

 private:
  CqlClient* _cql_client;

  CassPreparedPtr _prepared_insert_reference;

  CassPreparedPtr _prepared_select_references_by_student;

  CassPreparedPtr _prepared_delete_reference;

  CassPreparedPtr _prepared_delete_references_by_student;

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

  const char* CREATE_STUDENT_REFERENCES_TABLE =
      "CREATE TABLE IF NOT EXISTS schools.student_reference ( "
      "school int, "
      "student_id uuid, "
      "reference varchar, "
      "type int, "
      "PRIMARY KEY ((school, student_id), reference));";

  const char* INSERT_REFERENCE =
      "INSERT INTO schools.student_reference (school, student_id, reference, "
      "type ) VALUES (?, ?, ?, ?) IF NOT EXISTS ;";

  const char* SELECT_REFERENCES_BY_STUDENT =
      "SELECT school, student_id, reference, type FROM "
      "schools.student_reference WHERE school = ? AND student_id = ?;";

  const char* DELETE_REFERENCE =
      "DELETE FROM schools.student_reference WHERE school = ? AND student_id = "
      "? AND reference = ? IF EXISTS;";

  const char* DELETE_REFERENCES_BY_STUDENT =
      "DELETE FROM schools.student_reference WHERE school = ? AND student_id = "
      "?;";
};

CqlResult map_row_to_reference(const CassRow* row,
                               StudentReferenceObject& reference);

#endif