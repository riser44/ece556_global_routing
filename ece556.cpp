// ECE556 - Copyright 2014 University of Wisconsin-Madison.  All Rights Reserved.
#define LINE_LIMIT 500


#include "ece556.h"
#include <iostream>
#include <fstream>		//For all File I/O operations
#include <string.h>
#include <stdlib.h>
#include <cmath>
#include <bits/stdc++.h>
#include <time.h>
using namespace std; 

int *edgeGlobalUtilHistory;
int ripup_and_reroute=0;
time_t clock_timeout;


//Filtering the args before converting to integer 
//char* extractIntFromString(char* input)
//{
//    char* dest = input;
//    char* src = input;
//
//    while(*src)
//    {
//        if (!isdigit(*src)) { src++; continue; }
//        *dest++ = *src++;
//    }
//    *dest = '\0';
//    return input;
//}

int readBenchmark(const char *fileName, routingInst *rst){
  /*********** TO BE FILLED BY YOU **********/  
	clock_timeout = (clock()/CLOCKS_PER_SEC) + (15 * 60);
	FILE *ip_file;
	char *line;
	char *token;
	line = new char [LINE_LIMIT]();
	//Read the input file into an array
	ip_file = fopen (fileName, "r");
	
	//Read from the first line
	fgets(line, LINE_LIMIT, ip_file);
	
	//Using strtok to split the line when there is space new line or tab spacing
	token = strtok(line, " \n\t");
	//Need the second token which contains value of gx 
	token = strtok(NULL, " \n\t");
	//Convert string to int and assign it to gx
	rst->gx = atoi(token);
	
	//Moving to the third token which contains gy
	token = strtok(NULL, " \n\t");
	//Convert string to int and assign it to gy
	rst->gy = atoi(token);
	
	//Read from the second line
	fgets(line, LINE_LIMIT, ip_file);	
	
	//Using strtok to split the line when there is space new line or tab spacing
	token = strtok(line, " \n\t");
	//Need the second token which contains value of default edge capacity
	token = strtok(NULL, " \n\t");	
	//Convert string to int and assign it to capacity field in routing instance
	rst->cap = atoi(token);

	//Read from the third line
	fgets(line, LINE_LIMIT, ip_file);	
	
	//Using strtok to split the line when there is space new line or tab spacing
	token = strtok(line, " \n\t");
	//Need the third token which contains value of number of nets
	token = strtok(NULL, " \n\t");	
	token = strtok(NULL, " \n\t");
	//Convert string to int and assign it to numNets in routing instance	
	rst->numNets = atoi(token);

	//Create array of nets based on number of nets above
	rst->nets = new net[rst->numNets];

	//Parsing all the nets present in the file
	for(int net_index=0; net_index<(rst->numNets); net_index++)
	{
		//Read the first line under nets which contains net ID and number of pins in the net
		fgets(line, LINE_LIMIT, ip_file);
		//Using strtok to split the line while using 'n' to separate out the net index
		token = strtok(line, " n\n\t");
		//Assign net ID to the id field in nets structure
		rst->nets[net_index].id = atoi(token);
		//DEBUG printf("id %d\n", rst->nets[net_index].id);
		//Extracting the second token which contains number of pins in the net
		token = strtok(NULL, " \n\t");
		//Assign number of pins to the numPins field of nets structure
		rst->nets[net_index].numPins = atoi(token);
		//DEBUG printf("total pins %d\n", rst->nets[net_index].numPins);
		//Create array of pins based on the number of pins above
		rst->nets[net_index].pins = new point[rst->nets[net_index].numPins]();
		//Parsing all the pins for the given net
		for(int pin_index=0; pin_index<(rst->nets[net_index].numPins); pin_index++)
		{
			//Start reading the pins one by one
			fgets(line, LINE_LIMIT, ip_file);
			//Reading the X coordinate of the pin
			token = strtok(line, " \n\t");	
			rst->nets[net_index].pins[pin_index].x = atoi(token);
			//Reading the Y coordinate of the pin
			token = strtok(NULL, " \n\t");	
			rst->nets[net_index].pins[pin_index].y = atoi(token);
			//DEBUG printf("%d \n", rst->nets[net_index].pins[pin_index].y);		
		}
	}
	
	//Read from line containing the number of blockages
	fgets(line, LINE_LIMIT, ip_file);		
	//Using strtok to separate out number of blockages
	token = strtok(line, "\n\t");
	int blockages = atoi(token);
	//Create array to store x coordinates of 2 pins associated with edges that have different capacity
	int *block_x = new int[2*blockages]; //Check //*block_x, *(block_x+1) store the x co ordinates of the edge in question
	//Create array to store y coordinates of 2 pins associated with edges that have different capacity
	int *block_y = new int[2*blockages]; //Check //*block_y, *(block_y+1) store the y co ordinates of the edge in question
	//Parse the updated value of capacity for the edge
	int *new_cap = new int [blockages]; // Check
	
	for(int i=0; i<blockages; i++)
	{
			//Start reading the line containing the pins and new capacity information 
			fgets(line, LINE_LIMIT, ip_file);	
			//Parse the x co-ordinate of the first pin and store in the array created above
			token = strtok(line, " \n\t");	
			*(block_x + i*2) = atoi(token);
			
			//Parse the y co-ordinate of the first pin and store in the array created above
			token = strtok(NULL, " \n\t");	
			*(block_y + i*2) = atoi(token);
			
			//Parse the x co-ordinate of the second pin and store in the array created above
			token = strtok(NULL, " \n\t");	
			*(block_x + i*2 + 1) = atoi(token);
			
			//Parse the y co-ordinate of the second pin and store in the array created above
			token = strtok(NULL, " \n\t");	
			*(block_y + i*2 + 1) = atoi(token);
			
			//Store the updated capacity of the edge
			token = strtok(NULL, " \n\t");
			*(new_cap + i) = atoi(token);
	}
	
	//Total edges = gx*(gy-1) + gy*(gx-1)
	rst->numEdges = ((rst->gy) * ((rst->gx)-1)) + ((rst->gx)*((rst->gy)-1));

//BEGIN: Check if its needed for 1st milestone 
	
	//Create array for storing the capacity of each edge
	rst->edgeCaps = new int [rst->numEdges];
	
	//Create array for storing the utilization of each edge
	rst->edgeUtils = new int [rst->numEdges];

	//Array to store the global history of utilization of an edge to calculate edge weight MS2
	edgeGlobalUtilHistory = new int [rst->numEdges];

	//Setting default capacity of all the edges
	for(int j=0; j<(rst->numEdges); j++)
	{
		*(rst->edgeCaps + j) = rst->cap;
	}
	
	//Setting the global history of all the edges to one MS2
	for(int m=0; m<(rst->numEdges); m++)
	{
		edgeGlobalUtilHistory[m] = 1;

	}

	//Overriding the default capacities with new values
	for(int k=0; k<blockages; k++)
	{
		//Check if the given edge is a row
		if(*(block_y + k*2) == *(block_y + k*2 + 1))
		{
			//Check if the pins are aligned from left to right and always use the x value of the pin on the left
			if(*(block_x + k*2) < *(block_x + k*2 + 1))
			{
				//Change the capacity after calculating the edge id using y*(gx-1) + x
				rst->edgeCaps[((*(block_y + k*2)) * ((rst->gx)-1)) + (*(block_x + k*2))] = *(new_cap + k);
			}
			else
			{
				//Change the capacity after calculating the edge id using y*(gx-1) + x
				rst->edgeCaps[((*(block_y + k*2)) * ((rst->gx)-1)) + (*(block_x + k*2 + 1))] = *(new_cap + k);
			}
		}
		else if(*(block_x + k*2) == *(block_x + k*2 + 1))
		{
			//Check if the pins are aligned from bottom to top and always use the y value of the pin on the bottom
			if(*(block_x + k*2) < *(block_x + k*2 + 1))
			{
				//Change the capacity after calculating the edge id using (gy)*(gx-1) + gx*y + x
				rst->edgeCaps[((rst->gy) * ((rst->gx)-1)) + ((rst->gx) * (*(block_y + k*2))) + (*(block_x + k*2))]  = *(new_cap + k);
			}
			else
			{
				//Change the capacity after calculating the edge id using (gy)*(gx-1) + gx*y + x
				rst->edgeCaps[((rst->gy) * ((rst->gx)-1)) + ((rst->gx) * (*(block_y + k*2 + 1))) + (*(block_x + k*2))]  = *(new_cap + k);
			}
		}
	}
//END
	
  return 1;
}

