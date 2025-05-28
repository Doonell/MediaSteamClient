#include "IReader.h"
#include "JsonFileReader.h"

namespace Configuration {

class FileReaderFactory {
public:
  static std::shared_ptr<IReader>
  createFileReader(const std::string &filePath, const std::string &fileType) {
    if (fileType == "txt") {
      return std::make_shared<TxtFileReader>(filePath);
    } else if (fileType == "json") {
      return std::make_shared<JsonFileReader>(filePath);
    } else if (fileType == "xml") {
      return std::make_shared<XmlFileReader>(filePath);
    } else {
      return nullptr;
    }
  }
}
