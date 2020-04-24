/**
 * Starting point code for Project #9 (Parallel Game o' Life)
 *
 * Authors:
 * 	 - Sat Garcia (starter)
 * 	 - Eduardo Ortega
 * 	 - Eric Rosenburg
 */

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

// C doesn't have booleans so we resort to macros. BOO!
#define LIVE 1
#define DEAD 0

// You can modify these if you want your printed board to look a bit
// different.
#define LIVECHAR '@'
#define DEADCHAR '.'

// Sleep for 0.2 seconds (i.e. 200,000us) between turns
#define SLEEPTIME_US 200000

void timeval_subtract (struct timeval *result, 
						struct timeval *x, struct timeval *y);

/**
 * Given 2D coordinates, compute the corresponding index in the 1D array.
 *
 * @param x The x-coord we are converting (i.e. column number)
 * @param y The y-coord we are converting (i.e. row number)
 * @param width The width of the world (i.e. number of columns)
 * @param height The height of the world (i.e. number of rows)
 * @return Index into the 1D world array that corresponds to (x,y)
 */


static unsigned computeIndex(int x, int y, unsigned width, unsigned height) {
	// If x or y coordinates are out of bounds, wrap them arounds
	if ( x < 0 ) {
		x += width;
	}
	else if ( x >= (int)width ) {
		x -= width;
	}

	if ( y < 0 ) {
		y += height;
	}
	else if ( y >= (int)height ) {
		y -= height;
	}
	return x*width + y;
}


/**
 * Prints out the world.
 *
 * @note The exact characters being printed come from the LIVECHAR and
 * DEADCHAR macros
 *
 * @param world The world to print.
 * @param width The width of world.
 * @param height The height of world.
 */
//must one for all the threads to finish updating the world through a barrier
//and then execute the thread for print
static void printWorld(int *world, unsigned width, unsigned height) {
	unsigned i, j;
	unsigned index = 0;
	for (i = 0; i < width; ++i) {
		for (j = 0; j < height; ++j, ++index) {
			if (world[index] == LIVE)
				fprintf(stdout, "%c", LIVECHAR);
			else
				fprintf(stdout, "%c", DEADCHAR);
		}
		fprintf(stdout, "\n");
	}
}

/**
 * Creates and initializes the world based with a given set of cells which are
 * alive at the beginning.
 *
 * @note The set of live cells is given as an index into the world array, not
 * as a set of (x,y) coordinates
 *
 * @param width The width of the world.
 * @param height The height of the world.
 * @param init_set Array of cells that are alive at the beginning
 * @param init_set_size Number of elements in the init_set array
 * @return A 1D board that represents the initialized world.
 */

int* initWorld(unsigned width, unsigned height, 
				unsigned *init_set, unsigned init_set_size) {
	fprintf(stdout, "creating %u by %u world, with %u initial live cells\n",
		   	width, height, init_set_size);
	int *world = calloc(width*height, sizeof(int));

	unsigned i;
	for (i = 0; i < init_set_size; ++i) {
		printf("indexx of alive cell - %u\n", init_set[i]);
		world[init_set[i]] = LIVE;
	}

	return world;
}

/**
 * Returns the number of neighbors around a given (x,y) point that are alive.
 *
 * @param world The world we are simulating
 * @param x x-coord whose neighbors we are examining
 * @param y y-coord whose neighbors we are examining
 * @param width The width of the world
 * @param height The height of the world
 * @return The number of live neighbors around (x,y)
 */

//need to use a semaphore to ensure than each thread will execute this one t a
//time
static unsigned getNumLiveNeighbors(int *world, int x, int y,
									unsigned width, unsigned height) {
	unsigned sum = 0;
	for (int i = x - 1; i <= x + 1;i ++) {
		for (int j = y - 1; j <= y + 1; j++) {
			if( i == x && j == y){
				continue;
			}
			unsigned index = computeIndex(i, j, width, height);
			if (world[index] == LIVE) {
				++sum;
			}
		}
	}
	return sum;
}


/**
 * Updates cell at given coordinate.
 *
 * @param curr_world World for the current turn (read-only).
 * @param next_world World for the next turn.
 * @param x x-coord whose neighbors we are examining
 * @param y y-coord whose neighbors we are examining
 * @param width The width of the world
 * @param height The height of the world
 */

