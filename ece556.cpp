
#include "ece556.h"
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>


using namespace std;
char delim[] = " \t";
int netCount = 0;
int numPins = 0;
int *edgeUtilHistoryArray;
const int BUFFERLENGTH = 256;
bool ripAndReroute = false;
time_t clockOutTime;



char* extractIntFromString(char* input)
{
    char* dest = input;
    char* src = input;

    while(*src)
    {
        if (!isdigit(*src)) { src++; continue; }
        *dest++ = *src++;
    }
    *dest = '\0';
    return input;
}
/**method used to parse the given line of the file
 * and put the data in the struct
**/
void processData(routingInst *rst, char *buffer, FILE *fp){
    int lineNumber;

    for(lineNumber = 1; lineNumber <= 3; lineNumber = lineNumber + 1) {

        fgets(buffer,BUFFERLENGTH,fp);

        if(lineNumber == 1){
            int i = 0;
            //need to read the two integers for the grid size and store
            char *ptr = strtok(buffer, delim);

            // break up the string by spaces and set the gx and gy
            while(ptr != NULL) {
                if(i == 1) {
                    rst->gx = atoi(ptr);
                }
                if(i == 2) {
                    rst->gy = atoi(ptr);
                }

                i++;

                ptr = strtok(NULL, delim);
            }

            free(ptr);

            //printf("Gx: %d Gy: %d \n", rst->gx, rst->gy);
        }

        if(lineNumber == 2){
            //need to read the value of capacity and store
            int i = 0;
            //need to read the capacity
            char *ptr = strtok(buffer, delim);

            // break up the string by spaces and set the capacity
            while(ptr != NULL) {
                if(i == 1) {
                    rst->cap = atoi(ptr);
                }

                i++;

                ptr = strtok(NULL, delim);
            }

            free(ptr);

            //printf("Capacity: %d \n", rst->cap);
        }

        if(lineNumber == 3){
            //need to read the value of number of nets and store
            int i = 0;
            //need to read the number of nets
            char *ptr = strtok(buffer, delim);

            // break up the string by spaces and set the number of nets
            while(ptr != NULL) {
                if(i == 2) {
                    rst->numNets = atoi(ptr);
                }

                i++;

                ptr = strtok(NULL, delim);
            }

            free(ptr);

            //printf("Number of Nets: %d \n", rst->numNets);
        }
    }
}

// Processes the various nets
void processNetsAndPins(routingInst *rst, char *buffer, FILE *fp){
    int netIterator;
    net *nets = (net *)(malloc(sizeof(net) * rst->numNets));

    for(netIterator = 0; netIterator < rst->numNets; netIterator++) {
        //printf("Next Net: %d", netCount);

        fgets(buffer,BUFFERLENGTH,fp);

        // Check if it is the start of a net
        if(buffer[0] == 'n') {
            // if it is, pull the length of the net to know how long to loop
            int i = 0;
            char *ptr = strtok(buffer, delim);

            // Pull the number of pins
            while(ptr != NULL) {
                if(i == 1) {
                    numPins = atoi(ptr);
                }

                i++;

                ptr = strtok(NULL, delim);
            }

            // Create a new net instance
            net *currNet = new net;
            currNet->id = netCount;
            currNet->numPins = numPins;
            // Make the pin array
            point *pins = (point *)malloc(sizeof(point) * numPins);

            // Populate the pin array
            int pinIterator;
            for(pinIterator = 0; pinIterator < numPins; pinIterator++) {
                fgets(buffer,BUFFERLENGTH,fp);
                int i = 0;

                //need to read the values for the pin
                char *ptr = strtok(buffer, delim);

                // break up the string by spaces
                while(ptr != NULL) {
                    if(i == 0) {
                        pins[pinIterator].x = atoi(ptr);
                    }
                    if(i == 1) {
                        pins[pinIterator].y = atoi(ptr);
                    }

                    i++;

                    ptr = strtok(NULL, delim);
                }
            }

            // Set the pin array to the net
            currNet->pins = pins;

            nets[netIterator] = *currNet;

            free(ptr);

            //printf("Net ID: %d Num Pins: %d \n", netCount, numPins);

            // Increment the global net count
            netCount++;

        }
    }

    // Set the net array to all the nets
    rst->nets = nets;
}

