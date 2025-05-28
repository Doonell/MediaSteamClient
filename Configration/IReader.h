#include "IConfigurationFacade.h"

class IReader {
public:
  virtual ~IReader() = default;
  virtual std::shared_ptr<IConfigurationFacade> Parse() = 0;
};