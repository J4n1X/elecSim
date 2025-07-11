#include <format>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" {
#include "hope.h"
}
#define OLC_PGE_APPLICATION
#include "Grid.h"

const char* prog_desc = "Prober is a tool for simulating elecSim circuits.";
const char* prog_version = "0.1";

static hope_t initParser(const char* prog_name) {
  hope_t hope = hope_init(prog_name, prog_desc);
  hope_set_t paramSet = hope_init_set("Main");
  hope_add_param(&paramSet, hope_init_param("-f", "Grid file to load",
                                            HOPE_TYPE_STRING, 1));
  hope_add_param(&paramSet, hope_init_param("-t", "Test behavior file to load",
                                            HOPE_TYPE_STRING, 1));
  hope_add_param(&paramSet,
                 hope_init_param("-v", "Verbose mode: Print the log event",
                                 HOPE_TYPE_SWITCH, HOPE_ARGC_OPT));
  hope_set_t helpSet = hope_init_set("Help");
  hope_add_param(&helpSet, hope_init_param("-h", "Show this help message",
                                           HOPE_TYPE_SWITCH, HOPE_ARGC_OPT));

  hope_add_set(&hope, paramSet);
  hope_add_set(&hope, helpSet);
  return hope;
}

// The format of a test will be as follows:
//(Writing does not mark what tile is written to, but rather from what tile
// the write comes from)
// Writing: w x y (1/0) s
// Interacting: i x y
// Stepping in the simulation: s
// Reading: r x y (1/0)
// If the result of a read is not as expected, the test fails.
class TestParser {
 public:
  enum class CommandType { Write, Interact, Step, Read, Comment };
  struct Command {
    CommandType type;
    int x = 0;
    int y = 0;
    int value = 0;  // For write/read: 1 or 0. For step: unused.
    ElecSim::Direction dir = ElecSim::Direction::Top;
    std::string comment = "";
  };

 private:
  std::vector<Command> commands;

  // Helper to read an integer (possibly negative) from a stream, skipping
  // whitespace
  static bool ReadInt(std::istringstream& iss, int& out) {
    iss >> std::ws;
    char c = iss.peek();
    if (c == '-' || (c >= '0' && c <= '9')) {
      iss >> out;
      return !iss.fail();
    }
    return false;
  }

 public:
  TestParser() = default;
  ~TestParser() = default;

  constexpr const char* GetCommandTypeString(CommandType type) const {
    switch (type) {
      case CommandType::Write:
        return "Write";
      case CommandType::Interact:
        return "Interact";
      case CommandType::Step:
        return "Step";
      case CommandType::Read:
        return "Read";
      case CommandType::Comment:
        return "Comment";
      default:
        return "Unknown";
    }
  }

  std::string GetCommandString(const Command& command) const {
    return std::format("{} {} {}", GetCommandTypeString(command.type),
                       ElecSim::vi2d(command.x, command.y), command.value);
  }