// Translates two points to an edge
int calculateEdge(int pointx1, int pointy1, int pointx2, int pointy2, routingInst *rst) {
    // Horizontal Edge
    if(pointy1 == pointy2) {
        int minx = (pointx1 < pointx2) ? pointx1 : pointx2;
        return (minx + (pointy1 *(rst->gx - 1)));
    }

        // Vertical Edge
    else {
        int miny = (pointy1 < pointy2) ? pointy1 : pointy2;
        return (pointx1 + (rst->gx * miny + ((rst->gx - 1) * rst->gy)));
    }
}
// This is the method that is fucking everything up
void processBlockages(routingInst *rst, char *buffer, FILE *fp) {
    int numBlocks; // total number of blockages
    int blockIterator;
    int pointx1; // holds the x1 for the blockage
    int pointy1; // holds the y1 for the blockage
    int pointx2; // holds the x2 for the blockage
    int pointy2; // holds the y2 for the blockage
    int blockageValue; // holds the blockage value
    int *edgeCapArray = (int *)malloc(sizeof(int) * rst->numEdges); // Edge capacity array
    //Create the edgeUtilArray and then initialize the edge weights to zero
    int *edgeUtilArray = (int *)malloc(sizeof(int) * rst->numEdges);
    int numEdges = rst->numEdges;
    //Need to initialize all of the edge utils to zero
    for(int i = 0; i < numEdges; i++){
        edgeUtilArray[i] = 0;
    }
    rst->edgeUtils = edgeUtilArray;
    //Now initialize the global history array so it can be used for edge weight calculation
    edgeUtilHistoryArray = (int *)malloc(sizeof(int) * rst->numEdges);
    //Need to initialize all of the edge utils to one for the history array
    for(int i = 0; i < numEdges; i++){
        edgeUtilHistoryArray[i] = 1;
    }

    int arrayIterator;
    int calculated;

    // Get the number of blockages to know how many times to loop
    fgets(buffer,BUFFERLENGTH,fp);
    numBlocks = atoi(buffer);

    //printf("Num Blockages: %d Normal Edge Capacity: %d\n", numBlocks, rst->cap);

    // Populate the edgeCapacities
    for(arrayIterator = 0; arrayIterator < rst->numEdges; arrayIterator++) {
        // Set every spot in the array to the edge capacitance
        edgeCapArray[arrayIterator] = rst->cap;
    }

    // Modify the array spots where edge capacitance is different due to blocking
    for(blockIterator = 0; blockIterator < numBlocks; blockIterator++) {
        //need to read the values for the blockage location
        fgets(buffer,BUFFERLENGTH,fp);

        // Get the X1 coordinate of current blockage
        char *ptr = strtok(buffer, delim);
        pointx1 = atoi(ptr);

        // Get the Y1 coordinate of current blockage
        ptr = strtok(NULL, delim);
        pointy1 = atoi(ptr);

        // Get the X2 coordinate of current blockage
        ptr = strtok(NULL, delim);
        pointx2 = atoi(ptr);

        // Get the Y2 coordinate of current blockage
        ptr = strtok(NULL, delim);
        pointy2 = atoi(ptr);

        // Get the new capacity of current blockage
        ptr = strtok(NULL, delim);
        blockageValue = atoi(ptr);

        //printf("X: %d Y: %d X2: %d Y2: %d Blockage: %d \n", pointx1, pointy1, pointx2, pointy2, blockageValue);

        calculated = calculateEdge(pointx1, pointy1, pointx2, pointy2, rst);

        //printf("Calculated Edge %d: %d \n", blockIterator + 1, calculated);


        // Set the update capacity in the edge capacitance array
        edgeCapArray[calculated] = blockageValue;
    }

    rst->edgeCaps = edgeCapArray;

}


