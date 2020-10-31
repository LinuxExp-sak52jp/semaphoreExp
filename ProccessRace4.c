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

void DoJob(void)
{
}
void Move(void)
{
    usleep(20);
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
    // 共有メモリ上にSpinlockを作成
    pthread_spinlock_t *lock;
    int shmid;
    shmid = shmget(IPC_PRIVATE, sizeof(pthread_spinlock_t), 0600);
    if (shmid < 0) {
        perror("shmget");
        return -1;
    }
    lock = (pthread_spinlock_t *)shmat(shmid, NULL, 0);
    if (lock == (void *)-1) {
        perror("shmat");
        return -1;
    }
    if (pthread_spin_init(lock, PTHREAD_PROCESS_SHARED)) {
        perror("pthread_spin_init");
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
        pthread_spin_lock(lock);
        WriteSync("(2)Start 1st job of Child");
        DoJob();
        WriteSync("(3)Waiting Parent done 1st job");
        pthread_spin_unlock(lock);
        Move();

        pthread_spin_lock(lock);
        WriteSync("(6)Start 2nd job of Child");
        DoJob();
        WriteSync("(7)Child will exit");
        pthread_spin_unlock(lock);
        Move();

        shmdt((void *)lock);
        
    } else {
        //------------ 親プロセス ---------------------
        pthread_spin_lock(lock);
        WriteSync("(1)Waiting Child done 1st job");
        pthread_spin_unlock(lock);
        Move();

        pthread_spin_lock(lock);
        WriteSync("(4)Start 1st job of Parent");
        DoJob();
        WriteSync("(5)Waiting Child done 2nd job");
        pthread_spin_unlock(lock);
        Move();

        pthread_spin_lock(lock);
        WriteSync("(8)Waiting Child exits");
        pthread_spin_unlock(lock);

        int status;
        waitpid(cpid, &status, 0);
        WriteSync("(9)Got child exied");

        shmdt((void *)lock);
        if (shmctl(shmid, IPC_RMID, NULL) != 0) {
            perror("shmctl");
            return 1;
        }
    }
    
    return 0;
}