  void Parse(const std::string& testFile) {
    std::ifstream file(testFile);
    if (!file)
      throw std::runtime_error("Could not open test file: " + testFile);
    std::string line;
    int lineNum = 0;
    while (std::getline(file, line)) {
      lineNum++;
      std::istringstream iss(line);
      iss >> std::ws;
      char cmd;
      iss >> cmd;
      if (iss.fail() || line.empty()) continue;  // skip empty or comment lines
      if (cmd == '#') {
        // read until eol and store the text in a string
        std::string comment;
        std::getline(iss, comment);
        if (comment.empty()) continue;  // skip empty comments
        commands.push_back(
            {CommandType::Comment, 0, 0, 0, ElecSim::Direction::Top, comment});
        continue;
      }
      if (cmd == 'w') {
        int x, y, v, s;
        if (!ReadInt(iss, x)) goto malformed_write;
        if (!ReadInt(iss, y)) goto malformed_write;
        if (!ReadInt(iss, v)) goto malformed_write;
        if (!ReadInt(iss, s)) goto malformed_write;
        commands.push_back(
            {CommandType::Write, x, y, v, static_cast<ElecSim::Direction>(s)});
        continue;
      } else if (cmd == 'i') {
        int x, y;
        if (!ReadInt(iss, x)) goto malformed_interact;
        if (!ReadInt(iss, y)) goto malformed_interact;
        commands.push_back({CommandType::Interact, x, y});
        continue;
      } else if (cmd == 's') {
        commands.push_back({CommandType::Step});
        continue;
      } else if (cmd == 'r') {
        int x, y, v;
        if (!ReadInt(iss, x)) goto malformed_read;
        if (!ReadInt(iss, y)) goto malformed_read;
        if (!ReadInt(iss, v)) goto malformed_read;
        commands.push_back({CommandType::Read, x, y, v});
        continue;
      }
    // unknown_read:
    //   throw std::runtime_error(std::format("Unknown command '{}' at line {}",
    //   cmd, lineNum));
    malformed_write:
      throw std::runtime_error(
          std::format("Malformed write command at line {}", lineNum));
    malformed_interact:
      throw std::runtime_error(
          std::format("Malformed interact command at line {}", lineNum));
    malformed_read:
      throw std::runtime_error(
          std::format("Malformed read command at line {}", lineNum));
    }
  }
  const std::vector<Command>& GetCommands() const { return commands; }
};

int main([[maybe_unused]] int argc, char** argv) {
  hope_t hope = initParser(argv[0]);

  if (hope_parse_argv(&hope, argv))  // error occurred, print error message
    return 1;
  if (strcmp(hope.used_set_name, "Help") == 0) {  // no need to even check the switch
    hope_print_help(&hope, stdout);
    return 0;
  }
  std::string gridFile = hope_get_single_string(&hope, "-f");
  std::string testFile = hope_get_single_string(&hope, "-t");
  bool verbose = hope_get_single_switch(&hope, "-v");
  hope_free(&hope);

  auto grid = ElecSim::Grid();
  grid.Load(gridFile);
  grid.Simulate();

  auto testParser = TestParser();
  testParser.Parse(testFile);
  auto commands = testParser.GetCommands();

  for (auto& command : commands) {
    auto tileMaybe = grid.GetTile(command.x, command.y);
    switch (command.type) {
      case TestParser::CommandType::Write:
        if (tileMaybe.has_value()) {
          auto tile = tileMaybe.value();
          if (command.value == 1) {
            // Queue an update
            tile->SetActivation(command.value);
            grid.QueueUpdate(tile,
                             ElecSim::SignalEvent(tile->GetPos(), command.dir,
                                                  command.value));
          }
        }
        break;
      case TestParser::CommandType::Interact:
        if (tileMaybe.has_value()) {
          auto tile = tileMaybe.value();
          auto signals = tile->Interact();
          for (const auto& signal : signals) {
            grid.QueueUpdate(tile, signal);
          }
        }
        break;
      case TestParser::CommandType::Step:
        grid.Simulate();
        break;
      case TestParser::CommandType::Read:
        std::cout << std::format("Tile at {}:\n  Expected: {}\n  Actual: ",
                                 ElecSim::vi2d(command.x, command.y),
                                 (command.value ? "active" : "inactive"));
        if (tileMaybe.has_value()) {
          auto tile = tileMaybe.value();
          std::cout << (tile->GetActivation() ? "active" : "inactive");
          if (tile->GetActivation() != static_cast<bool>(command.value)) {
            std::cout << " (Test failed)";
            return 1;
          }
        } else {
          std::cout << "None";
        }
        std::cout << std::endl;
        break;
      case TestParser::CommandType::Comment:
        if (verbose) std::cout << command.comment << std::endl;
        break;
      default:
        std::cerr << "Unknown command type: "
                  << testParser.GetCommandTypeString(command.type)
                  << " of value: " << static_cast<int>(command.type)
                  << ", aborting" << std::endl;
        return 1;
    }
  }
  std::cout << "Test completed successfully." << std::endl;
  return 0;
}