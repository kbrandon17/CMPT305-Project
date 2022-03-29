#include<stdio.h>
#include<time.h>
#include<math.h>
#include<stdlib.h>
#include<unistd.h>
#include<assert.h>
#include<float.h>
#include<stdbool.h>
#include<string.h>
#include<ctype.h>

// Definition of a Queue Node including arrival and service time

struct QueueNode {
    double eval_arrival_time;     // customer evaluation arrival time
    double eval_service_time;     // customer evaluation service time (exponential)
    double eval_waiting_time;     // customer time spent waiting in eval queue
    double priority_arrival_time; // customer priority arrival time
    double priority_service_time; // customer priority service time
    double priority_waiting_time; // customer priority waiting time
    double time_to_clean_room;    // time it takes the janitor to clean their room
    int priority;                 // 1 - low, 2 - med, 3 - high
    struct QueueNode *next;       // next element in line; NULL if this is the last element
};

// Evaluation queue to track next arrival

struct EvalQueue {

    struct QueueNode* nextHighPri;   // next high priority to arrive
    struct QueueNode* nextMedPri;    // next medium priority to arrive
    struct QueueNode* nextLowPri;    // next low priority node to arrive
    int totalInSystem;               // total number of patients in evaluation queue (waiting and being evaluated)
    int availableNurses;             // number of nurses currently available to evaluate
    double cumulative_waiting;       // Accumulated waiting time for all effective departures
    int waiting_count;               // Current number of customers waiting for service
};

// Event queue to execute simulation in order

struct EventQueue {
  struct EventQueueNode* head;       // next event to occur that is queied 
  struct EventQueueNode* tail;       // last event to occur that is queued
};

// Nodes to be used in event queue tracking when the event will occur and what will happen

struct EventQueueNode {
  double event_time;                // time that the event will occur
  int event_type;                   // 1: eval queue arrival 2: eval service start 3: priority queue arrival 4: priority queue service start 5: patient leaves hospital 6: room cleaned
  struct QueueNode* qnode;
  struct EventQueueNode *next;
};

// TODO Priority Queue to track patients after evaluation

struct Queue {

    struct QueueNode* head;         // Point to the first node of the element queue
    struct QueueNode* tail;         // Point to the tail node of the element queue
    struct QueueNode* mediumTail;   // Head node of medium priority section of list
    struct QueueNode* highTail;      // Head node of high priority section of list
    int waiting_count;              // Current number of customers waiting for service
    double cumulative_response;     // Accumulated response time for all effective departures
    double cumulative_waiting;      // Accumulated waiting time for all effective departures
    double cumulative_idle_times;   // Accumulated times when the system is idle, i.e., no customers in the system
    double cumulative_serving;       // Accumulated number of customers in the system
    int totalInSystem;
};


// ------------Simulation variables------------------------------------------------------

double current_time = 0.0;          // current time during simulation (minutes past 12AM)
double prevCurrentTime = 0.0;       // to store previous current time to calculate averages
int departure_count;                // total departures of patients leaving hospital
double avgInSystem;                 // average number of patients in system
double avgResponseTimeAll;          // average response time of all patients
double avgResponseTimeHigh;         // average response time of high priority patients
double avgResponseTimeMed;          // average response time of medium priority patients
double avgResponseTimeLow;          // average response time of low priority patients
double avgEvalWaitingTime;          // average waiting time in evaluation queue
double avgPriorityWaitingTimeAll;   // average waiting time in priority queue of all patients
double avgPriorityWaitingTimeHigh;  // average waiting time in priority queue of high priority patients
double avgPriorityWaitingTimeMed;   // average waiting time in priority queue of medium priority patients
double avgPriorityWaitingTimeLow;   // average waiting time in priority queue of low priority patients
double avgCleanUpTime;              // average time to clean up the patient room
int numberOfTurnedAwayPatients;     // total number of turned away patients due to full capacity



