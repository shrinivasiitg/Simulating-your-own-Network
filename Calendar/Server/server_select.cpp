#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<unistd.h>
#include<vector>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<errno.h>
#include<string>
#include<time.h>
#include<unistd.h>

#define PORT_NO 9310

using namespace std;
typedef enum {ADD, REMOVE, UPDATE, GET} service_type;
/* Date structure */
struct date
{
	int day, month, year;
	bool operator==(struct date a)
	{
		return (a.day==day && a.month==month && a.year==year);
	}
};
/* Event Data Structure */
struct event
{
	struct date event_date;
	int start_time, end_time;
	char event[512];
};
/* User Data Structure */
struct user
{
	char username[32];
	vector<struct event> event_list; 	
};
/* User list to be maintained at the server */
vector<struct user> user_list;
/* function to quit after printing a message */
void quit(const char *error)
{
	printf("%s\n", error);
	exit(EXIT_FAILURE);
}
/* exracts the username from message i.e. the first 32 chars */
void get_username_from_message(unsigned char *buffer, char *username)
{
	strncpy(username, (char*)buffer, 32);
}
/* Deserialzes an integer value from a char array using big endian ddecoding */
int deserialize_int(unsigned char *buffer)
{
	int ret_val = 0x00000000;
	ret_val |= buffer[0] << 24;
	ret_val |= buffer[1] << 16;
	ret_val |= buffer[2] << 8; 
	ret_val |= buffer[3];
	return ret_val;
}
/* Deserializes the message (char array) received from the client into an event */
void deserialize_message(unsigned char *buffer, struct event *e)
{
	e->event_date.day = deserialize_int(buffer+33);
	e->event_date.month = deserialize_int(buffer+37);
	e->event_date.year = deserialize_int(buffer+41);
	e->start_time = deserialize_int(buffer+45);
	e->end_time = deserialize_int(buffer+49);
	strncpy(e->event, (char*)(buffer+53), 512);
}
/* Iterates the user_list and checks for the name, returns that user struct */
struct user * get_user_from_username(char *user_name)
{
	vector<struct user>::iterator iter;	
	for(iter=user_list.begin();iter!=user_list.end();iter++)
	{
		if(strcmp(user_name, (*iter).username)==0)
		{
			break;
		}
	}
	if(iter!=user_list.end())
		return &(*iter);
	else
		return NULL;	
}
/* Add event to the user 
	Checks for conflicting times, if no conflicts pushes the event to the back of the user's event list and returns true
	else returns false
*/
bool add_event_to_user(struct user *cur_user, unsigned char *message)
{
	vector<struct event>::iterator i;
	

	struct event new_event;
	deserialize_message(message, &new_event);
	
	if(cur_user!=NULL)
	{
		for(i=cur_user->event_list.begin();i!=cur_user->event_list.end();i++)
		{
			if(	(*i).event_date==new_event.event_date && 
				(	((*i).end_time>new_event.start_time && (*i).start_time<new_event.start_time) || 
					((*i).end_time>new_event.end_time && (*i).start_time<new_event.end_time) 	 ||
					(new_event.end_time>(*i).start_time && new_event.start_time<(*i).start_time) || 
					(new_event.end_time>(*i).end_time && new_event.start_time<(*i).end_time)	 ))
			{
				return false;
			}
		}
		cur_user->event_list.push_back(new_event);
		return true;
	}	
	else
	{
		//create new user
		struct user new_user;
		get_username_from_message(message, new_user.username);
		new_user.event_list.push_back(new_event);
		user_list.push_back(new_user);
		return true;
	}	
}
/* Updates an event in the user list
	Iterates the user's event list, searches for same date and start_time event, modifies the required fiels, returns true on success
	returns false is no matching event found
 */
bool update_event_for_user(struct user *cur_user, unsigned char *message)
{
	vector<struct event>::iterator i;

	
	struct event new_event;
	deserialize_message(message, &new_event);
	for(i=cur_user->event_list.begin();i!=cur_user->event_list.end();i++)
	{
	
		if((*i).event_date==new_event.event_date && ((*i).start_time==new_event.start_time))
		{
			(*i).end_time = new_event.end_time;
			strcpy((*i).event, new_event.event);
			break;
		}
	}
	if(i!=cur_user->event_list.end())	
		return true;
	else
		return false;	
}
/* Removes an event from user
	Iterates user's event list, searhces for matching event, deletes is, returns true
	returns false if no matching event found
 */
