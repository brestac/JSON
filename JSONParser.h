#pragma once

#include <functional>
#include <string_view>

#include "JSONData.h"
#include "constants.h"
#include "macro.h"
#include "scanner.h"
#include "types.h"
#include "util.h"

using namespace std;
using namespace JSON;

class JSONParser {

public:
  enum ParserState : uint8_t {
    IDLE = 0,
    KEY = 1,
    COLON = 2,
    VALUE = 3,
    COMMA = 4,
    END = 5,
    ERROR = 6,
    STOPPED = 7
  };

  const char *_json_string;
  size_t _json_length;
  const char *_position;
  ParserState _state;
  bool _automask;
  uint32_t keyMask;
  size_t nKeys;
  size_t nParsed;
  size_t nConverted;
  size_t nUpdated;

  JSONParser(const char *json_string, size_t len = MAX_JSON_LENGTH);
  ~JSONParser();

  template <typename... Args> void parse(Args &&...args);

  template <typename PV, typename V>
  ParseValueResult assign_integral_to_integral(PV &parsed_value, V &value);

  template <typename PV, typename V>
  ParseValueResult assign_same_type(PV &parsed_value, V &value);

  template <typename PV, typename V>
  ParseValueResult assign_convertible(PV &parsed_value, V &value);

  template <typename PV, typename V>
  ParseValueResult assign_string_view_to_char_array(PV &parsed_value, V &value);

  template <typename PV, typename V>
  ParseValueResult assign_null_ptr_to_pointer(PV &parsed_value, V &value);

  template <typename V>
  ParseValueResult assign_array_to_array(V &parsed_value, V &value);

  template <typename PV, typename V>
  ParseValueResult assign_not_handled(PV &parsed_value, V &value);

  template <typename PV, typename V>
  ParseValueResult assign_parsed_value_to_value(PV &parsed_value, V &value);

  template <typename V>
  ParseValueResult
  assign_string_view_to_unsigned_array(std::string_view parsed_value, V &value);

  template <typename PV>
  ParseValueResult assign_callback_object(PV parsed_value,
                                          JSONCallbackObject &value);

  template <typename PV, typename V>
  ParseValueResult assign_infinity_to_integral(PV &parsed_value, V &value);

  template <class From, class To> constexpr To clamp_to_max(From v);

  inline ParseValueResult searchValueArgumentForKey(size_t idx,
                                                    JSONKey parsed_key);

  template <typename V, typename... Args>
  ParseValueResult searchValueArgumentForKey(size_t idx, JSONKey parsed_key,
                                             JSONKey arg_key, V &arg_value,
                                             Args &&...args);
  template <typename V> ParseValueResult parse_into_value(V &arg_value);
  ParseValueResult parse_into_array_at_index(JSONCallbackObject cbObject, size_t index);
  template <typename T, size_t N> ParseValueResult parse_into_array_at_index(T (&array)[N], size_t index);
  template <typename T> ParseValueResult parse_into_array_at_index(std::vector<T> &array, size_t index);
  template <typename T, size_t N> ParseValueResult parse_into_array_at_index(std::array<T, N> &array, size_t index);
  
  template <typename V> 
  enable_if_t<container_info<V>::is_container || std::is_same_v<JSONCallbackObject, remove_cvref_t<V>>, ParseValueResult>
  parse_array(V &arg_value);

  size_t parsed_length();
  ParserState get_parser_state();
  bool error() { return _state == ERROR; }
  void setArrayIndex(int index) { _array_index = index; }

private:
  char *_key_start;
  size_t _key_length;
  bool _is_array;
  size_t _array_index;
  uint8_t _nArgs;

  void reset();
  bool parse_key();
  ParseValueResult parse_value(const JSONCallback &callback);
  template <class... Args>
  enable_if_t<args_are_pairs<Args...>, ParseValueResult>
  parse_value(Args &&...args);
  template <typename V> ParseValueResult parse_string(V &arg_value);
  template <typename V> ParseValueResult parse_floating_point(V &arg_value);
  template <typename V> ParseValueResult parse_integer(V &arg_value);
  template <typename V> ParseValueResult parse_numeric(V &arg_value);
  template <typename V> ParseValueResult parse_bool(V &arg_value);
  template <typename V> ParseValueResult parse_null(V &arg_value);
  template <typename V> ParseValueResult parse_nan(V &arg_value);
  template <typename V> ParseValueResult parse_infinity(V &arg_value);
  ParseValueResult parse_array(UnknownValueType arg_value);
  template <typename V> ParseValueResult parse_object(V &arg_value);
  template <typename V> ParseValueResult parse_any(V arg_value);
  ParseValueResult parse_unknown_value();

  size_t get_position();
  bool parse_colon();
  bool parse_comma();
  bool is_object_start();
  bool is_object_end();
  bool is_array_start();
  bool is_array_end();
  bool skip_spaces();
  size_t scan_digits(size_t max_length = 0);
  void set_state(ParserState state);
  void print_state(size_t iteration);
  std::string_view get_state();
};

