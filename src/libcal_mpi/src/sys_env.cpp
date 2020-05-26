/*
   Copyright (c) 2015-2018 by the parties listed in the AUTHORS file.
   All rights reserved.  Use of this source code is governed by
   a BSD-style license that can be found in the LICENSE file.
 */

#include <sys_env.hpp>

#include <cstring>

extern "C" {
#include <signal.h>
}

#ifdef _OPENMP
# include <omp.h>
#endif // ifdef _OPENMP

// These variables are autogenerated and compiled
// into the library by the version.cmake script
extern "C" {
extern const char * GIT_VERSION;
extern const char * RELEASE_VERSION;
}

// These are not in the original POSIX.1-1990 standard so we are defining
// them in case the OS hasn't.
// POSIX-1.2001
#ifndef SIGTRAP
# define SIGTRAP 5
#endif // ifndef SIGTRAP

// Not specified in POSIX.1-2001, but nevertheless appears on most other
// UNIX systems, where its default action is typically to terminate the
// process with a core dump.
#ifndef SIGEMT
# define SIGEMT 7
#endif // ifndef SIGEMT

// POSIX-1.2001
#ifndef SIGURG
# define SIGURG 16
#endif // ifndef SIGURG

// POSIX-1.2001
#ifndef SIGXCPU
# define SIGXCPU 24
#endif // ifndef SIGXCPU

// POSIX-1.2001
#ifndef SIGXFSZ
# define SIGXFSZ 25
#endif // ifndef SIGXFSZ

// POSIX-1.2001
#ifndef SIGVTALRM
# define SIGVTALRM 26
#endif // ifndef SIGVTALRM

// POSIX-1.2001
#ifndef SIGPROF
# define SIGPROF 27
#endif // ifndef SIGPROF

// POSIX-1.2001
#ifndef SIGINFO
# define SIGINFO 29
#endif // ifndef SIGINFO


cal::Environment::Environment() {
    // Check debug log-level
    char * envval = ::getenv("CAL_LOGLEVEL");

    if (envval == NULL) {
        loglvl_ = std::string("INFO");
    } else {
        loglvl_ = std::string(envval);
    }

    // Enable signals if desired
    signals_avail_.clear();
    signals_avail_.push_back("SIGHUP");
    signals_value_["SIGHUP"] = SIGHUP;
    signals_avail_.push_back("SIGINT");
    signals_value_["SIGINT"] = SIGINT;
    signals_avail_.push_back("SIGQUIT");
    signals_value_["SIGQUIT"] = SIGQUIT;
    signals_avail_.push_back("SIGILL");
    signals_value_["SIGILL"] = SIGILL;
    signals_avail_.push_back("SIGTRAP");
    signals_value_["SIGTRAP"] = SIGTRAP;
    signals_avail_.push_back("SIGABRT");
    signals_value_["SIGABRT"] = SIGABRT;
    signals_avail_.push_back("SIGEMT");
    signals_value_["SIGEMT"] = SIGEMT;
    signals_avail_.push_back("SIGFPE");
    signals_value_["SIGFPE"] = SIGFPE;

    // Should never try to trap SIGKILL...
    // signals_avail_.push_back("SIGKILL");
    signals_avail_.push_back("SIGBUS");
    signals_value_["SIGBUS"] = SIGBUS;
    signals_avail_.push_back("SIGSEGV");
    signals_value_["SIGSEGV"] = SIGSEGV;
    signals_avail_.push_back("SIGSYS");
    signals_value_["SIGSYS"] = SIGSYS;
    signals_avail_.push_back("SIGPIPE");
    signals_value_["SIGPIPE"] = SIGPIPE;
    signals_avail_.push_back("SIGALRM");
    signals_value_["SIGALRM"] = SIGALRM;
    signals_avail_.push_back("SIGTERM");
    signals_value_["SIGTERM"] = SIGTERM;
    signals_avail_.push_back("SIGURG");
    signals_value_["SIGURG"] = SIGURG;
    signals_avail_.push_back("SIGTSTP");
    signals_value_["SIGTSTP"] = SIGTSTP;
    signals_avail_.push_back("SIGXCPU");
    signals_value_["SIGXCPU"] = SIGXCPU;
    signals_avail_.push_back("SIGXFSZ");
    signals_value_["SIGXFSZ"] = SIGXFSZ;
    signals_avail_.push_back("SIGVTALRM");
    signals_value_["SIGVTALRM"] = SIGVTALRM;
    signals_avail_.push_back("SIGPIPE");
    signals_value_["SIGPIPE"] = SIGPIPE;

    signals_enabled_.clear();
    for (auto const & sig : signals_avail_) {
        signals_enabled_[sig] = false;
    }

    envval = ::getenv("CAL_SIGNALS");
    if (envval != NULL) {
        if (strncmp(envval, "ALL", 3) == 0) {
            // Enable everything
            for (auto const & sig : signals_avail_) {
                signals_enabled_[sig] = true;
            }
        } else {
            // Split comma-separated list of signals to enable
        }
    }

    // See if we should enable function timers.
    func_timers_ = false;
    envval = ::getenv("CAL_FUNCTIME");
    if (envval != NULL) {
        func_timers_ = true;
    }

    // OpenMP
    max_threads_ = 1;
    #ifdef _OPENMP
    max_threads_ = omp_get_max_threads();
    #endif // ifdef _OPENMP

    // Was cal configured to use MPI?  We put this setting here in the
    // non-MPI library so that we can always access it before trying load
    // the MPI library.
    use_mpi_ = true;

    have_mpi_ = false;
    #ifdef HAVE_MPI

    // Build system found MPI
    have_mpi_ = true;
    #endif // ifdef HAVE_MPI

    // See if the user explicitly disabled MPI in the runtime environment.
    bool disabled_mpi_ = false;
    envval = ::getenv("CAL_MPI_DISABLE");
    if (envval != NULL) {
        disabled_mpi_ = true;
    }

    // Handle special case of running on a NERSC login node, where MPI is
    // used for compilation, but cannot be used at runtime.
    envval = ::getenv("NERSC_HOST");
    if (envval == NULL) {
        at_nersc_ = false;
    } else {
        at_nersc_ = true;
    }
    envval = ::getenv("SLURM_JOB_NAME");
    if (envval == NULL) {
        in_slurm_ = false;
    } else {
        in_slurm_ = true;
    }

    if (!have_mpi_) {
        use_mpi_ = false;
    }
    if (disabled_mpi_) {
        use_mpi_ = false;
    }
    if (at_nersc_ && !in_slurm_) {
        // we are on a login node...
        use_mpi_ = false;
    }

    git_version_ = "0.0.0";
    release_version_ = "pre-alpha";

    // prefer the git version if it is set.
    if (git_version_.compare("") == 0) {
        version_ = release_version_;
    } else {
        version_ = git_version_;
    }

    // Set the buffer length for TOD operations that require large intermediate
    // data products.
    envval = ::getenv("CAL_TOD_BUFFER");
    if (envval != NULL) {
        try {
            tod_buffer_length_ = ::atol(envval);
        } catch (...) {
            tod_buffer_length_ = 1048576;
        }
    } else {
        tod_buffer_length_ = 1048576;
    }
}

