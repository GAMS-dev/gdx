#include "p3process.h"
#include "p3platform.h"
#include <stdexcept>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <cassert>
#include <cstring>
#include "../utils.h"

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#include <TlHelp32.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

	#if defined(__APPLE__)
	#include <sys/sysctl.h>
	#endif
#endif

using namespace std::literals::string_literals;
namespace fs = std::filesystem;

using namespace rtl::p3platform;

namespace rtl::p3process {

static int debug = 0;

class SocketInfo {
public:
  int n;                        // current size of storage
  int nProc;                    // count of (logical) processor stanzas
  int iSock;                    // socket number of current processor
  int numSockets;               // socket count
  int numCores;                 // core count
  int uniform;                  // are the sockets uniformly-sized?
  int sibs, cores;              // if sockets are uniform
  std::vector<int> cpuCores;
  std::vector<int> siblings;
  std::vector<int> procCount;

  explicit SocketInfo(int n_) : n{n_}, nProc{0}, iSock{-1}, numSockets{0}, numCores{-1}, uniform{0},
                       cpuCores(n_,-1), siblings(n_,-1), procCount(n_,0), sibs{}, cores{} {
    if (debug)
      std::cout << "  SocketInfo: core size = " << cpuCores.size()
                << "  n = " << n << "  cpuCores[n-1] " << cpuCores[n-1]
                << '\n';
  }
  // start of a new processor stanza
  void nextProc(int k) {
    if (k < 0)
      throw std::string("processor ID must be positive");
    nProc++;
    iSock = -1;
  } // nextProc
  // physical ID = socket index: should come early in processor stanza
  void setPhysicalID(int k) {
    if (iSock >= 0)
      throw std::string("physical ID already set for this processor");
    if (k < 0)
      throw std::string("physical ID must be positive");
    if (k >= n) {          // must grow
      std::cout << "Growing: n = " << n << "  physical ID = " << k << '\n';
      n = k+1;
      cpuCores.resize(n,-1);
      siblings.resize(n,-1);
      procCount.resize(n,0);
    }
    iSock = k;
    if (0 == procCount[iSock])
      numSockets++;
    procCount[iSock]++;
  } // setPhysicalID
  // setCores: how many cores on the socket containing this processor?
  void setCores(int k) {
    if (debug)
      std::cout << "  setCores(" << k << ") with iSock = " << iSock << '\n';
    if (iSock < 0)
      throw std::string("physical ID must be set prior to cpu cores");
    if (k < 0)
      throw std::string("cpu cores must be positive");
    // std::cout << "    cpuCores curr is " << cpuCores[iSock] << '\n';
    if (cpuCores[iSock] < 0) { // first "cpu cores" line
      cpuCores[iSock] = k;
    }
    else if (cpuCores[iSock] != k)
      throw std::string("conflicting cpu cores values detected");
  } // setCores
  void setSiblings(int k) {
    if (debug)
      std::cout << "  setSiblings(" << k << ") with iSock = " << iSock << '\n';
    if (iSock < 0)
      throw std::string("physical ID must be set prior to cpu cores");
    if (k < 0)
      throw std::string("siblings must be positive");
    // std::cout << "    siblings curr is " << siblings[iSock] << '\n';
    if (siblings[iSock] < 0) { // first "siblings" line
      siblings[iSock] = k;
    }
    else if (siblings[iSock] != k)
      throw std::string("conflicting siblings values detected");
  } // siblings
  // verify: when all the data is in, check it out and compute some totals
  void verify(void) {
    int s = 0;                  // socket count
    int p = 0;                  // processor count

    uniform = 1;
    sibs = cores = 0;
    numCores = 0;
    for (int i = 0;  i < n;  i++) {
      if (procCount[i] <= 0) {
        if ((-1 != siblings[i]) || (-1 != cpuCores[i]))
          throw std::string("incomplete/inconsistent cpuinfo detected");
      }
      else {
        if ((siblings[i] <= 0) || (cpuCores[i] <= 0))
          throw std::string("incomplete/inconsistent cpuinfo detected");
        if (procCount[i] != siblings[i])
          throw std::string("incomplete/inconsistent cpuinfo detected");
        s++;
        p += procCount[i];
        numCores += cpuCores[i];
        if (0 == sibs) {
          sibs = siblings[i];
          cores = cpuCores[i];
        }
        else
          uniform = uniform && (sibs == siblings[i]) && (cores == cpuCores[i]);
      }
    } // loop over sockets
    if (p != nProc)
      throw std::string("incomplete/inconsistent cpuinfo detected");
    if ((s != numSockets) || (s <= 0))
      throw std::string("incomplete/inconsistent cpuinfo detected");
  } // verify
};                              // end SocketInfo

