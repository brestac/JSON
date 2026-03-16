#pragma once

#ifdef __EXCEPTIONS
#include <stdexcept>
#include <typeinfo>
#endif

#include <array>
#include <cstring> // memcpy
#include <stdint.h>
#include <string_view>
#include <type_traits>
#include <variant>
#include <functional>

#include "constants.h"
#include "macro.h"
#include "scanner.h"
#include "util.h"
#include "JSONData.h"

struct JSONData;
struct JSONStream;
struct UnknownValueType;

// ---------------------------------------------------------------------------
//  équivalent C++17 de std::remove_cvref_t
// ---------------------------------------------------------------------------
template <class T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename... Ts> struct type_list {};

template <class T>
using base_array_type = remove_cvref_t<std::remove_extent_t<remove_cvref_t<T>>>;

template <class T>
constexpr bool is_array_value = std::is_array_v<remove_cvref_t<T>>;

template <class T>
constexpr bool is_unsigned_value = std::is_unsigned_v<base_array_type<T>>;

template <class T>
constexpr bool is_uint_array_v = is_array_value<T> &&is_unsigned_value<T>;

// ---------------------------------------------------------------------------
//   NullType
// ---------------------------------------------------------------------------
struct NullType {};

// ---------------------------------------------------------------------------
//   InfinityType, NaNType
// ---------------------------------------------------------------------------
struct InfinityType {};
struct NaNType {};

// ---------------------------------------------------------------------------
//   ParseValueResult
// ---------------------------------------------------------------------------
struct ParseValueResult {
    enum Flag : uint8_t {
        NO_RESULT = 0,
        KEY_FOUND = 1 << 0,
        VALUE_PARSED = 1 << 1,
        VALUE_CONVERTED = 1 << 2,
        VALUE_UPDATED = 1 << 3
    };

    uint8_t value;

    // Constructeurs
    constexpr ParseValueResult() : value(0) {}
    constexpr ParseValueResult(Flag f) : value(f) {}
    constexpr ParseValueResult(uint8_t v) : value(v) {}

    // Opérateur | (OR)
    constexpr ParseValueResult operator|(const ParseValueResult& other) const {
        return ParseValueResult(value | other.value);
    }

    constexpr ParseValueResult operator|(Flag f) const {
        return ParseValueResult(value | f);
    }

    // Opérateur |= (OR assignment)
    constexpr ParseValueResult& operator|=(const ParseValueResult& other) {
        value |= other.value;
        return *this;
    }

    constexpr ParseValueResult& operator|=(Flag f) {
        value |= f;
        return *this;
    }

    // Opérateur & (AND)
    constexpr ParseValueResult operator&(const ParseValueResult& other) const {
        return ParseValueResult(value & other.value);
    }

    constexpr ParseValueResult operator&(Flag f) const {
        return ParseValueResult(value & f);
    }

    // Opérateur &= (AND assignment)
    constexpr ParseValueResult& operator&=(const ParseValueResult& other) {
        value &= other.value;
        return *this;
    }

    constexpr ParseValueResult& operator&=(Flag f) {
        value &= f;
        return *this;
    }

    // Opérateur ~ (NOT)
    constexpr ParseValueResult operator~() const {
        return ParseValueResult(~value);
    }

    constexpr explicit operator bool() const {
        return value != 0;
    }

    constexpr operator uint8_t() const {
        return value;
    }

    constexpr bool key() const { return (value & KEY_FOUND) != 0; }
    constexpr bool parsed() const { return (value & VALUE_PARSED) != 0; }
    constexpr bool converted() const { return (value & VALUE_CONVERTED) != 0; }
    constexpr bool updated() const { return (value & VALUE_UPDATED) != 0; }
};

// template <typename T, typename... Ts>
// constexpr bool is_any_of = (std::is_same<T, Ts>::value || ...);

// template <typename T, typename... Ts>
// constexpr bool
//     is_convertible_from_one_of = (std::is_convertible<Ts, T>::value || ...);

// template <typename T, typename U>
// constexpr bool assign_null =
//     std::is_same_v<U, std::nullptr_t> &&std::is_pointer<T>::value;

