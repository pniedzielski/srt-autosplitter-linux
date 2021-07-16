#ifndef SRT_SETTINGS_HPP
#define SRT_SETTINGS_HPP

#include <string>         // std::string
#include <unordered_map>  // std::unordered_map<K,T>

struct setting {
  bool value;
  std::string description;
  std::string parent;
};

struct settings_map {
  std::unordered_map<std::string, setting> kv_storage;

  void add(const std::string& key, bool value, const std::string& description) {
    add(key, value, description, std::string{});
  }
  void add(const std::string& key, bool value, const std::string& description, const std::string& parent) {
    kv_storage[key] = setting{ value, description, parent };
  }

  bool operator[](std::string key) {
    auto value = true;

    // Walk the parentage, becoming false if we find any false setting.
    do {
      auto& setting_value = kv_storage[key];
      value         = value && setting_value.value;
      key           = setting_value.parent;
    } while (!key.empty());

    return value;
  }
};

#endif  // #ifndef SRT_SETTINGS_HPP