	static int getCPUInfo (	int *nSockets, int *nCores, int *nThreads,
                       		int *coresPerSocket, int *threadsPerCore);

	/* on success, return 0: this implies nSockets, nCores, nThreads are all good */
	/* coresPerSocket and threadsPerCore are -1 if there is any doubt about them */
	static int getCPUInfo (	int *nSockets, int *nCores, int *nThreads,
                       		int *coresPerSocket, int *threadsPerCore) {
#if defined(_WIN32)
		DWORD rc, len, pos, group;
		PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX proc;

		try {
			len = 0;
			LOGICAL_PROCESSOR_RELATIONSHIP relType = RelationAll;
			rc = GetLogicalProcessorInformationEx (relType, NULL, &len);
			if (rc || (ERROR_INSUFFICIENT_BUFFER != GetLastError()))
			return -1;		// failure
			std::unique_ptr<unsigned char[]> uPtr(new unsigned char[len]);
			unsigned char *buf = uPtr.get();
			proc = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX) buf;
			rc = GetLogicalProcessorInformationEx (relType, proc, &len);
			if (debug)
			std::cout << "GLPIEx returns " << rc << '\n';
			if (! rc)
			return -1;		// failure
			*nSockets = 0;
			*nThreads = 0;
			*nCores = 0;
			for (pos = 0;  pos < len;  ) {
			proc = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX) (buf+pos);
			switch (proc->Relationship) {
			case RelationProcessorCore:
			++*nCores;
			if (1 != proc->Processor.GroupCount)
			return -1;		// should always be 1
			{
			KAFFINITY mask = proc->Processor.GroupMask[0].Mask;
			while (0 != mask) {
				*nThreads += mask&1;
				mask >>= 1;
			}
			}
			if (debug) {
			std::cout << "  Found processor core relationship:" << '\n';
			BYTE flags = proc->Processor.Flags;
			std::cout << "    SMT: "
					<< ((0==flags) ? "no" : (LTP_PC_SMT==flags) ? "yes" : "unknown")
					<< '\n';
			}
			break;
			case RelationProcessorPackage:
			++*nSockets;
			if (debug) {
			std::cout << "  Found processor package relationship:" << '\n';
			std::cout << "    GroupCount: " << proc->Processor.GroupCount << '\n';
			}
			break;
			}	// switch
			pos += proc->Size;
			} // for loop over structures
		}				// try
		catch(...) {
			return -1;
		}

		*coresPerSocket = *nCores / *nSockets;
		*threadsPerCore = *nThreads / *nCores;
		if ((( *coresPerSocket * *nSockets) != *nCores)
			|| (( *threadsPerCore * *nCores) != *nThreads)) {
			*coresPerSocket = -1;
			*threadsPerCore = -1;
		}
		return 0;
