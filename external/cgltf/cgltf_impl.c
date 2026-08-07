/*
 * The single translation unit that instantiates the vendored cgltf headers.
 * cgltf is header-only: every other file includes cgltf.h / cgltf_write.h for
 * the declarations, and exactly one place defines the implementations.
 *
 * Pinned at cgltf v1.15 (commit 360db1a95480fe102ae9c69b27c5d101167ff5ba).
 * Do not edit the vendored headers; re-vendor a newer tag instead, so the
 * pinned version stays a fact about the tree rather than a claim.
 */

#if defined(_MSC_VER)
/* cgltf uses fopen/sscanf; MSVC's "safe" variants are not portable C. */
#  define _CRT_SECURE_NO_WARNINGS 1
#endif

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

/*
 * cgltf.h's declarations are include-guarded but its implementation block is
 * not, and cgltf_write.h includes cgltf.h again. Leaving the macro defined
 * would emit every parser function twice.
 */
#undef CGLTF_IMPLEMENTATION

#define CGLTF_WRITE_IMPLEMENTATION
#include "cgltf_write.h"
