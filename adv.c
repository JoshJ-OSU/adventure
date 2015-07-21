/*
     Program: johnsjo3.adventure.c
     Date Created: 7/10/2015
     Date Modified: 7/20/2015
     
     Description: This is an adventure type of game where a user explores rooms
                  looking for the exit. Mazes are randomly generated each time
                  the game is played.

*/

/*******************
*    INCLUDED LIBRARIES
********************/
#include <assert.h>                       // assert() to test preconditions
#include <stdlib.h>                       // exit(), srand, rand()
#include <string.h>                       // strncpy() function
#include <stdio.h>                        // printf()
#include <time.h>                         // time() for seeding random num gen
#include <sys/types.h>                    //
#include <sys/stat.h>
#include <unistd.h>                       // access to POSIX system API
#include <dirent.h>


/*************************
*    GLOBALS AND DEFINITIONS
*************************/

/*
* This is the number of room names available for the program to select from
* in function getRoomName().  When changing, modify function accordingly.
*/
#define NAMES_DEFINED 10

/* STRING_LENGTH_MAX is the number of characters used to store a room name. */
#define STRING_LENGTH_MAX 31

enum RoomType { START_ROOM, END_ROOM, MID_ROOM };

struct RoomNode {
     int id;
     char * name;
     int * connections;
     int numConnections;
     enum RoomType roomType;
};

struct RoomsContainer {
     struct RoomNode ** rooms;
     char ** roomNames;
     int numberOfRooms;
};

/**************************
*    FUNCTION DECLARATIONS
***************************/
const char * roomTypeToStr(enum RoomType);
char * getRoomName(int);
void initRoomNames(struct RoomsContainer *);
void deconstructRooms(struct RoomsContainer *);
struct RoomsContainer * initRoomNodes(const int);
void printRooms(struct RoomsContainer *);
void linkNodes(struct RoomsContainer *, int, int);
void buildRandomConnections(struct RoomsContainer *);
void printFile(struct RoomsContainer *, int, char *);
void printToFile(struct RoomsContainer *, char *);
int countFilesInDirectory(char *);
void loadFiles(struct RoomsContainer *, char *);
int fReadLine(FILE *, char *, const int);
int getRoomIndex(struct RoomsContainer *, char *);
struct RoomNode * getStartNode(struct RoomsContainer *);
void printConnections(struct RoomsContainer *, struct RoomNode *);
struct RoomNode * getNextRoom(struct RoomsContainer *, struct RoomNode * cur);

/**************************
*    MAIN
**************************/
/*
* We follow the following algorithm
  1. Declare Variables
  2. Generate Maze
     a. Initiate array
     b. 
  3. 
*/
int main() {
     
     /*
     * DIRECTORY_NAME takes to form of johnsjo3.rooms.<process id>
     * We assume that processID takes up no more than 14 characters
     * On eos-class server, processID max is 32768
     */
     
     char DIRECTORY_NAME[30];
     sprintf(DIRECTORY_NAME, "johnsjo3.rooms.%d", (int)getpid());
     
     // create the directory with permissions
     mkdir(DIRECTORY_NAME, 0777);
     
     // seed random number generator
     srand(time(0));
     
     /*******************/
     /* MAZE GENERATION */
     /*******************/
     // array of roomsNames (Elements = NUMBER_OF_ROOMS)
     // 7 rooms to be created
     /* If increased, must define additional names in function getRoomName() */
     struct RoomsContainer * roomsContainer = initRoomNodes(7);
     
     //set the room names
     initRoomNames(roomsContainer);
     
     // build the connections
     buildRandomConnections(roomsContainer);
     
     // create the room file
     printToFile(roomsContainer, DIRECTORY_NAME);
     
     // Output room names to screen (for debugging)
      printRooms(roomsContainer);
     
     //deallocate maze generation structures from memory
     deconstructRooms(roomsContainer);
     roomsContainer = NULL;
     
     /*************/
     /* LOAD MAZE */
     /*************/
     
     /*
     * At this point, the maze has been deallocated and no longer exists in memory
     * The only place the maze exists is within the files found in the directory
     * defined at the top of the main function
     */     
     
     // Initialize roomsContainer with the number of rooms available
     roomsContainer = initRoomNodes(countFilesInDirectory(DIRECTORY_NAME));
     
     //loads files to roomsContainer struct
     loadFiles(roomsContainer, DIRECTORY_NAME);
     
     
     /**************/
     /* PLAY MAZE  */
     /**************/
     
     const int MAXIMUM_MOVES = 100;
     struct RoomNode * currentLocation;
     struct RoomNode * userSelection;
     char * history[MAXIMUM_MOVES];
     int playerMoves = 0;
     int validInput = 0;
     int i;
     
     // Find the start node, and set the currentLocation to it.
     currentLocation = getStartNode(roomsContainer);
     
     //repeat until the player reaches the end room or the player reaches the maximum
     //possible moves allowed
     while (currentLocation->roomType != END_ROOM && playerMoves < MAXIMUM_MOVES) {          
          
          do{
               printf("CURRENT LOCATION: %s\r\n", currentLocation->name);
               
               printf("POSSIBLE CONNECTIONS: ");
               printConnections(roomsContainer, currentLocation);
               printf("\r\n");

               printf("WHERE TO? >");
               
               //get user input, store new location to current location
               if ( (userSelection = getNextRoom(roomsContainer, currentLocation)) == NULL ) {
                    validInput = 0;
                    printf("\r\n");
                    printf("HUH? I DON’T UNDERSTAND THAT ROOM. TRY AGAIN.\r\n");
               } else {
                    validInput = 1;
                    currentLocation = userSelection;
               }
               printf("\r\n");
          } while (validInput == 0);
          
          history[playerMoves] = currentLocation->name;
          
          playerMoves++;
     }
     
     if (currentLocation->roomType == END_ROOM) {
          printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\r\n");
          printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\r\n", playerMoves);
          //print names of rooms visited in order
          for(i = 0; i < playerMoves; i++) {
               printf("%s\r\n", history[i]);
          }
     } else {
          printf("AFTER WANDERING THE MAZE FOR AGES, YOU DIE OF STARVATION.\r\n");
     }
     
     return 0;
}