int readBenchmark(const char *fileName, routingInst *rst) {
    /*********** TO BE FILLED BY YOU **********/
    FILE *fp;

    char buffer[BUFFERLENGTH];

    clockOutTime = (clock()/CLOCKS_PER_SEC) + (5 * 60);

    fp = fopen(fileName,"r");

    if(fp == NULL){
        return 0;
    }

    // Parse in the input file
    processData(rst, buffer, fp);
    processNetsAndPins(rst, buffer, fp);

    // Calculate num edges
    rst->numEdges = (rst->gy * (rst->gx - 1)) + (rst->gx * (rst->gy - 1));

    // Parse in the edge blockages
    processBlockages(rst, buffer, fp);

    return 1;
}

int compare(const void *p1, const void *p2){
    point *point1 = (point*)p1;
    point *point2 = (point*)p2;
    return (point1->x - point2->x) == 0 ? point1->y - point2->y : point1->x - point2->x;
}
void netDecomposition(routingInst *rst){
    //loop through all of the nets
    for(int i = 0; i < rst->numNets; i++){
        //want to order the pins so that they ones that are closer together are routed first
        //first need to sort the pins by their x value
        point* pinArray = rst->nets[i].pins;
        qsort(pinArray, rst->nets[i].numPins, sizeof(point), compare);

//        //Testing that the pin ordering works
//        for(int j = 0; j < rst->nets[i].numPins; j++){
//            printf("(%d %d), ", rst->nets[i].pins[j].x,rst->nets[i].pins[j].y);
//        }
//        printf("\n");


    }
}
int computeEdgeWeight(routingInst *rst, int edgeNumber, int edgeWeight){
    edgeWeight = max((rst->edgeUtils[edgeNumber] - rst->edgeCaps[edgeNumber]), 0) * edgeUtilHistoryArray[edgeNumber];
    return edgeWeight;
}
void calculateNetOrdering(routingInst *rst) {
    int i;
    int weight;
    int currEdgeWeight;

    for(i = 0; i < rst->numNets; i++) {
        // Initialize the weight to zero and get the current net and route of the net
        weight = 0;
        net currNet = rst->nets[i];
        route currRoute = currNet.nroute;

        // Loop through the nets segments
        int j;
        for(j = 0; j < currRoute.numSegs; ++j) {
            segment currSegment = currRoute.segments[j];

            // Loop through the segments edges
            int k;
            for(k = 0; k < currSegment.numEdges; k++) {
                int currEdge = currSegment.edges[k];
                // Calculate the weight of the given edge
                // NOTE: edgeHist is for the not yet created edges history array
                currEdgeWeight = computeEdgeWeight(rst, currEdge, currEdgeWeight);

                // Sum the current edge weight with the weight of the net so far to get updated weight
                weight = weight + currEdgeWeight;
            }
        }

        // Update the net with the calculated weight
        rst->nets[i].cost = weight;
    }

    // BubbleSort the NetArray so the largest cost is at the front
    int holder = sizeof(rst->nets)/sizeof(rst->nets[0]);
    int y, z;

    for(y = 0; y < holder - 1; y++) {
        for(z = 0; z < holder - y - 1; z++) {
            // Check if a swap needs to occur
            if(rst->nets[z].cost < rst->nets[z+1].cost) {
                net temp = rst->nets[z];
                rst->nets[z] = rst->nets[z+1];
                rst->nets[z+1] = temp;
            }
        }
    }
}