///////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//Custom functions for MS2
//Calculate Edge number for given pair of points 
int EdgeNumber(int px_a, int py_a, int px_b, int py_b, routingInst *rst)
{
	int leftx,lowy;
	//If the given edge is horizontal
	if(py_a == py_b)
	{
		//Select the leftmost x coordinate
		leftx = (px_a < px_b) ? px_a : px_b;
		return (leftx + (py_a * (rst->gx - 1)));
	}
	//If the given edge is vertical
	else {
		//Select the lower y coordinate
		lowy = (py_a < py_b) ? py_a : py_b;
		return (px_a + (rst->gx * lowy + ((rst->gx - 1) * rst->gy)));
	}
}

//Defining the function to be used in to compare in qsort
int cmpfunc(const void *pa, const void *pb)
{
	point *p_1 = (point*)pa;
	point *p_2 = (point*)pb;
	//If vertical edge use the difference between y coordinates to be used to find closest pins. 
	//If horizontal edge use the difference between x coordinates to be used to find closest pins.
	return ((p_1->x - p_2->x) == 0) ? (p_1->y - p_2->y) : (p_1->x - p_2->x);
}

//Net decomposition to sort the pins in the net prior to routing
void NetDecomposition(routingInst *rst)
{
	for(int i=0; i<rst->numNets; i++)
	{
		point* pin_list = rst->nets[i].pins;
		//Sort the pins based on how close they are to each other to route them first using qsort
		qsort(pin_list, rst->nets[i].numPins, sizeof(point), cmpfunc);
  	}		
}

