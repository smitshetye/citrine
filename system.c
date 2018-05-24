#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <math.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <sys/wait.h>
#include <syslog.h>
#include <signal.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef forLinux
#include <bsd/stdlib.h>
#include <bsd/string.h>
#endif

#include "citrine.h"
#include "siphash.h"

// call this function to start a nanosecond-resolution timer
struct timespec timer_start(){
    struct timespec start_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);
    return start_time;
}

// call this function to end a timer, returning nanoseconds elapsed as a long
long timer_end(struct timespec start_time){
    struct timespec end_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time);
    long diffInNanos = (end_time.tv_sec - start_time.tv_sec) * (long)1e9 + (end_time.tv_nsec - start_time.tv_nsec);
    return diffInNanos;
}

int CtrTimerStartEnd = 0;

struct timespec vartime;
long time_elapsed_nanos;

/**
 * @internal
 * GarbageCollector Marker
 */
void ctr_gc_mark(ctr_object* object) {
	ctr_object* el;
	ctr_mapitem* item;
	ctr_object* o;
	ctr_object* k;
	long i;
	if (object->info.type == CTR_OBJECT_TYPE_OTARRAY) {
		for (i = 0; i < object->value.avalue->head; i++) {
			el = *(object->value.avalue->elements+i);
			el->info.mark = 1;
			ctr_gc_mark(el);
		}
	}
	item = object->properties->head;
	while(item) {
		k = item->key;
		o = item->value;
		o->info.mark = 1;
		k->info.mark = 1;
		ctr_gc_mark(o);
		item = item->next;
	} 
	item = object->methods->head;
	while(item) {
		o = item->value;
		k = item->key;
		o->info.mark = 1;
		k->info.mark = 1;
		ctr_gc_mark(o);
		item = item->next;
	} 
}

/**
 * @internal
 * GarbageCollector Sweeper
 */
void ctr_gc_sweep( int all ) {
	ctr_object* previousObject = NULL;
	ctr_object* currentObject = ctr_first_object;
	ctr_object* nextObject = NULL;
	ctr_mapitem* mapItem = NULL;
	ctr_mapitem* tmp = NULL;
	while(currentObject) {
		ctr_gc_object_counter ++;
		if ( ( currentObject->info.mark==0 && currentObject->info.sticky==0 ) || all){
			ctr_gc_dust_counter ++;
			/* remove from linked list */
			if (previousObject) {
				if (currentObject->gnext) {
					previousObject->gnext = currentObject->gnext;
					nextObject = currentObject->gnext;
				} else {
					previousObject->gnext = NULL;
					nextObject = NULL;
				}
			} else {
				if (currentObject->gnext) {
					ctr_first_object = currentObject->gnext;
					nextObject = currentObject->gnext;
				} else {
					ctr_first_object = NULL;
					nextObject = NULL;
				}
			}
			if (currentObject->methods->head) {
				mapItem = currentObject->methods->head;
				while(mapItem) {
					tmp = mapItem->next;
					ctr_heap_free( mapItem );
					mapItem = tmp;
				}
			}
			if (currentObject->properties->head) {
				mapItem = currentObject->properties->head;
				while(mapItem) {
					tmp = mapItem->next;
					ctr_heap_free( mapItem );
					mapItem = tmp;
				}
			}
			ctr_heap_free( currentObject->methods );
			ctr_heap_free( currentObject->properties );
			switch (currentObject->info.type) {
				case CTR_OBJECT_TYPE_OTSTRING:
					if (currentObject->value.svalue != NULL) {
						if (currentObject->value.svalue->vlen > 0) {
							ctr_heap_free( currentObject->value.svalue->value );
						}
						ctr_heap_free( currentObject->value.svalue );
					}
				break;
				case CTR_OBJECT_TYPE_OTARRAY:
					ctr_heap_free( currentObject->value.avalue->elements );
					ctr_heap_free( currentObject->value.avalue );
				break;
				case CTR_OBJECT_TYPE_OTEX:
					if (currentObject->value.rvalue != NULL) ctr_heap_free( currentObject->value.rvalue );
				break;
			}
			ctr_heap_free( currentObject );
			currentObject = nextObject;
		} else {
			ctr_gc_kept_counter ++;
			if (currentObject->info.sticky==1) ctr_gc_sticky_counter++;
			if (currentObject->info.mark == 1) {
				currentObject->info.mark = 0;
			}
			previousObject = currentObject;
			currentObject = currentObject->gnext;
		}
	}
}

/**
 * @internal
 * Garbage Collector sweep.
 */
void  ctr_gc_internal_collect() {
	ctr_object* context;
	int oldcid;
	ctr_gc_dust_counter = 0;
	ctr_gc_object_counter = 0;
	ctr_gc_kept_counter = 0;
	ctr_gc_sticky_counter = 0;
	context = ctr_contexts[ctr_context_id];
	oldcid = ctr_context_id;
	while(ctr_context_id > -1) {
		ctr_gc_mark(context);
		ctr_context_id--;
		context = ctr_contexts[ctr_context_id];
	}
	ctr_gc_sweep( 0 );
	ctr_context_id = oldcid;
}

/**
 * Broom
 *
 * GarbageCollector, to invoke use:
 *
 * [Broom] sweep.
 */
ctr_object* ctr_gc_collect (ctr_object* myself, ctr_argument* argumentList) {
	ctr_gc_internal_collect(); /* calls internal because automatic GC has to use this function as well and requires low overhead. */
	return myself;
}

/**
 * [Broom] dust
 *
 * Returns the number of objects collected.
 */
ctr_object* ctr_gc_dust(ctr_object* myself, ctr_argument* argumentList) {
	return ctr_build_number_from_float((ctr_number) ctr_gc_dust_counter);
}