void createAllEdges(point point1, point point2, int netNum, int segNum, routingInst *rst){
    point temp = point1;
    point next = temp;
    int edgeIndex = 0; //initialize the index for each segment
    int edgeNum;
    //horizontal edges
    while(temp.x != point2.x){
        if(temp.x > point2.x){
            next.x--;
        }else{
            next.x++;
        }
        edgeNum = calculateEdge(temp.x, temp.y, next.x, next.y, rst);
        rst->nets[netNum].nroute.segments[segNum].edges[edgeIndex] = edgeNum;
        rst->edgeUtils[edgeNum]++; //increment the edge utilization
        temp = next;
        edgeIndex++;
    }
    //vertical edges
    while(temp.y != point2.y){
        if(temp.y > point2.y){
            next.y--;
        }else{
            next.y++;
        }
        edgeNum = calculateEdge(temp.x, temp.y, next.x, next.y, rst);
        rst->nets[netNum].nroute.segments[segNum].edges[edgeIndex] = edgeNum;
        rst->edgeUtils[edgeNum]++;
        edgeIndex++;
        temp = next;
    }
}
void ripUp(routingInst *rst, int netNumber) {
    net* currNet = &rst->nets[netNumber];

    // Get the route to rip up
    route* toRipUp = &currNet->nroute;

    // Iterate through all of the segments and edges in that route
    int i;
    for(i = 0; i < toRipUp->numSegs; i++) {
        segment* currSeg = &toRipUp->segments[i];
        int j;
        for(j = 0; j < currSeg->numEdges; j++) {
            int currEdge = currSeg->edges[j];

            // Decrease the edgeUtilization of the edge being removed
            rst->edgeUtils[currEdge] = rst->edgeUtils[currEdge] - 1;
        }
        currSeg->numEdges = 0;
    }
    toRipUp->numSegs = 0;
}
dijkPair* createPair(int parentX, int parentY, int currNodeX, int currNodeY, int sumofWeights) {
    dijkPair* dijk = (dijkPair*) malloc(sizeof(dijkPair));
    dijk->parentX = parentX;
    dijk->parentY = parentY;
    dijk->currNodeX = currNodeX;
    dijk->currNodeY = currNodeY;
    dijk->sumOfWeights = sumofWeights;
    return dijk;
}

/// Stuff for Queue
Queue* createQueue(unsigned capacity)
{
    Queue* queue = (Queue*) malloc(sizeof(Queue));
    queue->capacity = capacity;
    queue->front = queue->size = 0;
    queue->rear = capacity - 1;
    queue->array = (dijkPair*) malloc(queue->capacity * sizeof(dijkPair));
    return queue;
}

// Queue is full when size becomes equal to the capacity
int isFull(Queue* queue)
{  return (queue->size == queue->capacity);  }

// Queue is empty when size is 0
int isEmpty(Queue* queue)
{  return (queue->size == 0); }

// Function to add an item to the queue.
// It changes rear and size
void enqueue(Queue* queue, dijkPair item)
{
    if (isFull(queue))
        return;
    queue->rear = (queue->rear + 1)%queue->capacity;
    queue->array[queue->rear] = item;
    queue->size = queue->size + 1;
    //printf("%d enqueued to queue\n", item);
}

// Function to remove an item from queue.
// It changes front and size
dijkPair dequeue(Queue* queue)
{
    dijkPair item = queue->array[queue->front];
    queue->front = (queue->front + 1)%queue->capacity;
    queue->size = queue->size - 1;
    return item;
}

// Function to get front of queue
dijkPair front(Queue* queue)
{
    return queue->array[queue->front];
}

// Extract the member in the queue with the lowest weight
dijkPair* extractMin(Queue* queue) {
    int i;
    // Start with the dijkPair at the front
    dijkPair* toReturn = &queue->array[queue->front];

    // Loop through the dijkPair array and find the pair with the lowest
    // weight value
    i = queue->front + 1;
    while(i != (queue->rear + 1)%queue->capacity) {
        if(queue->array[i].sumOfWeights < toReturn->sumOfWeights) {
            toReturn = &queue->array[i];
        }
        i = (i + 1)%queue->capacity;
    }

    return toReturn;

}

// Check if the current dijkPair has a member with the same position in Q2 or Q3
int notSecondField(dijkPair* currPair, Queue* q2, Queue* q3) {
    int i;

    int currX = currPair->currNodeX;
    int currY = currPair->currNodeY;

    // Check q2 for the same vertex
    i = q2->front;
    while(i != (q2->rear + 1)%q2->capacity) {
        dijkPair* comparePair = &q2->array[i];
        if(comparePair->currNodeX == currX && comparePair->currNodeY == currY) {
            // If there is somethin in the queue at the same current vertex, return 0
            return 0;
        }
        i = (i + 1) % q2->capacity;
    }

    i = q3->front;
    // Check q3 for the same vertex
    while(i != (q3->rear + 1)%q3->capacity) {
        dijkPair* comparePair = &q3->array[i];
        if(comparePair->currNodeX == currX && comparePair->currNodeY == currY) {
            // If there is somethin in the queue at the same current vertex, return 0
            return 0;
        }
        i = (i + 1) % q3->capacity;
    }

    // If a match isn't found, return 1 indicating no match was found
    return 1;

}