#elif defined(__linux__)
		SocketInfo sInfo(4);
		try {
			std::ifstream cpuinfo("/proc/cpuinfo");
			std::string s, f;
			size_t found;
			int k;
			while (cpuinfo.good()) {
			(void) std::getline(cpuinfo, s);
			if (debug >= 3)
				std::cout << "line s : " << s << '\n';
			if (0 == s.find("processor")) {
				found = s.find(":");
				f = s.substr(found+1);
				k = std::stoi(f);
				if (debug)
				std::cout << " GOT processor : " << f << "  " << k << '\n';
				sInfo.nextProc(k);
				if (k < 0)
				return -1;
			}
			else if (0 == s.find("physical id")) { // socket ID
				found = s.find(":");
				f = s.substr(found+1);
				k = std::stoi(f);
				if (debug)
				std::cout << " GOT physical id : " << f << "  " << k << '\n';
				sInfo.setPhysicalID(k);
			}
			else if (0 == s.find("siblings")) { // # of siblings on this socket
				found = s.find(":");
				f = s.substr(found+1);
				k = std::stoi(f);
				if (debug)
				std::cout << " GOT siblings : " << f << "  " << k << '\n';
				sInfo.setSiblings(k);
			}
			else if (0 == s.find("cpu cores")) { // # of cpu cores on this socket
				found = s.find(":");
				f = s.substr(found+1);
				k = std::stoi(f);
				if (debug)
				std::cout << " GOT cpu cores : " << f << "  " << k << '\n';
				sInfo.setCores(k);
			}
			else
				continue;
			}
			if (debug) {
			std::cout << "read of /proc/cpuinfo finished: nProc = " << sInfo.nProc << '\n';
			}
			sInfo.verify();
			*nSockets = sInfo.numSockets;
			*nCores = sInfo.numCores;
			*nThreads = sInfo.nProc;
		}
		catch(std::string& msg) {
			if (debug)
			std::cout << "Exception caught: msg = " << msg << '\n';
			return -1;
		}
		catch(...) {
			return -1;
		}

		*coresPerSocket = *nCores / *nSockets;
		*threadsPerCore = *nThreads / *nCores;
		if ((! sInfo.uniform)
			|| (( *coresPerSocket * *nSockets) != *nCores)
			|| (( *threadsPerCore * *nCores) != *nThreads)) {
			*coresPerSocket = -1;
			*threadsPerCore = -1;
		}
		return 0;
#elif defined(__APPLE__)
		int rc, k;
		size_t sz  = sizeof(k);

		rc = sysctlbyname ("hw.packages", &k, &sz, NULL, 0);
		if (rc || (k <= 0))
			return -1;
		*nSockets = k;
		rc = sysctlbyname ("hw.physicalcpu", &k, &sz, NULL, 0);
		if (rc || (k <= 0))
			return -1;
		*nCores = k;
		rc = sysctlbyname ("hw.logicalcpu", &k, &sz, NULL, 0);
		if (rc || (k <= 0))
			return -1;
		*nThreads = k;

		*coresPerSocket = *nCores / *nSockets;
		*threadsPerCore = *nThreads / *nCores;
		if ((( *coresPerSocket * *nSockets) != *nCores)
			|| (( *threadsPerCore * *nCores) != *nThreads)) {
			*coresPerSocket = -1;
			*threadsPerCore = -1;
		}
		return 0;
#else
		*nSockets = *nCores = *nThreads = *coresPerSocket = *threadsPerCore = 1;
		throw std::runtime_error("Unknown platform for getCPUInfo!");
