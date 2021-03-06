// Copyright (c) 2020 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
  
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

/*** Constants that define parameters of the simulation ***/

#define MAX_SEATS 3        /* Number of seats in the professor's office */
#define professor_LIMIT 10 /* Number of students the professor can help before he needs a break */
#define MAX_STUDENTS 1000  /* Maximum number of students in the simulation */

#define CLASSA 0
#define CLASSB 1
#define CLASSC 2
#define CLASSD 3
#define CLASSE 4


/* Adding synchronization variables here */

/* Basic information about simulation.  They are printed/checked at the end 
 * and in assert statements during execution.
 * 
 */

static int students_in_office;   /* Total numbers of students currently in the office */
static int classa_inoffice;      /* Total numbers of students from class A currently in the office */
static int classb_inoffice;      /* Total numbers of students from class B in the office */
static int students_since_break = 0;
static int consecutiveA=0;
static int consecutiveB=0;
static int waiting_A=0;
static int waiting_B=0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t semaphore;


typedef struct 
{
  int arrival_time;  // time between the arrival of this student and the previous student
  int question_time; // time the student needs to spend with the professor
  int student_id;
  int class;
} student_info;

static int initialize(student_info *si, char *filename);
static void take_break();
static void ask_questions(int t);
void *professorthread(void *junk);
void classa_enter();
void classb_enter();
static void classa_leave();
static void classb_leave();
void* classa_student(void *si);
void* classb_student(void *si);

/* Called at beginning of simulation.  
 * Create/initialize all synchronization
 * variables and other global variables that you add.
 */
static int initialize(student_info *si, char *filename) 
{
  students_in_office = 0;
  classa_inoffice = 0;
  classb_inoffice = 0;
  students_since_break = 0;

  /* Initialize  synchronization variables here
   */


  /* Read in the data file and initialize the student array */
  FILE *fp;

  if((fp=fopen(filename, "r")) == NULL) 
  {
    printf("Cannot open input file %s for reading.\n", filename);
    exit(1);
  }

  int i = 0;
  while ( (fscanf(fp, "%d%d%d\n", &(si[i].class), &(si[i].arrival_time), &(si[i].question_time))!=EOF) && 
           i < MAX_STUDENTS ) 
  {
    i++;
  }
 sem_init(&semaphore, 0, MAX_SEATS);
 fclose(fp);
 return i;
}

/* Code executed by professor to simulate taking a break 
 */
static void take_break() 
{
  printf("The professor is taking a break now.\n");
  sleep(5);
  assert( students_in_office == 0 );
  students_since_break = 0;
}

/* Code for the professor thread.*/
void *professorthread(void *junk) 
{
  printf("The professor arrived and is starting his office hours\n");

  /* Loop while waiting for students to arrive. */
  while (1) 
  {
    pthread_mutex_lock(&mutex);
    if(students_in_office == 0 && students_since_break >=10)
    {
    take_break(); 
    }
    pthread_mutex_unlock(&mutex);

  }
  pthread_exit(NULL);
}


/* Code executed by a class A student to enter the office. 
 */
void classa_enter() 
{

  
  /* Request permission to enter the office. */ 
 int flag=0, flag1=0;
  while(!flag1)
  {
  pthread_mutex_lock(&mutex);
  waiting_A +=1;
  if(students_in_office < MAX_SEATS)
   {
    if(consecutiveA<5 || waiting_B ==0)
    {
    if(classb_inoffice==0)
     {
      flag=1;
     }
    }
   }
    if(flag==1)
    {
    waiting_A -=1;
    consecutiveA +=1;
    consecutiveB =0;
    students_in_office +=1;
    students_since_break +=1;
    classa_inoffice +=1;
      flag1=1;
    }
    pthread_mutex_unlock(&mutex);
  }
  if(flag1==1)
  {
  sem_wait(&semaphore);
  }

}

/* Code executed by a class B student to enter the office.
 */
