#ifndef QUEUENODE
#include "QueueNode.h"
#define QUEUENODE
#endif
#ifndef EVENTQUEUE
#include "EventQueue.h"
#define EVENTQUEUE
#endif
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

struct EvalQueue* InitializeEvalQueue(struct EventQueue* eventQ, int numNurses, int seed, double highprilambda, double highprimu, double medprilambda, double medprimu, double lowprilambda, double lowprimu, double evalmu);
void ProcessEvalArrival(struct EventQueue* eventQ, struct EvalQueue* evalQ, struct EventQueueNode* arrival, int seed, double highprilambda, double highprimu, double medprilambda, double medprimu, double lowprilambda, double lowprimu, double evalmu, int maxCapacity, int);
void StartEvaluationService(struct EventQueue* eventQ, struct EvalQueue* evalQ, struct QueueNode* servNode);
void FreeEvalQueue(struct EvalQueue* elementQ);