//---------------Priority Queue--------------------------
void InsertPriorityQueue(struct Queue* queue, struct QueueNode* queuenode){
  queue->cumulative_number++;
  queue->cumulative_waiting++;
  queuenode->next = NULL;
  switch(queuenode->priority){
    case 1: //if low priority
      if(queue->head == NULL){ //if queue empty
        queue->head = queuenode;
        queue->tail = queuenode;
      }
      else { //if queue has nodes
        queue->tail->next = queuenode;
        queue->tail = queuenode;
      }
    case 2: //if medium priority
      if(queue->head == NULL){ //if queue empty
        queue->head = queuenode;
        queue->tail = queuenode;
        queue->mediumTail = queuenode;
      } else if (queue->mediumTail == NULL){//if medium section of queue is empty
        queue->mediumTail = queuenode;
        if (queue->highTail != NULL){ //if high priority section has members
          queuenode->next = queue->highTail->next;
          queue->highTail->next = queuenode;
        } else { //else if only low priority section has members
          queuenode->next = queue->head;
          queue->head = queuenode;
        }
      } else { //else if nodes exist in medium section of queue
        queuenode->next = queue->mediumTail->next;
        queue->mediumTail->next = queuenode;
        queue->mediumTail = queuenode;
      }


    case 3: //if high priority
      if(queue->head == NULL){ //if queue empty
        queue->head = queuenode;
        queue->tail = queuenode;
        queue->highTail = queuenode;
      } else if (queue->highTail == NULL) { //if high priority section empty
        queue->highTail = queuenode;
        queuenode->next = queue->head;
        queue->head = queuenode;
      } else { // if high priority section has nodes
        queuenode->next = queue->highTail->next;
        queue->highTail->next = queuenode;
        queue->highTail = queuenode;
      }
  }

}

struct QueueNode* PopPriorityQueue(struct Queue* queue){
  struct QueueNode* pop = queue->head;
  queue->cumulative_number--;
  if(pop == queue->tail) { //if queue empty after pop/pop is end of low priority queue
    queue->tail = NULL;
    queue->highTail = NULL;
    queue->mediumTail = NULL;
  } else if (pop == queue->mediumTail) { //if pop is last node of medium section
    queue->mediumTail = NULL;
  } else if(pop == queue->highTail) { //if pop is last node of high section
    queue->highTail = NULL;
  }
  //normal tasks for every pop
  queue->head = queue->head->next;
  return pop;
}


struct Queue* CreatePriorityQueue(){
  struct Queue* queue = malloc(sizeof(struct Queue));
  queue->head=NULL;
  queue->tail=NULL;
  queue->mediumHead=NULL;
  queue->lowhead=NULL;
  queue->waiting_count=0;
  queue->cumulative_response=0;
  queue->cumulative_waiting=0;
  queue->cumulative_idle_times=0;
  queue->cumulative_number=0;
  queue->totalInSystem=0;
  return queue;
}



//-----------------Queue Functions------------------------------------

struct QueueNode* CreateNode(double Narrival_time, double Nservice_time, double Neval_time) {

  struct QueueNode* newNode = malloc(sizeof(struct QueueNode));
  newNode->eval_arrival_time = Narrival_time;
  newNode->eval_service_time = Nservice_time;
  newNode->eval_waiting_time = Neval_time;
  newNode->priority_arrival_time = 0.0;
  newNode->priority_service_time = 0.0;
  newNode->priority_waiting_time = 0.0;
  newNode->next = NULL;
  return newNode;
}

// Function to insert events into event queue in order

void InsertIntoEventQueueInOrder(struct EventQueue* q, struct EventQueueNode* n) {
if((q->head)->next != NULL) {
  struct EventQueueNode* curr = (q->head)->next;
  struct EventQueueNode* prev = q->head;
  while (curr != NULL) {
    if(n->event_time >= prev->event_time && n->event_time <= curr->event_time) {
        prev->next = n;
        n->next = curr;
        return;
      }
    curr = curr->next;
    prev = prev->next;
    }
  prev->next = n;
  n->next = NULL;
  q->tail = n;
  return;
}
else {
    struct EventQueueNode* curr = q->head;
    if(n->event_time >= curr->event_time) {
      curr->next = n;
      n->next = NULL;
      q->tail = n;
      return;
    }
    else {
      n->next = curr;
      curr->next = n;
      return;
    }
  }
}