cal::Environment & cal::Environment::get() {
    static cal::Environment instance;

    return instance;
}

std::string cal::Environment::log_level() const {
    return loglvl_;
}

std::string cal::Environment::version() const {
    return version_;
}

void cal::Environment::set_log_level(char const * level) {
    loglvl_ = std::string(level);
}

bool cal::Environment::use_mpi() const {
    return use_mpi_;
}

bool cal::Environment::function_timers() const {
    return func_timers_;
}

int64_t cal::Environment::tod_buffer_length() const {
    return tod_buffer_length_;
}

int cal::Environment::max_threads() const {
    return max_threads_;
}

int cal::Environment::current_threads() const {
    return cur_threads_;
}

void cal::Environment::set_threads(int nthread) {
    if (nthread > max_threads_) {
        auto & log = cal::Logger::get();
        std::ostringstream o;
        o << "Requested number of threads (" << nthread
          << ") is greater than the maximum (" << max_threads_
          << ") using " << max_threads_ << " instead";
        log.warning(o.str().c_str());
        nthread = max_threads_;
    }
    #ifdef _OPENMP
    omp_set_num_threads(nthread);
    #endif // ifdef _OPENMP
    cur_threads_ = nthread;
    return;
}

std::vector <std::string> cal::Environment::signals() const {
    return signals_avail_;
}

std::vector <std::string> cal::Environment::info() const {
    std::vector <std::string> ret;
    std::ostringstream o;

    o.str("");
    o << "Source code version = " << version_;
    ret.push_back(o.str());

    o.str("");
    o << "Logging level = " << loglvl_;
    ret.push_back(o.str());

    std::vector <std::string> signals;
    for (auto const & sig : signals_avail_) {
        if (signals_enabled_.count(sig) > 0) {
            if (signals_enabled_.at(sig)) {
                signals.push_back(sig);
            }
        }
    }

    o.str("");
    o << "Handling enabled for " << signals.size() << " signals:";
    ret.push_back(o.str());

    for (auto const & sig : signals) {
        o.str("");
        o << "  " << sig;
        ret.push_back(o.str());
    }

    o.str("");
    o << "Max threads = " << max_threads_;
    ret.push_back(o.str());

    o.str("");
    if (have_mpi_) {
        o << "MPI build enabled";
    } else {
        o << "MPI build disabled";
    }
    ret.push_back(o.str());

    o.str("");
    if (use_mpi_) {
        o << "MPI runtime enabled";
    } else {
        o << "MPI runtime disabled";
        if (at_nersc_ && !in_slurm_) {
            ret.push_back(o.str());
            o.str("");
            o << "Cannot use MPI on NERSC login nodes";
        }
    }
    ret.push_back(o.str());

    return ret;
}

void cal::Environment::print() const {
    std::string prefix("cal ENV");

    auto strngs = info();
    for (auto const & str : strngs) {
        fprintf(stdout, "%s: %s\n", prefix.c_str(), str.c_str());
    }

    fflush(stdout);
    return;
}