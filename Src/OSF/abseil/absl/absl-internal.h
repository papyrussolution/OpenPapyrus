// ABSL-INTERNAL.H
//
#ifndef ABSL_INTERNAL_H
#define ABSL_INTERNAL_H

#define SLIB_INCLUDE_CPPSTDLIBS
#include <slib.h>
#ifndef _WIN32
	#include <unistd.h>
#else
	#include <io.h>
#endif
#include "absl/base/attributes.h"
#include "absl/base/config.h"
#include "absl/base/optimization.h"
#include "absl/base/port.h"
#include "absl/base/macros.h"
#include "absl/base/dynamic_annotations.h"
#include "absl/meta/type_traits.h"
#include "absl/base/internal/invoke.h"
#include "absl/base/internal/atomic_hook.h"
#include "absl/base/log_severity.h"
#include "absl/base/internal/raw_logging.h"
#include "absl/base/internal/scheduling_mode.h"
#include "absl/base/internal/low_level_scheduling.h"
#include "absl/base/internal/spinlock_wait.h"
#include "absl/base/call_once.h"
#include "absl/algorithm/algorithm.h"
#include "absl/base/internal/throw_delegate.h"
#include "absl/container/internal/inlined_vector.h"
#include "absl/memory/memory.h"
#include "absl/container/inlined_vector.h"
#include "absl/base/casts.h"
#include "absl/base/internal/unaligned_access.h"
#include "absl/base/internal/endian.h"
#include "absl/numeric/internal/bits.h"
#include "absl/numeric/bits.h"
#include "absl/numeric/int128.h"
#include "absl/strings/string_view.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/base/const_init.h"
#include "absl/base/internal/tsan_mutex_interface.h"
#include "absl/base/thread_annotations.h"
#include "absl/base/internal/spinlock.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/strip.h"
#include "absl/time/internal/cctz/include/cctz/civil_time_detail.h"
#include "absl/time/internal/cctz/include/cctz/civil_time.h"
#include "absl/time/civil_time.h"
#include "absl/time/internal/cctz/include/cctz/time_zone.h"
#include "absl/time/time.h"

#undef max
#undef min

#endif // ABSL_INTERNAL_H