#endif
	}


	const std::string CMD_WIN7 = "C:\\windows\\system32\\cmd.exe";
	const std::string CMD_WINNT = "C:\\winnt\\system32\\cmd.exe";

	int CppCreateProc(const std::string& exeName, const std::string& cmdLine, bool inheritedHandles, int& exeRC) {
#if _WIN32 || _WIN64
		STARTUPINFOA StartupInfo;
		PROCESS_INFORMATION ProcessInformation;
		DWORD exitcode;

		ZeroMemory(&StartupInfo, sizeof(STARTUPINFOA));
		StartupInfo.cb = sizeof(STARTUPINFOA);
		ZeroMemory(&ProcessInformation, sizeof(PROCESS_INFORMATION));

		GetStartupInfoA(&StartupInfo);

		std::vector<char> cmdLineBuf(cmdLine.size()+1);
		strcpy(cmdLineBuf.data(), cmdLine.c_str());

		if (!CreateProcessA(exeName.empty() ? nullptr : exeName.c_str(), cmdLineBuf.data(), nullptr, nullptr, inheritedHandles, 0, nullptr, nullptr, &StartupInfo, &ProcessInformation)) {
			exeRC = 0;
			return GetLastError();
		}
		else {
			WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
			GetExitCodeProcess(ProcessInformation.hProcess, &exitcode);
			CloseHandle(ProcessInformation.hThread);
			CloseHandle(ProcessInformation.hProcess);
			exeRC = static_cast<int>(exitcode);
			if (exeRC == 255) {
				exeRC = 0;
				return 1;
			}
			return 0;
		}
#else
		return 1;
#endif
	}

	int System4Win(const std::string& CmdPtr, bool inheritedHandles, int& ProgRC) {
		const auto cspec = getenv("COMSPEC");
		std::string cs = cspec ? cspec : ""s;
		if (cs.empty()) {
			if (fs::exists(CMD_WIN7)) cs = CMD_WIN7;
			else if (fs::exists(CMD_WINNT)) cs = CMD_WINNT;
			else return 1;
		}
		std::string arg = cs + " /C \""s + CmdPtr + "\"";
		return CppCreateProc(cs, arg, inheritedHandles, ProgRC);
	}

	int System4Unix(const std::string& CmdPtr, int& ProgRC) {
#if defined(_WIN32) || defined(_WIN64)
		return 1;
#else
		const std::string newPtr = CmdPtr.empty() ? "sh"s : CmdPtr;
		int rCode = system(newPtr.c_str());
		if(WIFEXITED(rCode)) { // success!
			ProgRC = WEXITSTATUS(rCode);
			if(ProgRC == 127) { // cmd was not run (e.g. file not found)
				ProgRC = 0;
				return 127;
			}
			else if(ProgRC == 126) { // cmd was not run (e.g. permission denied)
				ProgRC = 0;
				return 126;
			}
			return 0;
		}
		else if(WIFSIGNALED(rCode)) { // child stopped via signal
			ProgRC = WTERMSIG(rCode);
			return 1;
		} else { // shell failed
			ProgRC = 0;
			return 2;
		}
#endif
	}

	int P3SystemP(const std::string& CmdPtr, int& ProgRC)
	{
		int res{};
#if defined(_WIN32)
        res = System4Win(CmdPtr, true, ProgRC);
#else
        res = System4Unix(CmdPtr, ProgRC);
#endif
		return res;
	}

#ifndef _WIN32
	int LibcForkExec(int argc, char *const argv[], int *exeRC) {
		int result = 1;
		int pid, pid2;
		int wstat;
		pid = fork();
		if (pid < 0) {                /* could not fork */
			*exeRC = 0;
			result = 1;
		}
		else if (!pid) {          /* I am the child */
			if (execvp (argv[0], argv) == -1)
				std::cout << "Failure to execute because: " << strerror(errno) << '\n';
			execl("/bin/sh", "/bin/sh", "-c", "exit 255", nullptr);
			/* if we are here, it is trouble */
			/* _exit() is a more immediate termination, less likely to flush stdio */
			_exit (255);                /* -1 tells parent we could not exec */
		}
		else {                        /* I am the parent */
			for ( ; ; ) {
				wstat = 0;
				pid2 = waitpid (pid, &wstat, 0);
				if (-1 == pid2)
					continue;
				if (pid != pid2) {
					*exeRC = 0;
					return 1;    /* failed wait so fail the entire fork/exec */
				}
				else break;
			}
			if (WIFEXITED(wstat)) {     /* normal exit from child */
				if (255 == WEXITSTATUS(wstat)) { /* because it couldn't exec */
					*exeRC = 0;
					result = 1;
				}
				else {
					*exeRC = WEXITSTATUS(wstat);
					result = 0;
				}
			}
			else {                      /* abnormal return from child */
				*exeRC = 0;
				result = 1;
			}
		} /* end parent code */
		return result;
	} /* LibcForkExec */

	/* libcASyncForkExec does a fork/exec to start a process,
	* but it does not wait for it.  Instead it returns the PID.
	* also, it sets up a new process group for the child
	* result: 0 on success, ~0 on failure
	*/
	int libcASyncForkExec (int argc, char *const argv[], int *pid)
	{
		int result = 1;  /* failure */
		int lPid;        /* local pid, typed appropriately */

		*pid = ~0;
		lPid = fork();
		if (lPid < 0) {               /* could not fork */
			result = 1;
		}
		else if (0 == lPid) {         /* I am the child */
			(void) setpgid (0,0);       /* make this process a new process group */
			if (execvp (argv[0], argv) == -1)
				std::cout << "Failure to execute because: " << strerror(errno) << '\n';

			/* if we are here, it is trouble */
			execl("/bin/sh", "/bin/sh", "-c", "exit 127", NULL);
			_exit (127);                /* consistent with & usage in bash */
		}
		else {                        /* I am the parent */
			(void) setpgid (lPid,0);     /* make the child its own, new process group */
			/* we call setpgid for both parent and child to avoid a race condition */
			result = 0;
			*pid = (int)lPid;
		}
		return result;
	} /* libcASyncForkExec */



