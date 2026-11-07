#include "IExecutor.h"

namespace HomeCompa::Util {

std::atomic<size_t> IExecutor::Task::s_id { 0 };

}
