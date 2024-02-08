#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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
int my_cat(char *filename);
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
			return badcommandIncorrectUsage("echo");
		return echo(command_args[1]);
	}
	else if (strcmp(command_args[0], "my_ls") == 0)
	{
		// my_ls
		if (args_size != 1)
			return badcommandIncorrectUsage("my_ls");
		return my_ls();
	}
	else if (strcmp(command_args[0], "my_mkdir") == 0)
	{
		// my_mkdir
		if (args_size != 2)
			return badcommandIncorrectUsage("my_mkdir");
		return my_mkdir(command_args[1]);
	}
	else if (strcmp(command_args[0], "my_touch") == 0)
	{
		// my_touch
		if (args_size != 2)
			return badcommandIncorrectUsage("my_touch");
		return my_touch(command_args[1]);
	}
	else if (strcmp(command_args[0], "my_cd") == 0)
	{
		// my_cd
		if (args_size != 2)
			return badcommandIncorrectUsage("my_cd");
		return my_cd(command_args[1]);
	}
	else if (strcmp(command_args[0], "my_cat") == 0)
	{
		// my_cat
		if (args_size != 2)
			return badcommandIncorrectUsage("my_cat");
		return my_cat(command_args[1]);
	}
	else if (strcmp(command_args[0], "if") == 0)
	{
		// my_if
		// ensure enough args for "if identifier1 op identifier2 then command"
		if (args_size < 6)
			return badcommandIncorrectUsage("too few arguments in if-conditional");

		// second arg and fourth arg must be identifiers
		// get value of identifiers
		char *identifier1 = (char *)malloc(sizeof(char) * 101); // no larger than 101 char
		char *identifier2 = (char *)malloc(sizeof(char) * 101); // no larger than 101 char

		if (command_args[1][0] == '$') // if first char of identifier1 is $
		{
			strcpy(identifier1, mem_get_value(command_args[1] + 1)); // cut the "$" and get value
			if (strcmp(identifier1, "Variable does not exist") == 0) // if "Variable does not exist", then empty string
				strcpy(identifier1, "");
		}
		else
			strcpy(identifier1, command_args[1]); // else copy arg into identifier1 directly

		if (command_args[3][0] == '$') // if first char of identifier2 is $
		{
			strcpy(identifier2, mem_get_value(command_args[3] + 1)); // cut the "$" and get value
			if (strcmp(identifier2, "Variable does not exist") == 0) // if "Variable does not exist", then empty string
				strcpy(identifier2, "");
		}
		else
			strcpy(identifier2, command_args[3]); // else copy arg into identifier2 directly

		// third arg must be op
		// get bool for (identifier1 op identifier2)
		bool condition = true;
		if (strcmp(command_args[2], "==") == 0)
			condition = (strcmp(identifier1, identifier2) == 0);
		else if (strcmp(command_args[2], "!=") == 0)
			condition = (strcmp(identifier1, identifier2) != 0);
		else
		{
			free(identifier1);
			free(identifier2);
			return badcommandIncorrectUsage("invalid operator in if-conditional");
		}

		// fifth arg must be then
		// ensure it is indeed "then"
		if (strcmp(command_args[4], "then") != 0)
		{
			free(identifier1);
			free(identifier2);
			return badcommandIncorrectUsage("cannot find 'then' in if-conditional");
		}

		// sixth argument and beyond are commands
		// initialize command1
		extern int MAX_USER_INPUT;
		char *command1 = (char *)malloc(sizeof(char) * (MAX_USER_INPUT)); // no larger than MAX_USER_INPUT
		if (strcmp(command_args[5], "else") == 0 || strcmp(command_args[5], "fi") == 0 || strcmp(command_args[5], "\n") == 0)
		{ // ensure command1 is not empty
			printf("Empty if clause\n");
			free(identifier1);
			free(identifier2);
			free(command1);
			return 2;
		}
		strcpy(command1, command_args[5]); // first token of command1

		// iterate through tokens until else or fi or newline
		int i = 6;
		while (strcmp(command_args[i], "else") != 0 && strcmp(command_args[i], "fi") != 0 && strcmp(command_args[i], "\n") != 0)
		{
			if (i >= args_size) // prevent array index out of bounds
				break;

			char *space = " ";
			strcat(command1, space);		   // separate tokens with a space
			strcat(command1, command_args[i]); // concat new token
			i++;
		}

		// initialize command2
		char *command2 = (char *)malloc(sizeof(char) * MAX_USER_INPUT); // no larger than MAX_USER_INPUT

		// if there is an "else", then get command2
		if (i < args_size && strcmp(command_args[i], "else") == 0)
		{
			i++;
			strcpy(command2, command_args[i]); // first token of command2

			// iterate through tokens until fi or newline
			i++;
			while (strcmp(command_args[i], "fi") != 0 && strcmp(command_args[i], "\n") != 0)
			{
				if (i >= args_size) // prevent array index out of bounds
					break;

				char *space = " ";
				strcat(command2, space);		   // separate tokens with a space
				strcat(command2, command_args[i]); // concat new token
				i++;
			}
		}
		// if there is no "else", then command2 = ""
		else if (i < args_size && (strcmp(command_args[i], "fi") == 0 || strcmp(command_args[i], "\n") == 0))
			strcpy(command2, "");
		// if there is no else nor fi nor newline
		else
		{
			free(identifier1);
			free(identifier2);
			free(command1);
			free(command2);
			return badcommandIncorrectUsage("cannot find 'fi' in if-conditional");
		}

		// evaluation conditional
		// if condition == true, execute command1
		// if condition == false, execute command2
		int errCode = 0;
		if (condition)
		{
			// if empty command, then "Empty if clause"
			if (strcmp(command1, "") == 0)
			{
				printf("%s\n", "Empty if clause");
				errCode = 2;
			}
			// else execute command
			else
				errCode = parseInput(command1); // calls interpreter()
		}
		else
		{
			if (strcmp(command2, "") == 0) // if empty command, then "Empty if clause"
			{
				printf("%s\n", "Empty if clause");
				errCode = 2;
			}
			else								// else execute command
				errCode = parseInput(command2); // calls interpreter()
		}

		// return errCode
		free(identifier1);
		free(identifier2);
		free(command1);
		free(command2);
		return errCode;
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
	if (mkdir(dirname, 0777) != 0)
		return badcommandIncorrectUsage("my_mkdir");

	return 0;
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
	if (chdir(dirname) != 0)
		return badcommandIncorrectUsage("my_cd");

	return 0;
}

int my_cat(char *filename)
{
	// open file
	FILE *p;				  // pointer to file we want to open
	p = fopen(filename, "r"); // open file for reading
	if (p == NULL)			  // check if file exists
		return badcommandIncorrectUsage("my_cat");

	// print file contents
	char content_c = fgetc(p); // character in file
	while (content_c != EOF)   // iterate until end of file
	{
		printf("%c", content_c); // print char
		content_c = fgetc(p);	 // get next char
	}

	// close file
	fclose(p);
	return 0;
}