namespace JSON {
//  using enable_parse = std::enable_if_t<key_value_checker_v<parsed_types,
//  arguments_array_types, arguments_array_array_types, Args...>, int32_t >;

ParseResult parse(JSONStream stream, const JSONCallback &cb,
                  int arrayIndex = -1);

template <typename... Args>
std::enable_if_t<
    key_value_checker_v<parsed_types, arguments_types, arguments_array_types/*,arguments_array_array_types*/, Args...>,
    ParseResult>
parse(uint32_t &mask, JSONStream stream, Args &&...args);

template <typename T>
std::enable_if_t<is_derived_json_data_container_v<T>, ParseResult>
parse(uint32_t &mask, JSONStream stream, T &jsonObjects);

ParseResult parseResult(JSONParser &parser, uint64_t duration) {
  return ParseResult(parser.parsed_length(), parser.nKeys, parser.nParsed,
                     parser.nConverted, parser.nUpdated, parser.error(),
                     duration,
                     parser._state == JSONParser::ParserState::STOPPED);
}

JSON::ParseResult parse(JSONStream stream, const JSONCallback &cb, int arrayIndex) {
  uint64_t start = now();

  JSONParser parser(stream.buffer, stream.size);
  parser.setArrayIndex(arrayIndex);
  parser.parse(cb);

  uint64_t end = now();

  return parseResult(parser, end - start);
}

template <typename... Args>
std::enable_if_t<
    key_value_checker_v<parsed_types, arguments_types, arguments_array_types/*,
                        arguments_array_array_types*/, Args...>,
    JSON::ParseResult>
parse(uint32_t &mask, JSONStream stream, Args &&...args) {
  uint64_t start = now();

  JSONParser parser(stream.buffer, stream.size);
  parser._automask = are_keys(std::forward<Args>(args)...);
  parser.parse(std::forward<Args>(args)...);
  mask = parser.keyMask;

  uint64_t end = now();

  return parseResult(parser, end - start);
}

template <typename T>
std::enable_if_t<is_derived_json_data_container_v<T>, JSON::ParseResult>
parse(uint32_t &mask, JSONStream stream, T &jsonObjects) {
  uint64_t start = now();

  JSONParser parser(stream.buffer, stream.size);
  parser.parse_array(jsonObjects);
  mask = parser.keyMask;

  uint64_t end = now();

  return parseResult(parser, end - start);
}
} // namespace JSON

JSON::ParseResult UnknownValueType::fromJSON(JSONStream stream) {
  uint32_t m = 0;
  return JSON::parse(m, stream);
}

JSON::ParseResult JSONCallbackObject::fromJSON(JSONStream stream) {
  return JSON::parse(stream, callback, array_index);
}

JSONParser::JSONParser(const char *json_string, size_t len)
    : keyMask(0), nKeys(0), nParsed(0), nConverted(0), nUpdated(0) {
  _json_string = json_string;
  _json_length = len;
  _position = _json_string;
  _key_start = nullptr;
  _key_length = 0;
  _state = IDLE;
  _automask = false;
  _is_array = false;
  _array_index = 0;
  _nArgs = 0;

  JSON_DEBUG_INFO("JSONParser %p created json len = %zu\n", this, _json_length);
}

void JSONParser::reset() {
  _position = _json_string;
  _state = IDLE;
  _key_start = nullptr;
  _key_length = 0;
  _automask = false;
  keyMask = 0;
  nParsed = 0;
  _is_array = false;
  _array_index = 0;
  _nArgs = 0;
}

JSONParser::~JSONParser() {
  _json_string = nullptr;
  _position = nullptr;
  _key_start = nullptr;

  _state = IDLE;
  _key_length = 0;
  _automask = false;
  _json_length = 0;
  nParsed = 0;
  keyMask = 0;
  _is_array = false;
  _array_index = 0;
  _nArgs = 0;
  JSON_DEBUG_INFO("JSONParser %p destroyed\n", this);
}

size_t JSONParser::parsed_length() { return _position - _json_string; }

inline ParseValueResult
JSONParser::searchValueArgumentForKey(size_t idx, JSONKey parsed_key) {
  JSON_DEBUG_INFO("Did not find key '%.*s' in parameters\n",
                  (int)parsed_key.length(), parsed_key.data());
  return ParseValueResult::NO_RESULT;
} // Cas de base : aucun argument restant

template <typename V, typename... Args>
ParseValueResult
JSONParser::searchValueArgumentForKey(size_t idx, JSONKey parsed_key,
                                      JSONKey arg_key, V &arg_value,
                                      Args &&...args) {
  ParseValueResult result = ParseValueResult::NO_RESULT;

  if (nKeys >= _nArgs) {
    return result;
  } else if (parsed_key == arg_key) {
    result |= ParseValueResult::KEY_FOUND;
    result |= parse_into_value(arg_value);
    nKeys++;

    if (result.updated()) {
      if (_automask) {
        keyMask |= (1 << idx);
      } else if (arg_key.is_indexed()) {
        keyMask |= (1 << arg_key.index());
      }
      nUpdated++;
    }

    if (result.converted()) {
      nConverted++;
    }

    if (_state == STOPPED) {
      _state = END;
      JSON_DEBUG_INFO("JSON parsing stopped by callback\n");
      return result;
    }

  } else {
    result = searchValueArgumentForKey(idx + 1, parsed_key, std::forward<Args>(args)...);
  }

  return result;
}

