/*********************************************************************************
 * Written by Lance N. Le
 *
 * Free to use unmodified
 * Free to distribute unmodified
 * No warranty, expressed or implied, comes with this program.
 *
 * Once the web server is running, e.g. on the localhost (127.0.0.1), open a
 * browser and browse http://127.0.0.1
 * If running on a port other than 80, then http://127.0.0.1:portnumber
 *
 * Future updates may have threads and forks to handle multiple requests
 * simultaneously.
 *********************************************************************************/
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/limits.h>




/*********************************************************************************
 * Structure definitions.
 *********************************************************************************/
typedef enum {FALSE, TRUE} boolean;

typedef struct
{
   boolean bDebug;
   char    szHostIP   [128+1];
   char    szIndexFile[PATH_MAX+1];
   int     nPort;
} strCommandLine;




/*********************************************************************************
 * Function prototypes.
 *********************************************************************************/
int Debug(const char *, ...);
int DebugOn(int);
int OpenPort(int);
strCommandLine *ProcessCommandLine(strCommandLine *, int, char **);
int ReadClientRequest(int);
int SendIndexFile(int, const char *);




/*********************************************************************************
 * Function: main()
 * Params:   argc
 *           argv
 * Returns:  0 - no errors
 *           Less than 0 means a problem, such as opening socket, binding, etc.
 * Call by:  Shell
 * Call to:  Debug()
 *           DebugOn()
 *           ProcessCommandLine()
 *           ReadClientRequest()
 *           SendIndexFile()
 * Overview: Program entry point.
 *           Reads the command line options and executes a simple webserver.  The
 *           webserver listens for client requests, accepts request, and then sends
 *           the content of index.htm.
 * Notes:    Sample: --port 80 --listenip 127.0.0.1 --indexfile ./index.htm --debug
 *           Sample: --port 80 --listenip 127.0.0.1 --indexfile ./index.htm
 *           Running on a low number port requires priviledged access; otherwise,
 *           bind() will fail.
 *           127.0.0.1 is the local host this program is running on.
 *           --debug is the only optional argument.
 *           Exiting the program with CTRL-C may hold the port open for a few
 *           minutes.
 *********************************************************************************/
int main(int argc, char *argv[])
{
   strCommandLine strRunOptions;
   int nExitCode = 0;



   memset(&strRunOptions, 0, sizeof(strRunOptions));


   if (argc < 7)
   {
      printf("Example: %s --port 80 --listenip 127.0.0.1 --indexfile ./index.htm --debug\n", argv[0]);
      printf("Example: %s --port 80 --listenip 127.0.0.1 --indexfile ./index.htm\n", argv[0]);
      printf("Low port numbers may require root access to open.\n");
      printf("--debug is the only optional argument.\n");
      printf("CTRL-C to exit the program may leave the port open for a few minutes.\n");
   }
   else
   {
      int nSocketClient = 0;
      int nSocketHost   = 0;
      int nLength       = sizeof(struct sockaddr_in);
      struct sockaddr_in strClient;


      ProcessCommandLine(&strRunOptions, argc, argv);


      /***************************************************************************
       * First call to DebugOn() will determine whether Debug() will write debug
       * messages for the rest of the program.
       *
       * If bDebug is FALSE then no logging messages are displayed when calling
       * Debug(). 
       * If bDebug is TRUE then logging messages are displayed when calling Debug().
       ***************************************************************************/
      DebugOn(strRunOptions.bDebug);

      Debug("Port: %d; IP: %s; Index file: %s; Debug: %s;\n\n",
            strRunOptions.nPort,
            strRunOptions.szHostIP,
            strRunOptions.szIndexFile,
            strRunOptions.bDebug==TRUE?"On":"Off");


      nSocketHost = OpenPort(strRunOptions.nPort);
 

      /***************************************************************************
       * Listen to the server socket, and wait for an incoming client request to
       * accept.  Once accepted (a socket to the client will be created), read the
       * header request from the browser/client.  Then read the index.htm file and
       * send the content of index.htm back to the client.
       *
       * There's no clean exit from the program at the moment.  The program can be
       * stopped with CTRL-C or a kill command.
       ***************************************************************************/
      for (;;)
      {
         if (listen(nSocketHost, 3) < 0)
         {
            fprintf(stderr, "Failed to listen(). errno=%d.\n", errno);
            break;
         }
         else
         {
            Debug("Listening...\n");
         }

         memset(&strClient, 0, sizeof(strClient));

         if ((nSocketClient = accept(nSocketHost,(struct sockaddr *)&strClient,(socklen_t*)&nLength)) <= 0)
         {
            fprintf(stderr, "Failed to accept(). errno=%d.\n", errno);
            break;
         }
         else
         {
            Debug("Reading from client...\n");
            ReadClientRequest(nSocketClient);

            Debug("About to write to client...\n");
            SendIndexFile(nSocketClient, strRunOptions.szIndexFile);
         }
      }


      close(nSocketClient);
      close(nSocketHost);
   }



   exit(nExitCode);
}