// ---------------------------------------------------------------------------
//   JSONKey
// ---------------------------------------------------------------------------
#ifndef STRING_LENGTH
#define STRING_LENGTH
constexpr size_t str_length(const char *str) {
  size_t len = 0;
  while (str[len] != '\0') {
    len++;
    if (len >= JSON::MAX_JSON_LENGTH) break;
  }
  return len;
}
#endif

std::string_view constexpr get_json_key(const char *raw_key, size_t key_len) {
   const char *key_start = raw_key;
  if (scan_ranges(raw_key, JSON_KEY_CHARACTERS, key_len, true)) {
    return std::string_view(key_start, raw_key - key_start);
  }

  return std::string_view("");
}

int constexpr get_json_key_index(const char *raw_key, size_t key_len) {
  if (scan_ranges(raw_key, JSON_KEY_CHARACTERS, key_len, true)) {
    if (scan_char(raw_key, '[', true)) {
      char *end = nullptr;
      int idx = std::strtol(raw_key, &end, 10);
      if (end != raw_key) {
        raw_key = end;
        if (scan_char(raw_key, ']', true) && idx >= 0) {
          return idx;
        }
      }
    }
  }
  
  return -1;
}

bool constexpr is_key(const char *raw_key) {
  return get_json_key(raw_key, str_length(raw_key)).length() > 0 && get_json_key_index(raw_key, str_length(raw_key)) == -1;
}

inline bool are_keys() {
  return true;
}

template <typename Key, typename Value, typename... Rest>
constexpr bool are_keys(Key key, Value value, Rest... rest) {
  return is_key(key) && are_keys(rest...);
}

constexpr uint32_t hash32(const char *str, size_t len) {
  if (str == nullptr) return 0;
  
  uint32_t hash = 2166136261u;
  for (size_t i = 0; i < len; ++i) {
    hash ^= static_cast<uint32_t>(str[i]);
    hash *= 16777619u;
  }
  return hash;
}

// Opérateur littéral
constexpr uint32_t operator""_hash(const char *str, size_t len) {
  return hash32(str, len);
}

struct JSONKey {
  std::string_view _key;
  int _index;
  uint32_t _hash;
  int _array_index;
  
  constexpr JSONKey() : _key(""), _index(-1), _hash(0), _array_index(-1) {}

  constexpr JSONKey(int index) : _key(""), _index(index), _hash(index), _array_index(-1) {}
  
  constexpr JSONKey(const char *key, size_t len) : _key(get_json_key(key, len)), _index(get_json_key_index(key, len)), _hash(hash32(key, len)), _array_index(-1) {
    //get_key_and_index(key, k, idx);
  }

  constexpr JSONKey(const char *key) : JSONKey(key, str_length(key)) {}

  template <size_t N>
  constexpr JSONKey(const char (&key)[N]) : JSONKey((const char *)key, N - 1) {}
  
  bool operator==(const JSONKey &other) const { return _hash == other._hash; }
    
  constexpr operator uint32_t() const { return _hash; }
  
  constexpr operator std::string_view() const { return _key; }

  size_t length() const { return _key.length(); }

  const char *data() const { return _key.data(); }
 
  int index() const { return _index; }

  int getIndex() const { return _index; }
  
  void setIndex(int index) { _index = index; }

  int getArrayIndex() const { return _array_index; }

  void setArrayIndex(int index) { _array_index = index; }

  bool is_indexed() const { return _index >= 0; }
};


// ---------------------------------------------------------------------------
//   JSONValue
// ---------------------------------------------------------------------------

template <typename TypeList> struct to_variant;

template <typename... Ts> struct to_variant<type_list<Ts...>> {
  using type = std::variant<Ts...>;
};