template <typename V>
ParseValueResult JSONParser::parse_into_value(V &arg_value) {
  if constexpr (std::is_same_v<remove_cvref_t<V>, JSONCallbackObject>) {
    JSON_DEBUG_INFO(
        "JSONParser::parse_into_value parsing into callback object , stopped=%d\n", arg_value.stop);
    return parse_any(arg_value);
  } else if constexpr (std::is_same_v<V, bool>) {
    return parse_bool(arg_value);
  } else if constexpr (std::is_floating_point_v<V>) {
    return parse_floating_point(arg_value);
  } else if constexpr (std::is_integral_v<V>) {
    return parse_integer(arg_value);
  } else if constexpr (std::is_same_v<V, std::string_view> || is_char_array_v<V>) {
    return parse_string(arg_value);
  } else if constexpr (is_uint_array_v<V>) {
    bool parsed = parse_string(arg_value).parsed() || parse_array(arg_value).parsed();
    return parsed ? ParseValueResult::VALUE_PARSED : ParseValueResult::NO_RESULT;
  } else if constexpr (is_container_v<V>) {
    return parse_array(arg_value);
  } else if constexpr (std::is_same_v<remove_cvref_t<V>, UnknownValueType>) {
    return parse_any(arg_value);
  } else if constexpr (std::is_base_of_v<JSONData, remove_cvref_t<V>>) {
    return parse_object(arg_value);
  } else if constexpr (std::is_pointer_v<V>) {
    ParseValueResult result = ParseValueResult::NO_RESULT;
    result = parse_null(arg_value);

    if constexpr (!std::is_const_v<std::remove_pointer_t<V>> &&
                  !std::is_same_v<V, UnknownValueType>) {
      if (!result.parsed()) {
        if (arg_value != nullptr) {
          JSON_DEBUG_INFO("JSONParser::parse_into_value parsing into deref value of ");
          print_demangled_type("%s ", arg_value);
          result |= parse_into_value(*arg_value);
        } else {
          JSON_DEBUG_WARNING("JSONParser::parse_into_value cannot parse into null pointer\n");
        }
      }
    }

    return result;
  } else {
    print_demangled_type("Could not parse json value into value of type %s\n", arg_value);
#ifdef __EXCEPTIONS
    static_assert(false, "cannot not parse type in file " __FILE__);
#endif
  }

  return ParseValueResult::NO_RESULT;
}

template <typename PV, typename V>
ParseValueResult JSONParser::assign_parsed_value_to_value(PV &parsed_value,
                                                          V &value) {
  ParseValueResult result = ParseValueResult::NO_RESULT;

  if constexpr (std::is_same<PV, V>::value && is_container_from_list<V, arguments_array_types>::value && container_info<V>::dimensions == 1) {
    result |= ParseValueResult::VALUE_CONVERTED | assign_array_to_array(parsed_value, value);
  } else if constexpr (std::is_same<PV, V>::value) {
    result |= ParseValueResult::VALUE_CONVERTED |
              assign_same_type(parsed_value, value);
  } else if constexpr (std::is_convertible_v<PV, V> &&
                       std::is_integral<PV>::value &&
                       std::is_integral<V>::value) {
    result |= ParseValueResult::VALUE_CONVERTED |
              assign_integral_to_integral(parsed_value, value);
  } else if constexpr (std::is_convertible_v<PV, V> &&
                       std::is_floating_point_v<PV>) {
    result |= ParseValueResult::VALUE_CONVERTED |
              assign_convertible(parsed_value, value);
  } else if constexpr (std::is_same<PV, std::string_view>::value && is_char_array_v<V>) {
    result |= ParseValueResult::VALUE_CONVERTED |
              assign_string_view_to_char_array(parsed_value, value);
  } else if constexpr (std::is_same<PV, NullType>::value &&
                       std::is_pointer<V>::value) {
    result |= ParseValueResult::VALUE_CONVERTED |
              assign_null_ptr_to_pointer(parsed_value, value);
  } else if constexpr (std::is_same<PV, NaNType>::value) {
    return result;
  } else if constexpr (std::is_same<PV, InfinityType>::value) {
    result |= ParseValueResult::VALUE_CONVERTED |
      assign_infinity_to_integral(parsed_value, value);
  } else if constexpr (std::is_same<PV, std::string_view>::value && is_uint_array_v<V>) {
    result |= ParseValueResult::VALUE_CONVERTED |
              assign_string_view_to_unsigned_array(parsed_value, value);
  } else if constexpr (std::is_same_v<V, JSONCallbackObject>) {
    result |= ParseValueResult::VALUE_CONVERTED |
              assign_callback_object(parsed_value, value);
  } else if constexpr (std::is_same_v<V, UnknownValueType>) {
    return result;
  } else {
    result |= assign_not_handled(parsed_value, value);
  }

  return result;
}