/**
 * [Broom] objectCount
 *
 * Returns the total number of objects considered in the latest collect
 * cycle.
 */
ctr_object* ctr_gc_object_count(ctr_object* myself, ctr_argument* argumentList) {
	return ctr_build_number_from_float((ctr_number) ctr_gc_object_counter);
}

/**
 * [Broom] keptCount
 *
 * Returns the total number of objects that have been marked during the
 * latest cycle and have therefore been allowed to stay in memory.
 */
ctr_object* ctr_gc_kept_count(ctr_object* myself, ctr_argument* argumentList) {
	return ctr_build_number_from_float((ctr_number) ctr_gc_kept_counter);
}

/**
 * [Broom] keptAlloc
 *
 * Returns the amount of allocated memory.
 */
ctr_object* ctr_gc_kept_alloc(ctr_object* myself, ctr_argument* argumentList) {
	return ctr_build_number_from_float((ctr_number) ctr_gc_alloc);
}

/**
 * [Broom] stickyCount
 *
 * Returns the total number of objects that have a sticky flag.
 * These objects will never be removed.
 */
ctr_object* ctr_gc_sticky_count(ctr_object* myself, ctr_argument* argumentList) {
	return ctr_build_number_from_float((ctr_number) ctr_gc_sticky_counter);
}

/**
 * [Broom] memoryLimit
 *
 * Sets the memory limit, if this limit gets exceeded the program will produce
 * an out-of-memory error.
 */
ctr_object* ctr_gc_setmemlimit(ctr_object* myself, ctr_argument* argumentList) {
	ctr_gc_memlimit = (uint64_t) ctr_internal_cast2number( argumentList->object )->value.nvalue;
	return myself;
}

/**
 * [Broom] mode: [Number]
 *
 * Selects mode of operation for GC.
 *
 * Available Modes:
 * 0 - No Garbage Collection
 * 1 - Activate Garbage Collector (default)
 * 4 - Activate Garbage Collector for every single step (testing only)
 * 8 - Activate experimental Pool Memory Allocation Manager (experimental!)
 */
ctr_object* ctr_gc_setmode(ctr_object* myself, ctr_argument* argumentList) {
	ctr_gc_mode = (int) ctr_internal_cast2number( argumentList->object )->value.nvalue;
	if (ctr_gc_mode & 8) {
		ctr_pool_init(ctr_gc_memlimit/2);
	}
	return myself;
}

/**
 * [Broom] toString.
 *
 * Returns the number of objects that have been collected by the
 * garbage collector.
 */
ctr_object* ctr_gc_to_string(ctr_object* myself, ctr_argument* argumentList) {
	return ctr_internal_cast2string( ctr_gc_dust( myself, argumentList ) );
}

/**
 * [Broom] number.
 *
 * Returns the number of objects that have been collected by the
 * garbage collector.
 */
ctr_object* ctr_gc_to_number(ctr_object* myself, ctr_argument* argumentList) {
	return ctr_internal_cast2number( ctr_gc_dust( myself, argumentList ) );
}

/**
 * [Program] shell: [String]
 *
 * Performs a Shell operation. The Shell object uses a fluid API, so you can
 * mix shell code with programming logic. For instance to list the contents
 * of a directory use:
 *
 * Shell ls
 *
 * This will output the contents of the current working directly, you
 * can also pass keyword messages like so:
 *
 * Shell echo: 'Hello from the Shell!'.
 *
 * The example above will output the specified message to the console.
 * Every message you send will be turned into a string and dispatched to
 * the 'call:' message.
 */
ctr_object* ctr_program_shell(ctr_object* myself, ctr_argument* argumentList) {
	FILE* stream;
	char* outputBuffer;
	ctr_argument* newArgumentList;
	ctr_object* appendString;
	ctr_object* outputString;
	outputBuffer = ctr_heap_allocate( 512 );
	ctr_object* arg = ctr_internal_cast2string(argumentList->object);
	long vlen = arg->value.svalue->vlen;
	char* comString = ctr_heap_allocate( sizeof( char ) * ( vlen + 1 ) );
	memcpy(comString, arg->value.svalue->value, vlen);
	memcpy(comString+vlen,"\0",1);
	newArgumentList = (ctr_argument*) ctr_heap_allocate( sizeof( ctr_argument ) );
	if ( !( stream = popen( comString, "r" ) ) ) {
		CtrStdFlow = ctr_build_string_from_cstring( "Unable to execute command." );
	}
	outputString = ctr_build_empty_string();
	while ( fgets( outputBuffer, 512, stream ) ) {
		appendString = ctr_build_string_from_cstring( outputBuffer );
		newArgumentList->object = appendString;
		ctr_string_append( outputString, newArgumentList );
	}
	pclose( stream );
	ctr_heap_free( outputBuffer );
	ctr_heap_free( newArgumentList );
	ctr_heap_free( comString );
	return outputString;
}

/**
 * [Program] argument: [Number]
 *
 * Obtains an argument from the CLI invocation.
 */
ctr_object* ctr_program_argument(ctr_object* myself, ctr_argument* argumentList) {
	ctr_object* numberObject = ctr_internal_cast2number(argumentList->object);
	int n = (int) numberObject->value.nvalue;
	if (n >= ctr_argc || n < 0) return CtrStdNil;
	return ctr_build_string(ctr_argv[n], strlen(ctr_argv[n]));
}

/**
 * [Program] argCount
 *
 * Returns the number of CLI arguments passed to the script.
 */
ctr_object* ctr_program_num_of_args(ctr_object* myself, ctr_argument* argumentList) {
	return ctr_build_number_from_float( (ctr_number) ctr_argc );
}

