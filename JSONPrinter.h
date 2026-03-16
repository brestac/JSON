#pragma once

#ifdef __EXCEPTIONS
#include <stdexcept>
#endif
#include <string_view>
#include <type_traits>

#include "macro.h"
#include "types.h"
#include "util.h"
#include "JSONData.h"

template <typename... Args>
void constexpr JSONStream::write(const char *format, Args &&...args) {
  int written = 0;
  if constexpr (sizeof...(Args) == 0) {
    if (buffer == nullptr) {
        PRINT_FUNC("%s", format);
    } else {
      written = snprintf(buffer + position, size - position, "%s", format);
    }
  } else {
    if (buffer == nullptr) {
        PRINT_FUNC(format, std::forward<Args>(args)...);
    } else {
      written = snprintf(buffer + position, size - position, format,
                         std::forward<Args>(args)...);
    }
  }

  if (buffer != nullptr) {
    if (written > 0)
      position += written;

    buffer[position] = '\0';
  }
}

constexpr int get_last_bitwise_mask_index(uint32_t mask);

template <typename T, typename... Args>
void constexpr print_to(T &output, const char *format, Args &&...args);

template <typename Out, typename T, typename... Rest>
void constexpr print_key_value_pair(uint32_t mask, size_t idx, int &last_idx,
                                    Out &output, const char *key, T &value,
                                    Rest &&...rest);

template <typename... Args>
std::enable_if_t<key_value_checker_v<parsed_types, arguments_types, arguments_array_types,/* arguments_array_array_types,*/ Args...>, size_t>
constexpr print_json(uint32_t mask, JSONStream &output, Args &&...args);

template <typename... Args, size_t N>
size_t print_json(uint32_t mask, char (&str)[N], Args &&...args);

template <typename... Args>
size_t print_json(uint32_t mask, std::string_view sv, Args &&...args);

template <typename Out, typename T>
void constexpr print_array_to(Out &output, T &array);

template <typename Out, typename T, size_t N>
void constexpr print_hex_to(Out &output, T (&value)[N]);

template <typename Out, typename T>
void constexpr print_value_to(Out &output, T &value);

namespace JSON {
  template <typename T, typename... Args>
  enable_if_t<key_value_checker_v<parsed_types, arguments_types, arguments_array_types/*, arguments_array_array_types*/, Args...>, size_t>   
  print(uint32_t mask, T output, Args &&...args) {
    return print_json(mask, output, std::forward<Args>(args)...);
  }
} // namespace JSON

template <typename Out>
inline void constexpr print_key_value_pair(uint32_t mask, size_t idx,
                                           int &last_idx, Out &output) {}

template <typename Out, typename T, typename... Rest>
void constexpr print_key_value_pair(uint32_t mask, size_t idx, int &last_idx,
                                    Out &output, const char *key, T &value,
                                    Rest &&...rest) {
  if (mask == 0 || mask & (1 << idx)) {
    output.write("\"%s\":", key);
    print_value_to(output, value);

    if constexpr (sizeof...(Rest) > 0) {
      if (mask == 0 || idx < last_idx) {
        output.write(",");
      }
    }
  }
  // print_demangled_type("print_key_value_pair type: %s\n", value);

  print_key_value_pair(mask, ++idx, last_idx, output,
                       std::forward<Rest>(rest)...);
}

