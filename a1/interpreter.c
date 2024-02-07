#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "shellmemory.h"
#include "shell.h"

int MAX_ARGS_SIZE = 20;

int badcommand()
{
	printf("%s\n", "Unknown Command");
	return 1;
}

// For when command is called incorrectly
// Prints "Bad Command: <command>" with error code 2
// param -- command: name of command
// return -- error code 2
int badcommandIncorrectUsage(char *command)
{
	printf("Bad command: %s\n", command);
	return 2;
}

// For run command only
int badcommandFileDoesNotExist()
{
	printf("%s\n", "Bad command: File not found");
	return 3;
}

int help();
int quit();
int set(char *var, char *value);
int print(char *var);
int run(char *script);
int echo(char *token_string);
int my_ls();
int my_mkdir(char *dirname);
int my_touch(char *filename);
int my_cd(char *dirname);
int my_cat();
int badcommandFileDoesNotExist();

// Interpret commands and their arguments
int interpreter(char *command_args[], int args_size)
{
	int i;

	if (args_size < 1 || args_size > MAX_ARGS_SIZE)
	{
		return badcommand();
	}

	for (i = 0; i < args_size; i++)
	{ // strip spaces new line etc
		command_args[i][strcspn(command_args[i], "\r\n")] = 0;
	}

	if (strcmp(command_args[0], "help") == 0)
	{
		// help
		if (args_size != 1)
			return badcommand();
		return help();
	}
	else if (strcmp(command_args[0], "quit") == 0)
	{
		// quit
		if (args_size != 1)
			return badcommand();
		return quit();
	}
	else if (strcmp(command_args[0], "set") == 0)
	{
		// set
		if (args_size < 3 || args_size > 7)
			return badcommandIncorrectUsage("set");

		// iterate through the tokens
		char *val = (char *)malloc(sizeof(char) * 101 * (args_size - 2)); // each token no larger than 101 char
		strcpy(val, command_args[2]);									  // first token
		for (int i = 3; i < args_size; i++)								  // second token to last token
		{
			char *space = " ";
			strcat(val, space);			  // separate tokens with a space
			strcat(val, command_args[i]); // concat new token
		}

		int errCode = set(command_args[1], val);
		free(val);
		return errCode;
	}
	else if (strcmp(command_args[0], "print") == 0)
	{
		// print
		if (args_size != 2)
			return badcommand();
		return print(command_args[1]);
	}
	else if (strcmp(command_args[0], "run") == 0)
	{
		// run
		if (args_size != 2)
			return badcommand();
		return run(command_args[1]);
	}
	else if (strcmp(command_args[0], "echo") == 0)
	{
		// echo
		if (args_size != 2)
			return badcommand();
		return echo(command_args[1]);
	}
	else if (strcmp(command_args[0], "my_ls") == 0)
	{
		// my_ls
		if (args_size != 1)
			return badcommand();
		return my_ls();
	}
	else if (strcmp(command_args[0], "my_mkdir") == 0)
	{
		// my_mkdir
		if (args_size != 2)
			return badcommand();
		return my_mkdir(command_args[1]);
	}
	else if (strcmp(command_args[0], "my_touch") == 0)
	{
		// my_touch
		if (args_size != 2)
			return badcommand();
		return my_touch(command_args[1]);
	}
	else if (strcmp(command_args[0], "my_cd") == 0)
	{
		// my_cd
		if (args_size != 2)
			return badcommand();
		return my_cd(command_args[1]);
	}
	else
		return badcommand();
}

int help()
{

	char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
run SCRIPT.TXT		Executes the file SCRIPT.TXT\n ";
	printf("%s\n", help_string);
	return 0;
}

int quit()
{
	printf("%s\n", "Bye!");
	exit(0);
}

int set(char *var, char *value)
{
	char *link = "=";
	char buffer[1000];
	strcpy(buffer, var);
	strcat(buffer, link);
	strcat(buffer, value);

	mem_set_value(var, value);

	return 0;
}

int print(char *var)
{
	printf("%s\n", mem_get_value(var));
	return 0;
}

int run(char *script)
{
	int errCode = 0;
	char line[1000];
	FILE *p = fopen(script, "rt"); // the program is in a file

	if (p == NULL)
	{
		return badcommandFileDoesNotExist();
	}

	fgets(line, 999, p);
	while (1)
	{
		errCode = parseInput(line); // which calls interpreter()
		memset(line, 0, sizeof(line));

		if (feof(p))
		{
			break;
		}
		fgets(line, 999, p);
	}

	fclose(p);

	return errCode;
}

int echo(char *token_string)
{
	// if first char is $, then echo value of var
	if (token_string[0] == '$')
	{
		// get value of var
		char *value = (char *)malloc(sizeof(char) * 101); // no larger than 101 char
		strcpy(value, mem_get_value(token_string + 1));	  // cut the "$" and get value

		// print value
		if (strcmp(value, "Variable does not exist") == 0) // if "Variable does not exist", then empty line
			strcpy(value, "");
		printf("%s\n", value);

		free(value);
	}
	// if first char is not $, then echo value directly
	else
	{
		printf("%s\n", token_string);
	}

	return 0;
}

int my_ls()
{
	return system("ls");
}

int my_mkdir(char *dirname)
{
	if (mkdir(dirname, 0777) == 0)
		return 0;

	return badcommandIncorrectUsage("my_mkdir");
}

int my_touch(char *filename)
{
	FILE *p; // pointer to file we want to create

	p = fopen(filename, "w"); // create empty file for writing
	if (p == NULL)
		return badcommandIncorrectUsage("my_touch");

	fclose(p);
	return 0;
}

int my_cd(char *dirname)
{
	if (chdir(dirname) == 0)
		return 0;

	return badcommandIncorrectUsage("my_cd");
}