//Calculating the edge weight = max(utilization - capacity, 0) * history of utilization by this edge 
int EdgeWeight(routingInst *rst, int edge_number)
{
	return (max(rst->edgeUtils[edge_number] - rst->edgeCaps[edge_number],0) * edgeGlobalUtilHistory[edge_number]);
}

//Check if things should be outside the first for loop
void ComputeEdgeWeight_NetOrdering(routingInst *rst)
{
	//Compute Edge weights for all the edges used in net and store it as cost
	for(int i=0; i<rst->numNets; i++ )
	{
		int updated_cost = 0;
		int edge_weight;
		net CurrentNet = rst->nets[i];
		route CurrentRoute = CurrentNet.nroute;
		//Extract all the segments in a net
		for(int j=0; j<CurrentRoute.numSegs; j++) //CHECK if ++j is needed
		{
			segment CurrentSegment = CurrentRoute.segments[j];
			//Get all the edges for a given segment and compute edge weight
			for(int k=0; k<CurrentSegment.numEdges; k++)
			{
				//Compute the cost of given net based on edge weight
				int CurrentEdge = CurrentSegment.edges[k];
				edge_weight = EdgeWeight(rst, CurrentEdge);
				updated_cost = updated_cost + edge_weight;
			}
		}
		//Update the newly added cost element of the nets
		rst->nets[i].cost = updated_cost;
	}

	//Net reordering based on the cost for each net
	int itr = sizeof(rst->nets)/sizeof(rst->nets[0]);
	//Sort on based on cost per net --> based on edge weight 
	for(int m=0; m<(itr - 1); m++)
	{
		for(int n=0; n<(itr - m - 1); n++)
		{
			if(rst->nets[n].cost < rst->nets[n+1].cost)
			{
				net dummy = rst->nets[n];
				rst->nets[n] = rst->nets[n+1];
				rst->nets[n+1] = dummy;
			}
		}
	}
}

//Generating initial solution --> UPDATE: Moving from vertical first to horizontal first as it is seen to give better results with Rip Up and Re-route
void InitialSolution(point p1, point p2, int net_num, int seg_num, routingInst *rst)
{
	point curr = p1;
	point next = curr;
	int edge_index = 0;
	int edge_number;
	//Horizontal Edges	
	while(curr.x != p2.x)
	{
		//Traverse the edge horizontally either to the left or right
		if(curr.x < p2.x)
		{
			next.x++;
		}
		else {
			next.x--;
		}
		//Calculating the edge number and storing it into the array of edges and moving onto the next edge
		edge_number = EdgeNumber(curr.x, curr.y, next.x, next.y, rst);
		rst->nets[net_num].nroute.segments[seg_num].edges[edge_index] = edge_number; 
		rst->edgeUtils[edge_number]++;
		curr = next;
		edge_index++;
	}
	//Vertical Edges
	while(curr.y != p2.y)
	{
		//Traverse the edge vertically either upwards or down
		if(curr.y < p2.y)
		{
			next.y++;
		}
		else {
			next.y--;
		}
		//Calculating the edge number and storing it into the array of edges and moving onto the next edge
		edge_number = EdgeNumber(curr.x, curr.y, next.x, next.y, rst);
		rst->nets[net_num].nroute.segments[seg_num].edges[edge_index] = edge_number; //Check if you can use one used in old solve route
		rst->edgeUtils[edge_number]++;
		curr = next;
		edge_index++;
	}
}

