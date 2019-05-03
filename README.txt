PROJECT 4 README

OSS

Create child processes at random times, each child is given a control block in shared memory.  

CONTROL BLOCK FORMAT

	PID | START TIME | PRIORITY QUEUE | COMPLETE STATE |  QUANTUMS USED | COMPLETE TIME

Child process in priority queue 1 are selected from round robin as next quantum recipient

Child process in priority queue 2 are moved to queue 1

Child process in priorty queue 3 are moved to queue 2

If all processes are locked, the oldest process is selected for unlock

When complete a process is flagged as such in their control block

OSS terminates after 30 simulated seconds

Idle time is the total amount of time accured when a process is complete but still has quantum time remaining

moving processes between queues and scheduling the next active process creates overhead

log.txt records child process events (queue change, quantum delivery)

total amount of quantums (column 5) of all control blocks * quantumTime = total time of actual child process exection
