#pragma once 
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <sdkddkver.h>

#include <algorithm>
#include <queue>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>

#pragma warning(disable:4315)

#include <WinSock2.h>
#include <MSWSock.h>

#include "Log.h"
#include "Mutex.h"