/*********************************************************************************
 * Function: OpenPort()
 * Params:   nPort
 * Returns:  -1 - socket() failed
 *           -2 - bind() failed (possibly port is open from previous run)
 *           Socket id on success
 * Call by:  main()
 * Call to:  None
 * Overview: Opens a port for the server to listen to client connections.
 * Notes:    Port is passed to the program with --port option.
 *           Low port numbers may require root access to open.
 *********************************************************************************/
int OpenPort(int nPort)
{
   int nSocket     = 0;
   int nReturnCode = 0;



   if ((nSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
   {
      fprintf(stderr, "Failed to create socket with socket(). errno=%d.\n", errno);
      nReturnCode = -1;
   }
   else
   {
      struct sockaddr_in sockWebServer;


      nReturnCode = nSocket;
      memset(&sockWebServer, 0, sizeof(sockWebServer));


      sockWebServer.sin_family      = AF_INET;
      sockWebServer.sin_port        = htons(nPort);
      sockWebServer.sin_addr.s_addr = INADDR_ANY;


      if (bind(nSocket, (struct sockaddr *) &sockWebServer, sizeof(sockWebServer)) < 0) 
      {
         fprintf(stderr, "Failed to bind socket id %d with bind(). errno=%d.\n", nSocket, errno);
         fprintf(stderr, "Likely port %d is still open from a previous run.\n", nPort);
         nReturnCode = -2;
      }
      else
      {
         nReturnCode = nSocket;
      }
   }     



   return(nReturnCode);
}




/********************************************************************************
 * Function: ProcessCommandLine
 * Params:   pstrRunOptions - stores options read from the command line
 *           argc - number of command line parameters
 *           argv - each command line parameter is a string
 * Returns:  Pointer to strCommandLine with options from command line.
 * Call by:  main()
 * Call to:  None
 * Overview: Reads arguments from command line and fills options structure.
 *           Valid options are
 *           --debug (optional)
 *           --indexfile
 *           --listenip
 *           --port
 * Notes:    None
 ********************************************************************************/
strCommandLine *ProcessCommandLine(strCommandLine *pstrRunOptions, int argc, char **argv)
{
   int nIndex = 0;



   pstrRunOptions->bDebug = FALSE;


   for (nIndex=1; nIndex<argc; nIndex++)
   {
      if (strcmp(argv[nIndex], "--port") == 0)
      {
         pstrRunOptions->nPort = atoi(argv[nIndex+1]);
      }
      else if (strcmp(argv[nIndex], "--listenip") == 0)
      {
         strncpy(pstrRunOptions->szHostIP,
                 argv[nIndex+1],
                 sizeof(pstrRunOptions->szHostIP)-1);
      }
      else if (strcmp(argv[nIndex], "--indexfile") == 0)
      {
         strncpy(pstrRunOptions->szIndexFile,
                 argv[nIndex+1],
                 sizeof(pstrRunOptions->szIndexFile)-1);
      }
      else if (strcmp(argv[nIndex], "--debug") == 0)
      {
         pstrRunOptions->bDebug = TRUE;
      }
   }



   return (pstrRunOptions);
}




/********************************************************************************
 * Function: ReadClientRequest
 * Params:   nSocketClient
 * Returns:  0  - no errors
 *           -1 - failed to read header information from client
 * Call by:  main()
 * Call to:  Debug()
 * Overview: Reads from socket established with the client and gets info the
 *           browser header information.
 * Notes:    None
 ********************************************************************************/
int ReadClientRequest(int nSocketClient)
{
   char szBuffer[BUFSIZ+1] = "";
   int  nReturnCode        = 0;



   if ((nReturnCode = recv(nSocketClient, szBuffer, sizeof(szBuffer)-1, 0)) < 0)
   {
      fprintf(stderr, "Failed to recv(). errno=%d.\n", errno);
      nReturnCode = -1;
   }
   else
   {
      Debug("Read from client:\n%s\n", szBuffer);
   }



   return(nReturnCode);
}




/********************************************************************************
 * Function: SendIndexFile()
 * Params:   nSocketClient
 *           pszFile
 * Returns:  -1 - unable to open input file
 *           Number of characters read from input file
 * Call by:  main()
 * Call to:  Debug()
 * Overview: Sends head information back to client and the contents of index.htm.
 * Notes:    nSocketClient must be already be established to the client
 ********************************************************************************/
int SendIndexFile(int nSocketClient, const char *pszFile)
{
   FILE *fpinInput   = NULL;
   int   nReturnCode = 0;



   if ((fpinInput = fopen(pszFile, "r")) == NULL)
   {
      nReturnCode = -1;
   }
   else
   {
      unsigned char  szBuffer[BUFSIZ+1] = "";
      int            nBytes             = 0;

      const char szHead1[128+1] = "HTTP/1.1 200 OK\n";
      char       szHead2[128+1] = "";
      const char szHead3[128+1] = "Content-Type: text/html\n\n";


      for (; feof(fpinInput) == 0;)
      {
         nBytes = fread(szBuffer, 1, sizeof(szBuffer)-1, fpinInput);
         Debug("Read from file %s:\n%s\n", pszFile, szBuffer);
         Debug("Read %d bytes from file %s\n", nBytes, pszFile);
      }

      sprintf(szHead2, "Content-length: %d\n", nBytes);


      Debug("Writing \n%s\n", szHead1);
      Debug("Writing \n%s\n", szHead2);
      Debug("Writing \n%s\n", szHead3);
      Debug("Writing \n%s\n", szBuffer);

      write(nSocketClient, szHead1, strlen(szHead1));
      write(nSocketClient, szHead2, strlen(szHead2));
      write(nSocketClient, szHead3, strlen(szHead3));
      write(nSocketClient, szBuffer, nBytes);


      fclose(fpinInput);
      nReturnCode = nBytes;
   }



   return(nReturnCode);
}




/********************************************************************************
 * Function: Debug()
 * Params:   pszFormat - Formatting of variable parameters.
 *           ...
 * Returns:  Returns the number of characters printed by vprintf()
 * Call by:  most fuctions in this program
 * Call to:  DebugOn()
 * Overview: Debugging aid if needed, which prints messages to standard output.
 * Notes:    va_start, va_end uses stdarg.h
 *           DebugOn(0) the zero does not matter, it can be any value, but 0 for
 *           simplicity.
 *           Debug is enabled by calling the program with --debug
 ********************************************************************************/
int Debug(const char *pszFormat, ...)
{
    int nReturnCode = 0;
    va_list pstrList;



    if (DebugOn(0) == TRUE)
    {
       printf("DEBUG ");

       va_start(pstrList, pszFormat);
       nReturnCode = vprintf(pszFormat, pstrList);
       va_end(pstrList);
    }


    return(nReturnCode);
}




/********************************************************************************
 * Function: DebugOn()
 * Params:   bInit - value is either TRUE or FALSE on the first call.
 *           On subsequent calls, bInit doesn't matter, but should be something
             simple like 0.
 * Returns:  TRUE
 *           FALSE
 * Call by:  main()
 *           Debug()
 * Call to:  None
 * Overview: The first time DebugOn() is called, bDebug will always be -100.  The
 *           value of bInit will determine whether bDebug will be TRUE or FALSE.
 *           On subsequent calls to DebugOn(), bDebug will either have a value of
 *           TRUE or FALSE, that is, it's only -100 on the first call only.
 *
 *           if bInit is FALSE on the first call, debugging is turned off.
 *           if bInit is TRUE  on the first call, debugging is turned on.
 *           Subsequent calls to DebugOn() checks whether debug messages should be
 *            printed.
 *           The argument on subsequent calls does not matter, but use 0 for
 *           simplicity.
 * Notes:    This keeps us from having to utilize a global variable.
 ********************************************************************************/
int DebugOn(int bInit)
{
   static int bDebug = -100;



   if (bDebug == -100)
   {
      if (bInit == TRUE)
      {
         bDebug = TRUE;
      }
      else
      {
         bDebug = FALSE;
      }
   }



   return(bDebug);
}
