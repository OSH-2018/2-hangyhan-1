#include <stdio.h>  //基本输入输出头文件
#include <unistd.h> //用于linux系统的调用，包含了sleep(), fork()等函数
#include <string.h> //包含字符串操作的头文件
#include <sys/wait.h> //包含wait()，waitpid()函数的头文件
#include <sys/types.h> //包含size_t. time_t, pid_t等基本数据类型的头文件
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
//#include <tlpi_hdr.h>
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
int main() {
    char cmd[256]; //输入的命令行
    char *args[128]; //指针数组，每一个元素指向命令行中的一段
    while (1) {
        printf("# "); //输出提示符
        fflush(stdin); //清空输入缓冲区中的内容
        fgets(cmd, 256, stdin); //从输入流中读取256个字符，若输入流中不足256个字符，则将换行符也读入

        /* 清理结尾的换行符 */
        int k;
        for (k = 0; cmd[k] != '\n'; k++);
        cmd[k] = '\0';

        /* 拆解命令行 */
        args[0] = cmd;
        for (k = 0; *args[k]; k++) //将cmd中的空格用'\0'替换，同时让args[i]指向cmd中第i段字符所开始的地址
            for (args[k+1] = args[k] + 1; *args[k+1]; args[k+1]++)
                if (*args[k+1] == ' ') {
                    *args[k+1] = '\0';
                    args[k+1]++;
                    break;
                }
        args[k] = NULL;

        /* 没有输入命令 */
        if (!args[0])
            continue;

        //构建管道
        int i = 0; //i是循环变量，用于扫描cmd中的管道符
        int j = 0; //arg[ j ] 指向下一个待执行的命令
        int pfd[ 2 ]; //文件描述符数组
        int if_parent = 1; //标明该进程是否为父进程

        while( args[ i ] ){

          j = i;

          for(; args[ i ]; i++ ){ //将i定位到下一个|处
            if( strcmp( args[i], "|") == 0 )break;
          }

          if( ! args[ i ] )break; //已经没有|，可以退出循环

          args[ i ] = NULL;
          if( pipe( pfd ) == -1)//生成一个管道
              exit(0);

          switch( fork() ){
            case -1 : exit(0); //生成子进程异常


            case 0: //子进程连接到管道的读出端
              if_parent = 0; //将该进程标记为子进程
              if( close( pfd[ 1 ] ) == - 1 )exit(0);//关闭子进程对管道写入数据的端口

              if( pfd[ 0 ] != STDIN_FILENO){//将第一个子进程的输入端口与管道读出端口绑定
                if( dup2( pfd[ 0 ], STDIN_FILENO ) == -1 )exit(0);
                if( close( pfd[ 0 ] ) == -1 )exit(0);
              }
              i = i + 1; //指向管道符之后的那个命令
              continue; //子进程进入下一次循环，作为下一次循环的父进程


            default://如果是父亲进程将父进程的输出端口与管道的读入端口绑定
              if( close( pfd[ 0 ] ) == -1 )exit(0); //关闭父进程从管道读出数据的端口

              if( pfd[ 1 ] != STDOUT_FILENO ){//将父进程的输出端口与管道的读入端口绑定
                if( dup2( pfd[ 1 ], STDOUT_FILENO ) == -1 )exit(0);
                if( close( pfd[ 1 ] ) == -1 )exit(0);
              }

              break;
          }

        }


        /* 内建命令 */
        if (strcmp(args[0], "cd") == 0) { //如果输入的是cd命令，则将工作目录切换到cd命令之后所指示的目录下
            if (args[1])
                chdir(args[1]);
            continue;
        }
        if (strcmp(args[0], "pwd") == 0) { //如果输入的命令是pwd，则将当前工作目录复制到数组wd中，并输出
            char wd[4096];
            puts(getcwd(wd, 4096));
            continue;
        }

        if (strcmp(args[0], "exit") == 0) //如果输入exit则退出shell程序
            return 0;

        /* 外部命令 */

        /*修改环境变量*/

        if (strcmp( args[0], "export") == 0){
           char *p = args[1];
           for(; (*p != '=') && (*p); p++);
           if( *p != '\0' ){
             *p = '\0';
             args[ 2 ] = p + 1;
             setenv( args[1], args[2], 1);
             continue;
           }
        }



        pid_t pid = fork();  // 创建子进程
        if (pid == 0) { //在子进程中执行命令
            /* 子进程 */
            execvp(args[ 0 ], args  );
            /* execvp失败 */
            return 255;
        }
        /* 父进程 */
        

        
        wait(NULL); //暂停父进程直至子进程结束

    }
}
