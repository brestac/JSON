#pragma once

#ifdef ARDUINO
  #define PRINT_FUNC Serial.printf
#else
  #define PRINT_FUNC printf
#endif

#if JSON_DEBUG_LEVEL == 1
#define JSON_DEBUG_INFO(format, ...) PRINT_FUNC(format, ##__VA_ARGS__)
#else
#define JSON_DEBUG_INFO(format, ...)
#endif

#if JSON_DEBUG_LEVEL == 2 || JSON_DEBUG_LEVEL == 1
#define JSON_DEBUG_WARNING(format, ...)\
PRINT_FUNC("\x1b[33m" format "\x1b[0m", ##__VA_ARGS__)
#else
#define JSON_DEBUG_WARNING(format, ...)
#endif

#if JSON_DEBUG_LEVEL == 3 || JSON_DEBUG_LEVEL == 2 || JSON_DEBUG_LEVEL == 1
#define JSON_DEBUG_ERROR(format, ...)\
PRINT_FUNC("\x1b[31m" format "\x1b[0m", ##__VA_ARGS__)
#else
#define JSON_DEBUG_ERROR(format, ...)
#endif

// Macro pour créer les paires
#define PAIR(x) #x, x
#define PAIR_IDX(idx, x) #x "[" #idx "]", x

#define MACRO_1(a) PAIR(a)
#define MACRO_2(a,b) PAIR(a), PAIR(b)
#define MACRO_3(a,b,c) PAIR(a), PAIR(b), PAIR(c)
#define MACRO_4(a,b,c,d) PAIR(a), PAIR(b), PAIR(c), PAIR(d)
#define MACRO_5(a,b,c,d,e) PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e)
#define MACRO_6(a,b,c,d,e,f) PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f)
#define MACRO_7(a,b,c,d,e,f,g) PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g)
#define MACRO_8(a,b,c,d,e,f,g,h) PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h)
#define MACRO_9(a,b,c,d,e,f,g,h,i) PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h), PAIR(i)
#define MACRO_10(a,b,c,d,e,f,g,h,i,j) PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h), PAIR(i), PAIR(j)
#define MACRO_11(a,b,c,d,e,f,g,h,i,j,k) PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h), PAIR(i), PAIR(j), PAIR(k)
#define MACRO_12(a,b,c,d,e,f,g,h,i,j,k,l) PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h), PAIR(i), PAIR(j), PAIR(k), PAIR(l)

#define GET_MACRO(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,NAME,...) NAME

#define MACRO(...) \
    GET_MACRO(__VA_ARGS__, MACRO_12, MACRO_11, MACRO_10, MACRO_9, MACRO_8, MACRO_7, MACRO_6, MACRO_5, \
              MACRO_4, MACRO_3, MACRO_2, MACRO_1)(__VA_ARGS__)

#define TO_JSON(a,b,...) JSON::print(a, b, MACRO(__VA_ARGS__))
#define FROM_JSON(a,b,...) JSON::parse(a, b, MACRO(__VA_ARGS__))

#define TO_JSON_FROM_JSON(...)\
using JSONData::fromJSON;\
using JSONData::toJSON;\
JSON::ParseResult fromJSON(JSONStream stream) override {\
  return JSON::parse(this->updated, stream, MACRO(__VA_ARGS__));\
}\
size_t toJSON(JSONStream stream, bool updates = true) override {\
  size_t mask = updates ? this->updated : 0;\
  return JSON::print(mask, stream, MACRO(__VA_ARGS__));\
}