// Check if the current dijkPair has a member with the same position in Q3
int notSecondFieldQ3(dijkPair* currPair, Queue* q3) {
    int i;

    int currX = currPair->currNodeX;
    int currY = currPair->currNodeY;

    // Check q3 for the same vertex
    i = q3->front;
    while(i != (q3->rear + 1)%q3->capacity) {
        dijkPair* comparePair = &q3->array[i];
        if(comparePair->currNodeX == currX && comparePair->currNodeY == currY) {
            // If there is somethin in the queue at the same current vertex, return 0
            return 0;
        }
        i = (i + 1) % q3->capacity;
    }

    // If a match isn't found, return 1 indicating no match was found
    return 1;
}

// Retrace the path and place the new path for the net into the nets array
segment* retrace(Queue* q3, routingInst *rst, dijkPair* currPair, point source, point destination) {
    int i;
    int numEdges = 0;
    int *edgeArray1 = (int *) malloc(sizeof(int) * ((rst->gx - 1) + (rst->gy - 1)));
    int *edgeArray2 = (int *) malloc(sizeof(int) * ((rst->gx - 1) + (rst->gy - 1)));

    // Retrace back through the queue, computing the edges and adding them to an
    // edge array
    i = q3->rear;
    while(i != (q3->front - 1)%q3->capacity) {
        dijkPair* comparePair = &q3->array[i];
        // If this is true this is an edge
        if(comparePair->currNodeX == currPair->parentX && comparePair->currNodeY == currPair->parentY) {
            int edge = calculateEdge(comparePair->currNodeX, comparePair->currNodeY, currPair->currNodeX, currPair->currNodeY, rst);
            // Increment the edge utilization
            rst->edgeUtils[edge] = rst->edgeUtils[edge] + 1;

            // Add the edge to the edge array
            edgeArray1[numEdges] = edge;
            numEdges = numEdges + 1;

            currPair = comparePair;

        }

        i = (i - 1) % q3->capacity;
    }

    int j = 0;
    // The edge array is in reverse order so put it in the correct order
    for(i = numEdges - 1; i >= 0; --i) {
        edgeArray2[j] = edgeArray1[i];
        j = j + 1;
    }

    segment* toReturn = new segment;
    //Add the edges to the segment
    toReturn->edges = edgeArray2;
    //Set the start and the end pins
    toReturn->p1 = source;
    toReturn->p2 = destination;
    toReturn->numEdges = numEdges;

    return toReturn;
}

// This method compares the weight of the current pair with the pair already in Q2 to see if the weight is greater
dijkPair* checkWeightQ2(dijkPair* currPair, Queue* q2) {
    int i;

    int currX = currPair->currNodeX;
    int currY = currPair->currNodeY;

    // Check q2 for the same vertex
    i = q2->front;
    while(i != (q2->rear + 1)%q2->capacity) {
        dijkPair* comparePair = &q2->array[i];
        if(comparePair->currNodeX == currX && comparePair->currNodeY == currY) {
            // Return pair with same vertex
            return comparePair;
        }
        i = (i + 1) % q2->capacity;
    }
}

// This method removes the given dijkPair from the queue
void removeFromQueue(dijkPair* minPair, Queue* q2) {
    int i;
    int endPoint;

    int currX = minPair->currNodeX;
    int currY = minPair->currNodeY;

    //

    // Check q2 for the same vertex
    i = q2->front;
    endPoint = (q2->rear + 1)%q2->capacity;
    while(i != endPoint) {
        dijkPair comparePair = dequeue(q2);
        // If it is the pair to be removed don't re enqueue it
        if(comparePair.currNodeX == currX && comparePair.currNodeY == currY) {
            i = (i + 1) % q2->capacity;
            continue;
        }
        enqueue(q2, comparePair);
        i = (i + 1) % q2->capacity;
    }
}


