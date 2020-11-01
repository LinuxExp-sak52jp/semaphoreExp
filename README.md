# semaphoreExp
スケジューリングポリシーをSCHED_FIFOに設定した時に、プロセスのブロックに同期したタスクスイッチを実現するためにどのような方法が取れるかを検討した。  
最初、下記のようにセマフォやMutexをただ一つ用いて同期を試みた。
```
    pid_t cpid = fork();
    if (cpid == 0) {
        //-------------- 子プロセス -------------------
        WriteSync("(2)Start 1st job of Child");
        DoJob();
        WriteSync("(3)Waiting Parent done 1st job");
        sem_post(sem);
        sched_yield();

        sem_wait(sem);
        WriteSync("(6)Start 2nd job of Child");
        ....SNIP....
     } else {
        //------------ 親プロセス ---------------------
        WriteSync("(1)Waiting Child done 1st job");

        sem_wait(sem);
        WriteSync("(4)Start 1st job of Parent");
        DoJob();
        ....SNIP....
     }
```
意図したところは、子プロセスで(3)を実行してsem_post()した後sched_yield()すれば、親プロセスの(4)が実行されるということだった。しかし、殆どの場合は(3)の後にそのまま子プロセスの
(6)が実行されてしまった。これはSCHED_FIFOポリシーを使っても、(3)の後のsem_post()で親プロセスがプリエンプトされないことを示しており、RTOSのような動作を期待することは
出来ないことが分かった。sched_yield()の代わりにusleep()を適切な時間で呼んでやると所望の動作になったが、これでは不確実性が残ってしまい、同期を完全に保証することは出来ない。
セマフォで完全な同期を保証するためには、下記のような実装が必要であり、結局sched_yield()の出番はなかった。
```
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
        ....SNIP....
     } else {
        //------------ 親プロセス ---------------------
        WriteSync("(1)Waiting Child done 1st job");
        sem_post(childSem);

        sem_wait(parentSem);
        WriteSync("(4)Start 1st job of Parent");
        DoJob();
        ....SNIP....
     }
```
これだと、「sched_yield()のユースケースって何なんだろう？」と疑問に思ったりしたが、上述した「殆どの場合は(3)の後にそのまま子プロセスの
(6)が実行されてしまった。」というのがミソで、場合によっては(4)が実行されることもあるということになる。結局、Linuxのリアルタイムポリシーはこの程度が限界なのだろう。  
  
ついでに、Signalを使って同期させる実装も2種類検討した。これも原理的には「セマフォを２つ使う」のと同じことで、要は譲り元は、譲り先が発火させるまで確実にブロックするリソースを
使わなければいけないということである。なんだかなぁという感じだが、これでも優先度で順序保証できるだけ、.NETのスレッドプールよりも相当使い勝手が良い。
