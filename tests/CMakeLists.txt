set (SOURCES
    # source files
    ../src/cql_helpers/announcements_by_tag_cql_manager.cpp
    ../src/cql_helpers/answers_cql_manager.cpp
    ../src/cql_helpers/answers_by_announcement_or_question_cql_manager.cpp
    ../src/cql_helpers/course_cql_manager.cpp
    ../src/cql_helpers/courses_by_user_cql_manager.cpp
    ../src/cql_helpers/files_cql_manager.cpp
    ../src/cql_helpers/grades_cql_manager.cpp
    ../src/cql_helpers/tags_cql_manager.cpp
    ../src/cql_helpers/tags_by_user_cql_manager.cpp
    ../src/cql_helpers/student_references_cql_manager.cpp
    ../src/cql_helpers/lectures_cql_manager.cpp
    ../src/cql_helpers/tokens_cql_manager.cpp
    ../src/cql_helpers/users_cql_manager.cpp
    ../src/cql_helpers/users_by_course_cql_manager.cpp
    ../src/cql_helpers/users_by_tag_cql_manager.cpp
    ../src/cql_helpers/questions_cql_manager.cpp
    ../src/cql_helpers/questions_by_course_cql_manager.cpp
    ../src/cql_helpers/todos_cql_manager.cpp
    ../src/cql_helpers/todos_by_user_cql_manager.cpp

    ../src/database_objects/holiday_object.cpp
    ../src/database_objects/country_object.cpp
    ../src/database_objects/school_object.cpp
    ../src/database_objects/announcements_object.cpp
    ../src/database_objects/answer_object.cpp
    ../src/database_objects/course_object.cpp
    ../src/database_objects/file_object.cpp
    ../src/database_objects/grade_object.cpp
    ../src/database_objects/tag_object.cpp
    ../src/database_objects/student_reference_object.cpp
    ../src/database_objects/lecture_object.cpp
    ../src/database_objects/user_object.cpp
    ../src/database_objects/question_object.cpp
    ../src/database_objects/todo_object.cpp

    # test files
    main_test.cpp
    cql_helpers/test_school_cql_manager.cpp
    cql_helpers/test_country_cql_manager.cpp
    cql_helpers/test_holiday_cql_manager.cpp
    cql_helpers/test_announcements_cql_manager.cpp
    cql_helpers/test_announcements_by_tag_cql_manager.cpp
    cql_helpers/test_answers_by_announcement_or_question_cql_manager.cpp
    cql_helpers/test_answers_cql_manager.cpp
    cql_helpers/test_courses_by_user_cql_manager.cpp
    cql_helpers/test_courses_cql_manager.cpp
    cql_helpers/test_files_cql_manager.cpp
    cql_helpers/test_grades_cql_manager.cpp
    cql_helpers/test_lectures_cql_manager.cpp
    cql_helpers/test_questions_by_course_cql_manager.cpp
    cql_helpers/test_questions_cql_manager.cpp
    cql_helpers/test_student_references_cql_manager.cpp
    cql_helpers/test_tags_by_user_cql_manager.cpp
    cql_helpers/test_tags_cql_manager.cpp
    cql_helpers/test_todos_by_user_cql_manager.cpp
    cql_helpers/test_todos_cql_manager.cpp
    cql_helpers/test_tokens_cql_manager.cpp
    cql_helpers/test_users_by_course_cql_manager.cpp
    cql_helpers/test_users_by_tag_cql_manager.cpp
    cql_helpers/test_users_cql_manager.cpp
)

add_executable(${TEST_EXEC_NAME} ${SOURCES})
target_link_libraries(${TEST_EXEC_NAME} PUBLIC ${PROJECT_LIBS})
set_target_properties(${TEST_EXEC_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "..")

add_test(
    NAME ${TEST_EXEC_NAME}
    COMMAND ${TEST_EXEC_NAME}
)