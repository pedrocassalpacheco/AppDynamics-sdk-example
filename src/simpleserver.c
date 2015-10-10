#include <stdio.h>
#include <stdlib.h>
#include <resolv.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <arpa/inet.h>
#include <termios.h>
#include <signal.h>

#include <appdynamics_sdk.h>
#include <backtrace.h>

#define MY_PORT         9999
#define MAXBUF          1024

int gSockfd;

void assign_appd_value(APPD_SDK_HANDLE_BT, char*, long);
void reset_appdynamics();
void assign_appd_value_str(APPD_SDK_HANDLE_BT, char*, char*);

void signal_callback_handler(int signum)
{
        printf("\n**** Cntr-C detected. ****\n");

        close(gSockfd);
        reset_appdynamics();
        exit(signum);
}

void memory_profile(APPD_SDK_HANDLE_BT btHandle)
{
	if (appdynamics_bt_isSnapshotEnabled(btHandle)) 
	{
		printf("\n**** Snapshot request detected. ****\n");
        	
		struct rusage ru;
	        getrusage(RUSAGE_SELF, &ru);
		
		assign_appd_value(btHandle, "Maximum resident set size",ru.ru_maxrss);
                assign_appd_value(btHandle, "Integral shared memory size",ru.ru_ixrss);
		assign_appd_value(btHandle, "Integral unshared data size",ru.ru_idrss);
		assign_appd_value(btHandle, "Integral unshared stack size",ru.ru_isrss);
		assign_appd_value(btHandle, "Page reclaims (soft page faults)",ru.ru_minflt);
		assign_appd_value(btHandle, "Page faults (hard page faults)",ru.ru_majflt);
		assign_appd_value(btHandle, "Swaps",ru.ru_nswap);
		assign_appd_value(btHandle, "Block input operations",ru.ru_inblock);
		assign_appd_value(btHandle, "Block output operations",ru.ru_oublock);
		assign_appd_value(btHandle, "IPC messages sent",ru.ru_msgsnd);
		assign_appd_value(btHandle, "Signals received",ru.ru_nsignals);
		assign_appd_value(btHandle, "Voluntary context switches", ru.ru_nvcsw);
		assign_appd_value(btHandle, "Involuntary context switches",ru.ru_nivcsw);

		char* backtrace = concatenate_backtrace();
		assign_appd_value_str(btHandle, "Call Stack", backtrace);

		if (backtrace !=NULL) 
		{
			free(backtrace);
		}

	}
}

void assign_appd_value(APPD_SDK_HANDLE_BT btHandle, char* str_name, long long_value)
{

        APPD_SDK_STRING* name = APPD_SDK_STRING_new();
        APPD_SDK_STRING* value = APPD_SDK_STRING_new();

	APPD_SDK_STRING_assign(name, str_name, strlen(str_name)+1);
	char  *tmp = (char*)malloc(sizeof(char)*1024);
	snprintf(tmp,1024,"%li",long_value);

	APPD_SDK_STRING_assign(value, tmp, strlen(tmp)+1);

	free(tmp);

	appdynamics_bt_addUserData(btHandle,name,value);
        APPD_SDK_STRING_delete(name);
        APPD_SDK_STRING_delete(value);
} 

void assign_appd_value_str(APPD_SDK_HANDLE_BT btHandle, char* str_name, char* str_value)
{

        APPD_SDK_STRING* name = APPD_SDK_STRING_new();
        APPD_SDK_STRING* value = APPD_SDK_STRING_new();

        APPD_SDK_STRING_assign(name, str_name, strlen(str_name)+1);
        APPD_SDK_STRING_assign(value, str_value, strlen(str_value)+1);

        appdynamics_bt_addUserData(btHandle,name,value);
        APPD_SDK_STRING_delete(name);
        APPD_SDK_STRING_delete(value);
}