/********************************
*    FUNCTIONS
********************************/

const char * roomTypeToStr(enum RoomType t) {
     switch (t) {
          case START_ROOM:
               return "START_ROOM";
               break;
          case END_ROOM:
               return "END_ROOM";
               break;
          case MID_ROOM:
               return "MID_ROOM";
               break;
          default:
               printf("Could not parse RoomType");
               exit(1);
     }
}

/* 
*  char * getRoomName(int n)
*  -------------------------
*  Allocates space for a new string, and assigns a value to it given the
*  This is where the names of rooms are assigned.
*  Input: The integer value of the room name
*  Output: A newly allocated pointer to a string with room name stored
*/
char * getRoomName(int id) {
     //STRING_LENGTH_MAX is defined at top of program (see global constants)
     
     char * roomName;              // pointer to the name to return
     roomName = malloc(sizeof(char) * (STRING_LENGTH_MAX + 1));   // +1 for null char
     
     //Assign string a value.  Not to exceed STRING_LENGTH_MAX.
     // When modifiying this list, change value NAMES_DEFINED at top of program
     switch (id) {
          case 0:
               strncpy(roomName, "Engineering", STRING_LENGTH_MAX);
               break;
          case 1:
               strncpy(roomName, "Bridge", STRING_LENGTH_MAX);
               break;
          case 2:
               strncpy(roomName, "Torpedo", STRING_LENGTH_MAX);
               break;
          case 3:
               strncpy(roomName, "Cargo", STRING_LENGTH_MAX);
               break;
          case 4:
               strncpy(roomName, "Brig", STRING_LENGTH_MAX);
               break;
          case 5:
               strncpy(roomName, "Medical", STRING_LENGTH_MAX);
               break;
          case 6:
               strncpy(roomName, "Armory", STRING_LENGTH_MAX);
               break;
          case 7:
               strncpy(roomName, "Transporter", STRING_LENGTH_MAX);
               break;
          case 8:
               strncpy(roomName, "Captain", STRING_LENGTH_MAX);
               break;
          case 9:
               strncpy(roomName, "Holodeck", STRING_LENGTH_MAX);
               break;
          default:
               // More rooms requested than names available
               printf("Error: Could not assign Room %d a name.\r\n", id);
               exit(1);
               break;
     }
     
     //inserts null character at end of string for strings >= 20 STRING_LENGTH_MAX
     roomName[STRING_LENGTH_MAX] = '\0';
     
     return roomName;
}

