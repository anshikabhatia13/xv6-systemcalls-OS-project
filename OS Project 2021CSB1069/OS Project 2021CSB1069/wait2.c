#include "types.h"
#include "stat.h"
#include "user.h"

int main()
{
    int i = 0;

    while (i < 2) // loop executes twice
    {
        i++;
        int readytime, runningtime, sleepingtime;
        
        fork();
        
        int pid = wait2(&readytime, &runningtime, &sleepingtime);// wait2 used to wait for child's termination 
        
        if (pid == -1)//if wait2 returns -1
        {
            printf(1, "No terminated child found for pid = %d\n", getpid());
            continue;
        }
        
        printf(1, "Parent's pid=%d, Child's pid:%d, Ready time:%d, Sleeping Time:%d, Running time:%d\n", getpid(), pid, readytime, sleepingtime, runningtime);
    }
    exit();
}