// Dijkstras
void reroute(routingInst *rst, int netNumber) {
    net* currNet = &rst->nets[netNumber];

    // Loop through the pins and connect each one
    int j;
    for(j = 0; j < currNet->numPins - 1; ++j) {
       // printf("Pin Set: %d\n", j);
        point source = currNet->pins[j];
        point destination = currNet->pins[j+1];
		
		// Set the edges for the bounding box
		int boxMinX = (source.x < destination.x) ? source.x : destination.x;
        int boxMaxX = (source.x < destination.x) ? destination.x : source.x;
        int boxMinY = (source.y < destination.y) ? source.y : destination.y;
        int boxMaxY = (source.y < destination.y) ? destination.y : source.y;

        Queue *q2 = createQueue(rst->gx * rst->gy);
        Queue *q3 = createQueue(rst->gx * rst->gy);

        // -1, -1 as parent represents source, initial weight is 0
        dijkPair *sourcePair = createPair(-1, -1, source.x, source.y, 0);

        // Enqueue the source pair
        enqueue(q2, *sourcePair);

        // Loop through Q2 while it is not empty
        while(isEmpty(q2) == 0) {
            // Extract the minimum weight valued dijkPair
            dijkPair *minPair = extractMin(q2);
            // Remove the node from the queue
            removeFromQueue(minPair, q2);

            int edgeNumber, edgeWeight;

            // Check if the min pair is the destination
            if(destination.x == minPair->currNodeX && destination.y == minPair->currNodeY) {
                // Make a new segment from the retrace, then give it to the next
                segment* newSeg = retrace(q3, rst, minPair, source, destination);

                currNet->nroute.segments[j] = *newSeg;
                currNet->nroute.numSegs = currNet->nroute.numSegs + 1;
                break;
            }
                // If it isn't the destination, find all adjacent members of currentNode
            else {
                // Get each adjacent node
                int i;
                for(i = 0; i < 4; ++i) {
                    dijkPair *adjacentPair;
                    // Up
                    if(i == 0) {
                       // No adjacent edge, on border or top of bounding box
                        if(minPair->currNodeY == rst->gy || minPair->currNodeY > boxMaxY) {
                            continue;
                        }

                        // Find the edge number between the two
                        edgeNumber = calculateEdge(minPair->currNodeX, minPair->currNodeY, minPair->currNodeX, minPair->currNodeY + 1, rst);

                        // Calculate the edge weight
                        edgeWeight = computeEdgeWeight(rst, edgeNumber, edgeWeight) + abs(minPair->currNodeX - destination.x) + abs(minPair->currNodeY + 1 - destination.y);

                        // Up adjacent edge exists, make new pair
                        adjacentPair = createPair(minPair->currNodeX, minPair->currNodeY, minPair->currNodeX, minPair->currNodeY + 1, minPair->sumOfWeights + edgeWeight);
                    }
                        // Down
                    else if(i == 1) {
                        // No adjacent edge, on border or bottom of bounding box
                        if(minPair->currNodeY == 0 || minPair->currNodeY < boxMinY) {
                            continue;
                        }

                        // Find the edge number between the two
                        edgeNumber = calculateEdge(minPair->currNodeX, minPair->currNodeY, minPair->currNodeX, minPair->currNodeY - 1, rst);

                        // Calculate the edge weight
                        edgeWeight = computeEdgeWeight(rst, edgeNumber, edgeWeight) + abs(minPair->currNodeX - destination.x) + abs(minPair->currNodeY - 1 - destination.y);

                        // Up adjacent edge exists, make new pair
                        adjacentPair = createPair(minPair->currNodeX, minPair->currNodeY, minPair->currNodeX, minPair->currNodeY - 1, minPair->sumOfWeights + edgeWeight);
                    }
                        // Left
                    else if(i == 2) {
                        // No adjacent edge, on border or left side of bounding box
                        if(minPair->currNodeX == 0 || minPair->currNodeX < boxMinX) {
                            continue;
                        }

                        // Find the edge number between the two
                        edgeNumber = calculateEdge(minPair->currNodeX, minPair->currNodeY, minPair->currNodeX - 1, minPair->currNodeY, rst);

                        // Calculate the edge weight
                        edgeWeight = computeEdgeWeight(rst, edgeNumber, edgeWeight) + abs(minPair->currNodeX - 1 - destination.x) + abs(minPair->currNodeY - destination.y);

                        // Up adjacent edge exists, make new pair
                        adjacentPair = createPair(minPair->currNodeX, minPair->currNodeY, minPair->currNodeX - 1, minPair->currNodeY, minPair->sumOfWeights + edgeWeight);
                    }
                        // Right
                    else {
                        // No adjacent edge, on border or right side of bounding box
                        if(minPair->currNodeX == rst->gx || minPair->currNodeX > boxMaxX) {
                            continue;
                        }

                        // Find the edge number between the two
                        edgeNumber = calculateEdge(minPair->currNodeX, minPair->currNodeY, minPair->currNodeX + 1, minPair->currNodeY, rst);

                        // Calculate the edge weight
                        edgeWeight = computeEdgeWeight(rst, edgeNumber, edgeWeight) + abs(minPair->currNodeX + 1 - destination.x) + abs(minPair->currNodeY - destination.y);

                        // Up adjacent edge exists, make new pair
                        adjacentPair = createPair(minPair->currNodeX, minPair->currNodeY, minPair->currNodeX + 1, minPair->currNodeY, minPair->sumOfWeights + edgeWeight);
                    }


                    // Check if v is not second field of any entry in Q2 and Q3, i.e. never been placed
                    if(notSecondField(adjacentPair, q2, q3) == 1) {
                        enqueue(q2, *adjacentPair);
                    }
                    else {
                        // Check if v is in the second field of any entry in Q3
                        if(notSecondFieldQ3(adjacentPair, q3)) {
                            // Check if another pair at same currnet vertex in Q2, if there is and
                            // its weight is less then the current pair delete it and enqueue current
                            dijkPair* position;
                            position = checkWeightQ2(adjacentPair, q2);
                            if(position->sumOfWeights >= adjacentPair->sumOfWeights) {
                                // Delete old thing from Q2, this is wrong
                                removeFromQueue(position, q2);

                                // Enqueue pair with better weight
                                enqueue(q2, *adjacentPair);
                            }
                        }
                    }
                }

                // Place minPair into q3
                enqueue(q3, *minPair);

            }

        }

    }

}

