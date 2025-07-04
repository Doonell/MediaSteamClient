#include "JsonFileReader.h"
#include "ConfigurationBuilder.h"
#include <fstream>
#include <iostream>
namespace Configuration {

JsonFileReader::JsonFileReader(const std::string &filePath)
    : filePath_(filePath) {}

std::string JsonFileReader::get_value(const std::string &line) {
  auto pos1 = line.find(':');
  if (pos1 == std::string::npos)
    return "";

  auto pos2 = line.find_first_of("\"0123456789", pos1 + 1);
  if (pos2 == std::string::npos)
    return "";
  auto pos3 = line.find_first_of(",}", pos2);
  std::string val = line.substr(pos2, pos3 - pos2);
  // 去除引号和空格
  val.erase(remove(val.begin(), val.end(), '"'), val.end());
  val.erase(0, val.find_first_not_of(" \t"));
  val.erase(val.find_last_not_of(" \t") + 1);
  return val;
}

const std::shared_ptr<IConfigurationFacade> JsonFileReader::parse() {
  // 解析 JSON 文件并返回 IConfigurationFacade 实例
  std::shared_ptr<ConfigurationBuilder> configBuilder =
      std::make_shared<ConfigurationBuilder>();
  std::string line, section;
  std::ifstream file(filePath_);
  while (std::getline(file, line)) {
    // 去除前后空格
    line.erase(0, line.find_first_not_of(" \t"));
    line.erase(line.find_last_not_of(" \t\r\n") + 1);

    if (line.empty() || line[0] == '{' || line[0] == '}')
      continue;

    if (line.find("rtmpurl") != std::string::npos) {
      std::cout << "rtmpurl: " << get_value(line) << std::endl;
      configBuilder->set("rtmpUrl", get_value(line));
    } else if (line.find("\"audio\"") != std::string::npos) {
      section = "audio";
    } else if (line.find("\"Video\"") != std::string::npos) {
      section = "Video";
    } else if (line.find("\"Display\"") != std::string::npos) {
      section = "Display";
    } else if (line.find("\"Mic\"") != std::string::npos) {
      section = "Mic";
    } else if (line.find("}") != std::string::npos) {
      section.clear();
    } else if (!section.empty()) {
      std::string key = line.substr(0, line.find(':'));
      key.erase(remove(key.begin(), key.end(), '"'), key.end());
      key.erase(0, key.find_first_not_of(" \t"));
      key.erase(key.find_last_not_of(" \t") + 1);
      std::string value = get_value(line);
      std::cout << section << "." << key << ": " << value << std::endl;
      configBuilder->set(key, value);
    }
  }
  // 读取文件内容并填充 config
  return configBuilder;
}
} // namespace Configuration
