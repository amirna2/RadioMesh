#pragma once

#include "Options.h"

#ifdef RM_LOG_VERBOSE
#define LOG_ERROR
#define LOG_WARN
#define LOG_INFO
#define LOG_DEBUG
#define LOG_TRACE
#endif

#ifdef RM_LOG_DEBUG
#define LOG_ERROR
#define LOG_WARN
#define LOG_INFO
#define LOG_DEBUG

#endif

#ifdef RM_LOG_INFO
#define LOG_ERROR
#define LOG_WARN
#define LOG_INFO
#endif

#ifdef RM_LOG_CRITICAL
#define LOG_ERROR
#define LOG_WARN
#endif

#ifndef __FILENAME__
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#if defined(ARDUINO)
#define OUTPUT_PORT Serial
#else
#define PORT std::cout
#endif

// https://github.com/esp8266/Arduino/blob/65579d29081cb8501e4d7f786747bf12e7b37da2/cores/esp8266/Print.cpp#L50
[[maybe_unused]] // https://en.cppreference.com/w/cpp/language/attributes/maybe_unused
static size_t
rmPrintf(const char* format, ...)
{
   va_list arg;
   va_start(arg, format);
   char temp[64];
   char* buffer = temp;
   size_t len = vsnprintf(temp, sizeof(temp), format, arg);
   va_end(arg);
   if (len > sizeof(temp) - 1) {
      buffer = new (std::nothrow) char[len + 1];
      if (!buffer) {
         return 0;
      }
      va_start(arg, format);
      vsnprintf(buffer, len + 1, format, arg);
      va_end(arg);
   }
   len = OUTPUT_PORT.write((const uint8_t*)buffer, len);
   if (buffer != temp) {
      delete[] buffer;
   }
   return len;
}

#if defined(LOG_ERROR)
#define logerr(format, ...)                                                                        \
   do {                                                                                            \
      rmPrintf("[E]");                                                                             \
      rmPrintf("[** %s : %d] ", __FILENAME__, __LINE__);                                           \
      rmPrintf(format, ##__VA_ARGS__);                                                             \
   } while (0)

#define logerr_ln(format, ...)                                                                     \
   do {                                                                                            \
      rmPrintf("[E]");                                                                             \
      rmPrintf("[** %s : %d] ", __FILENAME__, __LINE__);                                           \
      rmPrintf(format, ##__VA_ARGS__);                                                             \
      rmPrintf("\n");                                                                              \
   } while (0)
#else
#define logerr(format, ...)                                                                        \
   {                                                                                               \
   }
#define logerr_ln(format, ...)                                                                     \
   {                                                                                               \
   }
#endif // LOG_ERROR

#if defined(LOG_WARN)
#define logwarn(format, ...)                                                                       \
   do {                                                                                            \
      rmPrintf("[W]");                                                                             \
      rmPrintf("[%s : %d] ", __FILENAME__, __LINE__);                                              \
      rmPrintf(format, ##__VA_ARGS__);                                                             \
   } while (0)

#define logwarn_ln(format, ...)                                                                    \
   do {                                                                                            \
      rmPrintf("[W]");                                                                             \
      rmPrintf("[%s : %d] ", __FILENAME__, __LINE__);                                              \
      rmPrintf(format, ##__VA_ARGS__);                                                             \
      rmPrintf("\n");                                                                              \
   } while (0)
#else
#define logwarn(format, ...)                                                                       \
   {                                                                                               \
   }
#define logwarn_ln(format, ...)                                                                    \
   {                                                                                               \
   }
#endif // LOG_WARN

#if defined(LOG_INFO)
#define loginfo(format, ...)                                                                       \
   do {                                                                                            \
      rmPrintf("[I]");                                                                             \
      rmPrintf("[%s] ", __FILENAME__);                                                             \
      rmPrintf(format, ##__VA_ARGS__);                                                             \
   } while (0)

#define loginfo_ln(format, ...)                                                                    \
   do {                                                                                            \
      rmPrintf("[I]");                                                                             \
      rmPrintf("[%s] ", __FILENAME__);                                                             \
      rmPrintf(format, ##__VA_ARGS__);                                                             \
      rmPrintf("\n");                                                                              \
   } while (0)
#else
#define loginfo(format, ...)                                                                       \
   {                                                                                               \
   }
#define loginfo_ln(format, ...)                                                                    \
   {                                                                                               \
   }
#endif // LOG_INFO

#if defined(LOG_DEBUG)
#define logdbg(format, ...)                                                                        \
   do {                                                                                            \
      rmPrintf("[D]");                                                                             \
      rmPrintf("[%s] ", __FILENAME__);                                                             \
      rmPrintf(format, ##__VA_ARGS__);                                                             \
   } while (0)

#define logdbg_ln(format, ...)                                                                     \
   do {                                                                                            \
      rmPrintf("[D]");                                                                             \
      rmPrintf("[%s] ", __FILENAME__);                                                             \
      rmPrintf(format, ##__VA_ARGS__);                                                             \
      rmPrintf("\n");                                                                              \
   } while (0)
#else
#define logdbg(format, ...)                                                                        \
   {                                                                                               \
   }
#define logdbg_ln(format, ...)                                                                     \
   {                                                                                               \
   }
#endif // LOG_DEBUG

#if defined(LOG_TARCE)
#define logtrace(format, ...)                                                                      \
   do {                                                                                            \
      rmPrintf("[T]");                                                                             \
      rmPrintf("[%s] ", __FILENAME__);                                                             \
      rmPrintf(format, ##__VA_ARGS__);                                                             \
   } while (0)

#define logtrace_ln(format, ...)                                                                   \
   do {                                                                                            \
      rmPrintf("[T]");                                                                             \
      rmPrintf("[%s] ", __FILENAME__);                                                             \
      rmPrintf(format, ##__VA_ARGS__);                                                             \
      rmPrintf("\n");                                                                              \
   } while (0)
#else
#define logtrace(format, ...)                                                                      \
   {                                                                                               \
   }
#define logtrace_ln(format, ...)                                                                   \
   {                                                                                               \
   }
#endif // LOG_TRACE