/**
 * [Program] exit
 * 
 * Exits program immediately.
 */
ctr_object* ctr_program_exit(ctr_object* myself, ctr_argument* argumentList) {
	CtrStdFlow = CtrStdExit;
	return CtrStdNil;
}

/**
 * [Program] env: [String]
 *
 * Returns the value of an environment variable.
 *
 * Usage:
 *
 * x := Command env: 'MY_PATH_VAR'.
 */
ctr_object* ctr_program_get_env(ctr_object* myself, ctr_argument* argumentList) {
	ctr_object* envVarNameObj;
	char*       envVarNameStr;
	char*       envVal;
	envVarNameObj = ctr_internal_cast2string(argumentList->object);
	envVarNameStr = ctr_heap_allocate((envVarNameObj->value.svalue->vlen+1)*sizeof(char));
	strncpy(envVarNameStr, envVarNameObj->value.svalue->value, envVarNameObj->value.svalue->vlen);
	*(envVarNameStr + (envVarNameObj->value.svalue->vlen)) = '\0';
	envVal = getenv(envVarNameStr);
	ctr_heap_free(envVarNameStr );
	if (envVal == NULL) {
		return CtrStdNil;
	}
	return ctr_build_string_from_cstring(envVal);
}

/**
 * [Program] env: [Key] val: [Value]
 *
 * Sets the value of an environment variable.
 */
ctr_object* ctr_program_set_env(ctr_object* myself, ctr_argument* argumentList) {
	ctr_object* envVarNameObj;
	ctr_object* envValObj;
	char*       envVarNameStr;
	char*       envValStr;
	envVarNameObj = ctr_internal_cast2string(argumentList->object);
	envValObj = ctr_internal_cast2string(argumentList->next->object);
	envVarNameStr = ctr_heap_allocate_cstring( envVarNameObj );
	envValStr = ctr_heap_allocate_cstring( envValObj );
	setenv(envVarNameStr, envValStr, 1);
	ctr_heap_free( envValStr );
	ctr_heap_free( envVarNameStr );
	return myself;
}

/**
 * [Program] waitForInput
 *
 * Ask a question on the command-line, resumes program
 * only after pressing the enter key.
 * Only reads up to 100 characters.
 *
 * Usage:
 *
 * Pen write: 'What is your name ?'.
 * x := Command waitForInput.
 * Pen write: 'Hello ' + x + ' !', brk.
 *
 * The example above asks the user for his/her name and
 * then displays the input received.
 */
ctr_object* ctr_program_waitforinput(ctr_object* myself, ctr_argument* argumentList) {
	int c;
	ctr_size bytes = 0;
	char* buff;
	ctr_size page = 10;
	ctr_object* userInput;
	buff = ctr_heap_allocate(page * sizeof(char));
	while ((c = getchar()) != '\n') {
		buff[bytes] = c;
		bytes++;
		if (bytes > page) {
			page *= 2;
			buff = (char*) ctr_heap_reallocate(buff, page * sizeof(char));
			if (buff == NULL) {
				CtrStdFlow = ctr_build_string_from_cstring("Out of memory");
				return CtrStdNil;
			}
		}
	}
	userInput = ctr_build_string(buff, bytes);
	ctr_heap_free(buff);
	return userInput;
}

/**
 * [Program] input.
 *
 * Reads all raw input from STDIN.
 * The input message reads the standard input stream of the application
 * which allows you to deal with pipes for instance. However this
 * mechanism can also be used to read raw POSTs in case of CGI applications.
 * Note that unlike other implementations the input messages also collects
 * null bytes, a null byte \0 in the input stream will NOT cause it to end.
 * Also note that the trailing newline (in case of CLI applications) will
 * be stripped so you don't have to do this manually. This allows for
 * one-liners like the one in the example below.
 * The input message is not allowed if message countdown has been activated
 * (Program remainingMessages:) because it might wait for content and this
 * is not allowed in a countdown sandbox.
 *
 * Usage:
 *
 * echo "hello" | ctr test.ctr
 *
 * x := Program input.
 * Pen write: x. #hello (without newline)
 *
 */
ctr_object* ctr_program_input(ctr_object* myself, ctr_argument* argumentList) {
	ctr_size bytes = 0;
	ctr_size page = 64;
	char buffer[page];
	size_t content_size = 0;
	char *content = ctr_heap_allocate(0);
	clearerr(stdin);
	int reading = 1;
	while(reading) {
		bytes = fread(buffer, sizeof(char), page, stdin);
		content_size += bytes;
		content = ctr_heap_reallocate(content, content_size);
		memcpy(content + (content_size - bytes),buffer,bytes);
		reading = !(bytes != page && feof(stdin));
	}
	/* strip the last newline */
	if ( content_size > 0 && *(content+(content_size-1))=='\n' ) {
		content_size -= 1;
	}
	ctr_object* str = ctr_build_string( content, content_size );
	ctr_heap_free( content );
	return str;
}

/**
 * [Program] flush.
 *
 * Flushes the STDOUT output buffer.
 */
ctr_object* ctr_program_flush(ctr_object* myself, ctr_argument* ctr_argumentList) {
	 fflush(stdout);
	 return myself;
}

/**
 * [Program] pid
 *
 * Returns the process identification number associated with the
 * program. If the program instance refers to the currently running
 * program PID 0 will be returned.
 */
ctr_object* ctr_program_pid(ctr_object* myself, ctr_argument* argumentList ) {
	ctr_object* pidObject;
	pidObject = ctr_internal_object_find_property(
		myself,
		ctr_build_string_from_cstring("pid"),
		CTR_CATEGORY_PRIVATE_PROPERTY
	);
	if (pidObject == NULL) return CtrStdNil;
	return ctr_internal_cast2number( pidObject );
}

