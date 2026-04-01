#include "config_utils.hpp"

#include <cctype>
#include <cstdlib>
#include <string>

namespace {

bool IsIntegerToken(const std::string &token) {
  if(token.empty()) {
    return false;
  }
  size_t pos = (token[0] == '-') ? 1 : 0;
  if(pos >= token.size()) {
    return false;
  }
  for(; pos < token.size(); ++pos) {
    if(!std::isdigit(static_cast<unsigned char>(token[pos]))) {
      return false;
    }
  }
  return true;
}

bool IsFloatToken(const std::string &token) {
  if(token.empty()) {
    return false;
  }
  char *end = nullptr;
  (void)std::strtod(token.c_str(), &end);
  if(end == token.c_str() || *end != '\0') {
    return false;
  }
  // Keep behavior close to lex/yacc: classify float only if token includes
  // decimal point or scientific notation.
  return (token.find('.') != std::string::npos) ||
         (token.find('e') != std::string::npos) ||
         (token.find('E') != std::string::npos);
}

std::string Trim(const std::string &value) {
  size_t first = 0;
  while(first < value.size() && std::isspace(static_cast<unsigned char>(value[first]))) {
    ++first;
  }
  size_t last = value.size();
  while(last > first && std::isspace(static_cast<unsigned char>(value[last - 1]))) {
    --last;
  }
  return value.substr(first, last - first);
}

}  // namespace

bool ParseConfigContent(const std::string &content, Configuration *config, unsigned int base_lineno) {
  std::string statement;
  statement.reserve(128);
  unsigned int lineno = base_lineno;
  bool in_comment = false;

  auto flush_statement = [&](unsigned int stmt_lineno) -> bool {
    const std::string trimmed = Trim(statement);
    statement.clear();
    if(trimmed.empty()) {
      return true;
    }

    const size_t equal_pos = trimmed.find('=');
    if(equal_pos == std::string::npos) {
      config->ParseError("Expected '=' in assignment statement", stmt_lineno);
      return false;
    }

    const std::string field = Trim(trimmed.substr(0, equal_pos));
    const std::string value = Trim(trimmed.substr(equal_pos + 1));

    if(field.empty() || value.empty()) {
      config->ParseError("Invalid assignment statement", stmt_lineno);
      return false;
    }

    if(IsIntegerToken(value)) {
      config->Assign(field, std::atoi(value.c_str()));
      return true;
    }
    if(IsFloatToken(value)) {
      config->Assign(field, std::atof(value.c_str()));
      return true;
    }
    config->Assign(field, value);
    return true;
  };

  unsigned int stmt_lineno = lineno;
  for(size_t i = 0; i < content.size(); ++i) {
    const char ch = content[i];
    if(ch == '\n') {
      ++lineno;
      in_comment = false;
      if(statement.empty()) {
        stmt_lineno = lineno;
      }
      continue;
    }

    if(in_comment) {
      continue;
    }
    if(ch == '/' && (i + 1) < content.size() && content[i + 1] == '/') {
      in_comment = true;
      ++i;
      continue;
    }

    if(ch == ';') {
      if(!flush_statement(stmt_lineno)) {
        return false;
      }
      stmt_lineno = lineno;
      continue;
    }

    if(statement.empty() && !std::isspace(static_cast<unsigned char>(ch))) {
      stmt_lineno = lineno;
    }
    statement.push_back(ch);
  }

  if(!Trim(statement).empty()) {
    if(!flush_statement(stmt_lineno)) {
      return false;
    }
  }

  return true;
}
