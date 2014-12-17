#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/socket.h>
#include<netdb.h>
#include<ctype.h>
#include<time.h>
#include<ctime>
#include<unistd.h>
typedef enum {ADD, REMOVE, UPDATE, GET_EVENT, GET_DATE, GET_ALL} service_type;
/* Date Data Structure */ 
struct date
{
	int day, month, year;
};
/* Data structure for client request to be sent */
struct client_request
{
	char username[32];
	service_type type;
	struct date event_date;
	int start_time, end_time;
	char event[512];
};
/* Serialises the integer values as big endian to be sent over network as char array */
void serialize_int(unsigned char *buffer, int val)
{
	
	/* Use Big Endian encoding of integers */
	buffer[0] = val >> 24 & 0xFF;	//highest 4 bits
	buffer[1] = val >> 16 & 0xFF;	//next 4 bits
	buffer[2] = val >> 8  & 0xFF;	
	buffer[3] = val >> 0 & 0xFF;
}
/* Serialises the entire client request, makes use of serialize_int */
void serialize_client_request(unsigned char *buffer, struct client_request val)
{
	strcpy((char *)buffer, val.username);			//username simply copied to buffer char array
	switch(val.type)								//one char for request type
	{
		case(ADD):
			buffer[32] = 'a';
			break;
		case(REMOVE):
			buffer[32] = 'r';
			break;
		case(UPDATE):
			buffer[32] = 'u';
			break;
		case(GET_EVENT):
			buffer[32] = 'e';
			break;
		case(GET_DATE):
			buffer[32] = 'd';
			break;
		case(GET_ALL):
			buffer[32] = 'l';
			buffer[33] = 's';
			return;
			break;		
		default:	
			break;				
	}
	serialize_int(buffer+33, val.event_date.day);				//day, month, year start_time, end_time -- all are integers
	serialize_int(buffer+37, val.event_date.month);
	serialize_int(buffer+41, val.event_date.year);
	serialize_int(buffer+45, val.start_time);
	serialize_int(buffer+49, val.end_time);
	strcpy((char *)(buffer+53), val.event);						//copy the event char array
}
/* To deserialise the char array received into and integer  value */
int deserialize_int(unsigned char *buffer)
{
	int ret_val = 0x000000;	
	ret_val |= buffer[0] << 24; 		
	ret_val |= buffer[1] << 16;
	ret_val |= buffer[2] << 8; 
	ret_val |= buffer[3];
	return ret_val;
}
/* function to quit after printing a message */
void quit(const char *error)
{
	printf("%s\n", error);
	exit(EXIT_FAILURE);
}
/* Check that the date is in correct format */
struct date check_and_return_date(unsigned char *date, bool *same_date_event)
{
	struct date ret_date;
	if(strlen((char *)date)!=6)
		quit("Invalid Date Format.");
	int i;
	
	for(i=0;i<6;i++)
		if(isdigit(date[i])==0)
			quit("Invalid Date.");
			
	time_t raw_time;
	time(&raw_time);
	struct tm * t = localtime(&raw_time);		//gets the current time from 1900
	int temp = atoi((char *)date);
	//parse the char date to day month and year
	ret_date.year = temp%100;
	ret_date.month = (temp/100)/100;
	ret_date.day = (temp/100)%100;
	
	
	if((2000+ret_date.year-1900<t->tm_year) ||				//earlier year
		(2000+ret_date.year-1900==t->tm_year && ret_date.month<t->tm_mon) ||		//same year earlier month
		(2000+ret_date.year-1900==t->tm_year && ret_date.month==t->tm_mon && ret_date.day<t->tm_mday) )		//same year and month earlier date
			quit("Event cannot be created in past.");
	else if(2000+ret_date.year-1900==t->tm_year && ret_date.month==t->tm_mon && ret_date.day==t->tm_mday)		
		*same_date_event = true;			//set same date event to true so that time is checked in check_and_return_time
	
	if(ret_date.month<1 || ret_date.month>12)
		quit("Invalid Date.");
	
