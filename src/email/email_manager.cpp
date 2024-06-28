#include "email/email_manager.hpp"

std::string EmailManager::generate_email_body(
    const std::string to_address, const std::string password,
    const std::optional<std::string> name) {
  std::string body;

  if (name.has_value())
    body = "Hello " + name.value() + ",\n\n";
  else
    body = "Hello,\n\n";

  body += "You have been registered to the School Management System.\n";
  body += "Your password is: " + password + "\n\n";
  body += "Best regards,\n";
  body += "School Management System Team";

  return body;
}

void EmailManager::send_email(const std::string to_address,
                              const std::string password) {
  if (email_plugin == nullptr)
    email_plugin = drogon::app().getPlugin<SMTPMail>();

  auto id = email_plugin->sendEmail(server_address, server_port, sender_email,
                                    to_address,
                                    "School Management System Registration",
                                    generate_email_body(to_address, password),
                                    sender_email, sender_password, false);
}

void EmailManager::send_email(const std::string to_address,
                              const std::string first_name,
                              const std::string last_name,
                              const std::string password) {
  if (email_plugin == nullptr)
    email_plugin = drogon::app().getPlugin<SMTPMail>();

  std::optional<std::string> name = first_name + " " + last_name;
  auto id = email_plugin->sendEmail(
      server_address, server_port, sender_email, to_address,
      "School Management System Registration",
      generate_email_body(to_address, password, name), sender_email,
      sender_password, false);
}

void EmailManager::send_grade_email(const std::string to_address,
                                    const int grade, const int out_of,
                                    const std::string course_name) {
  std::string body = "Hello,\n\n";
  body += "You have received a grade of " + std::to_string(grade) + " out of " +
          std::to_string(out_of) + " for the course " + course_name + ".\n\n";
  body += "Best regards,\n";
  body += "School Management System Team";

  if (email_plugin == nullptr)
    email_plugin = drogon::app().getPlugin<SMTPMail>();

  auto id =
      email_plugin->sendEmail(server_address, server_port, sender_email,
                              to_address, "School Management System Grade",
                              body, sender_email, sender_password, false);
}