template <typename PV, typename V>
ParseValueResult JSONParser::assign_same_type(PV &parsed_value, V &value) {
  if (value != parsed_value) {
    value = parsed_value;
    JSON_DEBUG_INFO("Assigned value by copying same type value\n");
    return ParseValueResult::VALUE_UPDATED;
  }

  return ParseValueResult::NO_RESULT;
}

template <typename PV, typename V>
ParseValueResult JSONParser::assign_convertible(PV &parsed_value, V &value) {
  if (value != parsed_value) {
    value = static_cast<V>(parsed_value);
    JSON_DEBUG_INFO("Assigned value by static casting value\n");
    return ParseValueResult::VALUE_UPDATED;
  }

  return ParseValueResult::NO_RESULT;
}

template <typename PV, typename V>
ParseValueResult JSONParser::assign_integral_to_integral(PV &parsed_value,
                                                         V &value) {
  // The type is different, the value may be the same or not.
  // The parsed value type needs to be coerced to the argument type.
  V new_value = clamp_to_max<PV, V>(parsed_value);

  if (value != new_value) {
    value = new_value;
    JSON_DEBUG_INFO("Assigned value by clamping integral\n");
    return ParseValueResult::VALUE_UPDATED;
  }

  return ParseValueResult::NO_RESULT;
}

template <typename PV, typename V>
ParseValueResult JSONParser::assign_infinity_to_integral(PV &parsed_value, V &value) {
  if constexpr (std::is_integral_v<V>) {
    V new_value = std::numeric_limits<V>::max();

    if (value != new_value) {
      value = new_value;
      JSON_DEBUG_INFO("Assigned infinity to integral\n");
      return ParseValueResult::VALUE_UPDATED;
    }
  }

  return ParseValueResult::NO_RESULT;
}

template <typename PV, typename V>
ParseValueResult JSONParser::assign_string_view_to_char_array(PV &parsed_value,
                                                              V &value) {
  if (memcmp(value, parsed_value.data(), parsed_value.length()) == 0) {
    return ParseValueResult::NO_RESULT;
  }

  size_t len = std::min(parsed_value.length(), sizeof(value) - 1);
  std::memcpy(value, parsed_value.data(), len);
  value[len] = '\0';

  JSON_DEBUG_INFO("Assigned string view to char array\n");
  if (len < parsed_value.length()) {
    JSON_DEBUG_WARNING("char array argument was truncated\n");
  }

  return ParseValueResult::VALUE_UPDATED;
}

template <typename PV, typename V>
ParseValueResult JSONParser::assign_null_ptr_to_pointer(PV &parsed_value,
                                                        V &value) {
  if (value != nullptr) {
    value = nullptr;
    JSON_DEBUG_INFO("Assigned null to pointer\n");
    return ParseValueResult::VALUE_UPDATED;
  }

  return ParseValueResult::NO_RESULT;
}

template <typename V>
ParseValueResult JSONParser::assign_array_to_array(V &parsed_value, V &value) {
  return copy_array(value, parsed_value) ? ParseValueResult::VALUE_UPDATED
                                         : ParseValueResult::NO_RESULT;
}

template <typename V>
ParseValueResult
JSONParser::assign_string_view_to_unsigned_array(std::string_view parsed_value,
                                                 V &value) {
  return copy_hex_be_to_h(value, parsed_value.data(), parsed_value.length())
             ? ParseValueResult::VALUE_UPDATED
             : ParseValueResult::NO_RESULT;
}

template <typename PV, typename V>
ParseValueResult JSONParser::assign_not_handled(PV &parsed_value, V &value) {
#if JSON_DEBUG_LEVEL > 0
  print_demangled_types("\x1b[31mCould not assign value from %s to %s\x1b[0m",
                        parsed_value, value);
  PRINT_FUNC("\x1b[31m for key '%.*s'\n\x1b[0m", (int)_key_length, _key_start);
#endif

  return ParseValueResult::NO_RESULT;
}

template <typename PV>
ParseValueResult
JSONParser::assign_callback_object(PV parsed_value,
                                   JSONCallbackObject &cbObject) {
  //print_demangled_type("JSONParser::assign_callback_object %s\n", parsed_value);
  cbObject.run(parsed_value, _array_index);

  if (cbObject.stop) {
    _state = STOPPED;
    JSON_DEBUG_INFO("JSON parsing stopped by callback\n");
  }

  return ParseValueResult::VALUE_UPDATED;
}

