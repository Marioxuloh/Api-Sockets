/*

** Fichero: servidor.c
** Autores:
** Mario Sánchez López DNI 70913738T
** Javier Cabo Correa DNI 70959954D
*
**
*          		S E R V I D O R
*
*	This is an example program that demonstrates the use of
*	sockets TCP and UDP as an IPC mechanism.
*
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define PUERTO 17289
#define ADDRNOTFOUND 0xffffffff /* return address for unfound host */
#define TAM_BUFFER 516
#define MAXHOST 128
#define MAXTRY 4

extern int errno;

/*
 *			M A I N
 *
 *	This routine starts the server.  It forks, leaving the child
 *	to do all the work, so it does not have to be run in the
 *	background.  It sets up the sockets.  It
 *	will loop forever, until killed by a signal.
 *
 */

void serverTCP(int s, struct sockaddr_in peeraddr_in);
void serverUDP(int s, struct sockaddr_in clientaddr_in);
void errout(char *); /* declare error out routine */

int FIN = 0; /* Para el cierre ordenado */
void finalizar() { FIN = 1; }

int main(argc, argv)
int argc;
char *argv[];
{

	int s_TCP, s_UDP; /* connected socket descriptor */
	int ls_TCP;		  /* listen socket descriptor */

	int cc; /* contains the number of bytes read */

	struct sigaction sa = {.sa_handler = SIG_IGN}; /* used to ignore SIGCHLD */

	struct sockaddr_in myaddr_in;	  /* for local socket address */
	struct sockaddr_in clientaddr_in; /* for peer socket address */
	int addrlen;

	fd_set readmask;
	int numfds, s_mayor;

	char buffer[TAM_BUFFER]; /* buffer for packets to be read into */

	struct sigaction vec;

	// crear peticiones.log
	FILE *archivo;
	// cerramos tambien el peticiones.log
	long timevar;
	time(&timevar);
	// Intentar abrir el archivo en modo de escritura al final ("w")
	archivo = fopen("peticiones.log", "w");

	// Verificar si se abrió correctamente
	if (archivo == NULL)
	{
		printf("Error al abrir el archivo\n");
		exit(1); // Terminar el programa con un código de error
	}
	fprintf(archivo, "Opening server at %s\n", (char *)ctime(&timevar));
	fclose(archivo);
	// fprintf(archivo, "\nINICIO SERVIDOR\n");

	/* Create the listen socket. */
	ls_TCP = socket(AF_INET, SOCK_STREAM, 0);
	if (ls_TCP == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
		exit(1);
	}
	/* clear out address structures */
	memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset((char *)&clientaddr_in, 0, sizeof(struct sockaddr_in));

	addrlen = sizeof(struct sockaddr_in);

	/* Set up address structure for the listen socket. */
	myaddr_in.sin_family = AF_INET;
	/* The server should listen on the wildcard address,
	 * rather than its own internet address.  This is
	 * generally good practice for servers, because on
	 * systems which are connected to more than one
	 * network at once will be able to have one server
	 * listening on all networks at once.  Even when the
	 * host is connected to only one network, this is good
	 * practice, because it makes the server program more
	 * portable.
	 */
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	myaddr_in.sin_port = htons(PUERTO);

	/* Bind the listen address to the socket. */
	if (bind(ls_TCP, (const struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind address TCP\n", argv[0]);
		exit(1);
	}
	/* Initiate the listen on the socket so remote users
	 * can connect.  The listen backlog is set to 5, which
	 * is the largest currently supported.
	 */
	if (listen(ls_TCP, 5) == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to listen on socket\n", argv[0]);
		exit(1);
	}

	/* Create the socket UDP. */
	s_UDP = socket(AF_INET, SOCK_DGRAM, 0);
	if (s_UDP == -1)
	{
		perror(argv[0]);
		printf("%s: unable to create socket UDP\n", argv[0]);
		exit(1);
	}
	/* Bind the server's address to the socket. */
	if (bind(s_UDP, (struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1)
	{
		perror(argv[0]);
		printf("%s: unable to bind address UDP\n", argv[0]);
		exit(1);
	}

	/* Now, all the initialization of the server is
	 * complete, and any user errors will have already
	 * been detected.  Now we can fork the daemon and
	 * return to the user.  We need to do a setpgrp
	 * so that the daemon will no longer be associated
	 * with the user's control terminal.  This is done
	 * before the fork, so that the child will not be
	 * a process group leader.  Otherwise, if the child
	 * were to open a terminal, it would become associated
	 * with that terminal as its control terminal.  It is
	 * always best for the parent to do the setpgrp.
	 */
	setpgrp();

	switch (fork())
	{
	case -1: /* Unable to fork, for some reason. */
		perror(argv[0]);
		fprintf(stderr, "%s: unable to fork daemon\n", argv[0]);
		exit(1);

	case 0: /* The child process (daemon) comes here. */

		/* Close stdin and stderr so that they will not
		 * be kept open.  Stdout is assumed to have been
		 * redirected to some logging file, or /dev/null.
		 * From now on, the daemon will not report any
		 * error messages.  This daemon will loop forever,
		 * waiting for connections and forking a child
		 * server to handle each one.
		 */
		fclose(stdin);
		fclose(stderr);

		/* Set SIGCLD to SIG_IGN, in order to prevent
		 * the accumulation of zombies as each child
		 * terminates.  This means the daemon does not
		 * have to make wait calls to clean them up.
		 */
		if (sigaction(SIGCHLD, &sa, NULL) == -1)
		{
			perror(" sigaction(SIGCHLD)");
			fprintf(stderr, "%s: unable to register the SIGCHLD signal\n", argv[0]);
			exit(1);
		}

		/* Registrar SIGTERM para la finalizacion ordenada del programa servidor */
		vec.sa_handler = (void *)finalizar;
		vec.sa_flags = 0;
		if (sigaction(SIGTERM, &vec, (struct sigaction *)0) == -1)
		{
			perror(" sigaction(SIGTERM)");
			fprintf(stderr, "%s: unable to register the SIGTERM signal\n", argv[0]);
			exit(1);
		}

		while (!FIN)
		{
			/* Meter en el conjunto de sockets los sockets UDP y TCP */
			FD_ZERO(&readmask);
			FD_SET(ls_TCP, &readmask);
			FD_SET(s_UDP, &readmask);
			/*
			Seleccionar el descriptor del socket que ha cambiado. Deja una marca en
			el conjunto de sockets (readmask)
			*/
			if (ls_TCP > s_UDP)
				s_mayor = ls_TCP;
			else
				s_mayor = s_UDP;

			if ((numfds = select(s_mayor + 1, &readmask, (fd_set *)0, (fd_set *)0, NULL)) < 0)
			{
				if (errno == EINTR)
				{
					FIN = 1;
					close(ls_TCP);
					close(s_UDP);
					perror("\nFinalizando el servidor. Se�al recibida en elect\n ");
				}
			}
			else
			{

				/* Comprobamos si el socket seleccionado es el socket TCP */
				if (FD_ISSET(ls_TCP, &readmask))
				{
					/* Note that addrlen is passed as a pointer
					 * so that the accept call can return the
					 * size of the returned address.
					 */
					/* This call will block until a new
					 * connection arrives.  Then, it will
					 * return the address of the connecting
					 * peer, and a new socket descriptor, s,
					 * for that connection.
					 */
					s_TCP = accept(ls_TCP, (struct sockaddr *)&clientaddr_in, &addrlen);
					if (s_TCP == -1)
						exit(1);
					switch (fork())
					{
					case -1: /* Can't fork, just exit. */
						exit(1);
					case 0:			   /* Child process comes here. */
						close(ls_TCP); /* Close the listen socket inherited from the daemon. */
						serverTCP(s_TCP, clientaddr_in);
						exit(0);
					default: /* Daemon process comes here. */
							 /* The daemon needs to remember
							  * to close the new accept socket
							  * after forking the child.  This
							  * prevents the daemon from running
							  * out of file descriptor space.  It
							  * also means that when the server
							  * closes the socket, that it will
							  * allow the socket to be destroyed
							  * since it will be the last close.
							  */
						close(s_TCP);
					}
				} /* De TCP*/
				/* Comprobamos si el socket seleccionado es el socket UDP */
				if (FD_ISSET(s_UDP, &readmask))
				{
					serverUDP(s_UDP, clientaddr_in);
				}
			}
		} /* Fin del bucle infinito de atenci�n a clientes */
		/* Cerramos los sockets UDP y TCP */
		close(ls_TCP);
		close(s_UDP);
		// cerramos tambien el peticiones.log
		time(&timevar);
		// Intentar abrir el archivo en modo de escritura al final ("w")
		archivo = fopen("peticiones.log", "a+");

		// Verificar si se abrió correctamente
		if (archivo == NULL)
		{
			printf("Error al abrir el archivo\n");
			exit(1); // Terminar el programa con un código de error
		}
		fprintf(archivo, "Closing server at %s", (char *)ctime(&timevar));
		fclose(archivo);
		printf("\nFin de programa servidor!\n");

	default: /* Parent process comes here. */
		exit(0);
	}
}

/*
 *				S E R V E R T C P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverTCP(int s, struct sockaddr_in clientaddr_in)
{
	int reqcnt = 0;			/* keeps count of number of requests */
	char buf[TAM_BUFFER];	/* This example uses TAM_BUFFER byte messages. */
	char hostname[MAXHOST]; /* remote host's name string */

	int len, len1, status;
	struct hostent *hp; /* pointer to host info for remote host */
	long timevar;		/* contains time returned by time() */

	struct linger linger; /* allow a lingering, graceful close; */
						  /* used when setting SO_LINGER */

	/* Look up the host information for the remote host
	 * that we have connected with.  Its internet address
	 * was returned by the accept call, in the main
	 * daemon loop above.
	 */
	// crear peticiones.log
	FILE *archivo;

	// Intentar abrir el archivo en modo de escritura al final ("w")
	archivo = fopen("peticiones.log", "a+");

	// Verificar si se abrió correctamente
	if (archivo == NULL)
	{
		printf("Error al abrir el archivo\n");
		exit(1); // Terminar el programa con un código de error
	}
	status = getnameinfo((struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in),
						 hostname, MAXHOST, NULL, 0, 0);
	if (status)
	{
		/* The information is unavailable for the remote
		 * host.  Just format its internet address to be
		 * printed out in the logging information.  The
		 * address will be shown in "internet dot format".
		 */
		/* inet_ntop para interoperatividad con IPv6 */
		if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
			perror(" inet_ntop \n");
	}
	/* Log a startup message. */
	time(&timevar);
	/* The port number must be converted first to host byte
	 * order before printing.  On most hosts, this is not
	 * necessary, but the ntohs() call is included here so
	 * that this program could easily be ported to a host
	 * that does require it.
	 */
	fprintf(archivo, "Startup TCP from %s port %u at %s", hostname, ntohs(clientaddr_in.sin_port), (char *)ctime(&timevar));
	printf("Startup TCP from %s port %u at %s", hostname, ntohs(clientaddr_in.sin_port), (char *)ctime(&timevar));

	/* Set the socket for a lingering, graceful close.
	 * This will cause a final close of this socket to wait until all of the
	 * data sent on it has been received by the remote host.
	 */
	linger.l_onoff = 1;
	linger.l_linger = 1;
	if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger,
				   sizeof(linger)) == -1)
	{
		errout(hostname);
	}

	/* Go into a loop, receiving requests from the remote
	 * client.  After the client has sent the last request,
	 * it will do a shutdown for sending, which will cause
	 * an end-of-file condition to appear on this end of the
	 * connection.  After all of the client's requests have
	 * been received, the next recv call will return zero
	 * bytes, signalling an end-of-file condition.  This is
	 * how the server will know that no more requests will
	 * follow, and the loop will be exited.
	 */

	// VARIABLES DE CONTROL PARA PATATA CALIENTE
	char response[TAM_BUFFER];
	int option = 0;
	int maxTry = MAXTRY;
	int numR = 0;
	int n1 = rand() % 11, n2 = rand() % 11;
	int result = n1 * n2;
	int flag = 0;

	while (len = recv(s, buf, TAM_BUFFER, 0))
	{
		if (len == -1)
			errout(hostname); /* error from recv */
							  /* The reason this while loop exists is that there
							   * is a remote possibility of the above recv returning
							   * less than TAM_BUFFER bytes.  This is because a recv returns
							   * as soon as there is some data, and will not wait for
							   * all of the requested data to arrive.  Since TAM_BUFFER bytes
							   * is relatively small compared to the allowed TCP
							   * packet sizes, a partial receive is unlikely.  If
							   * this example had used 2048 bytes requests instead,
							   * a partial receive would be far more likely.
							   * This loop will keep receiving until all TAM_BUFFER bytes
							   * have been received, thus guaranteeing that the
							   * next recv at the top of the loop will start at
							   * the begining of the next request.
							   */
		while (len < TAM_BUFFER)
		{
			len1 = recv(s, &buf[len], TAM_BUFFER - len, 0);
			if (len1 == -1)
				errout(hostname);
			len += len1;
		}
		/* Increment the request count. */
		reqcnt++;
		/* This sleep simulates the processing of the
		 * request that a real server might do.
		 */

		// LA LOGICA DEL SERVIDOR PARA HACER LA PATATA CALIENTE, utilizando buf como mensaje enviado por el clientcp y ya responder en base a eso
		fprintf(archivo, "(TCP)message from %s on port %u: %s", hostname, ntohs(clientaddr_in.sin_port), buf);
		char *command = strtok(buf, " ");

		if (strcmp(buf, "HOLA\r\n") == 0)
		{
			option = 1;
		}
		else if (strcmp(command, "RESPUESTA") == 0)
		{
			char *numR_str = strtok(NULL, "\r\n");
			numR = atoi(numR_str);
			option = 2;
		}
		else if (strcmp(buf, "+\r\n") == 0)
		{
			option = 3;
		}
		else if (strcmp(buf, "ADIOS\r\n") == 0)
		{
			option = 4;
		}
		else
		{
			option = 0;
		}

		switch (option)
		{
		case 1: // command hola
			snprintf(response, sizeof(response), "250 cuanto es %d x %d?#%d\r\n", n1, n2, maxTry);
			flag = 1;
			break;
		case 2: // command respuesta

			if (flag == 1)
			{

				if (maxTry == 0)
				{
					snprintf(response, sizeof(response), "375 FALLO\r\n");
				}
				else
				{
					if (numR == result)
					{
						maxTry = 0;
						snprintf(response, sizeof(response), "350 acierto\r\n");
					}
					else if (numR > result)
					{

						maxTry--;
						snprintf(response, sizeof(response), "250 menor#%d\r\n", maxTry);
					}
					else
					{

						maxTry--;
						snprintf(response, sizeof(response), "250 mayor#%d\r\n", maxTry);
					}
				}
			}
			else
			{
				snprintf(response, sizeof(response), "500 tienes que saludar primero, usa el comando hola\r\n");
			}
			break;
		case 3: // command +
			if (flag == 1)
			{
				maxTry = MAXTRY;
				n1 = rand() % 11;
				n2 = rand() % 11;
				result = n1 * n2;
				snprintf(response, sizeof(response), "250 cuanto es %d x %d?#%d\r\n", n1, n2, maxTry);
			}
			else
			{
				snprintf(response, sizeof(response), "500 tienes que saludar primero, usa el comando hola\r\n");
			}
			break;
		case 4: // command adios
			snprintf(response, sizeof(response), "221 cerrando el servicio\r\n");
			break;
		default:
			strcpy(response, "500 Syntax Error\r\n");
			break;
		}

		fprintf(archivo, "(TCP)response to %s on port %u: %s", hostname, ntohs(clientaddr_in.sin_port), response);
		/* Send a response back to the client. */
		if (send(s, response, TAM_BUFFER, 0) != TAM_BUFFER)
			errout(hostname);
	}

	/* The loop has terminated, because there are no
	 * more requests to be serviced.  As mentioned above,
	 * this close will block until all of the sent replies
	 * have been received by the remote host.  The reason
	 * for lingering on the close is so that the server will
	 * have a better idea of when the remote has picked up
	 * all of the data.  This will allow the start and finish
	 * times printed in the log file to reflect more accurately
	 * the length of time this connection was used.
	 */
	close(s);

	/* Log a finishing message. */
	time(&timevar);
	/* The port number must be converted first to host byte
	 * order before printing.  On most hosts, this is not
	 * necessary, but the ntohs() call is included here so
	 * that this program could easily be ported to a host
	 * that does require it.
	 */
	fprintf(archivo, "Completed TCP %s port %u, %d requests, at %s\n", hostname, ntohs(clientaddr_in.sin_port), reqcnt, (char *)ctime(&timevar));
	printf("Completed TCP %s port %u, %d requests, at %s\n", hostname, ntohs(clientaddr_in.sin_port), reqcnt, (char *)ctime(&timevar));
	fclose(archivo);
}

/*
 *	This routine aborts the child process attending the client.
 */
void errout(char *hostname)
{
	printf("Connection with %s aborted on error\n", hostname);
	exit(1);
}

/*
 *				S E R V E R U D P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverUDP(int s, struct sockaddr_in clientaddr_in)
{
	char buf[TAM_BUFFER];
	socklen_t addrlen = sizeof(struct sockaddr_in);

	// VARIABLES DE CONTROL PARA PATATA CALIENTE
	char response[TAM_BUFFER];
	int option = 0;
	int maxTry = MAXTRY;
	int numR = 0;
	int n1 = rand() % 11, n2 = rand() % 11;
	int result = n1 * n2;
	int flag = 0;
	long timevar;
	int reqcnt = 0;
	char hostname[MAXHOST];

	// crear peticiones.log
	FILE *archivo;

	// Intentar abrir el archivo en modo de escritura al final ("w")
	archivo = fopen("peticiones.log", "a+");

	// Verificar si se abrió correctamente
	if (archivo == NULL)
	{
		printf("Error al abrir el archivo\n");
		exit(1); // Terminar el programa con un código de error
	}

	if (recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&clientaddr_in, &addrlen) == -1)
	{
		printf("Unable to get response from");
		exit(1);
	}
	getnameinfo((struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in), hostname, MAXHOST, NULL, 0, 0);
	time(&timevar);
	fprintf(archivo, "Startup UDP from %s on port %u at %s", hostname, ntohs(clientaddr_in.sin_port), (char *)ctime(&timevar));
	printf("Startup UDP from %s on port %u at %s", hostname, ntohs(clientaddr_in.sin_port), (char *)ctime(&timevar));

	// escribir logica de patatcaliente
	// ##############################
	while (1)
	{
		fprintf(archivo, "(UDP)message from %s on port %u: %s", hostname, ntohs(clientaddr_in.sin_port), buf);
		reqcnt++;

		char *command = strtok(buf, " ");

		if (strcmp(buf, "HOLA\r\n") == 0)
		{
			option = 1;
		}
		else if (strcmp(command, "RESPUESTA") == 0)
		{
			char *numR_str = strtok(NULL, "\r\n");
			numR = atoi(numR_str);
			option = 2;
		}
		else if (strcmp(buf, "+\r\n") == 0)
		{
			option = 3;
		}
		else if (strcmp(buf, "ADIOS\r\n") == 0)
		{
			option = 4;
		}
		else
		{
			option = 0;
		}
		switch (option)
		{
		case 1: // command hola
			snprintf(response, sizeof(response), "250 cuanto es %d x %d?#%d\r\n", n1, n2, maxTry);
			flag = 1;
			break;
		case 2: // command respuesta

			if (flag == 1)
			{

				if (maxTry == 0)
				{
					snprintf(response, sizeof(response), "375 FALLO\r\n");
				}
				else
				{
					if (numR == result)
					{
						maxTry = 0;
						snprintf(response, sizeof(response), "350 acierto\r\n");
					}
					else if (numR > result)
					{

						maxTry--;
						snprintf(response, sizeof(response), "250 menor#%d\r\n", maxTry);
					}
					else
					{

						maxTry--;
						snprintf(response, sizeof(response), "250 mayor#%d\r\n", maxTry);
					}
				}
			}
			else
			{
				snprintf(response, sizeof(response), "500 tienes que saludar primero, usa el comando hola\r\n");
			}
			break;
		case 3: // command +
			if (flag == 1)
			{
				maxTry = MAXTRY;
				n1 = rand() % 11;
				n2 = rand() % 11;
				result = n1 * n2;
				snprintf(response, sizeof(response), "250 cuanto es %d x %d?#%d\r\n", n1, n2, maxTry);
			}
			else
			{
				snprintf(response, sizeof(response), "500 tienes que saludar primero, usa el comando hola\r\n");
			}
			break;
		case 4: // command adios
			snprintf(response, sizeof(response), "221 cerrando el servicio\r\n");
			break;
		default:
			strcpy(response, "500 Syntax Error\r\n");
			break;
		}

		fprintf(archivo, "(UDP)response to %s on port %u: %s", hostname, ntohs(clientaddr_in.sin_port), response);

		if (sendto(s, response, sizeof(response), 0, (struct sockaddr *)&clientaddr_in, addrlen) == -1)
		{
			perror("serverUDP");
			printf("Unable to send response");
			exit(1);
		}

		if (strcmp(response, "221 cerrando el servicio\r\n") == 0)
		{
			break;
		}

		memset(response, 0, TAM_BUFFER);
		memset(buf, 0, TAM_BUFFER);

		if (recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&clientaddr_in, &addrlen) == -1)
		{
			printf("Unable to get response from");
			exit(1);
		}
	}
	// ##############################
	fprintf(archivo, "Completed UDP %s on port %u, %d requests, at %s\n", hostname, ntohs(clientaddr_in.sin_port), reqcnt, (char *)ctime(&timevar));
	printf("Completed UDP %s on port %u, %d requests, at %s\n", hostname, ntohs(clientaddr_in.sin_port), reqcnt, (char *)ctime(&timevar));
	fclose(archivo);
}