//Rip up function to remove the existing route by decreasing the Utilizations 
//NOTE: Edge weight recalculation is done in the reroute function
void RipUp(routingInst *rst, int net_number)
{
	net* CurrentNet = &rst->nets[net_number];
	route* ripup_route = &CurrentNet->nroute;
	//Extract the segments in the net
	for(int i=0; i<(ripup_route->numSegs); i++)
	{
		segment* CurrentSeg = &ripup_route->segments[i];
		//Extract the edges in the segments
		for(int j=0; j<(CurrentSeg->numEdges); j++)
		{
			int CurrentEdge = CurrentSeg->edges[j];
			//Reduce the utilization of edges of the given net by one
			rst->edgeUtils[CurrentEdge] = rst->edgeUtils[CurrentEdge] - 1;
		}
		//Make the number of edges in segment zero
		CurrentSeg->numEdges = 0;
	}
	//Make the number of segments in the net to zero
	ripup_route->numSegs=0;
//	printf("Finished Rip up");
}

//Function to create the node data strucure to store the values of the coordinates and the overall cost of the route so far
node_inst* CreateNode(int prev_x, int prev_y, int curr_x, int curr_y, int currCost)
{
	node_inst* node = new node_inst;
	node->prev_x = prev_x;
	node->prev_y = prev_y;
	node->curr_x = curr_x;
	node->curr_y = curr_y;
	node->CostSoFar = currCost;
	return node;
}

//Function to create the New queue used for Dijkstra's algorithm
queue_inst* NewQueue(unsigned capacity)
{
	queue_inst* queue = new queue_inst;
	//Set head and size to zero
	queue->size = 0;
	queue->head = 0; 
	//Set the tail to the capacity-1
	queue->tail = capacity - 1;
	queue->capacity = capacity;
	//Array to store the nodes
	queue->nodes = new node_inst[queue->capacity];
	return queue;
}

//Function to add a new node to Q2 or Q3 queue in Dijkstra's algorithm
void enqueue(queue_inst* queue, node_inst item)
{
	//Check if queue is full
	if(queue->size == queue->capacity)
	{
	//	printf("Queue is full");
		return;
	}
	//Add the new node to the tail of the queue
	else {
	queue->tail = (queue->tail + 1) % (queue->capacity);
	queue->nodes[queue->tail] = item;
	queue->size = queue->size + 1;
	}
}

//Function to remove a node form Q2 or Q3 queue in Dijkstra's algorithm
node_inst dequeue(queue_inst* queue)
{
	//Remove the node at the head of the queue
	node_inst item = queue->nodes[queue->head];
	queue->head = (queue->head + 1) % queue->capacity;
	queue->size = queue->size - 1;
	return item;
}

//Function to delete a node from the queue if it is present
void delete_from_queue(node_inst* node, queue_inst* queue)
{
	//printf("delete while loop entered");
	int end_point;
	int currX = node->curr_x;
	int currY = node->curr_y;
	int i = queue->head;
	end_point = (queue->tail + 1) % queue->capacity;
//DEBUG For loop issue      for(int i=queue->head; i<end_point; i=((i+1)%queue->capacity))
	//Traverse the queue and check if the node is present and dequeue the same 	
	while(i != end_point)
	{
		node_inst comp_node = dequeue(queue);
		//If the node is in the queue skip the iteration
		if(comp_node.curr_x == currX && comp_node.curr_y == currY)
		{
			i = (i + 1) % queue->capacity;
			continue;
		}
		enqueue(queue, comp_node);
		i = (i + 1) % queue->capacity;
	}
}

//Function which checks if the given node is present in the given queue
//Used to check if the node extracted is not present in Q2 or Q3
int node_not_in_queue(node_inst* node, queue_inst* queue)
{
	//Traverse through the nodes in the given queue
	for(int i=queue->head; i<((queue->tail+1)%queue->capacity); i=((i+1)%queue->capacity))
	{
		node_inst* comp_node = &queue->nodes[i];
		if(comp_node->curr_x == node->curr_x && comp_node->curr_y == node->curr_y)
		{
			return 0;
		}
	}
	return 1;
}

//Return the node if it is present in the given queue
node_inst* extract_if_node_in_Q2(node_inst* node, queue_inst* queue)
{
	int currX = node->curr_x;
	int currY = node->curr_y;
	//Traverse th queue and return the node
	for(int i=queue->head; i<((queue->tail+1)%queue->capacity); i=((i+1)%queue->capacity))
	{
		node_inst* comp_node = &queue->nodes[i];
		if(comp_node->curr_x == currX && comp_node->curr_y == currY)
		{
			return comp_node;
		}
	}
}

