#ifndef _I_READER_H_
#define _I_READER_H_

#include <vector>

namespace Source {
class IReader {
public:
  virtual std::vector<char> readAll() = 0;
};

} // namespace Source

#endif // _I_READER_H_