/**
 * [Program] toString
 *
 * Returns a string representation of the program. This will be something like
 * [PID:2833], or in case of the currently active program: [PID:0].
 */
ctr_object* ctr_program_to_string(ctr_object* myself, ctr_argument* argumentList ) {
	return ctr_internal_cast2string(ctr_program_pid(myself, argumentList));
}

/**
 * [Program] number
 *
 * Returns the program pid as a number.
 */
ctr_object* ctr_program_to_number(ctr_object* myself, ctr_argument* argumentList ) {
       return ctr_internal_cast2number(ctr_program_pid(myself, argumentList));
}

/**
 * [Program] error: [String]
 *
 * Logs the specified message string using syslog using log level LOG_ERR.
 * Use this to log errors.
 */
ctr_object* ctr_program_err(ctr_object* myself, ctr_argument* argumentList) {
	char* message;
	message = ctr_heap_allocate_cstring(
		ctr_internal_cast2string(
			argumentList->object
		)
	);
	fwrite( message, sizeof(char), strlen(message), stderr);
	fwrite( "\n", sizeof(char), 1, stderr);
	ctr_heap_free( message );
	return myself;
}


/**
 * [Dice]
 *
 * The Dice object represents the random number generator.
 * Using a separate object for this makes it easy to spot randomness
 * in any program. The random number generator of Citrine uses the
 * BSD arc4random secure random number generator for high quality
 * randomness that is generally considered to be cryptographically
 * secure.
 */

/**
 * [Dice] rollWithSides: [Number]
 *
 * Rolls the dice, generates a pseudo random number.
 */
ctr_object* ctr_dice_sides(ctr_object* myself, ctr_argument* argumentList) {
	ctr_object* arg = ctr_internal_cast2number(argumentList->object);
	return ctr_build_number_from_float( (ctr_number) 1 + arc4random_uniform( (uint32_t) ( ceil( arg->value.nvalue ) ) ) );
}

/**
 * [Dice] roll
 *
 * Rolls a standard die with 6 sides.
 */
ctr_object* ctr_dice_throw(ctr_object* myself, ctr_argument* argumentList) {
	return ctr_build_number_from_float( (ctr_number) 1 + arc4random_uniform( (uint32_t) 6 ) );
}

/**
 * [Dice] rawRandomNumber
 *
 * Generates a random number, the traditional way (like rand()).
 */
ctr_object* ctr_dice_rand(ctr_object* myself, ctr_argument* argumentList) {
	return ctr_build_number_from_float( (ctr_number) (arc4random()) );
}

/**
 * [Dice] number
 *
 * On receiving this message, the Dice instance will send the message
 * 'rawRandomNumber' to itself and return the result as a string.
 * Note that you can override this behaviour with a custom rawRandomNumber implementation.
 *
 * Usage:
 *
 * #rig the dice ;)
 * ⚄ on: 'rawRandomNumber' do: { ↲ 6. }.
 * ✎ write: ⚄, brk. #6
 * ✎ write: ⚄ number, brk. #6
 */
ctr_object* ctr_dice_to_string(ctr_object* myself, ctr_argument* argumentList) {
       return ctr_internal_cast2string( ctr_send_message( myself, "rawRandomNumber", strlen("rawRandomNumber"), argumentList ) );
}

/**
 * [Dice] number
 *
 * On receiving this message, the Dice instance will send the message
 * 'rawRandomNumber' to itself and return the result.
 * Note that you can override this behaviour with a custom rawRandomNumber implementation.
 *
 * Usage:
 *
 * #rig the dice ;)
 * ⚄ on: 'rawRandomNumber' do: { ↲ 6. }.
 * ✎ write: ⚄, brk. #6
 * ✎ write: ⚄ number, brk. #6
 */
ctr_object* ctr_dice_to_number(ctr_object* myself, ctr_argument* argumentList) {
       return ctr_internal_cast2number( ctr_send_message( myself, "rawRandomNumber", strlen("rawRandomNumber"), argumentList ) );
}

/**
 * [Dice] drawFrom: [String] length: [Number].
 *
 * Returns a randomized string with the specified length using the pool of
 * characters contained in the String object. This message is useful for
 * creating random nonces, passwords, temporary files, keys or identifiers.
 */
ctr_object* ctr_dice_randomize_bytes(ctr_object* myself, ctr_argument* argumentList) {
	ctr_size i;
	ctr_size j;
	ctr_size k;
	ctr_size m;
	ctr_size byteOffset;
	ctr_size byteLength;
	ctr_size plen;
	ctr_size len;
	char* newBuffer;
	ctr_object* answer;
	ctr_object* pool = ctr_internal_cast2string(argumentList->object);
	plen = ctr_getutf8len( pool->value.svalue->value, pool->value.svalue->vlen );
	len = (size_t) ctr_internal_cast2number(argumentList->next->object)->value.nvalue;
	if ( len == 0 ) return ctr_build_empty_string();
	newBuffer = (char*) ctr_heap_allocate( len * 4 );
	m = 0;
	for( i = 0; i < len; i ++ ) {
		j = (ctr_size) arc4random_uniform( (uint32_t) plen );
		byteOffset = getBytesUtf8( pool->value.svalue->value, 0, j );
		byteLength = getBytesUtf8( pool->value.svalue->value, byteOffset, 1 );
		for (k = 0; k < byteLength; k++) {
			newBuffer[m++]=*(pool->value.svalue->value+byteOffset+k);
		}
	}
	answer = ctr_build_string(newBuffer, m);
	ctr_heap_free(newBuffer);
	return answer;
}