bool remove_event_from_user(struct user *cur_user, unsigned char *message)
{
	vector<struct event>::iterator i;
	
	struct event new_event;
	deserialize_message(message, &new_event);
	
	for(i=cur_user->event_list.begin();i!=cur_user->event_list.end();i++)
	{
		if((*i).event_date==new_event.event_date && (*i).start_time==new_event.start_time)
		{
			cur_user->event_list.erase(i);
			return true;
		}
	}
	return false;
}
/* Gets the event for a user
	param message has the required event
	Iterate the user's event list to find matching event and return the event struct
 */
struct event * get_event_for_user(struct user *cur_user, unsigned char *message)
{
	vector<struct event>::iterator i;
	
	struct event new_event;
	deserialize_message(message, &new_event);
	for(i=cur_user->event_list.begin();i!=cur_user->event_list.end();i++)	
	{
		if((*i).event_date==new_event.event_date && (*i).start_time==new_event.start_time)
		{
			return &(*i);
		}
	}
	return NULL;	
}
/* Gets all events for the date, works same as get_event */
vector<struct event> get_date_events_for_user(struct user *cur_user, unsigned char *message)
{
	vector<struct event>::iterator i;
	vector<struct event> ret_vector;
		
	ret_vector.clear();
	struct event new_event;
	deserialize_message(message, &new_event);
	for(i=cur_user->event_list.begin();i!=cur_user->event_list.end();i++)	
	{
		if((*i).event_date==new_event.event_date)
		{
			ret_vector.push_back((*i));
		}
	}	
	return ret_vector;
}
/* Gets all events for a user, simply returns the event list */
vector<struct event> get_all_events_for_user(struct user *cur_user)
{
	return cur_user->event_list;	
}
/* Serializes an integer value to char array using big endian encoding */
void serialize_int(unsigned char *buffer, int val)
{
	/* Use Big Endian encoding of integers */
	buffer[0] = val >> 24 & 0xFF;
	buffer[1] = val >> 16 & 0xFF;
	buffer[2] = val >> 8  & 0xFF;
	buffer[3] = val 	& 0xFF;
}
/* Serialzes a complete event to be sent over a network */
void serialize_event(unsigned char *buffer, struct event e)
{
	serialize_int(buffer, e.event_date.day);
	serialize_int(buffer+4, e.event_date.month);
	serialize_int(buffer+8, e.event_date.year);
	serialize_int(buffer+16, e.start_time);
	serialize_int(buffer+20, e.end_time);
	strcpy((char*)(buffer+24), e.event);
}
/* Removes the events expired till this time
	Gets the current time using the function localtime, 
	then iterates all users events list one by one and deletes any events whose starting time has already expired
 */
