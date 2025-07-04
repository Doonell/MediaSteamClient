#ifndef _JSON_FILE_READER_H_
#define _JSON_FILE_READER_H_

#include "IConfigurationFacade.h"
#include "IReader.h"
#include <memory>
#include <string>

namespace Configuration {

class JsonFileReader : public IReader {
public:
  JsonFileReader(const std::string &filePath);
  ~JsonFileReader() = default;

  const std::shared_ptr<IConfigurationFacade> parse() override;

private:
  std::string get_value(const std::string &line);

private:
  std::string filePath_;
};

} // namespace Configuration
#endif // _JSON_FILE_READER_H_