/**
 * [Clock] wait: [Number]
 *
 * Suspends program execution for the specified number of seconds.
 * This can be used for instance, together with a whileFalse loop as the
 * following example illustrates. The following example demonstrates the
 * use of the Clock object and the wait message to wait until an
 * exclusive lock on the specified file has been acquired.
 *
 * Usage:
 *
 * { ↲ file lock. }
 * whileFalse: { ⏰ wait: 1. }.
 *
 */
ctr_object* ctr_clock_wait(ctr_object* myself, ctr_argument* argumentList) {
	ctr_object* arg = ctr_internal_cast2number(argumentList->object);
	int n = (int) arg->value.nvalue;
	sleep(n);
	return myself;
}

/**
 * [Clock] new: [Number]
 *
 * Creates a new clock instance from a UNIX time stamp.
 */
ctr_object* ctr_clock_new_set( ctr_object* myself, ctr_argument* argumentList ) {
	ctr_object* clock;
	clock = ctr_clock_new( myself, argumentList );
	ctr_internal_object_add_property( clock, ctr_build_string_from_cstring( CTR_DICT_TIME ), ctr_internal_cast2number(argumentList->object), CTR_CATEGORY_PRIVATE_PROPERTY );
	return clock;
}

/**
 * @internal
 */
ctr_object* ctr_clock_get_time( ctr_object* myself, ctr_argument* argumentList, char part ) {
	struct tm* date;
	time_t timeStamp;
	ctr_object* answer;
	char* zone;
	timeStamp = (time_t) ctr_internal_cast2number(
		ctr_internal_object_find_property( myself, ctr_build_string_from_cstring(CTR_DICT_TIME), CTR_CATEGORY_PRIVATE_PROPERTY )
	)->value.nvalue;
	zone = ctr_heap_allocate_cstring(
		ctr_internal_cast2string(
			ctr_internal_object_find_property( myself, ctr_build_string_from_cstring(CTR_DICT_ZONE), CTR_CATEGORY_PRIVATE_PROPERTY )
		)
	);
	setenv( "TZ", zone, 1 );
	date = localtime( &timeStamp );
	setenv( "TZ", "UTC", 1 );
	switch( part ) {
		case 'Y':
			answer = ctr_build_number_from_float( (ctr_number) date->tm_year + 1900 );
			break;
		case 'm':
			answer = ctr_build_number_from_float( (ctr_number) date->tm_mon + 1 );
			break;
		case 'd':
			answer = ctr_build_number_from_float( (ctr_number) date->tm_mday );
			break;
		case 'H':
			answer = ctr_build_number_from_float( (ctr_number) date->tm_hour );
			break;
		case 'i':
			answer = ctr_build_number_from_float( (ctr_number) date->tm_min );
			break;
		case 's':
			answer = ctr_build_number_from_float( (ctr_number) date->tm_sec );
			break;
	}
	ctr_heap_free( zone );
	return answer;
}

/**
 * @internal
 */
ctr_object* ctr_clock_set_time( ctr_object* myself, ctr_argument* argumentList, char part ) {
	struct tm* date;
	time_t timeStamp;
	ctr_object* key;
	char* zone;
	key = ctr_build_string_from_cstring( CTR_DICT_TIME );
	timeStamp = (time_t) ctr_internal_cast2number(
		ctr_internal_object_find_property( myself, key, 0 )
	)->value.nvalue;
	zone = ctr_heap_allocate_cstring(
		ctr_internal_cast2string(
			ctr_internal_object_find_property( myself, ctr_build_string_from_cstring(CTR_DICT_ZONE), CTR_CATEGORY_PRIVATE_PROPERTY )
		)
	);
	setenv( "TZ", zone, 1 );
	date = localtime( &timeStamp );
	switch( part ) {
		case 'Y':
			date->tm_year = ctr_internal_cast2number(argumentList->object)->value.nvalue - 1900;
			break;
		case 'm':
			date->tm_mon = ctr_internal_cast2number(argumentList->object)->value.nvalue - 1;
			break;
		case 'd':
			date->tm_mday = ctr_internal_cast2number(argumentList->object)->value.nvalue;
			break;
		case 'H':
			date->tm_hour = ctr_internal_cast2number(argumentList->object)->value.nvalue;
			break;
		case 'i':
			date->tm_min = ctr_internal_cast2number(argumentList->object)->value.nvalue;
			break;
		case 's':
			date->tm_sec = ctr_internal_cast2number(argumentList->object)->value.nvalue;
			break;
	}
	ctr_heap_free( zone );
	ctr_internal_object_set_property( myself, key, ctr_build_number_from_float( (double_t) mktime( date ) ), 0 );
	setenv( "TZ", "UTC", 1 );
	return myself;
}

/**
 * [Clock] like: [Clock]
 *
 * Syncs a clock. Copies the time AND zone from the other clock.
 *
 * Usage:
 * clock := Clock new: timeStamp.
 * copyClock := Clock new like: clock.
 */
ctr_object* ctr_clock_like( ctr_object* myself, ctr_argument* argumentList ) {
	ctr_object* otherClock;
	ctr_object* time;
	ctr_object* zone;
	otherClock = argumentList->object;
	time = ctr_internal_object_find_property( otherClock, ctr_build_string_from_cstring( CTR_DICT_TIME ), CTR_CATEGORY_PRIVATE_PROPERTY );
	if ( time == NULL ) {
		time = ctr_build_number_from_float( 0 );
	} else {
		time = ctr_internal_cast2number( time );
	}
	zone = ctr_internal_object_find_property( otherClock, ctr_build_string_from_cstring( CTR_DICT_ZONE ), CTR_CATEGORY_PRIVATE_PROPERTY );
	if ( zone == NULL ) {
		zone = ctr_build_string_from_cstring( "UTC" );
	} else {
		zone = ctr_internal_cast2string( zone );
	}
	ctr_internal_object_set_property( myself, ctr_build_string_from_cstring( CTR_DICT_ZONE ), zone, CTR_CATEGORY_PRIVATE_PROPERTY );
	ctr_internal_object_set_property( myself, ctr_build_string_from_cstring( CTR_DICT_TIME ), time, CTR_CATEGORY_PRIVATE_PROPERTY );
	return myself;
}