template <typename... Args> void JSONParser::parse(Args &&...args) {

  JSON_DEBUG_INFO("JSONParser::parse %zu parameters pairs\n",
                  sizeof...(Args) / 2);

  _nArgs = sizeof...(Args) / 2;
  const char *max_position = _json_string + _json_length;
  size_t iteration = 0;

  while (_position < max_position && iteration <= MAX_ITERATIONS) {
    iteration++;
#if JSON_DEBUG_LEVEL == 1
    print_state(iteration);
#elif JSON_DEBUG_LEVEL == 3
    if (_state == ERROR) print_state(iteration);
#endif
    
    switch (_state) {
    case IDLE:
      skip_spaces();
      if (is_array_start()) {
        JSON_DEBUG_INFO("JSON parsing array\n");
        _is_array = true;
        _state = VALUE;
      } else if (is_object_start()) {
        _state = KEY;
        _position++;
      } else {
        _state = ERROR;
      }
      break;
    case KEY:
      skip_spaces();

      if (is_object_end()) {
        _state = END;
        continue;
      }

      if (parse_key()) {
        set_state(COLON);
      } else {
        _state = ERROR;
      }
      break;
    case COLON:
      if (parse_colon()) {
        set_state(VALUE);
      } else {
        _state = ERROR;
      }
      break;
    case VALUE: {
      skip_spaces();
      if (is_object_end()) {
        _state = END;
        continue;
      }

      ParseValueResult parse_result = parse_value(std::forward<Args>(args)...);

      if (!parse_result.key()) {
        parse_unknown_value();
        set_state(COMMA);
      } else if (parse_result.parsed()) {
        nParsed++;
        set_state(COMMA);
      } else {
        _state = ERROR;
      }

      break;
    }
    case COMMA:
      skip_spaces();
      if (is_object_end()) {
        JSON_DEBUG_INFO("JSON parsing ended\n");
        _state = END;
        continue;
      }
      if (parse_comma()) {
        set_state(KEY);
      } else {
        _state = ERROR;
      }
      break;
    case END:
      _position++;
      JSON_DEBUG_INFO(
          "JSON parsed, total iterations=%zu total keys parsed=%zu\n",
          iteration, nParsed);
      return;
    case ERROR:
      JSON_DEBUG_ERROR("JSON parsing error at position %zu\n", get_position());
      return;
    case STOPPED:
      JSON_DEBUG_INFO("JSON parsing stopped\n");
      return;
    default:
      return;
    }
  }
}

size_t JSONParser::get_position() { return _position - _json_string; }

void JSONParser::set_state(ParserState state) {
  if (_state == END || _state == ERROR || _state == STOPPED)
    return;

  _state = state;

  if (_state == COMMA) {
    _key_start = NULL;
    _key_length = 0;
  }
}

bool JSONParser::is_object_start() {
  return *_position == JSON_START_CHARACTER;
}
bool JSONParser::is_object_end() { return *_position == JSON_END_CHARACTER; }
bool JSONParser::is_array_start() {
  return *_position == JSON_ARRAY_START_CHARACTER;
}
bool JSONParser::is_array_end() {
  return *_position == JSON_ARRAY_END_CHARACTER;
}

bool JSONParser::parse_key() {
  if (!scan_char(_position, '"', true)) {
    _key_start = NULL;
    _key_length = 0;
    JSON_DEBUG_INFO("JSONParser::parse_key no begin quote found\n");
    return false;
  }
  _key_start = (char *)_position;

  if (!scan_ranges(_position, JSON_KEY_CHARACTERS, MAX_KEY_LENGTH, true)) {
    _key_start = NULL;
    _key_length = 0;
    JSON_DEBUG_INFO("JSONParser::parse_key no key found\n");
    return false;
  }

  if (!scan_char(_position, '"', true)) {
    _key_start = NULL;
    _key_length = 0;
    JSON_DEBUG_INFO("JSONParser::parse_key no end quote found\n");
    return false;
  }

  _key_length = _position - _key_start - 1;
  return true;
}

bool JSONParser::parse_colon() {
  skip_spaces();
  if (*_position == JSON_COLON_CHARACTER) {
    _position++;
    return true;
  }

  return false;
}

bool JSONParser::parse_comma() {

  if (scan_char(_position, JSON_COMMA_CHARACTER, true)) {
    return true;
  }

  return false;
}

ParseValueResult JSONParser::parse_value(const JSONCallback &callback) {
  JSON_DEBUG_INFO("JSONParser::parse_value with callback\n");

  JSONKey key(_key_start, _key_length);
  JSONCallbackObject cbObject(callback, key);

  return ParseValueResult::KEY_FOUND | parse_into_value(cbObject);
}

template <class... Args>
enable_if_t<args_are_pairs<Args...>, ParseValueResult>
JSONParser::parse_value(Args &&...args) {
  JSON_DEBUG_INFO("JSONParser::parse_value parameters size %zu\n",
                  sizeof...(Args));

  JSONKey parsed_key(_key_start, _key_length);

  return searchValueArgumentForKey(0, parsed_key, std::forward<Args>(args)...);
}

template <typename V> ParseValueResult JSONParser::parse_string(V &arg_value) {
  JSON_DEBUG_INFO("JSONParser::parse_string\n");
  ParseValueResult result = ParseValueResult::NO_RESULT;

  if (scan_char(_position, JSON_QUOTE_CHARACTER, true)) {
    const char *value_start = _position;

    if (scan_until(_position, JSON_QUOTE_CHARACTER, MAX_VALUE_LENGTH, true)) {
      
      result |= ParseValueResult::VALUE_PARSED;

      std::string_view parsed_value(value_start, _position - value_start);
      _position++;
      result |= assign_parsed_value_to_value(parsed_value, arg_value);
    }
  }
  
  return result;
}