//Function to extract the node with with minimum cost from Q2
node_inst* extract_min(queue_inst* queue)
{
	node_inst* min_node = &queue->nodes[queue->head];
	//Traverse through the queue to find the node with minimum cost
	for(int i=queue->head+1; i<((queue->tail+1)%queue->capacity); i=((i+1)%queue->capacity))
	{
		if(queue->nodes[i].CostSoFar < min_node->CostSoFar)
		{
			min_node = &queue->nodes[i];
		}
	}
	return min_node;
}

//Function to find endpoint of an edge given the starting point
point find_endpoint(point p1, int edge, routingInst *rst)
{
	//If the given edge is the horizontal edge
	if(edge < ((rst->gx - 1) * rst->gy))
	{	
		//Find if the edge is going right or the left
		point left = {p1.x - 1, p1.y};
		point right = {p1.x + 1, p1.y};
		if(EdgeNumber(p1.x, p1.y, left.x, left.y, rst) == edge)
		{
			return left;
		}
		else if(EdgeNumber(p1.x, p1.y, right.x, right.y, rst) == edge)
		{
			return right;
		}
	}
	//If the given edge is the vertical edge
	else {
		//Find if the edge is going top or the bottom
		point bottom = {p1.x, p1.y - 1};
		point top = {p1.x, p1.y + 1};
		if(EdgeNumber(p1.x, p1.y, bottom.x, bottom.y, rst) == edge)
		{
			return bottom;
		}
		else if(EdgeNumber(p1.x, p1.y, top.x, top.y, rst) == edge)
		{
			return top;
		}

	}
}

//Retrace the edges of a given segment in a net and return the segment
segment* retrace(queue_inst* Q3, routingInst *rst, node_inst* curr_node, point Source, point Target)
{
	int num_edges = 0;
	int *edges1 = new int [(rst->gx - 1)+(rst->gy - 1)];
	int *edges2 = new int [(rst->gx - 1)+(rst->gy - 1)];
//DEBUG: Check why for loop isnt working	for(int i=Q3->tail; i>((Q3->head-1)%Q3->capacity); i=((i-1)%Q3->capacity))
	int i = Q3->tail;
	//Extract the nodes in Q3 one by one and retrace these nodes and update the edge utilization along the path
	while(i != (Q3->head - 1) % Q3->capacity)
	{
		node_inst* comp_node = &Q3->nodes[i];
		//Compare two nodes at a time
		if(comp_node->curr_x == curr_node->prev_x && comp_node->curr_y == curr_node->prev_y)
		{
			int edge_number = EdgeNumber(comp_node->curr_x, comp_node->curr_y, curr_node->curr_x, curr_node->curr_y, rst);
			//Update the utilization of each edge in the segment
			rst->edgeUtils[edge_number] = rst->edgeUtils[edge_number] + 1;
	      		edges1[num_edges] = edge_number;
			//Move onto the next node while making the next node used in this iteration as current node
			num_edges = num_edges + 1;
			curr_node = comp_node;
		}
		i = (i-1) % Q3->capacity;
	}

	//Reverse the array contatining the edges before storing it in the edges array present in segement data structure
	int j=0;
	for(int k=(num_edges-1); k>=0; k--)
	{
		edges2[j] = edges1[k];
		j = j + 1;
	}
	//Store the retraced path into the segment data structure and return the value
	segment* result = new segment;
	result->p1 = Source;
	result->p2 = Target;
	result->edges = edges2;
	result->numEdges = num_edges;
	return result;
}

