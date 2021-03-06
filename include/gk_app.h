#ifndef _GK_APP_H_
#define _GK_APP_H_
#include <pthread.h>

/**
 * @brief DEBUG打印信息, 一般在调试时打开， 正式版本会屏蔽掉.
 */
#define __DEBUG__

#ifdef __DEBUG__
/**
 * @brief 是否显示所有DEBUG打印信息
 */
/* #define DEBUG_SHOW_ALL */


#define DEBUG(format, ...) printf("[%s : %s : %d] ",__FILE__,__func__,__LINE__); printf(format, ##__VA_ARGS__);

#define DEBUG_FUNC_START printf("++++++++++++++++++++++++++++++++ %s\n",__func__);
#define DEBUG_FUNC_END		   printf("-------------------------------- %s\n",__func__);

#else
#define DEBUG(format, ...)
#define DEBUG_FUNC_START
#define DEBUG_FUNC_END
#endif

/**
 * @brief 异常打印信息.
 */
#define PERROR(format, ...) printf("[%s : %s : %d] ",__FILE__,__func__,__LINE__); printf(format, ##__VA_ARGS__);

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief ���������Ļ�����. \n
 * 	Ŀǰ���Ѻ�ʶ�𶼲���ͬһ��callback��Ϊ�˱���callback���ز������룬�ڵ���callbackǰ������callback������.
 */

extern pthread_mutex_t ai_lock;
#define ai_mutex_lock()				\
	do {								\
		int i = 0;			\
		while (pthread_mutex_trylock(&ai_lock)) {			\
			if (i++ >= 100) {				\
				PERROR("####dead lock####\n");	\
				i = 0;	\
			}			\
			usleep(100 * 1000);				\
		}							\
	} while (0)

#define ai_mutex_unlock() 	\
	do {								\
		pthread_mutex_unlock(&ai_lock);\
	} while (0)

#ifdef __cplusplus
}
#endif
#endif

