#ifndef _FILE_READER_FACTORY_H_
#define _FILE_READER_FACTORY_H_

#include "IReader.h"
#include "JsonFileReader.h"
#include <memory>
#include <string>

namespace Configuration {

class FileReaderFactory {
public:
  static std::shared_ptr<IReader>
  createFileReader(const std::string &filePath, const std::string &fileType) {
    if (fileType == "txt") {
      // return std::make_shared<TxtFileReader>(filePath);
      return nullptr; // XML reader not implemented
    } else if (fileType == "json") {
      return std::make_shared<JsonFileReader>(filePath);
    } else if (fileType == "xml") {
      // return std::make_shared<XmlFileReader>(filePath);
      return nullptr; // XML reader not implemented
    } else {
      return nullptr;
    }
  }
};
} // namespace Configuration
#endif // _FILE_READER_FACTORY_H_