/* unixPidStatus returns:
 *   0: valid process but not a zombie
 *   1: zombie process
 *   2: process does not exist
 *   3: not implemented or other error
 */
int unixPidStatus (int p)
{
  char sbuf[1024];              /* buffer for content of stat file */
  char *tmp, *t2;
  char filename[80];
  int fd, p2;
  ssize_t numRead;
  char state;
  struct stat sb;               /* stat() buffer */

  sprintf (filename, "/proc/%d", p);
  if (-1 == stat(filename, &sb))
    return 2;

  sprintf (filename, "/proc/%d/stat", p);
  fd = open (filename, O_RDONLY, 0);
  if (-1 == fd)
    return 3;
  numRead = read (fd, sbuf, sizeof(sbuf) - 1);
  close (fd);
  if (numRead <= 0)
    return 3;

  /* start of file looks like
   * 2796 (firefox) S  where we have
   *  pid  cmdline  status */
  sbuf[numRead] = '\0';
  numRead = sscanf (sbuf, "%d", &p2);
  if (1 != numRead)
    return 3;
  tmp = strchr(sbuf, '(') + 1;
  t2 = strrchr(sbuf, ')');
  if (NULL == tmp || NULL == t2)
    return 3;
  *t2 = '\0';
  t2 = t2 + 2;                 // skip ") "
  numRead = sscanf (t2, "%c", &state);
  if (1 != numRead)
    return 3;
  // printf ("DEBUG: in isZombie: read %d (%s) %c\n", p2, tmp, state);
  switch (state) {
  case 'D':                     /* uninterruptible sleep */
  case 'R':                     /* running */
  case 'S':                     /* sleeping */
  case 'T':                     /* traced or stopped */
    return 0;                   /* valid, non-zombie process */
    break;
  case 'Z':                     /* zombie */
    return 1;
    break;
  default:
    return 3;
  }
}

/* killProcGroupUnix
 * return:  true if the process is(was) running
 *              and our termination attempt succeeded
 *          false o/w
 */
bool killProcGroupUnix (pid_t p, TKillHow how)
{
  bool result{};
  if (p > 0) {                  /* PIDs are positive */
    int rc = kill   (-p, (TKillHow::soft == how) ? SIGINT: SIGKILL);
    result = (0 == rc);
    /* clean up the zombie.  If this is our child the PID is still valid
     * until we wait on the process
     * remaining issue: will this wait if this was NOT a child process? */
    if (0 == rc) {    /* signal sent successfully */
      for (int i = 0;  i < 2;  i++) {
        rc = unixPidStatus (p);
        if (rc > 1)           /* not running */
          return result;
        else if (rc < 1) {      /* running, not a zombie */
          usleep (20000);     /* 20 millisecs */
          /* printf ("waiting for zombie\n"); */
        }
        else {                /* a zombie */
		  int wstat;
          (void) waitpid (p, &wstat, 0);
          return result;
        }
      } /* sleep loop */
    } /* if signal sent OK */
  }
  return result;
} /* killProcGroupUnix */