//Reroute the net based on Dijkstra's algorithm
void reroute(routingInst *rst, int net_id)
{	
	//printf("Begin reroute\n");
	net* curr_net = &rst->nets[net_id];
	//Sequential routing of 2-pin subnets i.e. taking 2 pins at a time
	for(int j=0; j<(curr_net->numPins - 1); j++)
	{
		//printf("Enterning for loop\n");
		//Taking two pins at a time as Source and Target
		point Source = curr_net->pins[j];
		point Target = curr_net->pins[j+1];
		//Set the Bounding Box for the two pins 
		int BBMin_x, BBMax_x, BBMin_y, BBMax_y;
		if(Target.x > Source.x)
		{
			BBMin_x = Source.x;
			BBMax_x = Target.x;
		}
		else {
			BBMin_x = Target.x;
			BBMax_x = Source.x;
		}
		if(Target.y > Source.y)
		{
			BBMin_y = Source.y;
			BBMax_y = Target.y;
		}
		else {
			BBMin_y = Target.y;
			BBMax_y = Source.y;
		}
		//Create the two queues for Dijkstra's algorithm
		queue_inst *Q2 = NewQueue(rst->gx * rst->gy);
		queue_inst *Q3 = NewQueue(rst->gx * rst->gy);
		
		//Initialize the souce node and load it to Q2
		node_inst *source_node = CreateNode(-1, -1, Source.x, Source.y, 0);
		enqueue(Q2, *source_node);
		
		//Check until the target node is reached and Q2 is empty
		while(Q2->size != 0)
		{	
			//DEBUG printf("Enterning while loop\n");
			//Extract the most promising node
			node_inst *min_node = extract_min(Q2);
			//DEBUG printf("Minimum extracted\n %d %d %d %d %d \n", min_node->prev_x, min_node->prev_y, min_node->curr_x, min_node->curr_y, min_node->CostSoFar);
			delete_from_queue(min_node, Q2);
			//DEBUG printf("Delete from queue done\n");
			int edge_number, edge_weight;
			//If the extracted node is the Target node then retrace and exit
			if(Target.x == min_node->curr_x && Target.y == min_node->curr_y)
			{
				//DEBUG printf("If condition entered\n");
				//Retrace and update the segments in nroute part of net data structure
				segment* new_seg = retrace(Q3, rst, min_node, Source, Target);
				curr_net->nroute.segments[j] = *new_seg;
				curr_net->nroute.numSegs = curr_net->nroute.numSegs + 1;
				//Exit
				break;
			}
			else {
				//DEBUG printf("Else condition entered\n");
				//Check the 4 adjacent nodes of the given node one at a time
				for(int i=0; i<4; i++)		
				{
					//DEBUG printf("4 index for loop entered\n");
					node_inst *adjacent_node;
					//Check for the node up
					if(i == 0)
					{
						//Check if the node beyond the Bounding box defined or beyond grid
						if((min_node->curr_y > BBMax_y) || (min_node->curr_y == rst->gy ))
						{
							continue;
						}
						edge_number = EdgeNumber(min_node->curr_x, min_node->curr_y, min_node->curr_x, min_node->curr_y + 1, rst);
						//Update the edge Global history of utilization of the edge based on overflow
						if(rst->edgeCaps[edge_number] < rst->edgeUtils[edge_number])
						{
							edgeGlobalUtilHistory[edge_number] = edgeGlobalUtilHistory[edge_number] + 1;
						}
						//Calculate the the edge weight and create the cost so far 
						edge_weight = EdgeWeight(rst, edge_number) + abs(min_node->curr_x - Target.x) + abs(min_node->curr_y + 1 - Target.y);
						adjacent_node = CreateNode(min_node->curr_x, min_node->curr_y, min_node->curr_x, min_node->curr_y + 1, min_node->CostSoFar + edge_weight);

						//printf("I0 %d %d \n", edge_number, edge_weight);
					}
					//Check for the node down
					else if(i == 1)
					{
						//Check if the node beyond the Bounding box defined or beyond grid
						if((min_node->curr_y < BBMin_y) || (min_node->curr_y == 0 ))
						{
							continue;
						}
						edge_number = EdgeNumber(min_node->curr_x, min_node->curr_y, min_node->curr_x, min_node->curr_y - 1, rst);
						//Update the edge Global history of utilization of the edge based on overflow
						if(rst->edgeCaps[edge_number] < rst->edgeUtils[edge_number])
						{
							edgeGlobalUtilHistory[edge_number] = edgeGlobalUtilHistory[edge_number] + 1;
						}
						//Calculate the the edge weight and create the cost so far 
						edge_weight = EdgeWeight(rst, edge_number) + abs(min_node->curr_x - Target.x) + abs(min_node->curr_y - 1 - Target.y);
						adjacent_node = CreateNode(min_node->curr_x, min_node->curr_y, min_node->curr_x, min_node->curr_y - 1, min_node->CostSoFar + edge_weight);
						//printf("I1 %d %d \n", edge_number, edge_weight);
					}
					//Check for the node left
					else if(i == 2)
					{
						//Check if the node beyond the Bounding box defined or beyond grid
						if((min_node->curr_x < BBMin_x) || (min_node->curr_x == 0 ))
						{
							continue;
						}
						edge_number = EdgeNumber(min_node->curr_x, min_node->curr_y, min_node->curr_x - 1, min_node->curr_y, rst);
						//Update the edge Global history of utilization of the edge based on overflow
						if(rst->edgeCaps[edge_number] < rst->edgeUtils[edge_number])
						{
							edgeGlobalUtilHistory[edge_number] = edgeGlobalUtilHistory[edge_number] + 1;
						}
						//Calculate the the edge weight and create the cost so far 
						edge_weight = EdgeWeight(rst, edge_number) + abs(min_node->curr_x - 1 - Target.x) + abs(min_node->curr_y - Target.y);
						adjacent_node = CreateNode(min_node->curr_x, min_node->curr_y, min_node->curr_x - 1, min_node->curr_y, min_node->CostSoFar + edge_weight);
						//printf("I2 %d %d \n", edge_number, edge_weight);
					}
					else
					//Check for the node right
					{
						//Check if the node beyond the Bounding box defined or beyond grid
						if((min_node->curr_x > BBMax_x) || (min_node->curr_x == rst->gx))
						{
							continue;
						}
						edge_number = EdgeNumber(min_node->curr_x, min_node->curr_y, min_node->curr_x + 1, min_node->curr_y, rst);
						//Update the edge Global history of utilization of the edge based on overflow
						if(rst->edgeCaps[edge_number] < rst->edgeUtils[edge_number])
						{
							edgeGlobalUtilHistory[edge_number] = edgeGlobalUtilHistory[edge_number] + 1;
						}
						//Calculate the the edge weight and create the cost so far 
						edge_weight = EdgeWeight(rst, edge_number) + abs(min_node->curr_x + 1 - Target.x) + abs(min_node->curr_y - Target.y);
						adjacent_node = CreateNode(min_node->curr_x, min_node->curr_y, min_node->curr_x + 1, min_node->curr_y, min_node->CostSoFar + edge_weight);
						//printf("I3 %d %d \n", edge_number, edge_weight);
					}
					//Check if the given adjacent node is not in both Q2 and Q3
					if(node_not_in_queue(adjacent_node, Q2) && node_not_in_queue(adjacent_node, Q3))
					{
						//printf("Enterning not in both Q2 and Q3\n");
						//Load the node into Q2 if not in Q2 and Q3
						enqueue(Q2, *adjacent_node);
					}
					else {
						//Check if the node is not in Q3. If so extract the node from Q2 
						if(node_not_in_queue(adjacent_node, Q3))
						{
							node_inst* matched;
							matched = extract_if_node_in_Q2(adjacent_node, Q2);
							//Check if the extracted node from Q2 has better cost than the adjacent node
							//If not delete the extracted node from Q2 and load the new adjacent node into Q2
							if(matched->CostSoFar >= adjacent_node->CostSoFar)
							{
								delete_from_queue(matched, Q2);
								enqueue(Q2, *adjacent_node);
							}
						}
					}
				}
				//Push the node extracted and processed into Q3
				enqueue(Q3, *min_node);
			}
		//printf("Q2 size %d\n", Q2->size);
		}
	}
}


