
namespace Configration {

class JsonFileReader : public IReader {
public:
  JsonFileReader(const std::string &filePath);
  ~JsonFileReader();

  std::shared_ptr<IConfigurationFacade> parse() override;

private:
  std::string get_value(const std::string &line);

private:
  std::string filePath_;
};
} // namespace Configration