#endif // _WIN32

    void uglyGmsUnzipHack(std::list<std::string> &args) {
        // AS: Very ugly hack to circumvent gmsunzip issue in gamslib/dplytest on macOS
        //     Does not like '-d"/some/path"' but it works with '-d "/some/path"'
        //     Solved by finding '-d...' part, removing stuff after -d and putting it as additional item afterwards
        if(utils::sameText(args.front(), "gmsunzip")) {
            std::list<std::string>::iterator it;
            std::string rest{};
            for(it = std::next(std::begin(args)); it != std::end(args); ++it) {
                if(utils::starts_with(*it,"-d"s)) {
                    rest = it->substr(2);
                    *it = "-d";
                    ++it;
                    break;
                }
            }
            if(!rest.empty()) {
                args.insert(it, rest);
            }
        }
    }

	int ForkWithSplitArgs(std::function<int(int, char *const[], int *)> forkExecFunc, const std::string &s, int &ProgRC) {
		std::list<std::string> args = utils::splitWithQuotedItems(s);
        uglyGmsUnzipHack(args);
		int argc = args.size();
		std::vector<char *> argv(argc+1);
		std::vector<std::vector<char>> argBufs(argc);
		int i{};
		// remove single and double quotes from result part
		const std::array<char, 2> quoteChars = {'\'', '\"'};
		for(const std::string &arg : args) {
			bool quoted = std::any_of(quoteChars.begin(), quoteChars.end(), [&arg](char qc) { return arg.front() == qc && arg.back() == qc; });
			argBufs[i].resize(arg.length()+1);
			strcpy(argBufs[i].data(), arg.c_str() + (quoted ? 1 : 0));
			if(quoted) argBufs[i][arg.length()-2] = '\0';
			argv[i] = argBufs[i].data();
			i++;
		}
		// by convention exec expects last argv entry to point to zero
		argv.back() = nullptr;
		return forkExecFunc(argc, argv.data(), &ProgRC);
	}

	int P3ExecP(const std::string& CmdPtr, int& ProgRC)
	{
		int res{ 1 };
#ifdef _WIN32
        res = CppCreateProc("", CmdPtr, true, ProgRC);
#else
        res = ForkWithSplitArgs(LibcForkExec, CmdPtr, ProgRC);
#endif
		return res;
	}

#ifdef _WIN32
	bool killProcessTree(DWORD myprocID) {
		PROCESSENTRY32 pe;
		HANDLE hSnap;
		HANDLE hChildProc, hProc;

		memset(&pe, 0, sizeof(PROCESSENTRY32));
		pe.dwSize = sizeof(PROCESSENTRY32);

		hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (INVALID_HANDLE_VALUE == hSnap) {
			return FALSE;               /* failure */
		}
		if (!Process32First(hSnap, &pe)) {
			CloseHandle(hSnap);        /* clean up the snapshot object */
			return FALSE;               /* failure */
		}

		/* kill the main process */
		hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, myprocID);
		if (hProc) {
			TerminateProcess(hProc, 1);
			CloseHandle(hProc);
		}

		for (; ; ) {
			if (pe.th32ParentProcessID == myprocID) {
				/* Recursion */
				killProcessTree(pe.th32ProcessID);

				hChildProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
				if (hChildProc) {
					TerminateProcess(hChildProc, 0);
					CloseHandle(hChildProc);
				}
			}
			if (!Process32Next(hSnap, &pe))
				break;
		}

		/* kill the main process */
		hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, myprocID);
		if (hProc) {
			TerminateProcess(hProc, 1);
			CloseHandle(hProc);
		}
		return true;
	}
#endif

    static const tCtrlHandler *CtrlHandler;

#ifdef _WIN32
	static BOOL WINAPI P3Handler(DWORD s) {
		if (CTRL_C_EVENT == s) {
			if (CtrlHandler) {
				(*CtrlHandler)();
				return TRUE;
			}
		}
		return FALSE;
	}
#else
    static sigset_t sigSet;
    static struct sigaction newAction, oldAction;
	extern "C" {
        static void (*oldHandler)(int);
        static void p3CtrlCHandler(int sig);
    }
	static void p3CtrlCHandler(int sig)
	{
		if (CtrlHandler) (*CtrlHandler)();
	}
#endif

}