/*
* initRoomNames()
* --------------------
* Creates an array of strings and stores names within
* Input: The RoomsContainer struct to define names to 
* Output: An array of strings containing the room names is stored to each room
*         as well as the struct
*/
void initRoomNames(struct RoomsContainer * roomsContainer) {
    
     //allocate space for the names to be stored
     roomsContainer->roomNames = malloc(sizeof(char *) * roomsContainer->numberOfRooms);
     
     // arr is an array that holds true/false values to test that a name has been
     // assigned
     //
     // NAMES_DEFINED is defined at top of program.  It is based on the switch found in
     // function getRoomName().  Update accordingly
     int i;
     int * arr = malloc(sizeof(int *) * NAMES_DEFINED);
     for (i = 0; i < NAMES_DEFINED; i++) {
          arr[i] = 0;
     }
     
     // create a name for each of the rooms, store in  roomsContainer->roomNames array
     for (i = 0; i < roomsContainer->numberOfRooms; i++) {
          int j = rand() % NAMES_DEFINED;
          while ( arr[j] == 1 ) {
               j = rand() % NAMES_DEFINED;
          }
          arr[j] = 1;
          
          roomsContainer->roomNames[i] = getRoomName(j);
          roomsContainer->rooms[i]->name = roomsContainer->roomNames[i];
     }
     
     free(arr);
}


/*
* initRoomNodes()
* Allocates space for a room container struct and space for each node in
* the maze.  Then it initiates each room
*/
struct RoomsContainer * initRoomNodes(const int NUMBER_OF_ROOMS) {
     //allocate space for the Rooms struct
     struct RoomsContainer * roomsContainer = malloc( sizeof (struct RoomsContainer) );
     
     roomsContainer->numberOfRooms = NUMBER_OF_ROOMS;
     
     //create and initialize array of nodes
     roomsContainer->rooms = malloc( sizeof( struct RoomNode * ) * NUMBER_OF_ROOMS);
     
     //room names must be allocated and set separately
     roomsContainer->roomNames = NULL;
     
     
     int i;
     //create the nodes; initialize
     for(i = 0; i < NUMBER_OF_ROOMS; i++) {
          //space for node
          roomsContainer->rooms[i] = malloc( sizeof( struct RoomNode ) );
          
          //starting values for all rooms
          roomsContainer->rooms[i]->id = i;
          roomsContainer->rooms[i]->name = NULL;
          
          roomsContainer->rooms[i]->connections = malloc( sizeof( int ) * NUMBER_OF_ROOMS);
          roomsContainer->rooms[i]->numConnections = 0;
          roomsContainer->rooms[i]->roomType = MID_ROOM;
          
          //initialize connections array with 0
          int j;
          for(j = 0; j < NUMBER_OF_ROOMS; j++) {
               roomsContainer->rooms[i]->connections[j] = 0;
          }
     }
     
     //set starting location of start and end rooms
     roomsContainer->rooms[0]->roomType = START_ROOM;
     roomsContainer->rooms[1]->roomType = END_ROOM;
     
     return roomsContainer;
}

void deconstructRooms(struct RoomsContainer * roomsContainer) {
     
     //iterate through array of rooms, and free memory in each.
     int i;
     for (i = 0; i < roomsContainer->numberOfRooms; i++) {
          //deallocate name pointer
          free(roomsContainer->rooms[i]->name);
          //deallocate connections array
          free(roomsContainer->rooms[i]->connections);
          //deallocate RoomNode struct
          free(roomsContainer->rooms[i]);
          roomsContainer->rooms[i] = NULL;
     }
     
     //deallocate arrays in RoomsContainer
     free(roomsContainer->roomNames);
     free(roomsContainer->rooms);
     
     //change values of struct
     roomsContainer->numberOfRooms = 0;
     roomsContainer->roomNames = NULL;
     roomsContainer->rooms = NULL;
     
}


//For Debugging.  Shows the contents of each room.
void printRooms(struct RoomsContainer * roomsContainer) {
     int i;
     for(i = 0; i < roomsContainer->numberOfRooms; i++) {
          printf("i = %d\r\n", i);
          printf("ID: %d\r\n", roomsContainer->rooms[i]->id);
          printf("Name: %s\r\n", roomsContainer->rooms[i]->name);
          printf("Connections: ");
          int j;
          for(j = 0; j < roomsContainer->numberOfRooms; j++) {
               printf("%d, ", roomsContainer->rooms[i]->connections[j]);
          }
          printf("\r\n");
          printf("Number of Connections: %d\r\n", roomsContainer->rooms[i]->numConnections);
          printf("Room Type: ");
          switch(roomsContainer->rooms[i]->roomType) {
               case START_ROOM:
                    printf("START_ROOM\r\n");
                    break;
               case END_ROOM:
                    printf("END_ROOM\r\n");
                    break;
               case MID_ROOM:
                    printf("MID_ROOM\r\n");
                    break;
               default:
                    printf("Something bad happened\r\n");
                    exit(1);
                    break;
          }
          printf("\r\n");
     }
}

