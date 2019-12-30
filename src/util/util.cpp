/*========================================================================\
|: [Filename] util.cpp                                                   :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement some utilities                                   :|
<------------------------------------------------------------------------*/

#include "util.h"
#include <pwd.h>
#include <fstream>
#include <iomanip>
#include <chrono>
using namespace std;

namespace _54ff
{

time_t getCurTime() { return chrono::system_clock::to_time_t(chrono::system_clock::now()); }
const time_t startTime = getCurTime();

void getInfoAboutCurProc()
{
	constexpr size_t bufSize = 256;
	static char buf[bufSize];
	const size_t pageSize = sysconf(_SC_PAGE_SIZE) / 1024;
/*
	#if defined(__GNUG__)
	cout << "Compiler Version  : g++ " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__
	     << " at " << __DATE__ << ", " << __TIME__ << "." << endl;
	#endif
*/
	ifstream stat;
	string tmp; size_t num; time_t time;
	auto readToken = [&stat, &tmp] { return bool(stat >> tmp); };
	auto readNum   = [&stat, &num] { return bool(stat >> num); };
	if(stat.open("/proc/self/stat"); !stat) { cerr << "[Error] Cannot open \"/proc/self/stat\"!" << endl; }
	cout << "Process ID         : " << (readToken() ? tmp.c_str() : "Unknown") << endl;
	cout << "Executable Name    : " << (readToken() ? tmp.substr(1, tmp.size()-2).c_str() : "Unknown") << endl;
	stat.close();
	if(stat.open("/proc/self/statm"); !stat) { cerr << "[Error] Cannot open \"/proc/self/statm\"!" << endl; }
	cout << "Vrtual Memory Size : ";
	if(readNum()) cout << num * pageSize << " kB"; else cout << "Unknown"; cout << endl;
	cout << "Resident Set Size  : ";
	if(readNum()) cout << num * pageSize << " kB"; else cout << "Unknown"; cout << endl;
	stat.close();
	cout << "Start Time         : " << ctime(&startTime);
	cout << "Current Time       : " << ctime(&(time=getCurTime()));
	struct passwd* pw = getpwuid(getuid());
	cout << "User Name          : " << (pw != 0 ? pw->pw_name : "Unknown") << endl;
	cout << "Home Directory     : " << (pw != 0 ? pw->pw_dir : "Unknown") << endl;
	cout << "Current Directory  : " << getcwd(buf, bufSize) << endl;
}

const char* getHomeDir()
{
	if(const char* home = getenv("HOME"))
		return home;
	if(const passwd* pw = getpwuid(getuid()))
		return pw->pw_dir;
	else return 0;
}

const char* replaceHomeDir(const char* str)
{
	if(*str != '\0' && *str == '~' && *(str+1) != '\0' && *(str+1) == '/')
		if(const char* home = getHomeDir())
		{
			const size_t homeLen = strlen(home);
			const size_t strLen  = strlen(str);
			char* tmp = new char[homeLen+strLen];
			memcpy(tmp, home, homeLen);
			strcpy(tmp+homeLen, str+1);
			return tmp;
		}
	return 0;
}

size_t getHashSize(size_t s)
{
	if (s < 8)         return 7;
	if (s < 16)        return 13;
	if (s < 32)        return 31;
	if (s < 64)        return 61;
	if (s < 128)       return 127;
	if (s < 512)       return 509;
	if (s < 2048)      return 1499;
	if (s < 8192)      return 4999;
	if (s < 32768)     return 13999;
	if (s < 131072)    return 59999;
	if (s < 524288)    return 100019;
	if (s < 2097152)   return 300007;
	if (s < 8388608)   return 900001;
	if (s < 33554432)  return 1000003;
	if (s < 134217728) return 3000017;
	if (s < 536870912) return 5000011;
	return 7000003;
}

Timer timer;

void
Timer::printTime()
{
	auto calSec1 = [](clock_t t) { return double(t) / CLOCKS_PER_SEC; };
	auto calSec2 = [](clock_t t) { return double(t) / sysconf(_SC_CLK_TCK); };
	clock_t prevClock = curClock,
	        _tms_utime = clockData.tms_utime,
	        _tms_stime = clockData.tms_stime;
	checkClock();
	cout << "Period Total Time   : " << setprecision(4) << calSec1(curClock - prevClock)             << " seconds" << endl
	     << "Period User Time    : " << setprecision(4) << calSec2(clockData.tms_utime - _tms_utime) << " seconds" << endl
	     << "Period System Time  : " << setprecision(4) << calSec2(clockData.tms_stime - _tms_stime) << " seconds" << endl
	     << "Overall Total Time  : " << setprecision(4) << calSec1(curClock - initClock)             << " seconds" << endl
	     << "Overall User Time   : " << setprecision(4) << calSec2(clockData.tms_utime)              << " seconds" << endl
	     << "Overall System Time : " << setprecision(4) << calSec2(clockData.tms_stime)              << " seconds" << endl;
}

}