/**
 * [Clock] zone: [String]
 *
 * Sets the time zone of the clock.
 */
ctr_object* ctr_clock_set_zone( ctr_object* myself, ctr_argument* argumentList ) {
	ctr_internal_object_set_property( myself, ctr_build_string_from_cstring(CTR_DICT_ZONE), ctr_internal_cast2string( argumentList->object ), CTR_CATEGORY_PRIVATE_PROPERTY );
	return myself;
}

/**
 * [Clock] zone
 *
 * Returns time zone of the clock.
 */
ctr_object* ctr_clock_get_zone( ctr_object* myself, ctr_argument* argumentList ) {
	return ctr_internal_object_find_property( myself, ctr_build_string_from_cstring(CTR_DICT_ZONE), CTR_CATEGORY_PRIVATE_PROPERTY );
}

/**
 * [Clock] zone: [Number]
 *
 * Sets the year of the clock.
 */
ctr_object* ctr_clock_set_year( ctr_object* myself, ctr_argument* argumentList ) {
	return ctr_clock_set_time( myself, argumentList, 'Y' );
}

/**
 * [Clock] zone: [Number]
 *
 * Sets the month of the clock.
 */
ctr_object* ctr_clock_set_month( ctr_object* myself, ctr_argument* argumentList ) {
	return ctr_clock_set_time( myself, argumentList, 'm' );
}

/**
 * [Clock] zone: [Number]
 *
 * Sets the day of the clock.
 */
ctr_object* ctr_clock_set_day( ctr_object* myself, ctr_argument* argumentList ) {
	return ctr_clock_set_time( myself, argumentList, 'd' );
}

/**
 * [Clock] zone: [Number]
 *
 * Sets the hour of the clock.
 */
ctr_object* ctr_clock_set_hour( ctr_object* myself, ctr_argument* argumentList ) {
	return ctr_clock_set_time( myself, argumentList, 'H' );
}

/**
 * [Clock] zone: [Number]
 *
 * Sets the minute of the clock.
 */
ctr_object* ctr_clock_set_minute( ctr_object* myself, ctr_argument* argumentList ) {
	return ctr_clock_set_time( myself, argumentList, 'i' );
}

/**
 * [Clock] zone: [Number]
 *
 * Sets the second of the clock.
 */
ctr_object* ctr_clock_set_second( ctr_object* myself, ctr_argument* argumentList ) {
	return ctr_clock_set_time( myself, argumentList, 's' );
}

/**
 * [Clock] year
 *
 * Returns year of the clock.
 */
ctr_object* ctr_clock_year( ctr_object* myself, ctr_argument* argumentList ) {
	return ctr_clock_get_time( myself, argumentList, 'Y' );
}

/**
 * [Clock] month
 *
 * Returns month of the clock.
 */
ctr_object* ctr_clock_month( ctr_object* myself, ctr_argument* argumentList ) {
	return ctr_clock_get_time( myself, argumentList, 'm' );
}

/**
 * [Clock] day
 *
 * Returns day of the clock.
 */
ctr_object* ctr_clock_day( ctr_object* myself, ctr_argument* argumentList ) {
	return ctr_clock_get_time( myself, argumentList, 'd' );
}

/**
 * [Clock] hour
 *
 * Returns hour of the clock.
 */
ctr_object* ctr_clock_hour( ctr_object* myself, ctr_argument* argumentList ) {
	return ctr_clock_get_time( myself, argumentList, 'H' );
}

/**
 * [Clock] minute
 *
 * Returns minute of the clock.
 */
ctr_object* ctr_clock_minute( ctr_object* myself, ctr_argument* argumentList ) {
	return ctr_clock_get_time( myself, argumentList, 'i' );
}

/**
 * [Clock] second
 *
 * Returns second of the clock.
 */
ctr_object* ctr_clock_second( ctr_object* myself, ctr_argument* argumentList ) {
	return ctr_clock_get_time( myself, argumentList, 's' );
}

/**
 * [Clock] yearday
 *
 * Returns day number of the year.
 */
ctr_object* ctr_clock_yearday( ctr_object* myself, ctr_argument* argumentList ) {
	struct tm* date;
	time_t timeStamp;
	timeStamp = (time_t) ctr_internal_cast2number(
		ctr_internal_object_find_property( myself, ctr_build_string_from_cstring(CTR_DICT_TIME), 0 )
	)->value.nvalue;
	date = localtime( &timeStamp );
	return ctr_build_number_from_float( (double_t) date->tm_yday );
}

/**
 * [Clock] weekday
 *
 * Returns the week day number of the clock.
 */
ctr_object* ctr_clock_weekday( ctr_object* myself, ctr_argument* argumentList ) {
	struct tm* date;
	time_t timeStamp;
	timeStamp = (time_t) ctr_internal_cast2number(
		ctr_internal_object_find_property( myself, ctr_build_string_from_cstring(CTR_DICT_TIME), 0 )
	)->value.nvalue;
	date = localtime( &timeStamp );
	return ctr_build_number_from_float( (double_t) date->tm_wday );
}

