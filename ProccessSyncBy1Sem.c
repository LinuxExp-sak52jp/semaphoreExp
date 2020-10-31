/*
 * セマフォ一つとsched_yield()で同期を試みたが、結局sleepしないと意図した
 * 動作にならなかった。sem_postとsched_yieldの間に相手がReady Queueに
 * つながる保証がないためにこうなってしまう。sem_postではなく、一度の呼び出しで
 * アトミックに相手がReadyになるリソースを用いないとsched_yield()は使い物に
 * ならない。
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

const char *semName = "/mysem";

void DoJob(void)
{
}

void Move(void)
{
    usleep(30);
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
    sem_t *sem = sem_open(semName, O_CREAT|O_RDWR, 0666, 0);
    if (sem == SEM_FAILED) {
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
        WriteSync("(2)Start 1st job of Child");
        DoJob();
        WriteSync("(3)Waiting Parent done 1st job");
        sem_post(sem);
        Move();

        sem_wait(sem);
        WriteSync("(6)Start 2nd job of Child");
        DoJob();
        WriteSync("(7)Child will exit");
        sem_post(sem);
        Move();

        sem_wait(sem);
        if (sem_close(sem)) {
            perror(__FUNCTION__);
            return -1;
        }
        
    } else {
        //------------ 親プロセス ---------------------
        WriteSync("(1)Waiting Child done 1st job");

        sem_wait(sem);
        WriteSync("(4)Start 1st job of Parent");
        DoJob();
        WriteSync("(5)Waiting Child done 2nd job");
        sem_post(sem);
        Move();
        
        sem_wait(sem);
        WriteSync("(8)Waiting Child exits");
        sem_post(sem);

        int status;
        waitpid(cpid, &status, 0);

        WriteSync("(9)Got child exied");
        
        if (sem_close(sem)) {
            perror(__FUNCTION__);
            int status;
            waitpid(cpid, &status, 0);
            return -1;
        }
        sem_unlink(semName);
    }
    
    return 0;
}
