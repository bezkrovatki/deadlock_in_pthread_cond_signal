# Summary
This is simple test application to reproduce the deadlock in pthread_cond_singal sporadically observed in the project I'm working on. 
It was the boost::asio::detail::conditionally_enabled_event where the problem originally manifested itself in the project, so the test tries to mimic the boost::asio in working with the condvar.

# Symptoms
In the presence of a high contention on a condition variable all signalling threads (producers) eventually get blocked on a futex call inside pthread_cond_signal. An attempt to take the process dump with a debugger or Google Breakpad revives the paralyzed process and the producers threads resume spontaneously without any other internal/external interference in the system. Individual producer threads can be also woken up by sending asynchronous signal (like SIGSTOP/SIGCONT), however the process returns to the paralyzed state very quickly. The problem is reproduced regularly but it may take from several minutes to several hours to observe it.

# The deadlock details
* backtraces in [odroid-n2.stacks](odroid-n2.stacks)
* compact view of the call graph in [odroid-n2.pdf](odroid-n2.pdf)

# The test description
The test allocates threads of 2 types: producer and consumer. Producer changes the shared state and signals the condvar. Consumer waits for the shared state change on the condvar.
The number of consumer threads is fixed and corresponds to the number of available CPU cores. The number of the producer threads is specified via the first argument as a number threads per single CPU core.
The second and the third arguments specify the producer and consumer rate.

# How to reproduce
The most tricky part is to pick up the "right" values for the arguments. The higher is the contention the more the chances are to reproduce the issue. For example,
it took several hours on ODROID N2 to reproduce the issue with [odroid-n2.sh](odroid-n2.sh). 
On a one many-core server with the command line 
    `./test_pthread_cond_singal_deadlock.aarch64-linux-gnu 10 500 1000`
I was able to reproduce the issue in a less than 10 minutes.

# Known workarounds
* reduce the contention
* ptrace the process when the deadlock situation is observed
* call *pthread_cond_signal* from under the lock

# Platforms
Seen on ODROID N2 and other aarch64-based devices/servers.

## OS
Ubuntu 18.04

## Kernel
* 4.9.187
* 4.15.0
* 4.19.114

## glibc
2.27
