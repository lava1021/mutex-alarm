#include <pthread.h>
#include <time.h>
#include "errors.h"

typedef struct alarm_tag {
    struct alarm_tag    *link;
    int                 seconds;
    time_t              time;   /* seconds from EPOCH */
    char                message[180];
    int                 thread_id;
    int                 alarm_id;
} alarm_t;

pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;
alarm_t *alarm_list = NULL;

pthread_t tid[50];
int thread_counter = 0;

int thread_creation_bool[50]; // if there is a 1 in the array then make a thread

void *alarm_thread (void *arg)
{    
    alarm_t *alarm, *next;
    int thread_pos = thread_counter;
    int sleep_time;
    time_t now;
    int status;   

    int alarm_counter;

    pthread_t threadid;
    threadid = pthread_self();      
    char buff[100];

    
    while (1) {
        status = pthread_mutex_lock (&alarm_mutex);
        alarm_counter = 0;
        if(alarm_list != NULL)
        {
			alarm = alarm_list;
			while (1) {
				if (alarm->thread_id == (int)threadid)
				{
					alarm_counter++;
				}
				if (alarm->link != NULL)
				{
					alarm= alarm->link;
				}
				else
				{
					break;
				}
			}
		}
        
		if(alarm_list != NULL)
		{
			if(alarm_counter < 3)
			{
				alarm = alarm_list;
				while (1) {
					if (alarm->thread_id == 0)
					{
						alarm->thread_id = (int)threadid;
						alarm_counter++;
						now = time (NULL);
						strftime (buff, 100, "%H:%M:%S", localtime (&now));
						printf("Alarm(%d) Assigned To Display Alarm Thread %u at %s: %s\n", alarm->alarm_id,threadid,buff,alarm->message);
					}
					if (alarm->link != NULL)
					{
						alarm= alarm->link;
					}
					else
					{
						break;
					}
				}
			}
			if (alarm_counter < 3)
			{
				thread_creation_bool[thread_pos] = 0; //dont make a thread
			}
			else if (alarm_counter == 3)
			{
				thread_creation_bool[thread_pos] = 1; // make a thread
			}
			if(alarm_counter > 0)
			{
				next = alarm_list;
				while (next != NULL) {
					if (next->thread_id == (int)threadid)
					{
						now = time (NULL);
						if (next->time <= now)
						{
							strftime (buff, 100, "%H:%M:%S", localtime (&now));
							printf("Alarm (%d) Printed by Alarm Thread %u at %s: %s\n",next->alarm_id,next->thread_id,buff,next->message);
							// printf ("(%d) %s\n", alarm->seconds, alarm->message);
							next->time = now + next->seconds;
						}
					}
					if (next->link != NULL)
					{
						next = next->link;
					}
					else
					{
						break;
					}
				}
			}
		}
		if(alarm_counter == 0)
		{
			now = time (NULL);
			strftime (buff, 100, "%H:%M:%S", localtime (&now));
			printf("Display Alarm Thread %u Terminated at %s\n",threadid,buff);
			status = pthread_mutex_unlock (&alarm_mutex);
			thread_counter--;
			return NULL;
		}

        
        status = pthread_mutex_unlock (&alarm_mutex);
        if (status != 0)
            err_abort (status, "Unlock mutex");
        sleep (1);
    }
}

