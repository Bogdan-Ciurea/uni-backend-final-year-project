#include <drogon/drogon.h>

#include <fstream>
#include <iostream>

#include "api_managers/announcement_api_manager.hpp"
#include "api_managers/course_api_manager.hpp"
#include "api_managers/environment_api_manager.hpp"
#include "api_managers/grade_api_manager.hpp"
#include "api_managers/tag_api_manager.hpp"
#include "api_managers/todo_api_manager.hpp"
#include "api_managers/user_api_manager.hpp"
#include "cql_helpers/student_references_cql_manager.hpp"
#include "email/email_manager.hpp"

using namespace drogon;

int main() {
  app().loadConfigFile("../conf.json");

  LOG_INFO << "Drogon version: " << drogon::getVersion();

  // Create the CqlClient object
  LOG_INFO << "Connecting to Cassandra";
  CqlClient *client =
      new CqlClient(app().getCustomConfig()["cassandra"]["host"].asString(),
                    app().getCustomConfig()["cassandra"]["port"].asInt());

  if (client->connect().code() != ResultCode::OK) {
    LOG_ERROR << "Failed to connect to Cassandra";
    return EXIT_FAILURE;
  }
  LOG_INFO << "Connected to Cassandra";

  std::string path_rsa_priv_key_jwt =
      app().getCustomConfig()["rsa_priv_key_jwt"].asString();
  std::string rsa_priv_key_jwt = "";
  std::ifstream file_rsa_priv_key_jwt(path_rsa_priv_key_jwt);

  if (file_rsa_priv_key_jwt.is_open()) {
    std::string line;
    while (getline(file_rsa_priv_key_jwt, line)) {
      rsa_priv_key_jwt += line;
    }
    file_rsa_priv_key_jwt.close();
  } else {
    LOG_ERROR << "Unable to open file " << path_rsa_priv_key_jwt;
    return EXIT_FAILURE;
  }

  std::string path_rsa_pub_key_jwt =
      app().getCustomConfig()["rsa_pub_key_jwt"].asString();
  std::string rsa_pub_key_jwt = "";
  std::ifstream file_rsa_pub_key_jwt(path_rsa_pub_key_jwt);

  if (file_rsa_pub_key_jwt.is_open()) {
    std::string line;
    while (getline(file_rsa_pub_key_jwt, line)) {
      rsa_pub_key_jwt += line;
    }
    file_rsa_pub_key_jwt.close();
  } else {
    LOG_ERROR << "Unable to open file " << path_rsa_pub_key_jwt;
    return EXIT_FAILURE;
  }

  EmailManager *email_manager =
      new EmailManager(app().getCustomConfig()["email"]["server"].asString(),
                       app().getCustomConfig()["email"]["port"].asInt(),
                       app().getCustomConfig()["email"]["email"].asString(),
                       app().getCustomConfig()["email"]["password"].asString());

  SchoolCqlManager *school_cql_manager = new SchoolCqlManager(client);
  school_cql_manager->configure(true);
  HolidayCqlManager *holiday_cql_manager = new HolidayCqlManager(client);
  holiday_cql_manager->configure(true);
  CountryCqlManager *country_cql_manager = new CountryCqlManager(client);
  country_cql_manager->configure(true);
  CoursesByUserCqlManager *courses_by_user_cql_manager =
      new CoursesByUserCqlManager(client);
  courses_by_user_cql_manager->configure(true);
  CoursesCqlManager *courses_cql_manager = new CoursesCqlManager(client);
  courses_cql_manager->configure(true);
  FilesCqlManager *files_cql_manager = new FilesCqlManager(client);
  files_cql_manager->configure(true);
  GradesCqlManager *grades_cql_manager = new GradesCqlManager(client);
  grades_cql_manager->configure(true);
  LecturesCqlManager *lectures_cql_manager = new LecturesCqlManager(client);
  lectures_cql_manager->configure(true);
  TagsCqlManager *tags_cql_manager = new TagsCqlManager(client);
  tags_cql_manager->configure(true);
  TokensCqlManager *tokens_cql_manager = new TokensCqlManager(client);
  tokens_cql_manager->configure(true);
  UsersByCourseCqlManager *users_by_course_cql_manager =
      new UsersByCourseCqlManager(client);
  users_by_course_cql_manager->configure(true);
  UsersByTagCqlManager *users_by_tag_cql_manager =
      new UsersByTagCqlManager(client);
  users_by_tag_cql_manager->configure(true);
  UsersCqlManager *users_cql_manager = new UsersCqlManager(client);
  users_cql_manager->configure(true);
  TagsByUserCqlManager *tags_by_user_cql_manager =
      new TagsByUserCqlManager(client);
  tags_by_user_cql_manager->configure(true);
  TodosCqlManager *todo_cql_manager = new TodosCqlManager(client);
  todo_cql_manager->configure(true);
  TodosByUserCqlManager *todo_by_user_cql_manager =
      new TodosByUserCqlManager(client);
  todo_by_user_cql_manager->configure(true);
  QuestionsCqlManager *questions_cql_manager = new QuestionsCqlManager(client);
  questions_cql_manager->configure(true);
  AnswersCqlManager *answers_cql_manager = new AnswersCqlManager(client);
  answers_cql_manager->configure(true);
  AnswersByAnnouncementOrQuestionCqlManager
      *answers_by_ann_or_question_cql_manager =
          new AnswersByAnnouncementOrQuestionCqlManager(client);
  answers_by_ann_or_question_cql_manager->configure(true);
  QuestionsByCourseCqlManager *questions_by_course_cql_manager =
      new QuestionsByCourseCqlManager(client);
  questions_by_course_cql_manager->configure(true);
  AnnouncementsCqlManager *announcements_cql_manager =
      new AnnouncementsCqlManager(client);
  announcements_cql_manager->configure(true);
  AnnouncementsByTagCqlManager *announcements_by_tag_cql_manager =
      new AnnouncementsByTagCqlManager(client);
  announcements_by_tag_cql_manager->configure(true);
  StudentReferencesCqlManager *student_references_cql_manager =
      new StudentReferencesCqlManager(client);
  student_references_cql_manager->configure(true);

  EnvironmentManager *env_manager = new EnvironmentManager(
      school_cql_manager, holiday_cql_manager, country_cql_manager);
  CourseManager *course_manager = new CourseManager(
      users_cql_manager, tokens_cql_manager, files_cql_manager,
      grades_cql_manager, courses_cql_manager, users_by_course_cql_manager,
      courses_by_user_cql_manager, lectures_cql_manager, tags_cql_manager,
      users_by_tag_cql_manager, questions_cql_manager, answers_cql_manager,
      answers_by_ann_or_question_cql_manager, questions_by_course_cql_manager);
  TagManager *tag_manager =
      new TagManager(tags_cql_manager, users_cql_manager, tokens_cql_manager,
                     users_by_tag_cql_manager, tags_by_user_cql_manager);
  TodoManager *todo_manager =
      new TodoManager(users_cql_manager, tokens_cql_manager, todo_cql_manager,
                      todo_by_user_cql_manager);
  UserManager *user_manager = new UserManager(
      users_cql_manager, tokens_cql_manager, school_cql_manager,
      users_by_course_cql_manager, courses_by_user_cql_manager,
      tags_by_user_cql_manager, users_by_tag_cql_manager,
      todo_by_user_cql_manager, todo_cql_manager, grades_cql_manager,
      questions_cql_manager, answers_cql_manager,
      answers_by_ann_or_question_cql_manager, questions_by_course_cql_manager);
  AnnouncementManager *announcement_manager = new AnnouncementManager(
      announcements_cql_manager, announcements_by_tag_cql_manager,
      tags_cql_manager, tokens_cql_manager, users_cql_manager,
      answers_by_ann_or_question_cql_manager, answers_cql_manager,
      files_cql_manager, tags_by_user_cql_manager);
  GradeManager *grade_manager = new GradeManager(
      grades_cql_manager, users_cql_manager, users_by_course_cql_manager,
      courses_by_user_cql_manager, tokens_cql_manager, courses_cql_manager);

  // Register the SayHello controller
  auto env_api = std::make_shared<EnvironmentAPIManager>(env_manager);
  auto course_api =
      std::make_shared<CourseAPIManager>(course_manager, rsa_pub_key_jwt);
  auto tag_api = std::make_shared<TagAPIManager>(tag_manager, rsa_pub_key_jwt);
  auto todo_api =
      std::make_shared<TodoAPIManager>(todo_manager, rsa_pub_key_jwt);
  auto user_api = std::make_shared<UserAPIManager>(
      user_manager, rsa_priv_key_jwt, rsa_pub_key_jwt, email_manager);
  auto announcement_api = std::make_shared<AnnouncementAPIManager>(
      announcement_manager, rsa_pub_key_jwt);
  auto grade_api = std::make_shared<GradeAPIManager>(
      grade_manager, rsa_pub_key_jwt, email_manager,
      student_references_cql_manager);

  app().registerController(env_api);
  app().registerController(course_api);
  app().registerController(tag_api);
  app().registerController(todo_api);
  app().registerController(user_api);
  app().registerController(announcement_api);
  app().registerController(grade_api);

  app().registerPreHandlingAdvice([](const drogon::HttpRequestPtr &req) {
    LOG_DEBUG << "----Received New Request----";
    LOG_DEBUG << req->path();
    LOG_DEBUG << req->methodString();
  });

  app().registerPostHandlingAdvice([](const drogon::HttpRequestPtr &req,
                                      const drogon::HttpResponsePtr &resp) {
    // LOG_DEBUG << "postHandling1";
    // LOG_DEBUG << "----Responding to the Request----";
    // LOG_DEBUG << req->path();
    LOG_DEBUG << resp->statusCode();
    // resp->addHeader("Access-Control-Allow-Origin",
    // app().getCustomConfig()["front_end_url"].asString());
    resp->addHeader("Access-Control-Allow-Origin", "*");
    resp->addHeader("Access-Control-Allow-Methods",
                    "GET, POST, PUT, DELETE, OPTIONS");
  });

  LOG_INFO << "Server running on http://localhost:8080";

  app().run();

  delete env_manager;
  delete course_manager;
  delete tag_manager;
  delete todo_manager;
  delete user_manager;
  delete announcement_manager;
  delete grade_manager;
  delete email_manager;

  delete school_cql_manager;
  delete holiday_cql_manager;
  delete country_cql_manager;
  delete courses_by_user_cql_manager;
  delete courses_cql_manager;
  delete files_cql_manager;
  delete grades_cql_manager;
  delete lectures_cql_manager;
  delete tags_cql_manager;
  delete tokens_cql_manager;
  delete users_by_course_cql_manager;
  delete users_by_tag_cql_manager;
  delete users_cql_manager;
  delete tags_by_user_cql_manager;
  delete todo_by_user_cql_manager;
  delete todo_cql_manager;
  delete announcements_cql_manager;
  delete announcements_by_tag_cql_manager;
  delete grades_cql_manager;
  delete questions_cql_manager;
  delete answers_cql_manager;
  delete answers_by_ann_or_question_cql_manager;
  delete questions_by_course_cql_manager;
  delete student_references_cql_manager;

  delete client;

  return EXIT_SUCCESS;
}