void linkNodes(struct RoomsContainer * roomsContainer, int room1, int room2) {
     //precondition: room1 and room2 cannot have more connections than possible
     assert (roomsContainer->rooms[room1]->numConnections < (roomsContainer->numberOfRooms - 1));
     assert (roomsContainer->rooms[room2]->numConnections < (roomsContainer->numberOfRooms - 1));
     assert (room1 != room2);
     
     //link room1 to room2 and room2 to room1
     roomsContainer->rooms[room1]->connections[room2] = 1;
     roomsContainer->rooms[room2]->connections[room1] = 1;
     
     //increase the number of connections
     roomsContainer->rooms[room1]->numConnections++;
     roomsContainer->rooms[room2]->numConnections++;
}


void buildRandomConnections(struct RoomsContainer * roomsContainer) {
     
     //For 7 rooms, MAXIMUM is 6, MINIMUM is 3
     const int MAXIMUM_CONNECTIONS = roomsContainer->numberOfRooms - 1;
     const int MINIMUM_CONNECTIONS = roomsContainer->numberOfRooms / 2;
     
     // Make MINIMUM_CONNECTIONS per room, plus an extra 'randomizer' connection
     int i;
     for (i = 0; i < MINIMUM_CONNECTIONS + 1; i++) {
          int j;
          for (j = 0; j < roomsContainer->numberOfRooms; j++) {
               //skip if room is fully connected OR
               // for the last connection, 1 in 4 chance of creating an additional connection
               if ( (i < MINIMUM_CONNECTIONS && roomsContainer->rooms[j]->numConnections <= i) ||
                    (i >= MINIMUM_CONNECTIONS && rand() % 4 == 0)
                  ) {
                    /*
                    * Find a random room to link to that
                    * 1. Does not already share a connection
                    * 2. Is not the same room
                    * Otherwise find another number
                    */
                    int r = rand() % roomsContainer->numberOfRooms;
                    while (
                              roomsContainer->rooms[j]->connections[r] == 1 ||
                              r == j
                          )
                    {
                         r = rand() % roomsContainer->numberOfRooms;
                    }
                    
                    //link the nodes, iterated room number and random room
                    linkNodes(roomsContainer, j, r);
               }
          }
     }
}


/*
* printFile
*-----------
* Input: the array of rooms, the id of the room to output, the number of rooms
* in the array, and the name of the file to output
* Output: A file with name FILE_NAME is created with roomname, connections, and node_type 
*
*/
void printFile( struct RoomsContainer * roomsContainer, int roomId, char * FILE_NAME) {
     FILE * fout = fopen(FILE_NAME, "w");
     
     struct RoomNode * room = roomsContainer->rooms[roomId];
     
     //Error check that file opened
     if (fout == NULL) {
          printf("Could not write to file %s\r\n", FILE_NAME);
          exit(1);
     }
     
     //Room name
     fprintf(fout, "ROOM NAME: %s\n", room->name);
     
     //iterate through room connections, and print the connection where it exists
     int i;
     int count = 0;
     for(i = 0; i < roomsContainer->numberOfRooms; i++) {
          if (room->connections[i] == 1) {      //if connection exists
               count++;
               fprintf(fout, "CONNECTION %d: %s\n", count, roomsContainer->rooms[i]->name);
          }
     }
     
     //Room type
     fprintf(fout, "ROOM TYPE: %s\n", roomTypeToStr(room->roomType) );
     
     fclose(fout);
}

/*
* printToFile
*-------------
* Outputs the rooms in memory to file output
* Calls printFile to do the actual file out operations.
* Input: The completed array of rooms, the number of rooms in the array, and the name of the directory
*        To output to
* Output: Calls function printFile to help with the operations.  Outputs room nodes to files
*         File names are numeric values (id) and are located in the directory passed in.
*/ 
void printToFile( struct RoomsContainer * roomsContainer, char * DIRECTORY_NAME) {
     char FILE_NAME[50];
     char fileNum[10];
     int i;
     
     // For each room, create a file called <directory>/<id>
     for(i = 0; i < roomsContainer->numberOfRooms; i++) {
          //construct file name
          FILE_NAME[0] = '\0';      //'clear' contents of FILE_NAME
          strcat(FILE_NAME, DIRECTORY_NAME);
          strcat(FILE_NAME, "/");
          sprintf(fileNum, "%d", roomsContainer->rooms[i]->id);
          strcat(FILE_NAME, fileNum);
          
          //print to filename
          printFile(roomsContainer, i, FILE_NAME);
     } 
}


