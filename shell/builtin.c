#include "builtin.h"

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	// Your code here
	if (strcmp(cmd, "exit") == 0) {
		return 1;
	}

	return 0;
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	// chequea si el comando comienza con "cd"
	if (strncmp(cmd, "cd", 2) == 0) {
		char *directorio = NULL;

		// chequea que sea solo cd
		if (cmd[2] == '\0' || (cmd[2] == ' ' && cmd[3] == '\0')) {
			directorio =
			        getenv("HOME");  // Obtener el directorio HOME
		}

		else {
			// extraer el argumento después de "cd " // cambia con forma aritmetica
			char *arg = cmd + 3;

			// si descomentas esta linea funciona y no se xq-> el
			// copilot me lo puso(ver ejemplos 2 y 3 que creo que
			// son como cd uno/dos y por eso falla) while (*arg == '
			// ') arg++;  // Skip leading spaces

			//
			if (strcmp(arg, ".") == 0) {
				return 1;
			} else if (strcmp(arg, "..") == 0) {
				directorio = "..";
			} else {
				directorio = arg;
			}
		}

		// si hay directorio valido
		if (directorio != NULL && chdir(directorio) == 0) {
			// cambio de direcciones exitosos
			return 1;
		} else {
			// cambio de directorio falló

			return 0;
		}
	}

	return 0;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	// Your code here
	if (strcmp(cmd, "pwd") == 0) {
		char path[BUFLEN];
		if (getcwd(path, sizeof(path)) != NULL) {
			printf("%s\n", path);
		} else {
			// error
			perror("getcwd");
		}
		return 1;
	}


	return 0;
}

// returns true if `history` was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
history(char *cmd)
{
	// Your code here

	return 0;
}