template <typename V> ParseValueResult JSONParser::parse_numeric(V &arg_value) {
  JSON_DEBUG_INFO("JSONParser::parse_numeric\n");
  ParseValueResult result = ParseValueResult::NO_RESULT;

  if constexpr (std::is_same_v<V, JSONCallbackObject> ||
                std::is_same_v<V, UnknownValueType>) {
    bool parsed = parse_floating_point(arg_value).parsed() || parse_integer(arg_value).parsed() || parse_nan(arg_value).parsed() || parse_infinity(arg_value).parsed();
    if (parsed) {
      result |= ParseValueResult::VALUE_PARSED;
    }
  } else if constexpr (std::is_floating_point_v<remove_cvref_t<V>>) {
    result |= parse_floating_point(arg_value);
  } else if constexpr (std::is_integral_v<remove_cvref_t<V>> &&
                       !(std::is_same_v<remove_cvref_t<V>, bool>)) {
    result |= parse_integer(arg_value);
  }

  return result;
}

template <typename V> ParseValueResult JSONParser::parse_integer(V &arg_value) {
  JSON_DEBUG_INFO("JSONParser::parse_integer\n");
  ParseValueResult result = ParseValueResult::NO_RESULT;

  char *end;
  int32_t parsed_value = std::strtol(_position, &end, 10);

  if (end != _position) {
    _position = end;

    result |= ParseValueResult::VALUE_PARSED;
    result |= assign_parsed_value_to_value(parsed_value, arg_value);

    // if there is a dot, it's a floating point number but we allow the parser
    // to continue
    if (scan_char(_position, '.', true)) {
      scan_digits(
          MAX_VALUE_LENGTH); // TO CHECK: did we pass the last digit here ?
    }
  }

  return result;
}

template <typename V>
ParseValueResult JSONParser::parse_floating_point(V &arg_value) {
  JSON_DEBUG_INFO("JSONParser::parse_floating_point\n");

  char *end;
  double parsed_value = std::strtod(_position, &end);

  if (end > _position) {
    _position = end;

    return ParseValueResult::VALUE_PARSED | assign_parsed_value_to_value(parsed_value, arg_value);
  }

  return ParseValueResult::NO_RESULT;
}

template <typename V> ParseValueResult JSONParser::parse_bool(V &arg_value) {
  JSON_DEBUG_INFO("JSONParser::parse_bool\n");
  ParseValueResult result = ParseValueResult::NO_RESULT;

  if (scan_keyword(_position, JSON_FALSE, true)) {
    bool parsed_value = false;
    result |= ParseValueResult::VALUE_PARSED;
    result |= assign_parsed_value_to_value(parsed_value, arg_value);
  } else if (scan_keyword(_position, JSON_TRUE, true)) {
    bool parsed_value = true;
    result |= ParseValueResult::VALUE_PARSED;
    result |= assign_parsed_value_to_value(parsed_value, arg_value);
  } else {
    return ParseValueResult::NO_RESULT;
  }

  return result;
}

template <typename V> ParseValueResult JSONParser::parse_null(V &arg_value) {
  JSON_DEBUG_INFO("JSONParser::parse_null\n");
  ParseValueResult result = ParseValueResult::NO_RESULT;

  if (scan_keyword(_position, JSON_NULL, true)) {
    NullType parsed_value;
    result |= ParseValueResult::VALUE_PARSED;
    result |= assign_parsed_value_to_value(parsed_value, arg_value);
  }

  return result;
}

template <typename V> ParseValueResult JSONParser::parse_nan(V &arg_value) {
  JSON_DEBUG_INFO("JSONParser::parse_nan\n");
  ParseValueResult result = ParseValueResult::NO_RESULT;

  if (scan_keyword(_position, JSON_NAN, true)) {
    NullType parsed_value;
    result |= ParseValueResult::VALUE_PARSED;
    result |= assign_parsed_value_to_value(parsed_value, arg_value);
  }

  return result;
}

template <typename V> ParseValueResult JSONParser::parse_infinity(V &arg_value) {
  JSON_DEBUG_INFO("JSONParser::parse_infinity\n");
  ParseValueResult result = ParseValueResult::NO_RESULT;

  if (scan_keyword(_position, JSON_INFINITY, true)) {
    InfinityType parsed_value;
    result |= ParseValueResult::VALUE_PARSED;
    result |= assign_parsed_value_to_value(parsed_value, arg_value);
  }

  return result;
}

ParseValueResult JSONParser::parse_array(UnknownValueType arg_value) {
  std::vector<UnknownValueType> values;

  return parse_array(values);
}

ParseValueResult JSONParser::parse_into_array_at_index(JSONCallbackObject cbObject, size_t index) {
  cbObject.array_index = index;
  return parse_into_value(cbObject);
}

