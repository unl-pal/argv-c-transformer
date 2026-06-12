#include "CountingVisitor.hpp"
#include "FilterFunctionsConsumer.hpp"

FilterFunctionsConsumer::FilterFunctionsConsumer(
    std::shared_ptr<std::unordered_map<std::string, CountingVisitor::attributes>> toFilter,
    std::shared_ptr<std::vector<std::string>> toRemove, std::map<std::string, int> *config)
    : _ToFilter(toFilter), _ToRemove(toRemove), _Config(config) {}

void FilterFunctionsConsumer::HandleTranslationUnit(clang::ASTContext & /*context*/) {
  FilterFunctions();
}

void FilterFunctionsConsumer::FilterFunctions() {
  if (_ToFilter->empty())
    return;
  for (const std::pair<const std::string, CountingVisitor::attributes> &func : *_ToFilter) {
    std::string key = func.first;
    CountingVisitor::attributes attr = func.second;
    if (key == "Program" || key == "main") {
      continue;
    } else if (attr.ForLoops > _Config->at("maxForLoops")) {
      _ToRemove->push_back(key);
    } else if (attr.WhileLoops > _Config->at("maxWhileLoops")) {
      _ToRemove->push_back(key);
    } else if (attr.CallFunc > _Config->at("maxCallFunc")) {
      _ToRemove->push_back(key);
    } else if (attr.Functions > _Config->at("maxFunctions")) {
      _ToRemove->push_back(key);
    } else if (attr.IfStmt > _Config->at("maxIfStmt")) {
      _ToRemove->push_back(key);
    } else if (attr.Param > _Config->at("maxParam")) {
      _ToRemove->push_back(key);
    } else if (attr.TypeArithmeticOperation > _Config->at("maxTypeArithmeticOperation")) {
      _ToRemove->push_back(key);
    } else if (attr.TypeCompareOperation > _Config->at("maxTypeCompareOperation")) {
      _ToRemove->push_back(key);
    } else if (attr.TypeIfStmt > _Config->at("maxTypeIfStmt")) {
      _ToRemove->push_back(key);
    } else if (attr.TypeParameters > _Config->at("maxTypeParameters")) {
      _ToRemove->push_back(key);
    } else if (attr.TypePostfix > _Config->at("maxTypePostfix")) {
      _ToRemove->push_back(key);
    } else if (attr.TypePrefix > _Config->at("maxTypePrefix")) {
      _ToRemove->push_back(key);
    } else if (attr.TypeUnaryOperation > _Config->at("maxTypeUnaryOperation")) {
      _ToRemove->push_back(key);
    } else if (attr.TypeVariableReference > _Config->at("maxTypeVariableReference")) {
      _ToRemove->push_back(key);
    } else if (attr.TypeVariables > _Config->at("maxTypeVariables")) {
      _ToRemove->push_back(key);
    } else if (attr.CallFunc < _Config->at("minCallFunc")) {
      _ToRemove->push_back(key);
    } else if (attr.ForLoops < _Config->at("minForLoops")) {
      _ToRemove->push_back(key);
    } else if (attr.Functions < _Config->at("minFunctions")) {
      _ToRemove->push_back(key);
    } else if (attr.IfStmt < _Config->at("minIfStmt")) {
      _ToRemove->push_back(key);
    } else if (attr.Param < _Config->at("minParam")) {
      _ToRemove->push_back(key);
    } else if (attr.TypeArithmeticOperation < _Config->at("minTypeArithmeticOperation")) {
      _ToRemove->push_back(key);
    } else if (attr.TypeCompareOperation < _Config->at("minTypeCompareOperation")) {
      _ToRemove->push_back(key);
    } else if (attr.TypeIfStmt < _Config->at("minTypeIfStmt")) {
      _ToRemove->push_back(key);
    } else if (attr.TypeParameters < _Config->at("minTypeParameters")) {
      _ToRemove->push_back(key);
    } else if (attr.TypePostfix < _Config->at("minTypePostfix")) {
      _ToRemove->push_back(key);
    } else if (attr.TypePrefix < _Config->at("minTypePrefix")) {
      _ToRemove->push_back(key);
    } else if (attr.TypeUnaryOperation < _Config->at("minTypeUnaryOperation")) {
      _ToRemove->push_back(key);
    } else if (attr.TypeVariableReference < _Config->at("minTypeVariableReference")) {
      _ToRemove->push_back(key);
    } else if (attr.TypeVariables < _Config->at("minTypeVariables")) {
      _ToRemove->push_back(key);
    } else if (attr.WhileLoops < _Config->at("minWhileLoops")) {
      _ToRemove->push_back(key);
    }
  }
}