template <typename Out, typename T>
void constexpr print_value_to(Out &output, T &value) {
  if constexpr (is_char_array_v<T>) {
    print_to(output, "\"%s\"", value);
  } else if constexpr (is_uint_array_v<T>) {
    if (JSON::PRINT_BUFFER_AS_HEX) {
      print_hex_to(output, value);
    } else {
      print_array_to(output, value);
    }
  } else if constexpr (is_container_v<T>) {
    print_array_to(output, value);
  } else if constexpr (std::is_same_v<remove_cvref_t<T>, bool>) {
    print_to(output, "%s", value ? "true" : "false");
  } else if constexpr (std::is_floating_point_v<remove_cvref_t<T>>) {
    print_to(output, "%.15g", value);
  // } else if constexpr (std::is_unsigned_v<remove_cvref_t<T>>) {
  //   print_to(output, "%u", value);
  } else if constexpr (std::is_integral_v<remove_cvref_t<T>>) {
    print_to(output, "%lld", (long long)value);
  } else if constexpr (std::is_same_v<remove_cvref_t<T>, std::string_view>) {
    print_to(output, "\"%.*s\"", (int)value.length(), value.data());
   } else if constexpr (std::is_base_of_v<JSONData, remove_cvref_t<T>>) {
     value.toJSON(output);
   } else if constexpr (std::is_pointer_v<T>) {
     if (value == nullptr) {
       print_to(output, "null");
     } else {
       if constexpr (std::is_base_of_v<JSONData, remove_cvref_t<std::remove_pointer_t<T>>>) {
         print_object_pointer_to(output, value);       
       } else {
         print_to(output, "%p", value);
       }
    }
  } else {
#if defined(__EXCEPTIONS) && defined(__GXX_RTTI)
    //static_assert(false, "In file " __FILE__ " line cannot print type " typeid(value).name());
#endif
    print_demangled_type("Cannot print type %s\n", value);
    print_to(output, "?");
  }
}

void print_object_pointer_to(JSONStream &output, void *value) {
#ifdef __EXCEPTIONS
  try {
#endif
    JSONData *jsonData = static_cast<JSONData *>(value);
    jsonData->toJSON(output);
#ifdef __EXCEPTIONS
  }
  catch (const std::exception &e) {
    print_to(output, "null");
  }
#endif
}

template <typename T, typename... Args>
void constexpr print_to(T &output, const char *format, Args &&...args) {
  output.write(format, std::forward<Args>(args)...);
}

template <typename T, size_t N>
size_t array_size(T (&array)[N]) {
  return N;
}

template <typename T>
size_t array_size(std::vector<T> &array) {
  return array.size();
}

template <typename T, size_t N>
size_t array_size(std::array<T, N> &array) {
  return N;
}

template <typename Out, typename T>
void constexpr print_array_to(Out &output, T &array) {
  size_t N = array_size(array);
  output.write("[");
  for (size_t i = 0; i < N; i++) {
    print_value_to(output, array[i]);
    if (i < N - 1) {
      output.write(",");
    }
  }

  output.write("]");
}

template <typename Out, typename T, size_t N>
void constexpr print_hex_to(Out &output, T (&value)[N]) {
  size_t hex_size = sizeof(T) * 2;

  output.write("\"");
  for (size_t i = 0; i < N; i++) {
    output.write("%0.*X", int(hex_size), value[i]);
  }
  output.write("\"");
}

template <typename... Args>
std::enable_if_t<key_value_checker_v<parsed_types, arguments_types, arguments_array_types, /*arguments_array_array_types,*/ Args...>, size_t>
constexpr print_json(uint32_t mask, JSONStream &stream, Args &&...args) {
  static_assert(sizeof...(Args) % 2 == 0,
                "Le nombre d'arguments doit être pair");

  int last_index = get_last_bitwise_mask_index(mask);

  stream.write("{");

  print_key_value_pair(mask, 0, last_index, stream, std::forward<Args>(args)...);

  stream.write("}");

  return stream.position;
}

template <typename... Args, size_t N>
size_t print_json(uint32_t mask, char (&str)[N], Args &&...args) {
  JSONStream output((char *)str, N - 1);
  size_t n = print_json(mask, output, std::forward<Args>(args)...);
  printf("print_json char (&str)[N] %zu\n", n);
  return n;
}

template <typename... Args>
size_t print_json(uint32_t mask, std::string_view sv, Args &&...args) {
  JSONStream output((char *)sv.data(), sv.length() - 1);
  size_t n = print_json(mask, output, std::forward<Args>(args)...);
  printf("print_json std::string_view %zu\n", n);
  return n;
}

constexpr int get_last_bitwise_mask_index(uint32_t mask) {
  if (mask == 0)
    return -1;

  size_t index = 0;
  while (mask >>= 1) {
    index++;
  }

  return index;
}