int main (int argc, char *argv[])
{
    int status;
    char line[128];
    alarm_t *alarm, **last, *next;
    alarm_t *prev;
    pthread_t thread;
    int alarm_counter;
    char command[128];
    int  id;

    time_t now;
    
    char buff[100];
    
    int i;
    
    int create_thread_bool;
    

    while (1) {
        printf ("alarm> ");
        if (fgets (line, sizeof (line), stdin) == NULL) exit (0);
        if (strlen (line) <= 1) continue;
        alarm = (alarm_t*)malloc (sizeof (alarm_t));
        if (alarm == NULL)
            errno_abort ("Allocate alarm");

        
        if (status != 0)
            err_abort (status, "Lock mutex");

        if(sscanf (line, "%[a-zA-Z_] ( %d ) %d %180[^\n]", command, &alarm->alarm_id, &alarm->seconds, alarm->message) == 4)
        {
            if (strcmp (command, "Start_Alarm") == 0)
            {
            	status = pthread_mutex_lock (&alarm_mutex);

                alarm->time = time (NULL) + alarm->seconds;

                last = &alarm_list;
                next = *last;
                while (next != NULL) {
                    if (next->alarm_id >= alarm->alarm_id) {
                        alarm->link = next;
                        *last = alarm;
                        break;
                    }
                    last = &next->link;
                    next = next->link;
                }
                if (next == NULL) {
                    *last = alarm;
                    alarm->link = NULL;
                }
                now = time (NULL);
                strftime (buff, 100, "%H:%M:%S", localtime (&now));
                printf("Alarm(%d) Inserted by Main Thread %u Into Alarm List at %s: %s\n",alarm->alarm_id,(int *)pthread_self(),buff,alarm->message);

                status = pthread_mutex_unlock (&alarm_mutex);
                if (status != 0)
                    err_abort (status, "Unlock mutex");
                
                if (alarm_counter == 0)//first ever alarm
                {
                    status = pthread_create (&tid[thread_counter], NULL, alarm_thread, (int *)pthread_self());
                    now = time (NULL);
                    strftime (buff, 100, "%H:%M:%S", localtime (&now));
                    printf("Created New Display Alarm Thread %u For Alarm(%d) at %s: %s\n", tid[thread_counter], alarm->alarm_id,buff, alarm->message);

                }
                create_thread_bool = 1;
                for (i = 0; i < thread_counter+1; i++) 
                {
                	if (thread_creation_bool[i] == 0)
                	{
                		create_thread_bool = 0;
                	}
                }
                if(create_thread_bool == 1)
                {
                	thread_counter++;
					status = pthread_create (&tid[thread_counter], NULL, alarm_thread, (int *)pthread_self());
					now = time (NULL);
					strftime (buff, 100, "%H:%M:%S", localtime (&now));
					printf("Created New Display Alarm Thread %u For Alarm(%d) at %s: %s\n", tid[thread_counter], alarm->alarm_id,buff, alarm->message);
                }
                //alarm->thread_id = thread_counter;
                alarm_counter++;

            }
            else
            {
                fprintf (stderr, "Bad command\n");
                free (alarm);
            }
        }

        else if(sscanf (line, "%[a-zA-Z_] ( %d )", command, &id) == 2)
        {
            free (alarm);
            if (strcmp (command, "Cancel_Alarm") == 0)
            {
                status = pthread_mutex_lock (&alarm_mutex);
                            
                prev = alarm_list;
                next = prev;
                if (next != NULL) // is list empty?
                {
					while (next != NULL) {
						if (next->alarm_id == id)
						{
							now = time (NULL);
							strftime (buff, 100, "%H:%M:%S", localtime (&now));
							printf("Alarm(%d) Canceled at %s: %s\n",next->alarm_id,buff,next->message);
							if(prev == next) //looking at the first element
							{
								if(prev->link != NULL) //there are atleast 2 elements
								{
									alarm_list = prev->link;
								}
								else // there is only one element
								{
									alarm_list = NULL;
								}
							}
							else //looking at elements past the first
							{
								if (next->link != NULL)
								{
									prev->link = next->link;
								}
								else
								{
									prev->link = NULL;
								}
							}
							alarm_counter--;
							break;
						}
						prev = next;
						if (next->link != NULL)
						{
							next = next->link;
						}
						else
						{
							break;
						}
					}
                }               
                
                status = pthread_mutex_unlock (&alarm_mutex);
                if (status != 0)
                    err_abort (status, "Unlock mutex");
            }
            else
            {
                fprintf (stderr, "Bad command\n");
            }
        }
        else
        {
            fprintf (stderr, "Bad command\n");
            free (alarm);
        }

        // printf ("[list: ");
        // for (next = alarm_list; next != NULL; next = next->link)
        //     printf ("%d(%d)[\"%s\"] ", next->time, next->time - time (NULL), next->message);
        // printf ("]\n");
    }
}