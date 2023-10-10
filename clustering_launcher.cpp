// Argument: the name of the taskset/schedule file:

#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <vector>
#include <math.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include "single_use_barrier.h"

enum rt_gomp_clustering_launcher_error_codes
{ 
	RT_GOMP_CLUSTERING_LAUNCHER_SUCCESS,
	RT_GOMP_CLUSTERING_LAUNCHER_FILE_OPEN_ERROR,
	RT_GOMP_CLUSTERING_LAUNCHER_FILE_PARSE_ERROR,
	RT_GOMP_CLUSTERING_LAUNCHER_UNSCHEDULABLE_ERROR,
	RT_GOMP_CLUSTERING_LAUNCHER_FORK_EXECV_ERROR,
	RT_GOMP_CLUSTERING_LAUNCHER_BARRIER_INITIALIZATION_ERROR,
	RT_GOMP_CLUSTERING_LAUNCHER_ARGUMENT_ERROR
};

int main(int argc, char *argv[])
{
	// Define the total number of timing parameters that should appear on the second line for each task
	const unsigned num_timing_params = 11;
	
	// Define the number of timing parameters to skip on the second line for each task
	const unsigned num_skipped_timing_params = 4;
	
	// Define the number of partition parameters that should appear on the third line for each task
	const unsigned num_partition_params = 3;
	
	// Define the name of the barrier used for synchronizing tasks after creation
	const std::string barrier_name = "RT_GOMP_CLUSTERING_BARRIER";
	
	// Verify the number of arguments
	if (argc != 2)
	{
		fprintf(stderr, "ERROR: The program must receive a single argument which is the taskset/schedule filename without any extension.");
		return RT_GOMP_CLUSTERING_LAUNCHER_ARGUMENT_ERROR;
	}
	
	// Determine the taskset (.rtpt) and schedule (.rtps) filenames from the program argument
	std::string taskset_filename(argv[1]);
	taskset_filename += ".rtpt";
	std::string schedule_filename(argv[1]);
	schedule_filename += ".rtps";
	
	// Check for an up to date schedule (.rtps) file. If not, create one from a taskset (.rtpt) file.
	struct stat taskset_stat, schedule_stat;
	int taskset_ret_val = stat(taskset_filename.c_str(), &taskset_stat);
	int schedule_ret_val = stat(schedule_filename.c_str(), &schedule_stat);
	if (schedule_ret_val == -1 || (taskset_ret_val == 0 && taskset_stat.st_mtime > schedule_stat.st_mtime))
	{
		if (taskset_ret_val == -1)
		{
			fprintf(stderr, "ERROR: Cannot open taskset file: %s", taskset_filename.c_str());
			return RT_GOMP_CLUSTERING_LAUNCHER_FILE_OPEN_ERROR;
		}
		
		fprintf(stderr, "Scheduling taskset %s ...\n", argv[1]);
		
		// We will call a python scheduler script and pass the taskset filename without the extension
		std::vector<const char *> scheduler_script_argv;
		scheduler_script_argv.push_back("python");
		scheduler_script_argv.push_back("cluster.py");
		scheduler_script_argv.push_back(argv[1]);
		
		// NULL terminate the argument vector
		scheduler_script_argv.push_back(NULL);
		
		// Fork and execv the scheduler script
		pid_t pid = fork();
		if (pid == 0)
		{
			// Const cast is necessary for type compatibility. Since the strings are
			// not shared, there is no danger in removing the const modifier.
			execvp("python", const_cast<char **>(&scheduler_script_argv[0]));
			
			// Error if execv returns
			perror("Execv-ing scheduler script failed");
			return RT_GOMP_CLUSTERING_LAUNCHER_FORK_EXECV_ERROR;
		}
		else if (pid == -1)
		{
			perror("Forking a new process for scheduler script failed");
			return RT_GOMP_CLUSTERING_LAUNCHER_FORK_EXECV_ERROR;
		}
		
		// Wait until the child process has terminated
		while (!(wait(NULL) == -1 && errno == ECHILD));	
	}
	
	// Open the schedule (.rtps) file
	std::ifstream ifs(schedule_filename.c_str());
	if (!ifs.is_open())
	{
		fprintf(stderr, "ERROR: Cannot open schedule file");
		return RT_GOMP_CLUSTERING_LAUNCHER_FILE_OPEN_ERROR;
	}
	
	// Count the number of tasks
	unsigned num_lines = 0;
	std::string line;
	while (getline(ifs, line)) { num_lines += 1; }
	unsigned num_tasks = (num_lines - 2) / 3;
	if (!(num_lines >= 2 && (num_lines - 2) % 3 == 0))
	{
		fprintf(stderr, "ERROR: Invalid number of lines in schedule file");
		return RT_GOMP_CLUSTERING_LAUNCHER_FILE_PARSE_ERROR;
	}
	
	// Seek back to the beginning of the file
	ifs.clear();
	ifs.seekg(0, std::ios::beg);
	
	// Check if the taskset is schedulable
	std::string schedulability_line;
	if (std::getline(ifs, schedulability_line))
	{
		unsigned schedulability;
		std::istringstream schedulability_stream(schedulability_line);
		if (schedulability_stream >> schedulability)
		{	
			if (schedulability == 0)
			{
				fprintf(stderr, "Taskset is schedulable: %s\n", argv[1]);
			}
			else if (schedulability == 1)
			{
				fprintf(stderr, "WARNING: Taskset may not be schedulable: %s\n", argv[1]);
			}
			else
			{
				fprintf(stderr, "ERROR: Taskset NOT schedulable: %s", argv[1]);
				return RT_GOMP_CLUSTERING_LAUNCHER_UNSCHEDULABLE_ERROR;
			}
		}
		else
		{
			fprintf(stderr, "ERROR: Schedulability improperly specified");
			return RT_GOMP_CLUSTERING_LAUNCHER_FILE_PARSE_ERROR;
		}
	}
	else
	{
		fprintf(stderr, "ERROR: Schedulability improperly specified");
		return RT_GOMP_CLUSTERING_LAUNCHER_FILE_PARSE_ERROR;
	}
	
	// Extract the core range line from the file; currently not used
	std::string core_range_line;
	if (!std::getline(ifs, core_range_line))
	{
		fprintf(stderr, "ERROR: Missing system first and last cores line");
		return RT_GOMP_CLUSTERING_LAUNCHER_FILE_PARSE_ERROR;
	}
	
	// Initialize a barrier to synchronize the tasks after creation
	int ret_val = init_single_use_barrier(barrier_name.c_str(), num_tasks);
	if (ret_val != 0)
	{
		fprintf(stderr, "ERROR: Failed to initialize barrier");
		return RT_GOMP_CLUSTERING_LAUNCHER_BARRIER_INITIALIZATION_ERROR;
	}
	
	// Iterate over the tasks and fork and execv each one
	std::string task_command_line, task_timing_line, task_partition_line;
	for (unsigned t = 1; t <= num_tasks; ++t)
	{
		if (
		    std::getline(ifs, task_command_line) && 
		    std::getline(ifs, task_timing_line) && 
		    std::getline(ifs, task_partition_line)
	    )
		{
			std::istringstream task_command_stream(task_command_line);
			std::istringstream task_timing_stream(task_timing_line);
			std::istringstream task_partition_stream(task_partition_line);
			
			// Add arguments to this vector of strings. This vector will be transformed into
			// a vector of char * before the call to execv by calling c_str() on each string,
			// but storing the strings in a vector is necessary to ensure that the arguments
			// have different memory addresses. If the char * vector is created directly, by
			// reading the arguments into a string and and adding the c_str() to a vector, 
			// then each new argument could overwrite the previous argument since they might
			// be using the same memory address. Using a vector of strings ensures that each
			// argument is copied to its own memory before the next argument is read.
			std::vector<std::string> task_manager_argvector;
			
			// Add the task program name to the argument vector
			std::string program_name;
			if (task_command_stream >> program_name)
			{
				task_manager_argvector.push_back(program_name);
			}
			else
			{
				fprintf(stderr, "ERROR: Program name not provided for task");
				kill(0, SIGTERM);
				return RT_GOMP_CLUSTERING_LAUNCHER_FILE_PARSE_ERROR;
			}
			
			// Add the partition parameters to the argument vector
			std::string partition_param;
			for (unsigned i = 0; i < num_partition_params; ++i)
			{
				if (task_partition_stream >> partition_param)
				{
					task_manager_argvector.push_back(partition_param);
				}
				else
				{
					fprintf(stderr, "ERROR: Too few partition parameters were provided for task %s", program_name.c_str());
					kill(0, SIGTERM);
					return RT_GOMP_CLUSTERING_LAUNCHER_FILE_PARSE_ERROR;
				}
			}
			
			// Check for extra partition parameters
			if (task_partition_stream >> partition_param)
			{
				fprintf(stderr, "ERROR: Too many partition parameters were provided for task %s", program_name.c_str());
				kill(0, SIGTERM);
				return RT_GOMP_CLUSTERING_LAUNCHER_FILE_PARSE_ERROR;
			}
			
			// Skip the first few timing parameters that were only needed by the scheduler
			std::string timing_param;
			for (unsigned i = 0; i < num_skipped_timing_params; ++i)
			{
				if (!(task_timing_stream >> timing_param))
				{
					fprintf(stderr, "ERROR: Too few timing parameters were provided for task %s", program_name.c_str());
					kill(0, SIGTERM);
					return RT_GOMP_CLUSTERING_LAUNCHER_FILE_PARSE_ERROR;
				}
			}
			
			// Add the timing parameters to the argument vector
			for (unsigned i = num_skipped_timing_params; i < num_timing_params; ++i)
			{
				if (task_timing_stream >> timing_param)
				{
					task_manager_argvector.push_back(timing_param);
				}
				else
				{
					fprintf(stderr, "ERROR: Too few timing parameters were provided for task %s", program_name.c_str());
					kill(0, SIGTERM);
					return RT_GOMP_CLUSTERING_LAUNCHER_FILE_PARSE_ERROR;
				}
			}
			
			// Check for extra timing parameters
			if (task_timing_stream >> timing_param)
			{
				fprintf(stderr, "ERROR: Too many timing parameters were provided for task %s", program_name.c_str());
				kill(0, SIGTERM);
				return RT_GOMP_CLUSTERING_LAUNCHER_FILE_PARSE_ERROR;
			}
			
			// Add the barrier name to the argument vector
			task_manager_argvector.push_back(barrier_name);
			
			// Add the task arguments to the argument vector
			task_manager_argvector.push_back(program_name);
			
			std::string task_arg;
			while (task_command_stream >> task_arg)
			{
				task_manager_argvector.push_back(task_arg);
			}
			
			// Create a vector of char * arguments from the vector of string arguments
			std::vector<const char *> task_manager_argv;
			for (std::vector<std::string>::iterator i = task_manager_argvector.begin(); i != task_manager_argvector.end(); ++i)
			{
				task_manager_argv.push_back(i->c_str());
			}
			
			// NULL terminate the argument vector
			task_manager_argv.push_back(NULL);
			
			fprintf(stderr, "Forking and execv-ing task %s\n", program_name.c_str());
			
			// Fork and execv the task program
			pid_t pid = fork();
			if (pid == 0)
			{   
				// Const cast is necessary for type compatibility. Since the strings are
				// not shared, there is no danger in removing the const modifier.
				execv(program_name.c_str(), const_cast<char **>(&task_manager_argv[0]));
				
				// Error if execv returns
				perror("Execv-ing a new task failed");
				kill(0, SIGTERM);
				return RT_GOMP_CLUSTERING_LAUNCHER_FORK_EXECV_ERROR;
			}
			else if (pid == -1)
			{
				perror("Forking a new process for task failed");
				kill(0, SIGTERM);
				return RT_GOMP_CLUSTERING_LAUNCHER_FORK_EXECV_ERROR;
			}	
		}
		else
		{
			fprintf(stderr, "ERROR: Provide three lines for each task in the schedule (.rtps) file");
			kill(0, SIGTERM);
			return RT_GOMP_CLUSTERING_LAUNCHER_FILE_PARSE_ERROR;
		}
	}
	
	// Close the file
	ifs.close();
	
	fprintf(stderr, "All tasks started\n");
	
	// Wait until all child processes have terminated
	while (!(wait(NULL) == -1 && errno == ECHILD));
	
	fprintf(stderr, "All tasks finished\n");
	return 0;
}