/*
* countFilesInDirectory()
* Counts the number of files in the directory given excluding '.' and '..'
* Input: Directory Name
* Output: The number of files listed in directory excluding '.' and '..'
*/
int countFilesInDirectory(char * DIRECTORY_NAME) {
  
  DIR *d;
  struct dirent *dir;
  int count = 0;
  
  d = opendir(DIRECTORY_NAME);
  assert (d != NULL);
  
    while ((dir = readdir(d)) != NULL)
    {
      count++;
    }

    closedir(d);
    
    //subtract two for '.' and '..'
    count = count - 2;
    return count;
}


/*
* loadFiles
* ---------
* Loads the files in DIRECTORY_NAME and parses information to RoomsContainer struct
* Input: The directory_name to search
* Output: The RoomsContainer is loaded with the parsed data from the room files
*/
void loadFiles(struct RoomsContainer * roomsContainer, char * DIRECTORY_NAME) {
     // STRING_LENGTH_MAX is defined at top of program

     //declarations
     const int BUFFER_SIZE = 100;
     DIR *d;
     struct dirent *dir;
     int count = 0;               //counts the number of files read (used for indexing)
     char buffer[BUFFER_SIZE];        //an input buffer when reading files
     char filePathName[100];  //file path and name to open is stored in here
     char * roomName = 0;     //pointer to roomName substring
     char * command = 0;      //pointer to command portion of buffer
     char * name = 0;         //pointer to name portion of buffer
     
     // Load names first
     // create an array for names
     roomsContainer->roomNames = malloc(sizeof(char *) * roomsContainer->numberOfRooms);
     
     d = opendir(DIRECTORY_NAME);
     assert (d != NULL);
    
     //skip '.' and '..'
     readdir(d);
     readdir(d);

     //for each entry in directory, load the name of the room stored in the file
     while ((dir = readdir(d)) != NULL)
     {
          buffer[0] = 0;                //'clear' the buffer
          
          //file and path name
          filePathName[0] = 0;
          strcpy(filePathName, DIRECTORY_NAME); 
          strcat(filePathName, "/");
          strcat(filePathName, dir->d_name);
          
          //open file
          FILE * fin = fopen(filePathName, "r");
          
          //get the room name and store into the buffer
          fReadLine(fin, buffer, BUFFER_SIZE);

          fclose(fin);
          
          //find location of roomName
          roomName = strstr(buffer, ": ") + 2;
          
          //allocate space for the room name to be stored
          roomsContainer->roomNames[count] = malloc( sizeof(char) * STRING_LENGTH_MAX);
          
          //copy roomname to our array and to the node
          strcpy(roomsContainer->roomNames[count], roomName);
          roomsContainer->rooms[count]->name = roomsContainer->roomNames[count];
          
          count++;
     }
     closedir(d);

     
     d = opendir(DIRECTORY_NAME);
     assert (d != NULL);
    
     //skip '.' and '..'
     readdir(d);
     readdir(d);
     
     count = 0;
     //for each entry in directory, load the name, connections, and room type.
     while ((dir = readdir(d)) != NULL)
     {
          //file and path name
          filePathName[0] = 0;
          strcpy(filePathName, DIRECTORY_NAME); 
          strcat(filePathName, "/");
          strcat(filePathName, dir->d_name);
          
          //open file
          FILE * fin = fopen(filePathName, "r");
          rewind(fin);
          while ( fReadLine(fin, buffer, BUFFER_SIZE) ) {
               
               //split string based on command and name.
               name = strstr(buffer, ": ") + 2;
               *strstr(buffer, ": ") = 0;
               command = buffer;
               
               if (strcmp(command, "ROOM NAME" ) == 0 ) {
                    //Commands for setting room name
                    assert(strcmp(name, roomsContainer->rooms[count]->name) == 0);
                    
                    //intentionally, no actions here, name is set in earlier loop
                    
               } else if ( strncmp(command, "CONNECTION X", 10) == 0 ) {
                    //set room connections
                    roomsContainer->rooms[count]->connections[getRoomIndex(roomsContainer, name)] = 1;
                    roomsContainer->rooms[count]->numConnections++;
                    
               } else if ( strcmp (command, "ROOM TYPE") == 0) {
                    if ( strcmp(name, "START_ROOM") == 0 ) {
                         roomsContainer->rooms[count]->roomType = START_ROOM;
                    } else if ( strcmp(name, "MID_ROOM") == 0 ) {
                         roomsContainer->rooms[count]->roomType = MID_ROOM;
                    } else if ( strcmp(name, "END_ROOM") == 0 ) {
                         roomsContainer->rooms[count]->roomType = END_ROOM;
                    } else {
                         //could not parse file
                         printf("Could not read ROOM TYPE: %s in file %s\r\n", name, filePathName);
                         exit(1);
                    }
               } else {
                    //error
                    printf("Could not parse command %s in file %s\r\n", command, filePathName);
                    exit(1);
               }
               
               
               
          }
     fclose(fin);    
     //increment count
     count++;
     }
     closedir(d);

}


