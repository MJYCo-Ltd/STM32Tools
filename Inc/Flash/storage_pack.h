#ifndef STM32TOOLS_STORAGE_PACK_H
#define STM32TOOLS_STORAGE_PACK_H

#if defined(_MSC_VER)
#define STORAGE_PACK_BEGIN __pragma(pack(push, 1))
#define STORAGE_PACK_END __pragma(pack(pop))
#else
#define STORAGE_PACK_BEGIN
#define STORAGE_PACK_END
#if defined(__GNUC__) || defined(__clang__)
#define STORAGE_STRUCT_PACKED __attribute__((packed))
#else
#define STORAGE_STRUCT_PACKED
#endif
#endif

#ifndef STORAGE_STRUCT_PACKED
#define STORAGE_STRUCT_PACKED
#endif

#endif /* STM32TOOLS_STORAGE_PACK_H */
