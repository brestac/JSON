#pragma once

#include <cstring>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstdint>

#ifdef __GXX_RTTI
#include <typeinfo>
#include <memory>
#include <tuple>
#include <cxxabi.h>
#endif
// include for ntohs
#ifdef ARDUINO_ARCH_ESP8266
#include <ESP8266WiFi.h>
#else
#include <arpa/inet.h>
#endif

#include <chrono>

#include "macro.h"
#include "scanner.h"
#include "constants.h"

template <typename T, typename... Args>
void print_demangled_type(const char *format, T &value, Args &&...args);

template <typename... Args>
void print_demangled_types(const char* format, Args&&... args);

#ifdef __GXX_RTTI
char *demangler(const char *name);
template <typename Tuple, size_t... Is>
void printf_impl(const char* format, Tuple& t, std::index_sequence<Is...>);
#endif

template <typename T, size_t N>
constexpr bool copy_bytes_be_to_h(T (&dst)[N], uint8_t *src, size_t src_size);
template <typename T, size_t N>
constexpr bool copy_bytes_be_to_h(T dst, uint8_t (&src)[N]);

template <typename T, size_t N>
constexpr bool copy_hex_be_to_h(T (&dst)[N], const char *src, size_t src_size);

bool get_byte_fromHexString(uint8_t &value, const char *src, size_t src_size);
template <typename T>
bool get_unsigned_integral_fromHexString(T &value, const char *src, size_t src_size);

template<typename T>
constexpr T be_to_h(T value);

uint64_t now();

#ifdef __GXX_RTTI
// Wrapper RAII minimal
struct DemangledName {
    const char* str;
    bool owned;

    explicit DemangledName(const char* name) {
        int status;
        char* d = abi::__cxa_demangle(name, nullptr, nullptr, &status);
        if (status == 0) { str = d; owned = true; }
        else             { str = name; owned = false; }
    }

    ~DemangledName() { 
        if (owned) std::free(const_cast<char*>(str));
        //printf("DemangledName destructor\n");
    }

    DemangledName(const DemangledName&)            = delete;
    DemangledName& operator=(const DemangledName&) = delete;
    DemangledName(DemangledName&& other) 
        : str(other.str), owned(other.owned) {
        other.owned = false;  // Transferer la propriete
    }

    operator const char*() const { return str; }
};

// Fonction helper pour appeler printf avec un tuple de DemangledName
template <typename Tuple, size_t... Is>
void printf_impl(const char* format, Tuple& t, std::index_sequence<Is...>) {
  PRINT_FUNC(format, static_cast<const char*>(std::get<Is>(t))...);
}
#endif

template <typename... Args>
void print_demangled_types(const char* format, Args&&... args) {
#ifdef __GXX_RTTI
    // Tous les DemangledName sont crees et vivent jusqu'a la fin de la fonction
    auto names = std::make_tuple(DemangledName(typeid(args).name())...);
    PRINT_FUNC("\x1b[31m");
    printf_impl(format, names, std::index_sequence_for<Args...>{});
    PRINT_FUNC("\x1b[0m");
#else
  JSON_DEBUG_WARNING("RTTI not enabled");
#endif
}

template <typename T, typename... Args>
void print_demangled_type(const char *format, T &value, Args &&...args) {
#ifdef __GXX_RTTI
  DemangledName demangled(typeid(value).name());
  PRINT_FUNC("\x1b[31m");
  printf(format, static_cast<const char*>(demangled), std::forward<Args>(args)...);
  PRINT_FUNC("\x1b[0m");
#else
  JSON_DEBUG_WARNING("RTTI not enabled");
#endif
}

template <typename T>
void print_demangled_type(T &value) {
  print_demangled_type("%s\n", value);
}

constexpr uint8_t _hex_to_dec(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  } else if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  } else if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  } else {
    return 0;
  }
}

template<typename T>
constexpr T be_to_h(T value) {
  if constexpr (sizeof(T) == 4) {
    return ntohl(value);
  } else if constexpr (sizeof(T) == 2) {
    return ntohs(value);
  } else {
    return value;
  }
}

template <typename T, size_t N>
constexpr bool copy_array(T (&dst)[N], T(&src)[N]) {
  bool modified = false;

  for(size_t i = 0; i < N; i++) {
    T new_value = src[i];

    if (dst[i] != new_value) {
      dst[i] = new_value;
      modified = true;
    }
  }

  return modified;
}

template <typename T, size_t N, size_t M>
constexpr bool copy_array(T (&dst)[N][M], T(&src)[N][M]) {
  bool modified = false;

  for(size_t i = 0; i < N; i++) {
    modified = copy_array(dst[i], src[i]);
  }

  return modified;
}

bool get_byte_fromHexString(uint8_t &value, const char *src, size_t src_size) {
  if (scan_ranges_once(src, JSON_HEX_CHARACTERS, false)) {
    uint8_t high_mid_byte = _hex_to_dec(src[0]);
    src++;
    if (scan_ranges_once(src, JSON_HEX_CHARACTERS, false)) {
      uint8_t low_mid_byte = _hex_to_dec(src[0]);
      value = (high_mid_byte << 4) | low_mid_byte;
      src++;
      return true;
    }
  }

  return false;
}

template <typename T>
bool get_unsigned_integral_fromHexString(T &value, const char *src, size_t src_size) {
  size_t target_length = sizeof(T);

  if (src_size < target_length * 2) {
    return false;
  }

  uint8_t bytes[target_length];
  for (size_t i = 0; i < target_length; i++) {
    if (!get_byte_fromHexString(bytes[i], src + i * 2, src_size - i * 2)) {
      return false;
    }
  }

  value = be_to_h(*(T *)bytes);

  return true;
}

template <typename T, size_t N>
constexpr bool copy_hex_be_to_h(T (&dst)[N], const char *src, size_t src_size) {

  size_t dst_element_size = sizeof(T);
  size_t dst_elements_count = N;
  size_t dst_size = dst_element_size * dst_elements_count;

  if (dst_size == 0 || src_size == 0) {
    return false;
  }

  bool modified = false;

  size_t max_elements_count = std::min(dst_elements_count, src_size / dst_element_size);

  for(size_t i = 0; i < max_elements_count; i++) {
    T new_element_value = 0;
    if (get_unsigned_integral_fromHexString(new_element_value, src + i * dst_element_size * 2, src_size - i * dst_element_size * 2)) {
      if (dst[i] != new_element_value) {
        dst[i] = new_element_value;
        modified = true;
      }
    }
  }

  size_t dst_final_size = max_elements_count * dst_element_size;

  if (dst_final_size > src_size) {
    memset((uint8_t *)dst + dst_final_size, 0, dst_final_size - src_size);
    modified = true;
  }

  return modified;
}

void print_bitwise_mask(size_t mask, size_t count) {

  PRINT_FUNC("mask: ");

  for (int i = 0; i < count; i++) {
    if (mask & (1 << i)) {
      PRINT_FUNC("1 ");
    } else {
      PRINT_FUNC("0 ");
    }
  }
}

uint64_t now() {
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(
             now.time_since_epoch())
      .count();
}

void str_replace(char *str, size_t size, char old_char, char new_char) {
  for (size_t i = 0; i < size; i++) {
    if (str[i] == old_char) {
      str[i] = new_char;
    }
  }
}