/**
 * Method that creates edges, segments, a route of all of the nets/pins
 * @param rst
 * @return
 */
int solveRouting(routingInst *rst, int d, int n) {
    //if d = 1, n = 0, apply net decomposition but no net ordering or rip up and reroute rip
    if(d == 1) {
        netDecomposition(rst);
    }

    if(n == 1){
        calculateNetOrdering(rst);
        ripAndReroute = true;
    }
    // Iterate through the nets
    for (int netNum = 0; netNum < rst->numNets; netNum++) {
        int numSegs = rst->nets[netNum].numPins - 1;
        rst->nets[netNum].nroute.numSegs = numSegs;
        rst->nets[netNum].nroute.segments = (segment *) malloc(sizeof(segment) * (numSegs));
        for (int segNum = 0; segNum < numSegs; segNum++) {
            int numEdges = 0; //initialize the number of edges
            point pin1 = rst->nets[netNum].pins[segNum]; //get the first pin
            point pin2 = rst->nets[netNum].pins[segNum + 1]; //get the second pin

            //create the segment
            rst->nets[netNum].nroute.segments[segNum].p1 = pin1;
            rst->nets[netNum].nroute.segments[segNum].p2 = pin2;

            numEdges = abs(pin1.x - pin2.x) + abs(pin1.y - pin2.y);

            rst->nets[netNum].nroute.segments[segNum].numEdges = numEdges;
            rst->nets[netNum].nroute.segments[segNum].edges = (int *)malloc(sizeof(int) * numEdges);
            createAllEdges(pin1, pin2, netNum, segNum, rst);
        }
    }
    if(ripAndReroute == true){
        for(int net = 0; net < rst->numNets; net++) {
            if(clockOutTime < (clock()/CLOCKS_PER_SEC)) {
                break;
            }
            printf("Net: %d \n", net);
			//if(net == 33818) {
			//	continue;
			//}
            ripUp(rst, rst->nets[net].id);
            reroute(rst, rst->nets[net].id);
        }

    }
    return 1;
}



