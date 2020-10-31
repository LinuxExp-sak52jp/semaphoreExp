#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>
#include <string.h>

const char *parentSemName = "/ParentSem";
const char *childSemName = "/ChildSem";

void DoJob(void)
{
#if 0
    for (int i = 0; i < 100000; i++) {
        printf("No = %06d\r", i);
    }
    printf("\n");
#endif
}

void WriteSync(const char *string)
{
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "pid=%d:%s\n", getpid(), string);
    write(STDOUT_FILENO, buffer, strlen(buffer));
    fsync(STDOUT_FILENO);
}

int main(int ac, char *av[])
{
    // セマフォはリソースなし状態で作成しておく
    sem_t *parentSem = sem_open(parentSemName, O_CREAT|O_RDWR, 0666, 0);
    if (parentSem == SEM_FAILED) {
        perror(__FUNCTION__);
        return -1;
    }
    sem_t *childSem = sem_open(childSemName, O_CREAT|O_RDWR, 0666, 0);
    if (childSem == SEM_FAILED) {
        perror(__FUNCTION__);
        return -1;
    }
    // 親子ともSCHED_FIFOで動作させる
    struct sched_param param = {0};
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    if (sched_setscheduler(getpid(), SCHED_FIFO, &param)) {
        perror(__FUNCTION__);
        return -1;
    }
    if (sched_setparam(getpid(), &param)) {
        perror(__FUNCTION__);
        return -1;
    }
    
    pid_t cpid = fork();
    if (cpid == 0) {
        //-------------- 子プロセス -------------------
        sem_wait(childSem);
        WriteSync("(2)Start 1st job of Child");
        DoJob();
        WriteSync("(3)Waiting Parent done 1st job");
        sem_post(parentSem);
        sem_wait(childSem);

        WriteSync("(6)Start 2nd job of Child");
        DoJob();
        WriteSync("(7)Child will exit");
        sem_post(parentSem);
        sem_wait(childSem);

        if (sem_close(parentSem)) {
            perror(__FUNCTION__);
            return -1;
        }
        if (sem_close(childSem)) {
            perror(__FUNCTION__);
            return -1;
        }
        
    } else {
        //------------ 親プロセス ---------------------
        WriteSync("(1)Waiting Child done 1st job");
        sem_post(childSem);
        sem_wait(parentSem);
        
        WriteSync("(4)Start 1st job of Parent");
        DoJob();
        WriteSync("(5)Waiting Child done 2nd job");
        sem_post(childSem);
        sem_wait(parentSem);

        WriteSync("(8)Waiting Child exits");
        sem_post(childSem);
        int status;
        waitpid(cpid, &status, 0);

        WriteSync("(9)Got child exied");

        if (sem_close(parentSem)) {
            perror(__FUNCTION__);
            int status;
            waitpid(cpid, &status, 0);
            return -1;
        }
        if (sem_close(childSem)) {
            perror(__FUNCTION__);
            int status;
            waitpid(cpid, &status, 0);
            return -1;
        }
        sem_unlink(parentSemName);
        sem_unlink(childSemName);
    }
    
    return 0;
}
