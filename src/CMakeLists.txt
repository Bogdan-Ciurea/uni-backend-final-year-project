set(SOURCES
    # Added the SMTPMail.cc file to the sources
    ../modules/SMTPMail-drogon/SMTPMail.cc

    # Added the bcrypt files to the sources
    ../modules/bcrypt/src/bcrypt.cpp
    ../modules/bcrypt/src/blowfish.cpp
    
    main.cpp
    email/email_manager.cpp

    cql_helpers/cql_client.cpp
    cql_helpers/country_cql_manager.cpp
    cql_helpers/holiday_cql_manager.cpp
    cql_helpers/school_cql_manager.cpp
    cql_helpers/announcements_cql_manager.cpp
    cql_helpers/announcements_by_tag_cql_manager.cpp
    cql_helpers/answers_cql_manager.cpp
    cql_helpers/answers_by_announcement_or_question_cql_manager.cpp
    cql_helpers/course_cql_manager.cpp
    cql_helpers/courses_by_user_cql_manager.cpp
    cql_helpers/files_cql_manager.cpp
    cql_helpers/grades_cql_manager.cpp
    cql_helpers/tags_cql_manager.cpp
    cql_helpers/tags_by_user_cql_manager.cpp
    cql_helpers/student_references_cql_manager.cpp
    cql_helpers/lectures_cql_manager.cpp
    cql_helpers/tokens_cql_manager.cpp
    cql_helpers/users_cql_manager.cpp
    cql_helpers/users_by_course_cql_manager.cpp
    cql_helpers/users_by_tag_cql_manager.cpp
    cql_helpers/questions_cql_manager.cpp
    cql_helpers/questions_by_course_cql_manager.cpp
    cql_helpers/todos_cql_manager.cpp
    cql_helpers/todos_by_user_cql_manager.cpp

    database_objects/holiday_object.cpp
    database_objects/country_object.cpp
    database_objects/school_object.cpp
    database_objects/announcements_object.cpp
    database_objects/answer_object.cpp
    database_objects/course_object.cpp
    database_objects/file_object.cpp
    database_objects/grade_object.cpp
    database_objects/tag_object.cpp
    database_objects/student_reference_object.cpp
    database_objects/lecture_object.cpp
    database_objects/user_object.cpp
    database_objects/question_object.cpp
    database_objects/todo_object.cpp

    relations_managers/environment_manager.cpp
    relations_managers/user_manager.cpp
    relations_managers/tag_manager.cpp
    relations_managers/todo_manager.cpp
    relations_managers/course_manager.cpp
    relations_managers/announcement_manager.cpp
    relations_managers/grade_manager.cpp

    api_managers/environment_api_manager.cpp
    api_managers/course_api_manager.cpp
    api_managers/tag_api_manager.cpp
    api_managers/todo_api_manager.cpp
    api_managers/user_api_manager.cpp
    api_managers/announcement_api_manager.cpp
    api_managers/grade_api_manager.cpp
)

add_library(${PROJECT_LIBS} ${SOURCES})
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} ${PROJECT_LIBS})
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "../")
