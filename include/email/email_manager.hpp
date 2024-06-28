/**
 * @file email_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief The purpose of this file is to define the manager
 * that will send emails to the users
 * @version 0.1
 * @date 2023-02-12
 * @copyright Copyright (c) 2023
 *
 */

#ifndef EMAIL_MANAGER_H
#define EMAIL_MANAGER_H

#include <SMTPMail.h>
#include <drogon/drogon.h>

#include <string>

class EmailManager {
 private:
  std::string server_address;
  uint16_t server_port;
  std::string sender_email;
  std::string sender_password;

  SMTPMail* email_plugin = nullptr;

  std::string generate_email_body(
      const std::string to_address, const std::string password,
      const std::optional<std::string> name = std::nullopt);

 public:
  EmailManager(const std::string serv_addr, const uint16_t serv_port,
               const std::string email, const std::string password)
      : server_address(serv_addr),
        server_port(serv_port),
        sender_email(email),
        sender_password(password){};
  ~EmailManager(){};

  void send_email(const std::string to_address, const std::string password);
  void send_email(const std::string to_address, const std::string first_name,
                  const std::string last_name, const std::string password);
  void send_grade_email(const std::string to_address, const int grade,
                        const int out_of, const std::string course_name);
};

#endif