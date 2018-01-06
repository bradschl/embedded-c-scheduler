# ---------------------------------------------------------------- DEFAULT GOAL
.DEFAULT_GOAL           := all


# -------------------------------------------------------------- VERBOSE OUTPUT

ifeq ("$(origin V)", "command line")
  ENABLE_VERBOSE        = $(V)
endif
ifndef ENABLE_VERBOSE
  ENABLE_VERBOSE        = 0
endif

ifeq ($(ENABLE_VERBOSE),1)
  Q                     =
else
  Q                     = @
endif


# ------------------------------------------------------------------ META RULES
# This must be included before any meta rules are used
include deps/metamake/Meta.mk


# --------------------------------------------------------- BUILD ARCHITECTURES
$(call BEGIN_DEFINE_ARCH, host_test, build/host_test)
  PREFIX        :=
  CF            := -O0 -g3 -Wall -Wextra -std=gnu11 -D_GNU_SOURCE=1
$(call END_DEFINE_ARCH)

$(call BEGIN_DEFINE_ARCH, host_c99, build/host_c99)
  PREFIX        :=
  CF            := -O2 -Wall -Wextra -std=c99
$(call END_DEFINE_ARCH)

$(call BEGIN_DEFINE_ARCH, host_c11, build/host_c11)
  PREFIX        :=
  CF            := -O2 -Wall -Wextra -std=c11
$(call END_DEFINE_ARCH)


# ------------------------------------------------------------- BUILD LIBRARIES
sched_SRC        := $(call FIND_SOURCE_IN_DIR, src)
$(call BEGIN_UNIVERSAL_BUILD)
  $(call IMPORT_DEPS,           deps)

  $(call ADD_C_INCLUDE,         src)
  $(call BUILD_SOURCE,          $(sched_SRC))
  $(call MAKE_LIBRARY,          sched)

  $(call EXPORT_SHALLOW_DEPS,   sched)

  # Build for all defined architectures, even if nothing depends on it
  $(call APPEND_ALL_TARGET_VAR)
$(call END_UNIVERSAL_BUILD)


deps_SRC        := $(call FIND_SOURCE_IN_DIR, deps)
$(call BEGIN_UNIVERSAL_BUILD)
  $(call ADD_C_INCLUDE,         deps)
  
  CF := -Wno-unused-variable
  $(call BUILD_SOURCE,          $(deps_SRC))
  $(call MAKE_LIBRARY,          deps)

  $(call EXPORT_SHALLOW_DEPS,   deps)
$(call END_UNIVERSAL_BUILD)


# ----------------------------------------------------------- BUILD EXECUTABLES

sched_ut_SRC     := test/sched_ut.c

$(call BEGIN_ARCH_BUILD,        host_test)
  $(call IMPORT_DEPS,           sched deps)
  $(call BUILD_SOURCE,          $(sched_ut_SRC))

  $(call CC_LINK,               sched_ut)

  # Always build
  $(call APPEND_ALL_TARGET_VAR)
$(call END_ARCH_BUILD)


# ---------------------------------------------------------------- GLOBAL RULES

.PHONY: all
all: METAMAKE_ALL
	@echo "===== All build finished ====="

.PHONY: clean
clean: METAMAKE_CLEAN
	@echo "===== Clean finished ====="

.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all        - Build all top level targets"
	@echo "  clean      - Clean intermediate build files"
