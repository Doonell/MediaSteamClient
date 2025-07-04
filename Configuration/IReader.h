#ifndef _I_READER_H_
#define _I_READER_H_
#include "IConfigurationFacade.h"
#include <memory>

namespace Configuration {

class IReader {
public:
  virtual const std::shared_ptr<IConfigurationFacade> parse() = 0;
};

} // namespace Configuration
#endif // _I_READER_H_