template <typename T, size_t N> ParseValueResult JSONParser::parse_into_array_at_index(T (&array)[N], size_t index) {
  //print_demangled_type("JSONParser::parse_into_array_at_index %s\n", array);
  
  if (index >= N) {
    JSON_DEBUG_WARNING("JSONParser::parse_into_array_at_index index %zu out of bounds\n", index);
    T dummy_value;
    return parse_into_value(dummy_value);
  }
  
  return parse_into_value(array[index]);
}

template <typename T> ParseValueResult JSONParser::parse_into_array_at_index(std::vector<T> &array, size_t index) {
  //print_demangled_type("JSONParser::parse_into_array_at_index %s %zu\n", array, index);
  T value;
  uint8_t result = parse_into_value(value);

  if (result & ParseValueResult::VALUE_PARSED) {
    array.push_back(value);
  }

  return ParseValueResult(result);
}

template <typename T, size_t N>
ParseValueResult JSONParser::parse_into_array_at_index(std::array<T, N> &array, size_t index) {
  //print_demangled_type("JSONParser::parse_into_array_at_index %s %zu\n", array, index);
  if (index >= N) {
    JSON_DEBUG_WARNING("JSONParser::parse_into_array_at_index index %zu out of bounds\n", index);
    T dummy_value;
    return parse_into_value(dummy_value);
  }

  return parse_into_value(array[index]);
}

template <typename T>
size_t max_size(T value) {
  if constexpr (is_container_v<T>) {
    return container_info<T>::extent;
  } else if constexpr (std::is_same_v<T, JSONCallbackObject>) {
    return MAX_ARRAY_LENGTH;
  } else {
    return 0;
  }
}

template <typename V>
enable_if_t<container_info<V>::is_container || std::is_same_v<JSONCallbackObject, remove_cvref_t<V>>, ParseValueResult>
JSONParser::parse_array(V &arg_value) {
  //print_demangled_type("JSONParser::parse_array %s\n", arg_value);
  ParseValueResult result = ParseValueResult::NO_RESULT;

  if (is_array_start()) {
    _position++;
  } else if (is_array_end()) {
    _position++;
    return ParseValueResult::VALUE_PARSED;
  } else {
    return ParseValueResult::NO_RESULT;
  }

  //using ValueType = std::remove_extent_t<V>;
  size_t max_len = max_size(arg_value);
  //JSON_DEBUG_INFO("JSONParser::parse_array max_len=%zu\n", max_len);
  
  size_t i = 0;

  while (/*!is_array_end() &&*/ i < max_len) {
    
    skip_spaces();

    result |= parse_into_array_at_index(arg_value, i);

    if (_state == STOPPED) {
      JSON_DEBUG_INFO("JSONParser::parse_array parsing stopped\n");
      return ParseValueResult::VALUE_PARSED;
    }

    if (!result.parsed()) {
      //JSON_DEBUG_INFO("JSONParser::parse_array parsing error at index %zu\n", i);
#if JSON_DEBUG_LEVEL > 0
      print_demangled_type("JSONParser::parse_array parsing error for type %s at index %zu\n",arg_value, i);
#endif
      return ParseValueResult::NO_RESULT;
    }

    skip_spaces();

    if (!scan_char(_position, JSON_COMMA_CHARACTER, true))
      break;

    i++;
  }
  
  if (!scan_char(_position, JSON_ARRAY_END_CHARACTER, true)) {
#if JSON_DEBUG_LEVEL > 0
    print_state(0UL);
    JSON_DEBUG_ERROR("JSONParser::parse_array no end bracket found at index %zu position %zu ARRAY OVERFLOW=%d\n", i, get_position(), i == MAX_ARRAY_LENGTH);
#endif
    return ParseValueResult::NO_RESULT;
  }

  skip_spaces();

  // result |= assign_parsed_value_to_value(values, arg_value);

  return result | ParseValueResult::VALUE_PARSED;;
}

template <typename V> ParseValueResult JSONParser::parse_object(V &arg_value) {
  JSON_DEBUG_INFO("JSONParser::parse_object\n");
  ParseValueResult result = ParseValueResult::NO_RESULT;

  if (!is_object_start()) {
    return ParseValueResult::NO_RESULT;
  }

  JSONStream stream((char *)_position, _json_length - get_position());
  JSON::ParseResult r = arg_value.fromJSON(stream);

  if (r.error == true) {
    print_demangled_type("JSONParser::parse_object error parsing %s\n", arg_value);
    _state = ERROR;
    return ParseValueResult::NO_RESULT;
  }
  
  if constexpr(std::is_same_v<remove_cvref_t<V>, JSONCallbackObject>) {
    if (r.stopped) {
      JSON_DEBUG_INFO("JSONParser::parse_object parsing stopped\n");
      _state = STOPPED;
    }
  }

#if JSON_DEBUG_LEVEL > 0
  JSON_DEBUG_INFO("JSONParser::parse_object result: ");
  r.print();
#endif

  // nKeys += r.nKeys;
  // nParsed += r.nParsed;
  // nConverted += r.nConverted;
  // nUpdated += r.nUpdated;
  //_elapsed += r.elapsed;

  result |= ParseValueResult::VALUE_PARSED | ParseValueResult::VALUE_UPDATED | ParseValueResult::VALUE_CONVERTED;

  _position += r.length;

  return result;
}