bool http_payload_reflector(APPD_SDK_BT_PAYLOAD_COMPONENT_TYPE payloadComponentType APPD_SDK_PARAM_IN,
         	           	const char* payloadComponentName APPD_SDK_PARAM_IN,
                	       	void* payloadVoid APPD_SDK_PARAM_IN,
                       		char* buffer APPD_SDK_PARAM_OUT,
                          	unsigned buffer_size APPD_SDK_PARAM_IN) 
{
    
    switch (payloadComponentType) {
    case APPD_BT_PAYLOAD_COMPONENT_TYPE(bt_name):
      {
        memcpy(buffer, "Test",strlen("Test")+1);
        return true;
      }
    default:
      {
        return false;
      }
    }
}

void initialize_appdynamics()
{
    APPD_SDK_ENV_RECORD env[8];
    
    env[0][0] = APPD_SDK_ENV_CONTROLLER_HOST;
    env[0][1] = "controller-mac";
    
    env[1][0] = APPD_SDK_ENV_CONTROLLER_PORT;
    env[1][1] = "8090";
    
    env[2][0] = APPD_SDK_ENV_CONTROLLER_SSL;
    env[2][1] = "0";
    
    env[6][0] = APPD_SDK_ENV_ACCOUNT_NAME;
    env[6][1] = "customer1";
    env[7][0] = APPD_SDK_ENV_ACCOUNT_ACCESS_KEY;
    env[7][1] = "33c68c2c-1e06-4553-9d92-314c370f64ed";
    
    env[3][0] = APPD_SDK_ENV_APPLICATION;
    env[3][1] = "SimpleServer";
    
    env[4][0] = APPD_SDK_ENV_TIER;
    env[4][1] = "Web Server";
    
    env[5][0] = APPD_SDK_ENV_NODE;
    env[5][1] = "Node0";
    
    APPD_SDK_STATUS_CODE res = APPD_SUCCESS;
    res=appdynamics_sdk_init(env,8);

	if (res == APPD_SUCCESS)
	{
	    printf("AppDynamcis C++ Native SDK initialized.\n");
	} 
 	else
 	{
	    printf("AppDynamics C++ Native SDK not initialized.\n");
	} 

}

void reset_appdynamics()
{
	appdynamics_sdk_term();
}

int main(int Count, char *Strings[])
{
    
    struct sockaddr_in self;
    char buffer[MAXBUF];
    
	// Initialize AppDynamics
	initialize_appdynamics();
   
	// Handles kill and cntr c 
        signal(SIGINT, signal_callback_handler);

    	// Create streaming socket
    	if ( (gSockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    	{
        	perror("Socket");
        	exit(errno);
    	}	
    
    	// Initialize address/port structure
    	bzero(&self, sizeof(self));
    	self.sin_family = AF_INET;
    	self.sin_port = htons(MY_PORT);
    	self.sin_addr.s_addr = INADDR_ANY;
    
    	// Assign a port number to the socket 
    	if ( bind(gSockfd, (struct sockaddr*)&self, sizeof(self)) != 0 )
    	{
        	perror("socket--bind");
        	exit(errno);
    	}
    
    	// Make it a "listening socket
    	if ( listen(gSockfd, 20) != 0 )
    	{
        	perror("socket--listen");
        	exit(errno);
    	}
   
	printf("\n**** Listening for request on port %d ****\n", MY_PORT);

        while (true)
        {   
            int clientfd;
            struct sockaddr_in client_addr;
            int addrlen=sizeof(client_addr);
        
            // Accept a connection (creating a data pipe)
            clientfd = accept(gSockfd, (struct sockaddr*)&client_addr, &addrlen);
            printf("\n**** %s:%d connected ****\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            // Declare a handle for this business transaction
            APPD_SDK_HANDLE_BT   btHandle = NULL;
            APPD_SDK_STATUS_CODE res      = APPD_SUCCESS;
        
            // Payload for HTTP
            int bytesIn = recv(clientfd, buffer, MAXBUF,0);

            res = appdynamics_bt_begin(APPD_BT_TYPE(native),
                                        &http_payload_reflector,
                                        &buffer,
                                        &btHandle);

            // Memory profile
            memory_profile(btHandle);

            // Echo back anything sent
            send(clientfd, buffer, bytesIn, 0);
        
            // Close data connection
            close(clientfd);
        
            res = appdynamics_bt_end_success(btHandle);
        }
   
        // All cleanup done by signal handler

    return 0;
}