// Initializes the evaluation queue, setting the first arrival of each of the three priorities

struct EvalQueue* InitializeEvalQueue(int numNurses, int seed, double highprilambda, double highprimu, double medprilambda, double medprimu, double lowprilambda, double lowprimu, double evalmu){
  struct EvalQueue* newQueue = malloc(sizeof(struct EvalQueue));

  srand(seed);

    double highPriArr = ((-1/highprilambda) * log(1-((double) (rand()+1) / RAND_MAX)));
    double highPriSer = ((-1/highprimu) * log(1-((double) (rand()+1) / RAND_MAX)));
    double medPriArr = ((-1/highprilambda) * log(1-((double) (rand()+1) / RAND_MAX)));
    double medPriSer = ((-1/highprimu) * log(1-((double) (rand()+1) / RAND_MAX)));
    double lowPriArr = ((-1/highprilambda) * log(1-((double) (rand()+1) / RAND_MAX)));
    double lowPriSer = ((-1/highprimu) * log(1-((double) (rand()+1) / RAND_MAX)));
    double evalSer = ((-1/evalmu) * log(1-((double) (rand()+1) / RAND_MAX)));

    newQueue->nextHighPri = CreateNode(highPriArr, highPriSer, evalSer);
    (newQueue->nextHighPri)->priority = 3;

    newQueue->nextMedPri = CreateNode(medPriArr, medPriSer, evalSer);
    (newQueue->nextMedPri)->priority = 2;

    newQueue->nextLowPri = CreateNode(lowPriArr, lowPriSer, evalSer);
    (newQueue->nextLowPri)->priority = 1;

    newQueue->availableNurses = numNurses;
    newQueue->cumulative_waiting = 0.0;
    newQueue->totalInSystem = 0;

  return newQueue;
}


struct EventQueueNode* CreateEvalArrivalEventNode(struct QueueNode* q) {

  struct EventQueueNode* newNode = malloc(sizeof(struct EventQueueNode));
  newNode->event_time = q->eval_arrival_time;
  newNode->event_type = 1;
  newNode->qnode = q;
  newNode->next = NULL;
  return newNode;
}


struct EventQueueNode* CreateEvalServiceEventNode(struct QueueNode* q) {

  struct EventQueueNode* newNode = malloc(sizeof(struct EventQueueNode));
  newNode->event_time = current_time;
  newNode->event_type = 2;
  newNode->qnode = q;
  newNode->next = NULL;
  return newNode;
}


struct EventQueueNode* CreatePriorityArrivalEventNode(struct QueueNode* q) {

  struct EventQueueNode* newNode = malloc(sizeof(struct EventQueueNode));
  newNode->event_time = q->eval_arrival_time + q->eval_service_time + q->eval_waiting_time;
  newNode->event_type = 3;
  newNode->qnode = q;
  newNode->next = NULL;
  return newNode;
}


struct EventQueueNode* CreatePriorityStartServiceEventNode(struct QueueNode* q) {

  struct EventQueueNode* newNode = malloc(sizeof(struct EventQueueNode));
  newNode->event_time = current_time;
  newNode->event_type = 4;
  newNode->qnode = q;
  newNode->next = NULL;
  return newNode;
}


struct EventQueueNode* CreateExitHospitalEventNode(struct QueueNode* q) {

  struct EventQueueNode* newNode = malloc(sizeof(struct EventQueueNode));
  newNode->event_time = q->priority_arrival_time + q->priority_service_time + q->priority_waiting_time;
  newNode->event_type = 5;
  newNode->qnode = q;
  newNode->next = NULL;
  return newNode;
}


