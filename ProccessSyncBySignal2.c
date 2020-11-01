/*
 * Signalで確実な同期制御を行う実装。特にリアルタイムスケジューリングに
 * しなくても要件を満たせる方式。sigwaitで実装。
 */

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
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <signal.h>

void DoJob(void)
{
}

void WriteSync(const char *string)
{
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "pid=%d:%s\n", getpid(), string);
    write(STDOUT_FILENO, buffer, strlen(buffer));
    fsync(STDOUT_FILENO);
}

void SendSig(int pid)
{
    kill(pid, SIGUSR1);
}

int main(int ac, char *av[])
{
    // SIGUSR1で待つ設定
    sigset_t sset;
    sigemptyset(&sset);
    sigaddset(&sset, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &sset, NULL)) {
        perror("sigprocmask");
        return -1;
    }
    int sig;
    
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
        WriteSync("(2)Start 1st job of Child");
        DoJob();
        WriteSync("(3)Waiting Parent done 1st job");
        SendSig(getppid());

        sigwait(&sset, &sig);
        WriteSync("(6)Start 2nd job of Child");
        DoJob();
        WriteSync("(7)Child will exit");
        SendSig(getppid());
        
    } else {
        //------------ 親プロセス ---------------------
        WriteSync("(1)Waiting Child done 1st job");

        sigwait(&sset, &sig);
        WriteSync("(4)Start 1st job of Parent");
        DoJob();
        WriteSync("(5)Waiting Child done 2nd job");
        SendSig(cpid);
        
        sigwait(&sset, &sig);
        WriteSync("(8)Waiting Child exits");
        //SendSig(cpid);

        int status;
        waitpid(cpid, &status, 0);
        WriteSync("(9)Got child exied");
    }
    
    return 0;
}