//End of Custom functions for MS2
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

int solveRouting(routingInst *rst, int d, int n){
  /*********** TO BE FILLED BY YOU **********/
  //Enable Net decomposition if it is enabled
	if(d == 1)
	{
		NetDecomposition(rst);
	}

  //Breaking multi terminal nets into two terminal sub nets and creating a segment out of it
  	point p_1, p_2;
  
  	for(int net_index=0; net_index<(rst->numNets); net_index++) 
  	{
  		//Filling out the number of segments in a given net which is = number of pins - 1
  		rst->nets[net_index].nroute.numSegs = rst->nets[net_index].numPins - 1;
  		//Create an array of all the segments
  		rst->nets[net_index].nroute.segments = new segment[rst->nets[net_index].nroute.numSegs]();
  	
  		int pin_index = 0;
  	
  		//Evaluating and routing every segment of the net by taking 2 pins at a time
  		for(int seg_index=0; seg_index<(rst->nets[net_index].nroute.numSegs); seg_index++)
  		{
  			//Obtain the x and y co-ordinates of two pins in the net and in the next iteration take the second pin and the third pin in the net if it exists
  			p_1.x = rst->nets[net_index].pins[pin_index].x;
  			p_1.y = rst->nets[net_index].pins[pin_index].y;
  			//Changing the pin index to obtain the next pin in the net
  			pin_index++;
  			p_2.x = rst->nets[net_index].pins[pin_index].x;
  			p_2.y = rst->nets[net_index].pins[pin_index].y;
  		
  			//Adding the two pins to form a segment
  			rst->nets[net_index].nroute.segments[seg_index].p1 = p_1;
  			rst->nets[net_index].nroute.segments[seg_index].p2 = p_2;
  		
			//DEBUG printf("For segment%d, P1=[%d,%d], P2=[%d,%d]\n",seg_index, p_1.x,p_1.y,p_2.x,p_2.y);	
  			//Calculating the number of minimum edges required to connect p_1 and p_2 which is sum of absolute values of difference between the x and y coordinates of the two pins
  			rst->nets[net_index].nroute.segments[seg_index].numEdges = abs(p_2.x - p_1.x) + abs(p_2.y - p_1.y);
  			//Since for milestone 1 there is no restriction on capacity or overflow we route it using minimum number of edges, hence creating an array of edges for the same  
  			rst->nets[net_index].nroute.segments[seg_index].edges = new int [rst->nets[net_index].nroute.segments[seg_index].numEdges];
			//Generating initial solution	
			InitialSolution(p_1, p_2, net_index, seg_index, rst);
		}
  	}
  
	//Rip up and reroute if enabled
  	if(n == 1)
  	{
		//Check if the timeout limit of 15 mins is reached
  		while(clock_timeout > (clock()/CLOCKS_PER_SEC))
  		{	
			//Compute the edge weight and costs of each net and do net ordering
			ComputeEdgeWeight_NetOrdering(rst);
			//printf("Net: %d \n", n);
			//Check for timeout
			for(int n=0; n<rst->numNets; n++)
			{
				//NetOrdering(rst);
				if(clock_timeout < (clock()/CLOCKS_PER_SEC))
				{
					break;
				}
			//printf("Net: %d \n", n);
			//RipUp and reroute the nets one by one
			RipUp(rst, rst->nets[n].id);
			reroute(rst, rst->nets[n].id);
			//printf("Net: %d \n", n);
			//NetOrdering(rst);
			}
  		}
  	}
  return 1;
 }