point calculateSecondPointFromEdge(point point1, int edge, routingInst *rst) {
    //It is a horizontal edge
    if (edge < ((rst->gx - 1) * rst->gy)) {
        point xLower = {point1.x - 1, point1.y};
        point xHigher = {point1.x + 1, point1.y};
        //Check if this is the actual edge
        if (calculateEdge(point1.x, point1.y, xLower.x, xLower.y, rst) == edge) {
            return xLower;
        } else if (calculateEdge(point1.x, point1.y, xHigher.x, xHigher.y, rst) == edge) {
            return xHigher;
        }
    } else {
        //vertical edge
        point yLower = {point1.x, point1.y - 1};
        point yHigher = {point1.x, point1.y + 1};
        //Check if it the actual edge
        if (calculateEdge(point1.x, point1.y, yLower.x, yLower.y, rst) == edge) {
            return yLower;
        } else if (calculateEdge(point1.x, point1.y, yHigher.x, yHigher.y, rst) == edge) {
            return yHigher;
        }
    }
}



int writeOutput(const char *outRouteFile, routingInst *rst) {
    /*********** TO BE FILLED BY YOU **********/
    FILE *fp;

    fp = fopen(outRouteFile, "w");

    if (fp == NULL) {
        return 0;
    }

    //Loop through each net
    for (int net = 0; net < rst->numNets; net++) {
        fprintf(fp, "n%d\n", rst->nets[net].id);
        int numSegs = rst->nets[net].nroute.numSegs;
        for (int seg = 0; seg < numSegs; seg++) {
            int numEdges = rst->nets[net].nroute.segments[seg].numEdges;
            //Need to do the first iteration outside of the for loop
            point startPoint = rst->nets[net].nroute.segments[seg].p1;
            int *edges = rst->nets[net].nroute.segments[seg].edges;
            int prevEdge = edges[0];
            point endPoint = calculateSecondPointFromEdge(startPoint, prevEdge, rst);

            int currEdge = 0;
            int edgeDifference = 0;
            //Need to move through the edges and print the points of both sides of the edge when the edges change directions
            for (int i = 1; i < numEdges; i++) {
                //printf("i : %d endPoint: (%d,%d)\n", i, endPoint.x, endPoint.y);
                currEdge = edges[i];
                edgeDifference = abs(currEdge - prevEdge);
				
                //If it is a bend then print out the startpoint and the endpoint then make the startpoint the prev endpoint
                if (edgeDifference != 1 && edgeDifference != rst->gx) {
                    fprintf(fp, "(%d,%d)-(%d,%d)\n", startPoint.x, startPoint.y, endPoint.x, endPoint.y);
                    startPoint = endPoint;
                }
                //it is not a bend so don't move the start point but do increase the endpoint
                endPoint = calculateSecondPointFromEdge(endPoint, currEdge, rst);
                prevEdge = currEdge;
            }
            fprintf(fp, "(%d,%d)-(%d,%d)\n", startPoint.x, startPoint.y, endPoint.x, endPoint.y);
        }
        fprintf(fp,"!\n");
    }
 return 1;
}

int release(routingInst *rst) {
    // Loop through the routing instance and free all the pointers
    int i;
    int j;
    for (i = 0; i < rst->numNets; i++) {
        net currNet = rst->nets[i];

        // Free the array of points
        free(currNet.pins);

        route currRoute = currNet.nroute;

        // Free the edges array for each segment
        for (j = 0; j < currRoute.numSegs; j++) {
            segment currSegment = currRoute.segments[j];
            free(currSegment.edges);
        }

        // Free the segments array for curr net
        free(currRoute.segments);
    }

    // Free the nets array
    free(rst->nets);

    // Free the edge capacity and utilization arrays
    free(rst->edgeCaps);
    free(rst->edgeUtils);
    free(edgeUtilHistoryArray);

    // Free the routing instance
    free(rst);

    return 1;
}


