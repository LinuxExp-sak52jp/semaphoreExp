/*
 * sched_yield()では全然タスクスイッチしない。usleep(1)でも駄目。
 * usleep(4)だったらうまくいった。やはり、待っているリソースを取得する
 * までの時間をかせいであげないと、Ready queueに上がってこれないようだ。
 * Semaphoreよりはレスポンス良いが、LinuxのFIFO schedはあまり
 * あてにならないか？
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

const char *semName = "/mysem";

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

int main(int ac, char *av[])
{
    // 共有メモリ上にMutexを作成
    pthread_mutex_t *mtx;
    pthread_mutexattr_t mat;
    int shmid;
    shmid = shmget(IPC_PRIVATE, sizeof(pthread_mutex_t), 0600);
    if (shmid < 0) {
        perror("shmget");
        return -1;
    }
    mtx = (pthread_mutex_t *)shmat(shmid, NULL, 0);
    if (mtx == (void *)-1) {
        perror("shmat");
        return -1;
    }
    pthread_mutexattr_init(&mat);
    if (pthread_mutexattr_setpshared(&mat, PTHREAD_PROCESS_SHARED)) {
        perror("pthread_mutexattr_setpshared");
        return -1;
    }
    pthread_mutex_init(mtx, &mat);

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
        pthread_mutex_lock(mtx);
        WriteSync("(1)Start 1st job of Child");
        DoJob();
        WriteSync("(2)Waiting Parent done 1st job");
        pthread_mutex_unlock(mtx);
        Move();

        pthread_mutex_lock(mtx);
        WriteSync("(3)Start 2nd job of Child");
        DoJob();
        WriteSync("(4)Child will exit");
        pthread_mutex_unlock(mtx);
        Move();

        shmdt(mtx);
        
    } else {
        //------------ 親プロセス ---------------------
        pthread_mutex_lock(mtx);
        WriteSync("(1)Waiting Child done 1st job");
        pthread_mutex_unlock(mtx);
        Move();

        pthread_mutex_lock(mtx);
        WriteSync("(2)Start 1st job of Parent");
        DoJob();
        WriteSync("(3)Waiting Child done 2nd job");
        pthread_mutex_unlock(mtx);
        Move();

        pthread_mutex_lock(mtx);
        WriteSync("(4)Waiting Child exits");
        pthread_mutex_unlock(mtx);

        int status;
        waitpid(cpid, &status, 0);
        WriteSync("(5)Got child exied");

        shmdt(mtx);
        if (shmctl(shmid, IPC_RMID, NULL) != 0) {
            perror("shmctl");
            return 1;
        }
    }
    
    return 0;
}
