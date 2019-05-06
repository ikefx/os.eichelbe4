PROJECT 4 README

OSS

Create child processes at random times, each child is given a control block in shared memory.  

CONTROL BLOCK FORMAT

	PID | START TIME | PRIORITY QUEUE | COMPLETE STATE |  QUANTUMS USED | COMPLETE TIME | PROCESS TYPE (1=user, 0=process)

Child process in priority queue 1 are selected from round robin as next quantum recipient

Child process in priority queue 2 are moved to queue 1

Child process in priorty queue 3 are moved to queue 2

After a process is finished with the quantum, they are moved to Queue 3 (if user process), system processes are sent to Queue 2

If all processes are locked, the oldest process is selected for unlock

When unlocked and criteria met: a process is given the quantum.

A Quantum last 4 seconds for this simulation.  (logical clock updates by 4 seconds each time a quantum is issued)

Every second of a quantum there exist the possiblity that the child will terminate successfully.

If a child completes before their quantum expires, excess time is added to the total idle cpu time counter

Idle time is the total amount of time accured when a process is complete but still has quantum time remaining

moving processes between queues and scheduling the next active process creates half second overhead added to idle time

OSS terminates after 10 realtime seconds, or 60 simulated logical seconds, or over 18 children

log.txt records child process events (queue change, quantum delivery)

total amount of quantums (column 5) of all control blocks * quantumTime = total time of actual child process exection