void remove_expired_events()
{
	vector<struct user>::iterator u;
	vector<struct event>::iterator i;
	
	time_t raw_time;
	time(&raw_time);
	struct tm * t = localtime(&raw_time);
	
	for(u=user_list.begin();u!=user_list.end();u++)
	{
		for(i=u->event_list.begin();i!=u->event_list.end();)
		{
			if(
				(2000+i->event_date.year-1900 < t->tm_year) ||
				(2000+i->event_date.year-1900 == t->tm_year && i->event_date.month < t->tm_mon) ||	
				(2000+i->event_date.year-1900 == t->tm_year && i->event_date.month == t->tm_mon && i->event_date.day < t->tm_mday) ||
				(2000+i->event_date.year-1900 == t->tm_year && i->event_date.month == t->tm_mon && i->event_date.day < t->tm_mday && 
																									i->start_time < t->tm_hour*100 + t->tm_min)
				)	
			{
				i = u->event_list.erase(i);
			}
			else
			{
				i++;
			}
		}
	}	
}
int main()
{
	int socket_fd, connect_fd;
	struct sockaddr_in server_addr, client_addr;
	unsigned char message[565];
	char user_name[32];
	unsigned char reply[1];
	unsigned char ret_event[536];
	fd_set master_read_fds, copy_read_fds;
	int max_fd;
	struct user *cur_user;
	int i, j;
	
	FD_ZERO(&master_read_fds);
	FD_ZERO(&copy_read_fds);
	//create socket
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	//set values in the server address
	bzero((char *)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NO);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	//bind the socket
	if(bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr))<0)
	{
		printf("Connection could not be established.\n");
		exit(EXIT_FAILURE);
	}	
	//listen with maximum waiting queue of 5 clients
	listen(socket_fd, 5);	
	FD_SET(socket_fd, &master_read_fds);
	max_fd = socket_fd;
	
	while(1)
	{
		//copy because slect system call changes the array
		copy_read_fds = master_read_fds;
		//select a request, blocks untill infintely is selected
		if(select(max_fd+1, &copy_read_fds, NULL, NULL, NULL)<0)
		{
			exit(EXIT_FAILURE);
		}	
		for(i=0;i<=max_fd;i++)
		{
			//check which one is set
			if(FD_ISSET(i, &copy_read_fds))
			{
				if(i==socket_fd)
				{
					//new client has connected to this socket, add it to the list
					unsigned int client_len = sizeof(client_addr);			
					connect_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &client_len);
					if(connect_fd<0)
						quit("Some client tried to connect, error in connection");
					else
					{
						FD_SET(connect_fd, &master_read_fds);
						if(connect_fd>max_fd)
							max_fd = connect_fd;
					}	
				}
				else
				{
					//we got some data to read from an existing client 
					int fd_chosen = i;
					int data_size_read = read(fd_chosen, message, 565);
					//if no data read, continue after closing the connection
					if(data_size_read == 0)
					{
						close(i);
						FD_CLR(i, &master_read_fds);
						continue;
					}
					else if(data_size_read<0)
					{
						printf("Error in reading the message from client.\n");	
						continue;
					}
					else
					{
						/* Exactly same working as the iterative server */
						remove_expired_events();
					
						get_username_from_message(message, user_name);
		
						cur_user = get_user_from_username(user_name);
						if(cur_user==NULL && message[32]!='a')
						{
							reply[0] = 'u';
							if(write(fd_chosen, reply, 1)<0)
								printf("Error writing on socket.\n");
							continue;
						}	
						switch(message[32])
						{
							case('a'):
							{
								bool event_added = add_event_to_user(cur_user, message);
								if(!event_added)
								{
									reply[0] = 'f';
									write(fd_chosen, reply, 1);
								}
								else
								{
									reply[0] = 's';
									write(fd_chosen, reply, 1);
								}
								break;
							}	
							case('u'):
							{
								bool event_updated = update_event_for_user(cur_user, message);
								if(!event_updated)
								{
									reply[0] = 'f';
									write(fd_chosen, reply, 1);
								}
								else
								{
									reply[0] = 's';
									write(fd_chosen, reply, 1);
								}
								break;
							}	
							case('e'):
							{
								struct event *req_event = get_event_for_user(cur_user, message);
								if(req_event==NULL)
								{
									reply[0] = 'f';
									write(fd_chosen, reply, 1);
								}
								else
								{
									reply[0] = 's';
									write(fd_chosen, reply, 1);
									serialize_event(ret_event, *req_event);
									if(write(fd_chosen, ret_event, 536)<0)
										printf("Error writing onto the socket.\n");
								}
								break;
							}	
							case('d'):
							{
								vector<struct event> day_events = get_date_events_for_user(cur_user, message);
								reply[0] = 's';
								if(write(fd_chosen, reply, 1)<0)
									printf("Error writing onto socket.\n");
								unsigned char size[4];
								serialize_int(size, day_events.size());
								if(write(fd_chosen, size, 4)<0)
									printf("Error writing onto the socket.");
								for(j=0;j<(int)day_events.size();j++)
								{
									serialize_event(ret_event, day_events[j]);
									if(write(fd_chosen, ret_event, 536)<0)
									{
										printf("Error writing onto the socket.\n");
										continue;
									}	
								}
								break;
							}
							case('l'):
							{
								vector<struct event> all_events = get_all_events_for_user(cur_user);
								if(message[33] == 's')
								{
									reply[0] = 's';
									if(write(fd_chosen, reply, 1)<0)
									{
										printf("Error writing onto socket.\n");
									}	
									unsigned char size[4];
									serialize_int(size, all_events.size());
									if(write(fd_chosen, size, 4)<0)
										printf("Error writing onto the socket.");
								}		
								else if(message[33] == 'r')
								{
									int event_requested = deserialize_int(message+34);
									serialize_event(ret_event, all_events[event_requested]);
									if(write(fd_chosen, ret_event, 536)<0)
									{
										printf("Error writing onto the socket.\n");
									}
								}
								else if(message[33] == 'e')
								{
									//connection ended
									close(fd_chosen);
									FD_CLR(fd_chosen, &master_read_fds);
								}				
								break;	
							}
							case('r'):
							{
								bool event_removed = remove_event_from_user(cur_user, message);
								if(!event_removed)
								{
									reply[0] = 'f';
									write(fd_chosen, reply, 1);
								}
								else
								{
									reply[0] = 's';
									write(fd_chosen, reply, 1);
								}
							}				
						}
					}
				}
			}
		}
	}
	return 0;
}