void classb_enter() 
{

  
  /* Request permission to enter the office.  */ 
  int flag=0, flag1=0;
  while(!flag1)
  {
  pthread_mutex_lock(&mutex);
  waiting_B +=1;
  if(students_in_office < MAX_SEATS)
   {
    if(consecutiveB<5 || waiting_A ==0)
    {
    if(classa_inoffice==0)
     {
      flag=1;
     }
    }
   }
    if(flag==1)
    {
    waiting_B -=1;
    consecutiveB +=1;
    consecutiveA =0;
    students_in_office +=1;
    students_since_break +=1;
    classb_inoffice +=1;
      flag1=1;
    }
    pthread_mutex_unlock(&mutex);
  }
  if(flag1==1)
  {
  sem_wait(&semaphore);
  }
}

/* Code executed by a student to simulate the time he spends in the office asking questions
 
 */
static void ask_questions(int t) 
{
  sleep(t);
}


/* Code executed by a class A student when leaving the office.
 
 */
static void classa_leave() 
{
  
  pthread_mutex_lock(&mutex);
  students_in_office -= 1;
  classa_inoffice -= 1;
  pthread_mutex_unlock(&mutex);
  sem_post(&semaphore);

}

/* Code executed by a class B student when leaving the office.
 
 */
static void classb_leave() 
{
  
  pthread_mutex_lock(&mutex);
  students_in_office -= 1;
  classb_inoffice -= 1;
  pthread_mutex_unlock(&mutex);
  sem_post(&semaphore);
}

/* Main code for class A student threads.  
 */
void* classa_student(void *si) 
{
  student_info *s_info = (student_info*)si;

  /* enter office */
  classa_enter();

  printf("Student %d from class A enters the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classb_inoffice == 0 );
  
  /* ask questions  --- do not make changes to the 3 lines below*/
  printf("Student %d from class A starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
  ask_questions(s_info->question_time);
  printf("Student %d from class A finishes asking questions and prepares to leave\n", s_info->student_id);

  /* leave office */
  classa_leave();  

  printf("Student %d from class A leaves the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);

  pthread_exit(NULL);
}

/* Main code for class B student threads.
 */
void* classb_student(void *si) 
{
  student_info *s_info = (student_info*)si;

  /* enter office */
  classb_enter();

  printf("Student %d from class B enters the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
  assert(classa_inoffice == 0 );

  printf("Student %d from class B starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
  ask_questions(s_info->question_time);
  printf("Student %d from class B finishes asking questions and prepares to leave\n", s_info->student_id);

  /* leave office */
  classb_leave();        

  printf("Student %d from class B leaves the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);

  pthread_exit(NULL);
}

/* Main function sets up simulation and prints report
 * at the end.
 * GUID: 355F4066-DA3E-4F74-9656-EF8097FBC985
 */
int main(int nargs, char **args) 
{
  int i;
  int result;
  int student_type;
  int num_students;
  void *status;
  pthread_t professor_tid;
  pthread_t student_tid[MAX_STUDENTS];
  student_info s_info[MAX_STUDENTS];

  if (nargs != 2) 
  {
    printf("Usage: officehour <name of inputfile>\n");
    return EINVAL;
  }

  num_students = initialize(s_info, args[1]);
  if (num_students > MAX_STUDENTS || num_students <= 0) 
  {
    printf("Error:  Bad number of student threads. "
           "Maybe there was a problem with your input file?\n");
    return 1;
  }

  printf("Starting officehour simulation with %d students ...\n",
    num_students);

  result = pthread_create(&professor_tid, NULL, professorthread, NULL);

  if (result) 
  {
    printf("officehour:  pthread_create failed for professor: %s\n", strerror(result));
    exit(1);
  }

  for (i=0; i < num_students; i++) 
  {

    s_info[i].student_id = i;
    sleep(s_info[i].arrival_time);
                
    student_type = random() % 2;

    if (s_info[i].class == CLASSA)
    {
      result = pthread_create(&student_tid[i], NULL, classa_student, (void *)&s_info[i]);
    }
    else // student_type == CLASSB
    {
      result = pthread_create(&student_tid[i], NULL, classb_student, (void *)&s_info[i]);
    }

    if (result) 
    {
      printf("officehour: thread_fork failed for student %d: %s\n", 
            i, strerror(result));
      exit(1);
    }
  }

  /* wait for all student threads to finish */
  for (i = 0; i < num_students; i++) 
  {
    pthread_join(student_tid[i], &status);
  }

  /* tell the professor to finish. */
  pthread_cancel(professor_tid);

  printf("Office hour simulation done.\n");

  return 0;
}