int writeOutput(const char *outRouteFile, routingInst *rst){
  /*********** TO BE FILLED BY YOU **********/
	//File name "outRouteFile" is to given by the user from main.cpp 
	ofstream myopfile(outRouteFile);
	//Standard file validity check
	if(!(myopfile.is_open()))
	{
		printf("Unable to open output file \n");
		return 0;
	}
	else
	{
		//Loop to print each net data one at a time
		for(int i=0; i<(rst->numNets); i++)
		{
			//Printing the name of the net for the iteration
			myopfile << "n" << rst->nets[i].id << endl;
			//Printing all the segments in the net
			for(int j=0; j<(rst->nets[i].nroute.numSegs); j++)
			{
				segment seg_h = rst->nets[i].nroute.segments[j];
				int *edges = seg_h.edges;
				int numEdges = seg_h.numEdges;
				point startpoint = seg_h.p1;
				int prev_edge = edges[0];
				point endpoint = find_endpoint(startpoint, prev_edge, rst);
				int currEdge = 0;
				int edgeDifference = 0;
				//Checking each edge in the segment
				for (int k=1; k<numEdges; k++)
				{
					currEdge = edges[k];
					edgeDifference = abs(currEdge -prev_edge);
					//Check if the next edge is the bend then assign the pivot as the next startpoint
					if (edgeDifference != 1 && edgeDifference != rst->gx)
					{
						myopfile << "(" << startpoint.x << "," << startpoint.y << ")-";
						myopfile << "(" << endpoint.x << "," << endpoint.y << ")" << endl;
						startpoint = endpoint;
					}
					//If its a stright edge find the end point
					endpoint = find_endpoint(endpoint, currEdge, rst);
					prev_edge = currEdge;
				}
				myopfile << "(" << startpoint.x << "," << startpoint.y << ")-";
				myopfile << "(" << endpoint.x << "," << endpoint.y << ")" << endl;
			}
			myopfile << "!" << endl;
		}
	}
	myopfile.close();
  return 1;
}


int release(routingInst *rst){
  /*********** TO BE FILLED BY YOU **********/
	//Assigning all the int members of the struct to zero except numNets which is required to delete the net structure
	rst->gx = 0;
	rst->gy = 0;
	rst->cap = 0;
	for (int i = 0; i < rst->numNets; i++)
	{
		delete [] rst->nets[i].pins;
		delete [] rst->nets[i].nroute.segments;
	}
	rst->numNets = 0;
	delete rst->nets;
	delete [] rst->edgeCaps;
	delete [] rst->edgeUtils;
	delete [] edgeGlobalUtilHistory;
	delete rst;
      	return 1;
}
  
