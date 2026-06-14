#define SCRATCHINTERPRETER_VERSION "1.0.0"
#include <iostream>
#include <json.hpp>
#include <string>
#include <unistd.h>
#include <zip.h>

using json = nlohmann::json;

std::string readProject(const std::string &projectPath) {
  std::string fileContents;
  int err = 0;
  zip_t *archive = zip_open(projectPath.c_str(), ZIP_RDONLY, &err);
  struct zip_stat st;
  zip_stat_init(&st);
  zip_stat(archive, "project.json", 0, &st);
  fileContents.resize(st.size);
  zip_file *projectJson = zip_fopen(archive, "project.json", 0);
  zip_fread(projectJson, &fileContents[0], st.size);
  zip_fclose(projectJson);
  zip_close(archive);
  return fileContents;
}

void cleanString(std::string &str) { str = str.substr(0, str.length()); }

bool checkCondition(const json &condition, const json &codeObj) {
  if (condition["opcode"] == "operator_equals") {
    auto t1 = condition["inputs"]["OPERAND1"][1][1];
    auto t2 = condition["inputs"]["OPERAND2"][1][1];
    return (t1 == t2);
  } else if (condition["opcode"] == "operator_gt") {
    auto t1Str = condition["inputs"]["OPERAND1"][1][1].get<std::string>();
    auto t2Str = condition["inputs"]["OPERAND2"][1][1].get<std::string>();
    return (std::stoi(t1Str) > std::stoi(t2Str));
  } else if (condition["opcode"] == "operator_lt") {
    auto t1Str = condition["inputs"]["OPERAND1"][1][1].get<std::string>();
    auto t2Str = condition["inputs"]["OPERAND2"][1][1].get<std::string>();
    return (std::stoi(t1Str) < std::stoi(t2Str));
  }
  return false;
}
void executeCode(const json &block, const json &codeObj) {
  auto opcode = block["opcode"].get<std::string>();
  if (opcode.empty()) {
    return;
  } else if (opcode == "looks_say") {
    auto message = block["inputs"]["MESSAGE"][1][1].get<std::string>();
    cleanString(message);
    std::cout << message << std::endl;
  } else if (opcode == "looks_sayforsecs") {
    auto message = block["inputs"]["MESSAGE"][1][1].get<std::string>();
    auto secsStr = block["inputs"]["SECS"][1][1].get<std::string>();
    cleanString(secsStr);
    auto secs = std::stoi(secsStr);
    std::cout << message << std::endl;
    sleep(secs);
    std::cout << "\033[A\033[2K" << std::flush;
  } else if (opcode == "control_wait") {
    auto secsStr = block["inputs"]["DURATION"][1][1].get<std::string>();
    cleanString(secsStr);
    auto secs = std::stoi(secsStr);
    sleep(secs);
  } else if (opcode == "control_repeat") {
    auto timesStr = block["inputs"]["TIMES"][1][1].get<std::string>();
    cleanString(timesStr);
    auto times = std::stoi(timesStr);
    auto firstChildId = block["inputs"]["SUBSTACK"][1].get<std::string>();
    auto firstChild = codeObj["targets"][1]["blocks"][firstChildId];
    for (auto i = 0; i < times; i++) {
      json currentBlock = firstChild;
      while (true) {
        executeCode(currentBlock, codeObj);
        if (currentBlock["next"].is_null()) {
          break;
        }
        std::string nextBlockId = currentBlock["next"];
        currentBlock = codeObj["targets"][1]["blocks"][nextBlockId];
      }
    }
  } else if (opcode == "control_if") {
    auto conditionId = block["inputs"]["CONDITION"][1];
    auto condition = codeObj["targets"][1]["blocks"][conditionId];
    bool passedCondition = checkCondition(condition, codeObj);
    if (!passedCondition) {
      return;
    }
    auto firstChildId = block["inputs"]["SUBSTACK"][1];
    auto firstChild = codeObj["targets"][1]["blocks"][firstChildId];
    json currentBlock = firstChild;
    while (true) {
      executeCode(currentBlock, codeObj);
      if (currentBlock["next"].is_null()) {
        break;
      }
      std::string nextBlockId = currentBlock["next"];
      currentBlock = codeObj["targets"][1]["blocks"][nextBlockId];
    }
  }
  return;
}
int main(int argc, char *argv[]) {
  auto scratchCode = readProject(argv[1]);
  auto codeObj = json::parse(scratchCode);
  std::cout << "[+] Scratch VM version: ";
  auto scratchVmVer = codeObj["meta"]["vm"].get<std::string>();
  cleanString(scratchVmVer);
  std::cout << scratchVmVer << std::endl;
  std::cout << "[+] Scratch Interpreter version: " << SCRATCHINTERPRETER_VERSION
            << std::endl;
  json entryPoint;
  for (auto &[key, val] : codeObj["targets"][1]["blocks"].items()) {
    if (val["opcode"].get<std::string>() == "event_whenflagclicked") {
      entryPoint = val;
      break;
    }
  }
  if (entryPoint.is_null()) {
    return 1;
  }
  json nextBlock = entryPoint;
  while (!nextBlock["next"].is_null()) {
    std::string nextBlockId = nextBlock["next"];
    nextBlock = codeObj["targets"][1]["blocks"][nextBlockId];
    executeCode(nextBlock, codeObj);
  }
  std::cout << "[+] Finished execution." << std::endl;
  return 0;
}