template <typename TypeList> class AnyValue {
  using to_variant_t = typename to_variant<TypeList>::type;

public:
  template <typename To> AnyValue(To &&value) : data(std::forward<To>(value)) {}

  template <typename To> operator To() const noexcept {

    return std::visit(
        [](auto &&arg) -> To {
          using From = decltype(arg);

          if constexpr (std::is_pointer_v<To>) {
            if constexpr (std::is_same_v<From, NullType>) {
              return nullptr;
            }
            else {
              return To();
            }
          } else if constexpr (std::is_same_v<From, To>) {
            return arg;
          } else if constexpr (std::is_convertible_v<From, To>) {
            return static_cast<To>(arg);
          } else {
#if __GXX_RTTI
            print_demangled_types("BAD CAST when assigning %s to %s\n", arg, To());
#endif
#if __EXCEPTIONS
            throw std::bad_cast();
#endif
          }
        },
        data);
  }

  template <typename T> bool is() const noexcept {
    return std::holds_alternative<T>(data);
  }

  template <typename T> T &get() noexcept { return std::get<T>(data); }

  template <typename T> const T &get() const noexcept {
    return std::get<T>(data);
  }

  // If data is a string_view, and value is an unsigned array, copy the hex
  // string to the array
  template <typename T> std::enable_if_t<is_uint_array_v<T>, void> copyTo(T &value) const noexcept {
      if (is<std::string_view>()) {
        copy_hex_be_to_h(value, get<std::string_view>().data(), get<std::string_view>().length());
      }
  }

private:
  template <typename T> friend class AnyValue;

  template <typename T> AnyValue(const AnyValue<T> &other) : data(other.data) {}

  template <typename T> AnyValue &operator=(const AnyValue<T> &other) noexcept;

public:
  to_variant_t data;
};

// ---------------------------------------------------------------------------
//   UnknownValueType
// ---------------------------------------------------------------------------
struct ParseResult;
struct UnknownValueType : JSONData {

  constexpr UnknownValueType() = default;
  constexpr bool operator==(const UnknownValueType &other) const { return true; }
  constexpr bool operator!=(const UnknownValueType &other) const { return false; }
  JSON::ParseResult fromJSON(JSONStream stream) override;

  size_t toJSON(JSONStream stream, bool updates) override {
    strncpy(stream.buffer, "{}", 2);
    return stream.position;
  }

  using JSONData::fromJSON;
  using JSONData::toJSON;
};

// ---------------------------------------------------------------------------
//   Type checker
// ---------------------------------------------------------------------------


template <class... Args> constexpr bool args_exist = (sizeof...(Args) > 0);

// template <class... Args> constexpr bool no_args = sizeof...(Args) == 0;

template <class... Args>
constexpr bool args_are_pairs = (sizeof...(Args) % 2) == 0;

template <class... Args>
constexpr bool args_are_valid = args_exist<Args...> &&args_are_pairs<Args...>;

// template <typename T>
// constexpr bool is_not_pointer = !std::is_pointer<T>::value;

// template <class T>
// constexpr bool is_char_value = std::is_same_v<base_array_type<T>, char>;

// template <class T>
// constexpr bool is_char_array_v = is_array_value<T> &&is_char_value<T>;

// template <class T>
// constexpr bool is_char_array_array_v = is_array_value<T>
//     &&is_array_value<base_array_type<T>> &&is_char_value<base_array_type<T>>;

// template <class T>
// struct is_vector : std::false_type {};

// template <class T>
// struct is_vector<std::vector<T>> : std::true_type {};

// template <class T>
// constexpr bool is_vector_v = is_vector<T>::value;

// template <class T>
// struct is_std_array : std::false_type {};

// template <class T, size_t N>
// struct is_std_array<std::array<T, N>> : std::true_type {};

// template <class T>
// constexpr bool is_std_array_v = is_std_array<T>::value;

template <typename T, typename From, typename = void>
struct is_static_castable_from : std::false_type {};

template <typename T, typename From>
struct is_static_castable_from<
    T, From, std::void_t<decltype(static_cast<T>(std::declval<From>()))>>
    : std::true_type {};

template <typename T, typename TypeList> struct is_castable_from_any;

template <typename T, typename... Ts>
struct is_castable_from_any<T, type_list<Ts...>>
    : std::disjunction<is_static_castable_from<T, Ts>...> {};

template <typename T, typename TypeList>
struct is_in_type_list : std::false_type {};

template <typename T, typename... Ts>
struct is_in_type_list<T, type_list<Ts...>> 
    : std::disjunction<std::is_same<T, Ts>...> {};

// Type de container
enum class ContainerKind {
    NOT_CONTAINER,
    C_ARRAY,
    STD_ARRAY,
    STD_VECTOR
};