struct EventQueueNode* CreateJanitorCleanedRoomEventNode(struct QueueNode* q) {

  struct EventQueueNode* newNode = malloc(sizeof(struct EventQueueNode));
  newNode->event_time = current_time + q->time_to_clean_room;
  newNode->event_type = 6;
  newNode->qnode = q;
  newNode->next = NULL;
  return newNode;
}

// Printing out the report of statistics at every hour

void PrintStatistics(struct Queue* elementQ, struct EvalQueue* evalQ){

   if(current_time >= 1440) {
     printf("End of Simulation - at 12AM the next day:\n", departure_count);
   }
   else printf("At %f O'Clock:\n", current_time/60);



  printf("Total departures: %d\n", departure_count);
  printf("Average in system: %f\n", avgInSystem);
  printf("Average response time for all patients: %f\n", avgResponseTimeAll);
  printf("Average response time for high priority patients: %f\n", avgResponseTimeHigh);
  printf("Average response time for medium priority patients: %f\n", avgResponseTimeMed);
  printf("Average response time for low priority patients: %f\n", avgResponseTimeLow);
  printf("Average Evaluation Waiting Time: %f\n", avgEvalWaitingTime);
  printf("Average waiting time for all priority patients: %f\n", avgPriorityWaitingTimeAll);
  printf("Average waiting time for high priority patients: %f\n", avgPriorityWaitingTimeHigh);
  printf("Average waiting time for medium priority patients: %f\n", avgPriorityWaitingTimeMed);
  printf("Average waiting time for low priority patients: %f\n", avgPriorityWaitingTimeLow);
  printf("Average cleaning time for patient rooms: %f\n", avgCleanUpTime);
  printf("Number of turned away patients due to max capacity: %d\n", numberOfTurnedAwayPatients);

}

// Function to process the arrival of a patient to the hospital.

void ProcessEvalArrival(struct EvalQueue* evalQ, struct QueueNode* arrival, int seed, double highprilambda, double highprimu, double medprilambda, double medprimu, double lowprilambda, double lowprimu, double evalmu){

prevCurrentTime = current_time;
current_time = arrival->eval_arrival_time;

    srand(seed);
    double evalSer = ((-1/evalmu) * log(1-((double) (rand()+1) / RAND_MAX)));
if(arrival->priority == 3) {

    double highPriArr = ((-1/highprilambda) * log(1-((double) (rand()+1) / RAND_MAX)));
    double highPriSer = ((-1/highprimu) * log(1-((double) (rand()+1) / RAND_MAX)));
    evalQ->nextHighPri = CreateNode(highPriArr, highPriSer, evalSer);
}

else if(arrival->priority == 2) {

    double medPriArr = ((-1/medprilambda) * log(1-((double) (rand()+1) / RAND_MAX)));
    double medPriSer = ((-1/medprimu) * log(1-((double) (rand()+1) / RAND_MAX)));
    evalQ->nextMedPri = CreateNode(medPriArr, medPriSer, evalSer);
}

else if(arrival->priority == 1) {

    double lowPriArr = ((-1/lowprilambda) * log(1-((double) (rand()+1) / RAND_MAX)));
    double lowPriSer = ((-1/lowprimu) * log(1-((double) (rand()+1) / RAND_MAX)));
    evalQ->nextLowPri = CreateNode(lowPriArr, lowPriSer, evalSer);
}

evalQ->totalInSystem++;

if(evalQ->availableNurses > 0) {
  StartEvaluationService(evalQ, arrival);
}
else {
  evalQ->waiting_count++;
}


}

// Function to start nurse evaluation

void StartEvaluationService(struct EvalQueue* evalQ, struct QueueNode* servNode)
{
  evalQ->availableNurses--;
  (servNode)->eval_waiting_time = current_time-(servNode->eval_arrival_time);
   evalQ->cumulative_waiting += (servNode)->eval_waiting_time;



  // elementQ->waiting_count--;

  evalQ->totalInSystem--;
}

