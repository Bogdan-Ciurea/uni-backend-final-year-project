#include "database_objects/file_object.hpp"

Json::Value FileObject::to_json(bool secure) {
  Json::Value value;
  if (!secure) {
    value["school_id"] = _school_id;
    value["size"] = _size;
  }

  char* id_uuid_string = (char*)malloc(sizeof(char) * 37);
  cass_uuid_string(_id, id_uuid_string);

  value["id"] = id_uuid_string;

  char* _added_by_user_uuid_string = (char*)malloc(sizeof(char) * 37);
  cass_uuid_string(_added_by_user, _added_by_user_uuid_string);

  value["created_by_user_id"] = _added_by_user_uuid_string;

  free(id_uuid_string);
  free(_added_by_user_uuid_string);

  if (_type == CustomFileType::FILE) {
    std::string extension =
        _path_to_file.substr(_path_to_file.find_last_of(".") + 1);
    value["name"] = _name + "." + extension;
  } else {
    value["name"] = _name;
  }
  // The path to file will be of the next example:
  // .././files/schools/<school-id>/<announcements or courses>/<course or
  // announcement id>/<file-id>.<extension> We want to get <announcement or
  // course>/<announcement or course id>/files?file_id=<file-id>

  // Get the string between the 5th and 7th slash
  std::string temp_path_to_file = _path_to_file;
  for (int i = 0; i < 5; i++) {
    temp_path_to_file =
        temp_path_to_file.substr(temp_path_to_file.find("/") + 1);
  }
  // We remain with the next string
  //  <announcements or courses>/<course or announcement
  // id>/<file-id>.<extension>
  // An uuid is 36 characters long, so we can get the string between the 2nd and
  // 3rd slash
  if (temp_path_to_file.at(0) == 'a') {
    temp_path_to_file =
        temp_path_to_file.substr(temp_path_to_file.find("/") + 1);
    std::string announcement_id =
        temp_path_to_file.substr(0, temp_path_to_file.find("/"));
    value["path"] = "announcement/" + announcement_id +
                    "/files?file_id=" + value["id"].asString();

  } else {
    temp_path_to_file =
        temp_path_to_file.substr(temp_path_to_file.find("/") + 1);
    std::string course_id =
        temp_path_to_file.substr(0, temp_path_to_file.find("/"));
    value["path"] =
        "course/" + course_id + "/files?file_id=" + value["id"].asString();
  }

  value["files"] = Json::Value(Json::arrayValue);
  for (auto file : _files) {
    char* file_uuid_string = (char*)malloc(sizeof(char) * 37);
    cass_uuid_string(file, file_uuid_string);
    value["files"].append(file_uuid_string);
  }
  value["type"] = static_cast<int>(_type);
  value["visible_to_students"] = _visible_to_students;
  value["students_can_add"] = _students_can_add;

  return value;
}