/**
 * @file announcement_api_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief The purpose of this file is to define the manager of
 * the apis for the announcement related objects
 * @version 0.1
 * @date 2023-02-23
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef ANNOUNCEMENT_API_MANAGER_HPP
#define ANNOUNCEMENT_API_MANAGER_HPP

#include <filesystem>
#include <unordered_map>

#include "../relations_managers/announcement_manager.hpp"
#include "jwt-cpp/jwt.h"

using namespace drogon;

class AnnouncementAPIManager
    : public HttpController<AnnouncementAPIManager, false> {
 public:
  AnnouncementAPIManager(AnnouncementManager *announcementManager,
                         std::string public_key)
      : _announcement_manager(announcementManager), _public_key(public_key) {
    // For the actual announcement
    app().registerHandler("/api/announcements",
                          &AnnouncementAPIManager::create_announcement, {Post});
    app().registerHandler("/api/user_announcements",
                          &AnnouncementAPIManager::get_user_announcements,
                          {Get});
    app().registerHandler("/api/announcement/{announcement-id}",
                          &AnnouncementAPIManager::delete_announcement,
                          {Delete});

    // For the announcement files
    app().registerHandler("/api/announcement/{announcement-id}/files",
                          &AnnouncementAPIManager::create_announcement_file,
                          {Post});
    app().registerHandler(
        "/api/announcement/{announcement-id}/files?file_id={file-id}",
        &AnnouncementAPIManager::get_announcement_file, {Get});
    app().registerHandler(
        "/api/announcement/{announcement-id}/files?file_id={file-id}",
        &AnnouncementAPIManager::delete_announcement_file, {Delete});

    // For the announcement related users
    app().registerHandler("/api/announcement/{announcement-id}/tags",
                          &AnnouncementAPIManager::add_tags_to_announcement,
                          {Post});
    app().registerHandler("/api/announcement/{announcement-id}/tags",
                          &AnnouncementAPIManager::get_announcement_tags,
                          {Get});
    app().registerHandler(
        "/api/announcement/{announcement-id}/tags",
        &AnnouncementAPIManager::remove_tags_from_announcement, {Delete});

    // For answers
    app().registerHandler("/api/announcement/{announcement-id}/answers",
                          &AnnouncementAPIManager::create_announcement_answer,
                          {Post});
    app().registerHandler(
        "/api/announcement/{announcement-id}/answers?answer_id={answer-id}",
        &AnnouncementAPIManager::delete_announcement_answer, {Delete});
  };

  ~AnnouncementAPIManager(){};
  METHOD_LIST_BEGIN
  METHOD_LIST_END

 protected:
  ///
  /// Functions that would be used to manage the announcements
  ///

  void create_announcement(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback);
  void get_user_announcements(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback);
  void delete_announcement(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      std::string announcement_id);

  ///
  /// Functions that would be able to manage the announcements files
  ///
  void create_announcement_file(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      std::string announcement_id);
  void get_announcement_file(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      std::string announcement_id, std::string file_id);
  void delete_announcement_file(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      std::string announcement_id, std::string file_id);

  ///
  /// Functions that would be able to manage the announcements users
  ///
  void add_tags_to_announcement(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      std::string announcement_id);
  void get_announcement_tags(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      std::string announcement_id);
  void remove_tags_from_announcement(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      std::string announcement_id);

  ///
  /// Functions that would be able to manage the announcements questions and
  /// answers
  ///

  void create_announcement_answer(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      std::string announcement_id);
  void delete_announcement_answer(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      std::string announcement_id, std::string answer_id);

 private:
  AnnouncementManager *_announcement_manager;
  std::string _public_key;

  void send_response(std::function<void(const HttpResponsePtr &)> &&callback,
                     const drogon::HttpStatusCode status_code,
                     const std::string &message);

  bool is_file_name_valid(const std::string file_name);

  /**
   * @brief Get the credentials from the authorization header.
   * It will return a pair of the school id and the token.
   * If there was an error, the school id will be -1 and the token will be
   * empty.
   *
   * @param authorization_header - The authorization header.
   * @return std::pair<int, std::string>
   */
  std::pair<int, std::string> get_credentials(
      const std::string authorization_header);

  // Suported mime types (order alphabetically)
  const std::unordered_map<std::string, std::string> _mime_types{
      {".jpg", "image/jpeg"},
      {".jpeg", "image/jpeg"},
      {".png", "image/png"},
      {".gif", "image/gif"},
      {".bmp", "image/bmp"},
      {".txt", "text/plain"},
      {".doc", "application/msword"},
      {".docx",
       "application/"
       "vnd.openxmlformats-officedocument.wordprocessingml.document"},
      {".pdf", "application/pdf"},
      {".xls", "application/vnd.ms-excel"},
      {".xlsx",
       "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
      {".ppt", "application/vnd.ms-powerpoint"},
      {".pptx",
       "application/"
       "vnd.openxmlformats-officedocument.presentationml.presentation"},
      {".html", "text/html"},
      {".htm", "text/html"},
      {".xml", "application/xml"},
      {".mp3", "audio/mpeg"},
      {".wav", "audio/x-wav"},
      {".mp4", "video/mp4"},
      {".mov", "video/quicktime"},
      {".flv", "video/x-flv"},
      {".zip", "application/zip"},
      {".rar", "application/x-rar-compressed"},
      {".tar", "application/x-tar"},
      {".gz", "application/gzip"},
      {".swf", "application/x-shockwave-flash"},
      {".js", "application/javascript"},
      {".css", "text/css"},
      {".rtf", "application/rtf"},
      {".psd", "image/vnd.adobe.photoshop"},
      {".ai", "application/postscript"},
      {".eps", "application/postscript"},
      {".tiff", "image/tiff"},
      {".tif", "image/tiff"},
      {".svg", "image/svg+xml"},
      {".eot", "application/vnd.ms-fontobject"},
      {".ttf", "application/x-font-ttf"},
      {".otf", "application/x-font-otf"},
      {".woff", "application/x-font-woff"},
      {".woff2", "application/x-font-woff2"},
      {".ico", "image/x-icon"},
      {".midi", "audio/midi"},
      {".mid", "audio/midi"},
      {".amr", "audio/amr"},
      {".aif", "audio/x-aiff"},
      {".aiff", "audio/x-aiff"},
      {".m4a", "audio/x-m4a"},
      {".m4v", "video/x-m4v"},
      {".3gp", "video/3gpp"},
      {".3g2", "video/3gpp2"},
      {".ogv", "video/ogg"},
      {".webm", "video/webm"},
      {".mkv", "video/x-matroska"}};
};

#endif