static void computeCell(int *curr_world, int *next_world, 
		unsigned width, unsigned height, int x, int y){

	unsigned index = computeIndex(x, y,width, height);
	unsigned num_live_neighbors = getNumLiveNeighbors(curr_world, 
			x, y, width, height);
	if (curr_world[index] == LIVE
			&& (num_live_neighbors < 2 || num_live_neighbors > 3)) {
		/*
		 * With my cross-bow,
		 * I shot the albatross.
		 */
		next_world[index] = DEAD;
	}
	else if (num_live_neighbors == 3) {
		/*
		 * Oh! Dream of joy! Is this indeed
		 * The light-house top I see?
		 */
		next_world[index] = LIVE;
	}
}

/**
 * Performs one turn of the GOL simulation.
 *
 * @param world The world we are simulating (i.e. game baord)
 * @param width The width of the world
 * @param height The height of the world
 */
static void doTurn(int *world, unsigned width, 
		unsigned height, pthread_barrier_t *barrier, 
		int thread_start, int thread_end) {
	//world copy will contain an unmodified version of the world
	int *world_copy = calloc(width*height, sizeof(int));
	//if there is a error with the malloc, there will be printed an error for
	//it
	if (!world_copy){
		perror("malloc");
	}
	// create copy of world (to read from later)
	for (int i = 0; i < (int)height; i++) {
		for(int j = 0; j < (int)width; j++){
			unsigned index = computeIndex(j, i, width, height);
			world_copy[index] = world[index];
		}
	}
	//waits for all of the threads to make it through
	pthread_barrier_wait(barrier);
	for (int j  = thread_start; j <= thread_end; j++) {
		for (int i = 0; i < (int)width; i++) {
			computeCell(world_copy, world, width, height, i, j);
		}
	}
	free(world_copy);
}

/**
 * Prints a helpful message for how to use the program.
 *
 * @param prog_name The name of the program as called from the command line.
 */
static void usage(char *prog_name) {
	fprintf(stderr, "usage: %s [-v] -c <config-file>\n", prog_name);
	exit(1);
}

/**
 * Creates a world based on the given configuration file.
 *
 * @param config_filename The name of the file which contains the configuration
 * 	for the world, including its initial state.
 * @param width Location to store the world width that is read in from the file.
 * @param height Location to store the world height that is read in from the file.
 * @param num_iters Location to store the number of simulation iterations.
 * @return The newly created world.
 */
static int *createWorld(unsigned *width, unsigned *height,
		unsigned *num_iters, char *config_filename) {
	FILE *config_file = fopen(config_filename, "r");
	if (config_file == NULL) { printf("Error with file\n"); }
	
	int ret;
	unsigned init_set_size;
	ret = fscanf(config_file, "%u", width);
	if (ret != 1) { printf("success\n"); }
	ret = fscanf(config_file, "%u", height);
	if (ret != 1) { printf("success\n"); }
	ret = fscanf(config_file, "%u", num_iters);
	if (ret != 1) { printf("success\n"); }
	ret = fscanf(config_file, "%u", &init_set_size);
	if (ret != 1) { printf("success\n"); }
	unsigned x, y;
	unsigned i = 0;
	unsigned *init_set = malloc((long)init_set_size*sizeof(unsigned));

	while (fscanf(config_file, "%u %u", &x, &y) != EOF) {
		if (i == init_set_size) {
			fprintf(stderr, "ran out of room for coords\n");
			exit(1);
		}
		fprintf(stdout, "%u %u\n", x, y);

		init_set[i] = computeIndex(x, y, *width, *height);
		++i;
	}

	fclose(config_file);

	if((i == init_set_size)) {
		fprintf(stderr, "read in %u coordinate pairs\n", i);
	}
	else{
		printf("the init set size was off\n"
				"init set size = %u \n"
				"value compared to = %u \n",
				init_set_size, i);
		exit(1);
	}
	int *world = initWorld(*width, *height, init_set, init_set_size);

	free(init_set);

	return world;
}


/**
 * Simulates the given world for the specified number of turns.
 *
 * @param world The world to simulate.
 * @param width The height of the world.
 * @param height The height of the world.
 * @param num_iters The number of turns to simulate.
 * @param print_world Whether to print out the world.
 */