// Called after patient has been helped by nurse and begins waiting in priority queue

void ProcessPriorityArrival(struct EvalQueue* evalQ, struct Queue* elementQ, struct QueueNode* arrival){

evalQ->availableNurses++;
// prevCurrentTime = current_time;
// current_time = arrival->arrival_time;

// //printf("node about to arrive: %d\n", elementQ->totalInSystem);
// elementQ->cumulative_number += (current_time - prevCurrentTime)*elementQ->totalInSystem;
// elementQ->totalInSystem++;



// if(elementQ->first == NULL)
// {
//   elementQ->cumulative_idle_times += current_time-prevCurrentTime;
//   elementQ->firstWaiting = arrival;
//   elementQ->last = arrival;
//   StartService(elementQ);
// }

// else if(elementQ->firstWaiting == NULL)
// {
//   elementQ->firstWaiting = arrival;
//   elementQ->last = arrival;
//   elementQ->waiting_count++;
// }
// else {
//   elementQ->last = arrival;
//   elementQ->waiting_count++;
// }
  elementQ->totalInSystem++;
}

// Function to put patient from priority queue into a room

void StartRoomService(struct Queue* elementQ)
{
  // (elementQ->firstWaiting)->waiting_time = current_time-((elementQ->firstWaiting)->arrival_time);
  // elementQ->cumulative_waiting += (elementQ->firstWaiting)->waiting_time;
  // elementQ->first = elementQ->firstWaiting;

  // if(((elementQ->firstWaiting)->next != NULL) && 
  //     ((elementQ->firstWaiting)->next)->arrival_time < current_time) {
  //     elementQ->firstWaiting = (elementQ->firstWaiting)->next;
  // }
  // else {
  //   elementQ->firstWaiting = NULL;
  // }
  // elementQ->waiting_count--;


}

// Function for when a patient is finished in a room and leaves (adds an event to janitor queue)

void ProcessPatientDeparture(struct Queue* elementQ){

// prevCurrentTime = current_time;
// current_time = (elementQ->first)->arrival_time + (elementQ->first)->service_time + (elementQ->first)->waiting_time;

// //printf("node about to depart: %d\n", elementQ->totalInSystem);
// elementQ->cumulative_number += (current_time - prevCurrentTime)*elementQ->totalInSystem;

// elementQ->totalInSystem--;

// elementQ->cumulative_response += (elementQ->first)->service_time + (elementQ->first)->waiting_time;

// if(elementQ->firstWaiting != NULL) {
//     StartService(elementQ);
//   }
// else
//   {
//     elementQ->first = NULL;
//   }


//TODO: SET JANITOR CLEAN TIME BEFORE CREATING JANITOR EVENT
elementQ->totalInSystem--;
departure_count++;
}

// Called when a janitor has finished cleaning a room

void JanitorCleanedRoom(struct Queue* elementQ) {

}

// This is the main simulator function
// Runs until 24 hours (1440 minutes)
// Determines what the next event is based on current_time
// Print statistics if current time has passed a full hour

void Simulation(struct Queue* elementQ, double lambda, double mu, int print_period, int total_departures)
{
  // while(departure_count != total_departures)
  // {
  //     if((elementQ->first != NULL) && (elementQ->last)->next != NULL
  //     && (((elementQ->first)->arrival_time + (elementQ->first)->service_time + (elementQ->first)->waiting_time)
  //           == (((elementQ->last)->next)->arrival_time))) {
  //             ProcessDeparture(elementQ);
  //             if((elementQ->last)->next != NULL) {
  //               ProcessArrival(elementQ, (elementQ->last)->next);
  //             }
  //               continue;

  //     }



  //     if(elementQ->first != NULL
  //       && ((elementQ->last)->next == NULL || (((elementQ->first)->arrival_time + (elementQ->first)->service_time + (elementQ->first)->waiting_time)
  //         <= (((elementQ->last)->next)->arrival_time))))
  //     {

  //       ProcessDeparture(elementQ);
  //       if(departure_count%print_period == 0)
  //       {
  //           PrintStatistics(elementQ, total_departures, print_period, lambda);
  //       }
  //     }
  //     else
  //     {
  //       if(departure_count == 0 && (elementQ->first == NULL))
  //       {
  //         //printf("TEST");
  //         ProcessArrival(elementQ, elementQ->head);
  //       }
  //       else
  //       {

  //         ProcessArrival(elementQ, (elementQ->last)->next);
  //       }
  //     }
  // }
  //           PrintStatistics(elementQ, total_departures, print_period, lambda);
}