/*
* fReadLine()
* -----------
* Reads a line from a file and returns it when if reaches either a newline or eof
* Input: File input stream, buffer to store in
* Output: The line from the file excluding any newline characters is stored in buffer.
*         Advances file iterator.
*         Null character added to end of string.
*/
int fReadLine(FILE *fin, char * buffer, const int BUFFER_SIZE) {
     char ch;
     int i = 0;          //index
     int endFlag = 0;
     
     while (endFlag == 0) {
          //retrieve the next character
          ch = fgetc(fin);
          
          if (ch == '\n' || ch == '\r' || ch == 0 || ch == -1) {
               buffer[i] = 0;
               endFlag = 1;
               return i;
          } else {
               buffer[i] = ch;
          }
          
          //error check, prevent buffer overflows
          assert(i < BUFFER_SIZE);
          
          i++;
     }
     
     return i;
}


/*
* getRoomIndex()
* -----------
* Searches through names in RoomsContainer roomNames array and returns the index 
* for the room name (or -1 if not found)
* Input: The RoomsContainer to search through and the string with the name of the room
* Output: Returns the index of the room where name is found
*/
int getRoomIndex(struct RoomsContainer * roomsContainer, char * str) {
     int i;
     for (i = 0; i < roomsContainer->numberOfRooms; i++) {
          if ( strcmp( roomsContainer->roomNames[i], str ) == 0 ) {
               return i;
          }
     }
     
     //index not found
     return -1;
}


/*
* getStartNode()
* -----------
* Finds the start node in RoomsContainer and returns a pointer to the RoomNode
* Input: RoomsContainer completed
* Output: A pointer to RoomNode containing START_NODE
*/
struct RoomNode * getStartNode(struct RoomsContainer * roomsContainer) {
     int i;
     struct RoomNode * ptr;
     
     for(i = 0; i < roomsContainer->numberOfRooms; i++) {
          ptr = roomsContainer->rooms[i];
          if (ptr->roomType == START_ROOM) {
               return ptr;
          }
     }
     
     printf("ERROR: Could not find START_NODE\r\n");
     exit(1); 
}


/*
* printConnections()
* ----------------
* Prints the names of the connections for the given node in the format:
* <connection1>, <connection2>, ..., <connectionN>.
*/
void printConnections(struct RoomsContainer * roomsContainer, struct RoomNode * room) {
     int i;
     int count = 0;
     
     for (i = 0; i < roomsContainer->numberOfRooms; i++) {
          if (room->connections[i] == 1) {
               count++;
               
               if (count < room->numConnections) {
                    printf("%s, ", roomsContainer->roomNames[i]);
               } else {
                    //last connection to print
                    printf("%s.", roomsContainer->roomNames[i]);
               }
                    
          }
     }
}


/*
* setNextRoom
* -----------
* Gets user input. 
* If valid room name, returns pointer to room.
* if invalid, returns NULL.
*/
struct RoomNode * getNextRoom(struct RoomsContainer * roomsContainer, struct RoomNode * cur) {
     char ch;
     int idx;
     const int BUFFER_SIZE = 100;
     char buffer[BUFFER_SIZE];
     
     //get user input
     fgets (buffer, BUFFER_SIZE, stdin);
     *strchr(buffer, '\n') = 0;
     
     if ((idx = getRoomIndex(roomsContainer, buffer)) >= 0) {
          if (cur->connections[idx] == 1)
               return roomsContainer->rooms[idx];
     }
     
     return NULL;

}     