/**
 * [Clock] time.
 *
 * Returns the UNIX time stamp representation of the time.
 * Note: this is the time OF CREATION OF THE OBJECT. To get the actual time use:
 *
 * [Clock] new time.
 */
ctr_object* ctr_clock_time( ctr_object* myself, ctr_argument* argumentList ) {
	time_t timeStamp;
	timeStamp = (time_t) ctr_internal_cast2number(
		ctr_internal_object_find_property( myself, ctr_build_string_from_cstring(CTR_DICT_TIME), 0 )
	)->value.nvalue;
	return ctr_build_number_from_float( (double_t) timeStamp );
}

/**
 * [Clock] week
 *
 * Returns the week number of the clock.
 */
ctr_object* ctr_clock_week( ctr_object* myself, ctr_argument* argumentList ) {
	ctr_object* weekNumber;
	char*  str;
	time_t timeStamp;
	timeStamp = (time_t) ctr_internal_cast2number(
		ctr_internal_object_find_property( myself, ctr_build_string_from_cstring(CTR_DICT_TIME), 0 )
	)->value.nvalue;
	str = ctr_heap_allocate( 4 );
	strftime( str, 3, "%W", localtime( &timeStamp ) );
	weekNumber = ctr_internal_cast2number( ctr_build_string_from_cstring( str ) );
	ctr_heap_free( str );
	return weekNumber;
}

/**
 * [Clock] format: [String]
 *
 * Returns a string describing the date and time represented by the clock object
 * according to the specified format. See strftime for format syntax details.
 */
ctr_object* ctr_clock_format( ctr_object* myself, ctr_argument* argumentList ) {
	char*       zone;
	char*       description;
	ctr_object* answer;
	time_t      timeStamp;
	char*       format;
	format = ctr_heap_allocate_cstring( ctr_internal_cast2string( argumentList->object ) );
	zone = ctr_heap_allocate_cstring(
		ctr_internal_cast2string(
			ctr_internal_object_find_property( myself, ctr_build_string_from_cstring(CTR_DICT_ZONE), CTR_CATEGORY_PRIVATE_PROPERTY )
		)
	);
	timeStamp = (time_t) ctr_internal_cast2number(
		ctr_internal_object_find_property( myself, ctr_build_string_from_cstring(CTR_DICT_TIME), 0 )
	)->value.nvalue;
	description = ctr_heap_allocate( 41 );
	setenv( "TZ", zone, 1 );
	strftime( description, 40, format, localtime( &timeStamp ) );
	setenv( "TZ", "UTC", 1 );
	answer = ctr_build_string_from_cstring( description );
	ctr_heap_free( description );
	ctr_heap_free( format );
	ctr_heap_free( zone );
	return answer;
}

/**
 * [Clock] toString
 *
 * Returns a string describing the date and time
 * represented by the clock object. On receiving this message, the Clock
 * instance will send the message 'format:' to itself and the argument:
 * '%Y-%m-%d %H:%M:%S'. It will return the answer as a string. Note that you
 * can override this behaviour by adding your own 'format:' implementation.
 *
 * Usage:
 *
 * #build a time machine! ;)
 * ⏰ on: 'format:' do: { ↲ 'beautiful moment'. }.
 * ⏰ on: 'time' do: { ↲ '999'. }.
 *
 * write: ⏰, brk.
 * ✎ write: ⏰ number, brk.
 */
ctr_object* ctr_clock_to_string( ctr_object* myself, ctr_argument* argumentList ) {
	ctr_argument* newArgumentList;
	ctr_object*   answer;
	newArgumentList = ctr_heap_allocate( sizeof( ctr_argument ) );
	newArgumentList->object = ctr_build_string_from_cstring( "%Y-%m-%d %H:%M:%S" );
	answer = ctr_send_message( myself, CTR_DICT_FORMAT, strlen(CTR_DICT_FORMAT), newArgumentList );
	ctr_heap_free( newArgumentList );
	return answer;
}

/**
 * [Clock] number
 *
 * Returns a time stamp describing the date and time
 * represented by the clock object. On receiving this message, the Clock
 * instance will send the message 'time' to itself
 * and return the answer as a number. Note that you
 * can override this behaviour by adding your own 'time' implementation.
 *
 * Usage:
 *
 * #build a time machine! ;)
 * ⏰ on: 'format:' do: { ↲ 'beautiful moment'. }.
 * ⏰ on: 'time' do: { ↲ '999'. }.
 *
 * ✎ write: ⏰, brk.
 * ✎ write: ⏰ number, brk.
 */
ctr_object* ctr_clock_to_number( ctr_object* myself, ctr_argument* argumentList ) {
	return ctr_send_message( myself, "time", strlen("time"), argumentList );
}

/**
 * @internal
 */
void ctr_clock_init( ctr_object* clock ) {
	ctr_internal_object_add_property( clock, ctr_build_string_from_cstring( CTR_DICT_TIME ), ctr_build_number_from_float( (double_t) time( NULL ) ), 0 );
	ctr_internal_object_add_property( clock, ctr_build_string_from_cstring( CTR_DICT_ZONE ), ctr_build_string_from_cstring( "UTC" ), 0 );
}

/**
 * @internal
 */