ParseValueResult JSONParser::parse_unknown_value() {
  JSON_DEBUG_INFO("JSONParser::parse_unknown_value\n");
  UnknownValueType arg_value;
  return parse_any(arg_value);
}

template <typename T>
ParseValueResult JSONParser::parse_any(T arg_value) {
  bool result =
      parse_object(arg_value).parsed() || parse_array(arg_value).parsed() ||
      parse_string(arg_value).parsed() || parse_numeric(arg_value).parsed() ||
      parse_bool(arg_value).parsed() || parse_null(arg_value).parsed();
  
  return result ? ParseValueResult::VALUE_PARSED : ParseValueResult::NO_RESULT;
}

bool JSONParser::skip_spaces() {
  return scan_chars(_position, JSON_SPACE_CHARACTERS, _json_length - get_position(), true);
}

size_t JSONParser::scan_digits(size_t max_length) {
  return scan_range(_position, JSON_DIGIT_CHARACTERS_RANGE, max_length);
}

template <class From, class To> constexpr To JSONParser::clamp_to_max(From v) {
#ifdef __EXCEPTIONS
  static_assert(std::is_integral_v<From>, "From doit être un type entier");
  static_assert(std::is_integral_v<To>, "To doit être un type entier");
#endif
  constexpr auto max_to = std::numeric_limits<To>::max();

  if constexpr (std::is_same_v<From, bool>) {
    JSON_DEBUG_INFO("clamping bool to integral\n");
    return static_cast<To>(v ? 1 : 0);
  } else if constexpr (std::is_same_v<To, bool>) {
    JSON_DEBUG_INFO("clamping integral to bool\n");
    return static_cast<To>(v != 0);
  } else if constexpr (std::is_signed_v<From> && std::is_unsigned_v<To>) {
    // signed → unsigned : on ne garde que les valeurs positives
    JSON_DEBUG_INFO("clamping signed to unsigned\n");
    return static_cast<To>(
        v < 0 ? 0 : (v > static_cast<From>(max_to) ? max_to : v));
  } else if constexpr (std::is_unsigned_v<From> && std::is_unsigned_v<To>) {
    // unsigned → unsigned : on limite à max_to
    JSON_DEBUG_INFO("clamping unsigned to unsigned\n");
    return static_cast<To>(v > static_cast<From>(max_to) ? max_to : v);
  } else if constexpr (std::is_signed_v<From> && std::is_signed_v<To>) {
    JSON_DEBUG_INFO("clamping signed to signed\n");
    // signed → signed : même logique
    return static_cast<To>(v > static_cast<From>(max_to) ? max_to : v);
  } else if constexpr (std::is_unsigned_v<From> && std::is_signed_v<To>) {
    JSON_DEBUG_INFO("clamping unsigned to signed\n");
    // unsigned → signed : on ne garde que les valeurs positives
    return static_cast<To>(v > static_cast<From>(max_to) ? max_to : v);
  } else { // signed → unsigned
    JSON_DEBUG_INFO("clamping signed to unsigned\n");
    if (v < 0) {
      return static_cast<To>(0); // négatif → 0
    }

    return static_cast<To>(v > static_cast<From>(max_to) ? max_to : v);
  }
}

std::string_view JSONParser::get_state() {
  switch (_state) {
  case IDLE:
    return "IDLE";
    break;
  case KEY:
    return "KEY";
    break;
  case COLON:
    return "COLON";
    break;
  case VALUE:
    return "VALUE";
    break;
  case COMMA:
    return "COMMA";
    break;
  case END:
    return "END";
    break;
  case ERROR:
    return "ERROR";
    break;
  case STOPPED:
    return "STOPPED";
    break;
  default:
    return "UNKNOWN";
  }
}

JSONParser::ParserState JSONParser::get_parser_state() { return _state; }

void JSONParser::print_state(size_t iteration) {
  size_t max_length = JSON::DEBUG_COLUMN_WIDTH;
  size_t length = std::min(_json_length, max_length);
  [[maybe_unused]] size_t col_number = get_position() / length;
  [[maybe_unused]] size_t col_pos = get_position() % length;
  [[maybe_unused]] const char *dots = _json_length > max_length ? "..." : "";

  // char json_string[length + 1];
  // strncpy(json_string, _json_string + col_number * length, length);
  // json_string[length] = '\0';
  // str_replace(json_string, length, '\n', ' ');

  PRINT_FUNC("%.*s %s pos=%zu it=%zu, p=%p\n", (int)length, _json_string + col_number * length, dots, get_position(), iteration, this);
  PRINT_FUNC("\x1b[32m%*c%s\x1b[0m\n", (int)(col_pos + 1), '^', get_state().data());
}