static void simulate(int *world, unsigned width, unsigned height, 
		unsigned num_iters, int print_world, pthread_barrier_t *barrier,
		int thread_start, int thread_end) {
	unsigned i = 0;
	int check;
	do {
		check = pthread_barrier_wait(barrier);
		if (print_world && (check == PTHREAD_BARRIER_SERIAL_THREAD)) {
			system("clear");
			fprintf(stdout, "Time step: %u\n", i);
			printWorld(world, width, height);
			usleep(SLEEPTIME_US);
		}
		check = pthread_barrier_wait(barrier);
		if(check != 0 && check != PTHREAD_BARRIER_SERIAL_THREAD){
			perror("Error with wait barrier\n");
			exit(1);
		}
		doTurn(world, width, height, barrier, thread_start, thread_end);
		++i;
	} while ( i <= num_iters );
}
//make the arg struct that we will pass into the threads
struct arg_thread{
	unsigned width;
	unsigned height;
	unsigned num_iters;
	int * world;
	int thread_start;
	int thread_end;
	int num_threads;
	int print_out_threads;
	int print_world;
	pthread_barrier_t *barrier;
	int tid;
};
//thread function we will use - it's forward declaration
void *the_threads(void *th_args);

int main(int argc, char *argv[]) {
	int print_world = 0;
	char *filename = NULL;
	char ch;
	extern char *optarg;
	int num_threads = 4;
	int print_out_threads = 0;
	printf("starting case swicth\n");
	while ((ch = getopt(argc, argv, "vc:t:p")) != -1) {
		switch (ch) {
			case 'v':
				printf("got case v\n");
				print_world = 1;
				break;
			case 'c':
				printf("got case c\n");
				filename = optarg;
				break;
			case 't':
				printf("got case t\n");
				num_threads = strtol(optarg, NULL, 10);
				break;
			case 'p':
				printf("got case p\n");
				print_out_threads = 1;
				break;
			default:
				usage(argv[0]);
		}
	}
	printf("start the program\n");
	if (filename == NULL) {
		usage(argv[0]);
	}
	if(num_threads <= 0){
		printf("we can not run the game with that many threads.\n");
		return 0;
	}
	unsigned width, height, num_iters;
	printf("making the world\n");
	int *world = createWorld(&width, &height, &num_iters, filename);
	printf("num_threads = %d \n", num_threads);
	printf("height = %d \n", height);
	printf("width = %d \n", width);
	printf("num_iters = %d \n", num_iters);
	if(num_threads > (int)height){
		printf("We must specificy less threads than the number of rows\n");
		return 0;
	}
	//setting the struct for the timeval parameters
	struct timeval start_time, end_time, elapsed_time;
	gettimeofday(&start_time, NULL);
	//this is our struct we will use for our array, this array is what/how we will contain our arguements per thread
	struct arg_thread arg_thread;
	struct arg_thread *arg_thread_array = calloc(num_threads, sizeof(arg_thread));
	//the array of threads we will use to pass through simulate
	pthread_t *threads = calloc(num_threads, sizeof(pthread_t));
	//initialize the barrier
	//pthread_barrier_t barrier;
	int wall;
	pthread_barrier_t barrier;
   	wall = pthread_barrier_init(&barrier, NULL, num_threads);
	if(wall != 0){
		perror("error initializing barrier\n");
		exit(1);
	}
	
	
	//wi1ll input the row partition into the struct parameters of
	//thread_begin and thread_end, will do this through our counter i, a
	//remainder and a division
	int i = 0;
	int rows_per_thread = (height / num_threads);
	printf("rows_per_thread = %d \n", rows_per_thread);
	int get_plus_one = (height % num_threads);
	printf("get_plus_one = %d \n", get_plus_one);
	int thread_id = 0;
	//while(i < num_threads){
	while( i < num_threads) {
		while(i < get_plus_one){
			arg_thread_array[i].width = width;
			arg_thread_array[i].height = height;
			arg_thread_array[i].num_iters = num_iters;
			arg_thread_array[i].world = world;
			arg_thread_array[i].print_world = print_world;
			arg_thread_array[i].print_out_threads = print_out_threads;
			arg_thread_array[i].num_threads = num_threads;	
			arg_thread_array[i].barrier = &barrier;
			if(i == 0){
				arg_thread_array[i].thread_start = 0;
				arg_thread_array[i].thread_end = rows_per_thread;
			}
			else{
				arg_thread_array[i].thread_start = (arg_thread_array[i-1].thread_end + 1);
				arg_thread_array[i].thread_end = (arg_thread_array[i-1].thread_end + rows_per_thread + 1);
			}
			arg_thread_array[i].tid = thread_id;
			thread_id++;
			i++;
			}
		arg_thread_array[i].width = width;
		arg_thread_array[i].height = height;
		arg_thread_array[i].num_iters = num_iters;
		arg_thread_array[i].world = world;
		arg_thread_array[i].print_world = print_world;
		arg_thread_array[i].print_out_threads = print_out_threads;
		arg_thread_array[i].num_threads = num_threads;	
		arg_thread_array[i].barrier = &barrier;	
		if( i == 0){
			arg_thread_array[i].thread_start = 0;
			arg_thread_array[i].thread_end = (rows_per_thread - 1);
		}
		else{
			arg_thread_array[i].thread_start = (arg_thread_array[i-1].thread_end + 1);
			arg_thread_array[i].thread_end = (arg_thread_array[i-1].thread_end + rows_per_thread);
		}
		arg_thread_array[i].tid = thread_id;
		thread_id++;
		i++;
		}
	//creates the threads as well as passes in the arg_thread struct that goes
	//into the thread command
	for(int k = 0; k < num_threads; k ++){
		pthread_create(&threads[k], NULL, the_threads, &arg_thread_array[k]);
	}
	//Waits and joins the threads, from merger we can end teh threads
	for(int j = 0; j < num_threads; j++){
		pthread_join(threads[j], NULL);
	}
	gettimeofday(&end_time, NULL);

	timeval_subtract(&elapsed_time, &end_time, &start_time);
	printf("Total time for %d iterations of %dx%d world is %ld.%06ld\n",
			num_iters, width, height, 
			elapsed_time.tv_sec, (long)elapsed_time.tv_usec);
	pthread_barrier_destroy(&barrier);
	free(arg_thread_array);
	free(threads);
	free(world);

	return 0;
}
/*
 *void *the_threads
 *
 *@param1 - void *th_arg - a void pointer for a struct of args
 */
