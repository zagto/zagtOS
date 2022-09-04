#undef  LIB_SPEC
#define LIB_SPEC "-lc"

#undef STARTFILE_SPEC
#define STARTFILE_SPEC "crt1.o%s crti.o%s crtbegin.o%s"

#undef  ENDFILE_SPEC
#define ENDFILE_SPEC "crtend.o%s crtn.o%s"

#undef  OPTION_MUSL
#define OPTION_MUSL 1

#undef  CC1_SPEC
#define CC1_SPEC ""

#ifndef TARGET_SUB_OS_CPP_BUILTINS
#define TARGET_SUB_OS_CPP_BUILTINS()
#endif

#undef  TARGET_OS_CPP_BUILTINS
#define TARGET_OS_CPP_BUILTINS() \
    do { \
      builtin_define ("__zagtos__");      \
      builtin_define ("__unix__");        \
      builtin_assert ("system=zagtos");   \
      builtin_assert ("system=unix");     \
      builtin_assert ("system=posix");    \
      /* GNU libstdc++ requires this */   \
      if (c_dialect_cxx ())               \
          builtin_define ("_GNU_SOURCE"); \
    } \
    while (0);

#undef STANDARD_STARTFILE_PREFIX
#define STANDARD_STARTFILE_PREFIX "/lib/"

#undef LINK_SPEC
#define LINK_SPEC "-z max-page-size=0x1000"