	else 
	{
		if(ret_date.month%2!=0 && (ret_date.day<1 || ret_date.day>31))
			quit("Invalid Date.");
		else if(ret_date.month == 2 && 
				(	(ret_date.year%4!=0 && (ret_date.day<1 || ret_date.day>28)) ||
					(ret_date.year%4==0 && (ret_date.day<1 || ret_date.day>29))   ))
			quit("Invalid Date.");		
		else if(ret_date.month%2==0 && (ret_date.day<1 || ret_date.day>30))
			quit("Invalid Date.");
	}
	return ret_date;
}
/* Check the time format and return time */
/* If param check_time is set, means the date is same as today so check that the time has not already passed */
int check_and_return_time(unsigned char *time_in, bool check_time)
{
	int ret_time;
	if(strlen((char *)time_in)!=4)
		quit("Invalid time format.");
	int i;
	
	for(i=0;i<4;i++)
		if(isdigit(time_in[i])==0)
			quit("Invalid Time.");
			
	time_t raw_time;
	time(&raw_time);
	struct tm * t = localtime(&raw_time);
			
	ret_time = atoi((char *)time_in);
	
	if(check_time && ret_time < t->tm_hour*100+t->tm_min)				//if check time is set => same date so check for time
		quit("Event cannot be created in past.");	
		
	if(ret_time<0 || ret_time>2400)
		quit("Invalid time");
	return ret_time;
}
int main(int argc, char *argv[])
{
	struct hostent *p;
	int socket_fd;
	unsigned char message[565];
	struct client_request request;
	struct sockaddr_in server_addr;
	int i;
	unsigned char reply[1], ret_event[536], size[4];
	bool same_date_event;
	
	//check if min no of arguments are present, if not print error
	if(argc<5)
		quit("Too few arguments");
	//get the IP from the host name provided in the arguments, returns a pointer to struct hostent
	p = gethostbyname(argv[1]);
	if(p==NULL)
		quit("Hostname conversion error.");
	
	//create a socket
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	//set values in the server address
	bzero((char *)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[2]));
	bcopy((char *)p->h_addr, (char *)&server_addr.sin_addr.s_addr, p->h_length);
	//connect to the socket
	if(connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr))<0)
		quit("Connection could not be established.");
	//clear the memory of the request and then fill it according the the arguments
	memset((char *)&request, 0, sizeof(request));
	strcpy(request.username, argv[3]);				//copy username
	/* next the details would be filled according to the request type */
	if(strcmp(argv[4], "add") == 0)
	{
		if(argc!=9)
			quit("Invalid add event request.\n");
		request.type = ADD;
		request.event_date = check_and_return_date((unsigned char *)argv[5], &same_date_event);
		request.start_time = check_and_return_time((unsigned char *)argv[6], same_date_event);
		request.end_time = check_and_return_time((unsigned char *)argv[7], same_date_event);
		if(request.end_time<request.start_time)
			quit("End Time cannot be smaller than Start Time");
		strcpy(request.event, argv[8]);
	}
	else if(strcmp(argv[4], "remove") == 0)
	{
		if(argc!=7)
			quit("Invalid remove event request.\n");
			
		request.type = REMOVE;
		request.event_date = check_and_return_date((unsigned char *)argv[5], &same_date_event);
		request.start_time = check_and_return_time((unsigned char *)argv[6], false);
	}
	else if(strcmp(argv[4], "update") == 0)
	{
		if(argc!=9)
			quit("Invalid update event request.\n");
		request.type = UPDATE;
		request.event_date = check_and_return_date((unsigned char *)argv[5], &same_date_event);
		request.start_time = check_and_return_time((unsigned char *)argv[6], same_date_event);
		request.end_time = check_and_return_time((unsigned char *)argv[7], same_date_event);
		if(request.end_time<request.start_time)
			quit("End Time cannot be smaller than Start Time");
		strcpy(request.event, argv[8]);
	}
	else if(strcmp(argv[4], "get") == 0)
	{
		if(argc<6)
			quit("Invalid get event request.\n");
			
		request.type = GET_DATE;
		request.event_date = check_and_return_date((unsigned char *)argv[5], &same_date_event);
		if(argv[6]!=NULL)
		{
			request.type = GET_EVENT;
			request.start_time = check_and_return_time((unsigned char *)argv[6], false);
		}	
	}
	else if(strcmp(argv[4], "getall") == 0)
	{
		if(argc!=5)
			quit("Too few or too many arguments.");
			
		request.type = GET_ALL;	
	}
	else
		quit("Invalid request to calendar.\n");
		
	//once the request is set, serialize it so that it can be sent over a socket
	serialize_client_request(message, request);

	//write onto the socket
	if(write(socket_fd, message, 565)<0)
		quit("Error writing onto the socket.");
	
	/* Now depending on the sent request type, take the response */
	/* 
		If as the first response in reply, we get 'u' means that the username was incorrect
		'f' means that the operation failed
		'u' means that the operation was successfull
	 */
	switch(message[32])
	{
		case('a'):
		{
			if(read(socket_fd, reply, 1)<0)
				quit("Error reading from the socket");
			if(reply[0]=='f')
				printf("Event could not be added, conflict detected.\n");	
			else if (reply[0]=='s')
				printf("Event added successfully.\n");	
			break;
		}
		case('u'):
		{
			if(read(socket_fd, reply, 1)<0)
				quit("Error reading from the socket");
			if(reply[0]=='u')
				printf("User not found.\n");	
			else if(reply[0]=='f')
				printf("Event could not be updated.\n");	
			else if (reply[0]=='s')
				printf("Event updated successfully.\n");	
			break;
		}
		case('r'):
		{
			if(read(socket_fd, reply, 1)<0)
				quit("Error reading from the socket");
			if(reply[0]=='u')
				printf("User not found.\n");	
			else if(reply[0]=='f')
				printf("Event could not be removed.\n");	
			else if (reply[0]=='s')
				printf("Event removed successfully.\n");	
			break;
		}
		case('e'):
		{
			if(read(socket_fd, reply, 1)<0)
				quit("Error reading from the socket");
			if(reply[0]=='u')
				printf("User not found.\n");	
			else if(reply[0]=='f')
				printf("Event could not be retrieved.\n");	
			else if (reply[0]=='s')
			{
				if(read(socket_fd, ret_event, 536)<0)
					quit("Error reading from the socket");			
				/* Print the event just receieved */	
				printf("Event Details:\n");
				printf("Date: %02d/%02d/%02d\n", deserialize_int(ret_event), deserialize_int(ret_event+4), deserialize_int(ret_event+8));
				printf("Time:\n\tFrom: %d\n\tTo: %d\n", deserialize_int(ret_event+16), deserialize_int(ret_event+20));
				printf("Event: %s\n", ret_event+24);	
			}
			break;
		}
		case('d'):
		{
			if(read(socket_fd, reply, 1)<0)
				quit("Error reading from the socket");
			if(reply[0]=='u')
				printf("User not found.\n");	
			else if(reply[0]=='f')
				printf("Event could not be retrieved.\n");	
			else if (reply[0]=='s')
			{
				if(read(socket_fd, size, 4)<0)
					quit("Error reading from the socket");			
				/* Print the events in a loop */	
				for(i=0;i<deserialize_int(size);i++)
				{
					if(read(socket_fd, ret_event, 536)<0)
						quit("Error reading from the socket");			
					printf("Event #%d:\n", i+1);
					printf("Date: %02d/%02d/%02d\n", deserialize_int(ret_event), deserialize_int(ret_event+4), deserialize_int(ret_event+8));
					printf("Time:\n\tFrom: %d\n\tTo: %d\n", deserialize_int(ret_event+16), deserialize_int(ret_event+20));
					printf("Event: %s\n", ret_event+24);	
					printf("\n");
				}
			}
			break;
		}
		case('l'):
		{
			if(read(socket_fd, reply, 1)<0)
				quit("Error reading from the socket");
			if(reply[0]=='u')
				printf("User not found.\n");	
			else if(reply[0]=='f')
				printf("Event could not be retrieved.\n");	
			else if (reply[0]=='s')
			{
				if(read(socket_fd, size, 4)<0)
					quit("Error reading from the socket");	
					
				/* Loop around and keep on sending requests for next events */	
				for(i=0;i<deserialize_int(size);i++)
				{
					sleep(2);
					memset(message, 0, sizeof(message));
					strncpy((char *)message, request.username, 32);
					message[32] = 'l';
					message[33] = 'r';
					serialize_int(message+34, i);
					if(write(socket_fd, message, 565)<0)
						quit("Error writing onto the socket.");
					if(read(socket_fd, ret_event, 536)<0)
						quit("Error reading from the socket");			
					printf("Event #%d:\n", i+1);
					printf("Date: %02d/%02d/%02d\n", deserialize_int(ret_event), deserialize_int(ret_event+4), deserialize_int(ret_event+8));
					printf("Time:\n\tFrom: %04d\n\tTo: %04d\n", deserialize_int(ret_event+16), deserialize_int(ret_event+20));
					printf("Event: %s\n", ret_event+24);	
					printf("\n");
				}
				memset(message, 0, sizeof(message));
				strncpy((char *)message, request.username, 32);
				message[32] = 'l';
				message[33] = 'e';
				if(write(socket_fd, message, 565)<0)
					quit("Error writing onto the socket.");
					
			}
			break;
		}
	}
	//close the sconnection
	close(socket_fd);
	return 0;
}