ctr_object* ctr_clock_change( ctr_object* myself, ctr_argument* argumentList, uint8_t forward ) {
	ctr_number number;
	ctr_object* numberObject;
	ctr_object* qual;
	char* zone;
	ctr_object* timeObject;
	ctr_size l;
	time_t time;
	char* unit;
	struct tm* date;
	numberObject = ctr_internal_cast2number( argumentList->object );
	number = numberObject->value.nvalue * (forward ? 1 : -1);
	qual = ctr_internal_object_find_property( argumentList->object, ctr_build_string_from_cstring(CTR_DICT_QUALIFICATION), CTR_CATEGORY_PRIVATE_PROPERTY );
	if (qual == NULL) {
		return myself;
	}
	qual = ctr_internal_cast2string( qual );
	unit = qual->value.svalue->value;
	l    = qual->value.svalue->vlen;
	timeObject = ctr_internal_object_find_property( myself, ctr_build_string_from_cstring(CTR_DICT_TIME), CTR_CATEGORY_PRIVATE_PROPERTY );
	if ( timeObject == NULL ) {
		return myself;
	}
	time = (time_t) ctr_internal_cast2number( timeObject )->value.nvalue;
	zone = ctr_heap_allocate_cstring(
		ctr_internal_cast2string(
			ctr_internal_object_find_property( myself, ctr_build_string_from_cstring(CTR_DICT_ZONE), CTR_CATEGORY_PRIVATE_PROPERTY )
		)
	);
	setenv( "TZ", zone, 1 );
	date = localtime( &time );
	if ( strncmp( unit, CTR_DICT_HOURS, l ) == 0 || strncmp( unit, CTR_DICT_HOUR, l ) == 0 || strncmp( unit, CTR_DICT_HOURS_ABBR, l ) == 0  ) {
		date->tm_hour += number;
	} else if ( strncmp( unit, CTR_DICT_MINUTES, l ) == 0 || strncmp( unit, CTR_DICT_MINUTE, l ) == 0 || strncmp( unit, CTR_DICT_MINUTES_ABBR, l ) == 0  ) {
		date->tm_min += number;
	} else if ( strncmp( unit, CTR_DICT_SECONDS, l ) == 0 || strncmp( unit, CTR_DICT_SECOND, l ) == 0 || strncmp( unit, CTR_DICT_SECONDS_ABBR, l ) == 0 ) {
		date->tm_sec += number;
	} else if ( strncmp( unit, CTR_DICT_DAYS, l ) == 0 || strncmp( unit, CTR_DICT_DAY, l ) == 0 ) {
		date->tm_mday += number;
	} else if ( strncmp( unit, CTR_DICT_MONTHS, l ) == 0 || strncmp( unit, CTR_DICT_MONTH, l ) == 0 ) {
		date->tm_mon += number;
	} else if ( strncmp( unit, CTR_DICT_YEARS, l ) == 0 || strncmp( unit, CTR_DICT_YEAR, l ) == 0 ) {
		date->tm_year += number;
	} else if ( strncmp( unit, CTR_DICT_WEEKS, l ) == 0 || strncmp( unit, CTR_DICT_WEEK, l ) == 0 ) {
		date->tm_mday += number * 7;
	}
	ctr_internal_object_set_property( myself, ctr_build_string_from_cstring(CTR_DICT_TIME), ctr_build_number_from_float( (ctr_number) mktime( date ) ), CTR_CATEGORY_PRIVATE_PROPERTY  );
	setenv( "TZ", "UTC", 1 );
	ctr_heap_free( zone );
	return myself;
}

/**
 * [Clock] add: [Number].
 *
 * Adds the number to the clock, updating its time accordingly.
 * Note that this is typically used with a qualifier.
 * If the qualifier is 'hours' the number is treated as hours and
 * the specified number of hours will be added to the time.
 *
 * The Clock object understands the following qualifiers
 * if the selected language is English:
 *
 * sec, second, seconds,
 * min, minute, minutes,
 * hrs, hour, hours,
 * day, days,
 * week, weeks,
 * month, months,
 * year, years
 *
 * Note that it does not matter which form you use, 2 hour means
 * the same as 2 hours (plural).
 *
 * Usage:
 *
 * clock add: 3 minutes. #adds 3 minutes
 * clock add: 1 hour.    #adds 1 hour
 * clock add: 2 second.  #adds 2 seconds
 */
ctr_object* ctr_clock_add( ctr_object* myself, ctr_argument* argumentList ) {
	return ctr_clock_change( myself, argumentList, 1 );
}

/**
 * [Clock] subtract: [Number].
 *
 * Same as '[Clock] add:' but subtracts the number instead of adding it to
 * the clock's time.
 */
ctr_object* ctr_clock_subtract( ctr_object* myself, ctr_argument* argumentList ) {
	return ctr_clock_change( myself, argumentList, 0 );
}

/**
 * [Clock] new
 *
 * Creates a new clock, by default a clock will be set to
 * the UTC timezone having the current time.
 */
ctr_object* ctr_clock_new( ctr_object* myself, ctr_argument* argumentList ) {
	ctr_object* clock;
	clock = ctr_internal_create_object( CTR_OBJECT_TYPE_OTOBJECT );
	clock->link = myself;
	ctr_clock_init( clock );
	return clock;
}

/**
 * [Pen] write: [String]
 *
 * Writes string to console. 
 */
ctr_object* ctr_console_write(ctr_object* myself, ctr_argument* argumentList) {
	ctr_object* argument1 = argumentList->object;
	ctr_object* strObject = ctr_internal_cast2string(argument1);
	fwrite(strObject->value.svalue->value, sizeof(char), strObject->value.svalue->vlen, stdout);
	return myself;
}

/**
 * [Pen] brk
 *
 * Outputs a newline character.
 */
ctr_object* ctr_console_brk(ctr_object* myself, ctr_argument* argumentList) {
	fwrite("\n", sizeof(char), 1, stdout);
	return myself;
}

/**
 * [Pen] tab
 *
 * Prints a tab character to the console output.
 */
ctr_object* ctr_console_tab(ctr_object* myself, ctr_argument* argumentList) {
	fwrite( "\t", sizeof(char), strlen("\t"), stdout);
	return myself;
}
