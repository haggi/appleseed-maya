#ifndef MAYATO_DEFS_H
#define MAYATO_DEFS_H

#include "boost/shared_ptr.hpp"
#include "boost/smart_ptr.hpp"
#include "boost/thread.hpp"
#define sharedPtr boost::shared_ptr
#define autoPtr boost::shared_ptr
#define staticPtrCast boost::static_pointer_cast
#define releasePtr(x) x.reset()
#define sleepFor(x) boost::this_thread::sleep(boost::posix_time::milliseconds(x))
#define threadObject boost::thread

#endif