// Free memory allocated for priority queue at the end of simulation

void FreeQueue(struct Queue* elementQ) {
  struct QueueNode* curr;
  while (elementQ->head != NULL) {
    curr = elementQ->head;
    elementQ->head = (elementQ->head)->next;
    free(curr);
  }
  free(elementQ);
}

// Free evaluation queue 

void FreeEvalQueue(struct EvalQueue* elementQ) {
  free(elementQ->nextHighPri);
  free(elementQ->nextLowPri);
  free(elementQ->nextMedPri);
  free(elementQ);
}

// Function to check if input is a number

bool isNumber(char number[]) {
  if(number[0] == 0) {
    return false;
  }
  for(int i = 0; i < strlen(number); i++) {
    if(!isdigit(number[i]) && number[i] != '.') {
      return false;
    }
  }
  return true;
}

// Function to check if input is integer

bool isInteger(char number[]) {
  for(int i = 0; i < strlen(number); i++) {
    if(number[i] == '.') {
      return false;
    }
  }
  return true;
}

// Program's main function

int main(int argc, char* argv[]){


	// input arguments lambda(high pri), lambda(med pri), lambda(low pri), mu(eval), mu(high pri), mu(med pri), mu(low pri), mu(clean), B(max capacity), R(num rooms), 
  // m1(num nurses), m2(num janitors), S(seed)
	if(argc == 14){

		double highPriLambda = atof(argv[1]);
		double medPriLambda = atof(argv[2]);
		double lowPriLambda = atof(argv[3]);
    double evalMu = atof(argv[4]);
    double highPriMu = atof(argv[5]);
    double medPriMu = atof(argv[6]);
    double lowPriMu = atof(argv[7]);
    double cleanMu = atof(argv[8]);
		int maxCapacity = atoi(argv[9]);
    int numRooms = atoi(argv[10]);
    int numNurses = atoi(argv[11]);
    int numJanitors = atoi(argv[12]);
		int random_seed = atoi(argv[13]);

    for(int i = 1; i < 14; i++) {
      if(!isNumber(argv[i])) {
        printf("Provide proper input.\n");
        exit(1);
      }
    }
    for(int i = 9; i < 14; i++) {
      if(!isInteger(argv[i])) {
      printf("Please make sure last three arguments are integers!\n");
      exit(1);
      }
    }

   // Start Simulation
		printf("Simulating Major Hospital Emergency Department with high priority lambda = %f, medium priority lambda = %f, low priority lambda = %f, evaluation mu = %f, high priority mu = %f, medium priority mu - %f, low priority mu = %f, clean mu = %f, max capacity = %d, number of rooms = %d, number of nurses = %d, number of janitors = %d, S = %d\n", highPriLambda, medPriLambda, lowPriLambda, evalMu, highPriMu, medPriMu, lowPriMu, cleanMu, maxCapacity, numRooms, numNurses, numJanitors, random_seed);
		struct EvalQueue* evalQ = InitializeEvalQueue(numNurses, random_seed, highPriLambda, highPriMu, medPriLambda, medPriMu, lowPriLambda, lowPriMu, evalMu);

  //  Simulation(elementQ, lambda, mu, print_period, total_departures);
    FreeEvalQueue(evalQ);
	}
	else printf("Insufficient number of arguments provided!\n");

	return 0;
}
