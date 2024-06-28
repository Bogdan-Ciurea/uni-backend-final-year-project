/**
 * @file answers_by_announcement_or_question_cql_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the relationship between answers and announcements or
 * questions
 * @version 0.1
 * @date 2023-01-02
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef ANSWERS_BY_ANNOUNCEMENT_OR_QUESTION_CQL_MANAGER_H
#define ANSWERS_BY_ANNOUNCEMENT_OR_QUESTION_CQL_MANAGER_H

#include "cql_client.hpp"

class AnswersByAnnouncementOrQuestionCqlManager {
 public:
  AnswersByAnnouncementOrQuestionCqlManager(CqlClient* cql_client)
      : _cql_client(cql_client) {}
  ~AnswersByAnnouncementOrQuestionCqlManager() {}

  /**
   * @brief Configures the class
   *
   * @param init_db_schema - If true, the keyspace and table will be created
   * @return CqlResult - The result of the operation
   */
  CqlResult configure(bool init_db_schema);

  /**
   * @brief Creates a relationship between an answer and an announcement or
   * question
   *
   * @param school_id - The id of the school
   * @param announcement_or_question_id - The id of the announcement or question
   * @param type - The type of the announcement - 0 or question - 1
   * @param answer_id - The id of the answer
   * @return CqlResult - The result of the operation
   */
  CqlResult create_relationship(const int school_id,
                                const CassUuid& announcement_or_question_id,
                                const int type, const CassUuid& answer_id);

  /**
   * @brief Get the answers by announcement or question
   *
   * @param school_id - The id of the school
   * @param announcement_or_question_id - The id of the announcement or question
   * @param type - The type of the announcement - 0 or question - 1
   * @return std::pair<CqlResult, std::vector<CassUuid>> - The result of the
   * operation and the vector of answer ids
   */
  std::pair<CqlResult, std::vector<CassUuid>>
  get_answers_by_announcement_or_question(
      const int school_id, const CassUuid& announcement_or_question_id,
      const int type);

  /**
   * @brief Deletes a relationship between an answer and an announcement or
   * question
   *
   * @param school_id - The id of the school
   * @param announcement_or_question_id - The id of the announcement or question
   * @param type - The type of the announcement - 0 or question - 1
   * @param answer_id - The id of the answer
   * @return CqlResult - The result of the operation
   */
  CqlResult delete_relationship(const int school_id,
                                const CassUuid& announcement_or_question_id,
                                const int type, const CassUuid& answer_id);

  /**
   * @brief Deletes all relationships between an answer and an announcement or
   * question
   *
   * @param school_id - The id of the school
   * @param announcement_or_question_id - The id of the announcement or question
   * @param type - The type of the announcement - 0 or question - 1
   * @return CqlResult - The result of the operation
   */
  CqlResult delete_relationships_by_announcement_or_question(
      const int school_id, const CassUuid& announcement_or_question_id,
      const int type);

 private:
  CqlClient* _cql_client;

  CassPreparedPtr _prepared_insert_relationship;

  CassPreparedPtr _prepared_get_answers_by_announcement_or_question;

  CassPreparedPtr _prepared_delete_relationship;

  CassPreparedPtr _prepared_delete_relationships_by_announcement_or_question;

  /**
   * @brief Initialize the prepared statements used by this class
   *
   * @return CqlResult - The result of the operation
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

  const char* CREATE_ANSWERS_BY_ANNOUNCEMENT_OR_QUESTION_TABLE =
      "CREATE TABLE IF NOT EXISTS schools.answers_by_announcement_or_question "
      "( "
      "school int, "
      "announcement_or_question_id uuid, "
      "type int, "
      "answer_id uuid, "
      "PRIMARY KEY ((school, announcement_or_question_id, type), answer_id));";

  const char* INSERT_RELATIONSHIP =
      "INSERT INTO schools.answers_by_announcement_or_question (school, "
      "announcement_or_question_id, type, answer_id) VALUES (?, ?, ?, ?) IF "
      "NOT EXISTS;";

  const char* SELECT_ANSWERS =
      "SELECT answer_id FROM "
      "schools.answers_by_announcement_or_question WHERE school = ? AND "
      "announcement_or_question_id = ? AND type = ?;";

  // Will delete a specific relationship. Would be used if you delete an answer
  const char* DELETE_RELATIONSHIP =
      "DELETE FROM schools.answers_by_announcement_or_question WHERE school = "
      "? AND announcement_or_question_id = ? AND type = ? AND answer_id = ? IF "
      "EXISTS;";

  // Will delete all relationships related to an announcement or question.
  // Would be used if you delete an announcement or question
  const char* DELETE_RELATIONSHIPS_BY_ANNOUNCEMENT_OR_QUESTION =
      "DELETE FROM schools.answers_by_announcement_or_question WHERE school = "
      "? AND announcement_or_question_id = ? AND type = ?;";
};

#endif  // ANSWERS_BY_ANNOUNCEMENT_OR_QUESTION_CQL_MANAGER_H
