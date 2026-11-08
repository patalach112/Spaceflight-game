#pragma once
#if defined(_WIN32)
#  if defined(AEGIS_ENGINE_EXPORTS)
#    define AEGIS_API __declspec(dllexport)
#  else
#    define AEGIS_API __declspec(dllimport)
#  endif
#elif defined(__GNUC__) || defined(__clang__)
#  define AEGIS_API __attribute__((visibility("default")))
#else
#  define AEGIS_API
#endif
