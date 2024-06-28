/**
 * @file file_object.hpp
 * @author Bogdan Ciurea (sc20bac@leeds.ac.uk)
 * @brief Class that stores the file object
 * @version 0.1
 * @date 2023-01-03
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef FILE_OBJECT_H
#define FILE_OBJECT_H

#include <json/json.h>

#include <string>
#include <vector>

#include "cassandra.h"

enum class CustomFileType { FILE = 0, FOLDER = 1 };

class FileObject {
 public:
  FileObject(){};
  FileObject(int school_id, std::string id, CustomFileType type,
             std::string name, std::vector<std::string> files,
             std::string path_to_file, int size, std::string added_by_user,
             bool visible_to_students, bool students_can_add)
      : _school_id(school_id),
        _name(std::move(name)),
        _type(type),
        _path_to_file(std::move(path_to_file)),
        _size(size),
        _visible_to_students(visible_to_students),
        _students_can_add(students_can_add) {
    cass_uuid_from_string(id.c_str(), &_id);
    cass_uuid_from_string(added_by_user.c_str(), &_added_by_user);
    for (auto &file : files) {
      CassUuid file_id;
      cass_uuid_from_string(file.c_str(), &file_id);
      _files.push_back(file_id);
    }
  }
  FileObject(int school_id, CassUuid id, CustomFileType type, std::string name,
             std::vector<CassUuid> files, std::string path_to_file, int size,
             CassUuid added_by_user, bool visible_to_students,
             bool students_can_add)
      : _school_id(school_id),
        _id(id),
        _type(type),
        _name(std::move(name)),
        _files(std::move(files)),
        _path_to_file(std::move(path_to_file)),
        _size(size),
        _added_by_user(added_by_user),
        _visible_to_students(visible_to_students),
        _students_can_add(students_can_add) {}
  ~FileObject(){};

  /**
   * @brief Returns a Json object containing the information about this object
   *
   * @param secure If we just want the most important information
   * @return The Json object
   */
  Json::Value to_json(bool secure);

  int _school_id;
  CassUuid _id;
  CustomFileType _type;
  std::string _name;
  std::vector<CassUuid> _files;
  std::string _path_to_file;
  int _size;
  CassUuid _added_by_user;
  bool _visible_to_students;
  bool _students_can_add;
};

#endif  // FILE_OBJECT_H