template <typename T>
struct container_info {
    using base_type = T;
    static constexpr size_t dimensions = 0;
    static constexpr ContainerKind kind = ContainerKind::NOT_CONTAINER;
    static constexpr bool is_container = false;
};

// C-array
template <typename T, size_t N>
struct container_info<T[N]> {
    using base_type = typename container_info<T>::base_type;
    static constexpr size_t dimensions = container_info<T>::dimensions + 1;
    static constexpr ContainerKind kind = ContainerKind::C_ARRAY;
    static constexpr size_t extent = N;
    static constexpr bool is_container = true;
};

// std::array
template <typename T, size_t N>
struct container_info<std::array<T, N>> {
    using base_type = typename container_info<T>::base_type;
    static constexpr size_t dimensions = container_info<T>::dimensions + 1;
    static constexpr ContainerKind kind = ContainerKind::STD_ARRAY;
    static constexpr size_t extent = N;
    static constexpr bool is_container = true;
};

// std::vector
template <typename T>
struct container_info<std::vector<T>> {
    using base_type = typename container_info<T>::base_type;
    static constexpr size_t dimensions = container_info<T>::dimensions + 1;
    static constexpr ContainerKind kind = ContainerKind::STD_VECTOR;
    static constexpr size_t extent = _MAX_ARRAY_LENGTH;
    static constexpr bool is_container = true;
};

// ==========================================
// Container from list checker
// ==========================================
template <typename T>
constexpr bool is_container_v = container_info<T>::is_container;

template <typename T, typename TypeList>
struct is_container_from_list : std::false_type {};

// C-array
template <typename T, size_t N, typename TypeList>
struct is_container_from_list<T[N], TypeList> 
    : is_in_type_list<typename container_info<T[N]>::base_type, TypeList> {};

// std::array
template <typename T, size_t N, typename TypeList>
struct is_container_from_list<std::array<T, N>, TypeList> 
    : is_in_type_list<typename container_info<std::array<T, N>>::base_type, TypeList> {};

// std::vector
template <typename T, typename TypeList>
struct is_container_from_list<std::vector<T>, TypeList> 
    : is_in_type_list<typename container_info<std::vector<T>>::base_type, TypeList> {};

template <typename T, typename TypeList>
inline constexpr bool is_container_from_list_v = is_container_from_list<T, TypeList>::value;

// ==========================================
// char array, char array array
// ==========================================

template <typename T>
struct is_char_array : std::false_type {};

template <typename T, size_t N>
struct is_char_array<T[N]> : std::integral_constant<bool, std::is_same_v<T, char>> {};

template <typename T>
inline constexpr bool is_char_array_v = is_char_array<T>::value;

template <typename T>
struct is_char_array_array : std::false_type {};

template <typename T, size_t N, size_t M>
struct is_char_array_array<T[N][M]> : std::integral_constant<bool, std::is_same_v<T, char>> {};

template <typename T>
inline constexpr bool is_char_array_array_v = is_char_array_array<T>::value;

// ==========================================
// JSONData
// ==========================================
template <typename T> struct is_derived_json_data : std::is_base_of<JSONData, remove_cvref_t<T>> {};

template <typename T> struct is_derived_json_data<T*> : is_derived_json_data<T> {};

template <typename T>
inline constexpr bool is_derived_json_data_v = is_derived_json_data<T>::value;

template <typename T>
constexpr bool is_derived_json_data_container_v = container_info<T>::is_container && is_derived_json_data<typename container_info<T>::base_type>::value;

// ==========================================
// Key Value checker
// ==========================================
template <typename T, typename = void>
struct is_convertible_to_indexed_key : std::false_type {};

template <typename T>
struct is_convertible_to_indexed_key<
    T, std::void_t<decltype(JSONIndexedKey(std::declval<T>()))>>
    : std::true_type {};

// template <typename T>
// inline constexpr bool is_convertible_to_indexed_key_v =
//     is_convertible_to_indexed_key<T>::value;

// template <typename CastableTypeList, typename ArrayTypeList>
// struct key_value_checker<CastableTypeList, ArrayTypeList> : std::true_type {};

// template <typename CastableTypeList, typename ArrayTypeList, typename T>
// struct key_value_checker<CastableTypeList, ArrayTypeList, T> : std::false_type {
// };

