/*
 * セマフォ一つの代わりにミューテックス一つで実装した。
 * セマフォよりはレスポンスが良いが、sleepでないと同期がとれない
 * 特性は全く同じ。
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
void Move(void)
{
    usleep(4);
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

void handler(int sig)
{
}

int main(int ac, char *av[])
{
    // SIGUSRをハンドラーで受ける
    struct sigaction act = {0};
    act.sa_handler = handler;
    if (sigaction(SIGUSR1, &act, NULL)) {
        perror("sigaction");
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
        WriteSync("(2)Start 1st job of Child");
        DoJob();
        WriteSync("(3)Waiting Parent done 1st job");
        SendSig(getppid());

        pause();
        WriteSync("(6)Start 2nd job of Child");
        DoJob();
        WriteSync("(7)Child will exit");
        SendSig(getppid());
        
    } else {
        //------------ 親プロセス ---------------------
        WriteSync("(1)Waiting Child done 1st job");

        pause();
        WriteSync("(4)Start 1st job of Parent");
        DoJob();
        WriteSync("(5)Waiting Child done 2nd job");
        SendSig(cpid);
        
        pause();
        WriteSync("(8)Waiting Child exits");
        SendSig(cpid);

        int status;
        waitpid(cpid, &status, 0);
        WriteSync("(9)Got child exied");
    }
    
    return 0;
}