void *the_threads(void *th_arg){
		struct arg_thread *threads = (struct arg_thread*) th_arg;
		//we looped through an array of threads to create them, we will now
		//wait for all of them to ensure that they are runnign at the same
		//time
		int check;
		check = pthread_barrier_wait((threads->barrier));
		if(check != 0 && check != (PTHREAD_BARRIER_SERIAL_THREAD)){
			printf("there is an error with the wait barrier\n");
		}
		simulate((threads->world), (threads->width), (threads->height), 
				(threads->num_iters), (threads->print_world), (threads->barrier),
				(threads->thread_start), (threads->thread_end));
		//ensures that all of the threads are here and then will continue
		pthread_barrier_wait((threads->barrier));
		//prints out the thread info
		if((threads->print_out_threads) == 1){
			printf("tid %d: rows: %d:%d (%d)\n", 
					(threads->tid), (threads->thread_start), (threads->thread_end), 
					((threads->thread_end + 1) - threads->thread_start));
		}
		return NULL;
}

/**
 * Subtracts two timevals, storing the result in third timeval.
 *
 * @param result The result of the subtraction.
 * @param end The end time (i.e. what we are subtracting from)
 * @param start The start time (i.e. the one being subtracted)
 *
 * @url https://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html
 */
void timeval_subtract (struct timeval *result, 
						struct timeval *end, struct timeval *start)
{
	// Perform the carry for the later subtraction by updating start.
	if (end->tv_usec < start->tv_usec) {
		int nsec = (start->tv_usec - end->tv_usec) / 1000000 + 1;
		start->tv_usec -= 1000000 * nsec;
		start->tv_sec += nsec;
	}
	if (end->tv_usec - start->tv_usec > 1000000) {
		int nsec = (end->tv_usec - start->tv_usec) / 1000000;
		start->tv_usec += 1000000 * nsec;
		start->tv_sec -= nsec;
	}

	// Compute the time remaining to wait.tv_usec is certainly positive.
	result->tv_sec = end->tv_sec - start->tv_sec;
	result->tv_usec = end->tv_usec - start->tv_usec;
}