template <typename CastableTypeList, typename TypeList, typename ArrayTypeList, /*typename ArrayArrayTypeList,*/ typename Value>
struct value_checker
    : std::disjunction<
          is_castable_from_any<remove_cvref_t<Value>, CastableTypeList>,
          is_in_type_list<remove_cvref_t<Value>, TypeList>,
          is_container_from_list<remove_cvref_t<Value>, ArrayTypeList>,
          is_char_array<remove_cvref_t<Value>>,
          is_char_array_array<remove_cvref_t<Value>>,
          is_derived_json_data<remove_cvref_t<Value>>,
          std::is_pointer<remove_cvref_t<Value>>> {};

template <typename Key>
  struct key_checker : std::is_constructible<JSONKey, Key> {};

template <typename CastableTypeList, typename ArrayTypeList, typename... Args>
struct key_value_checker;

template <typename CastableTypeList, typename TypeList, /*typename ArrayTypeList,*/ typename... Args>
struct key_value_checker<CastableTypeList, TypeList, /*ArrayTypeList,*/ type_list<Args...>>
    : std::true_type {};

template <typename CastableTypeList, typename TypeList, typename ArrayTypeList,
          /*typename ArrayArrayTypeList,*/ typename T>
struct key_value_checker<CastableTypeList, TypeList, ArrayTypeList/*, ArrayArrayTypeList*/, T>
    : std::false_type {};

template <typename CastableTypeList, typename TypeList, typename ArrayTypeList/*,
          typename ArrayArrayTypeList*/>
struct key_value_checker<CastableTypeList, TypeList, ArrayTypeList/*, ArrayArrayTypeList*/>
    : std::false_type {};

template <typename CastableTypeList, typename TypeList, typename ArrayTypeList,
          /*typename ArrayArrayTypeList,*/ typename Key, typename Value, class... Rest>
struct key_value_checker<CastableTypeList, TypeList, ArrayTypeList, /*ArrayArrayTypeList,*/ Key,
                         Value, Rest...>
    : std::conjunction<
          key_checker<Key>,
          value_checker<CastableTypeList, TypeList, ArrayTypeList, /*ArrayArrayTypeList,*/ Value>,
          key_value_checker<CastableTypeList, TypeList, ArrayTypeList/*, ArrayArrayTypeList*/,
                            Rest...>> {};

template <typename CastableTypeList, typename TypeList, typename ArrayTypeList,
          /*typename ArrayArrayTypeList,*/ typename... Args>
bool constexpr key_value_checker_v = args_are_pairs<Args...> &&key_value_checker<CastableTypeList, TypeList, ArrayTypeList/*,
                                                ArrayArrayTypeList*/, Args...>::value;

using parsed_types =
    type_list<bool, int, float, double, std::string_view, NullType>;
using JSONValue = AnyValue<parsed_types>;

using JSONCallback = std::function<void(JSONKey, JSONValue, bool &)>;
//using JSONArrayCallback = std::function<void(T &jsonObject, size_t index, bool &)>;

// ---------------------------------------------------------------------------
//   CallBack Object
// ---------------------------------------------------------------------------
struct JSONCallbackObject {
  JSONCallback callback;
  JSONKey key;
  bool stop;
  int array_index;

  JSON::ParseResult fromJSON(JSONStream stream);

  JSONCallbackObject() : callback(nullptr), key(""), stop(false), array_index(-1) {}
  JSONCallbackObject(JSONCallback callback, JSONKey key) : callback(callback), key(key), stop(false), array_index(-1) {
    JSON_DEBUG_INFO("JSONCallbackObject created\n");
  }

  void run(JSONValue value, int arrayIndex) {
    if (callback) {
      key.setArrayIndex(arrayIndex);
      callback(key, value, stop);
      if (stop) {
        JSON_DEBUG_INFO("JSONCallbackObject stopped\n");
      }
    }
  }
};


using arguments_types = type_list<UnknownValueType/*, JSONCallbackObject*/>;
using arguments_array_types = type_list<int8_t, int16_t, int32_t, uint8_t, uint16_t, uint32_t, char, float, UnknownValueType>;
using arguments_array_array_types = type_list<char>;
