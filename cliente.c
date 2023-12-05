#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>

extern int errno;

#define ADDRNOTFOUND 0xffffffff
#define RETRIES 5
#define PUERTO 17289
#define TIMEOUT 6
#define MAXHOST 512
#define TAM_BUFFER 516

void handler()
{
	printf("Alarma recibida \n");
}

void clienteTCP(char *name, char *host, char *route);
void clienteUDP(char *name, char *host, char *route);
void strtrim(char *str);
int main(argc, argv)
int argc;
char *argv[];
{
	if (argc != 4)
	{
		fprintf(stderr, "Usage:  %s <remote host> <protocol> <route to .txt>\n", argv[0]);
		exit(1);
	}

	if (strcmp(argv[2], "TCP") == 0)
	{
		clienteTCP(argv[0], argv[1], argv[3]);
	}
	else if (strcmp(argv[2], "UDP") == 0)
	{
		clienteUDP(argv[0], argv[1], argv[3]);
	}
}

void clienteTCP(char *name, char *host, char *route)
{
	int s; /* connected socket descriptor */
	struct addrinfo hints, *res;
	long timevar;					/* contains time returned by time() */
	struct sockaddr_in myaddr_in;	/* for local socket address */
	struct sockaddr_in servaddr_in; /* for server socket address */
	int addrlen, i, j, errcode;
	/* This example uses TAM_BUFFER byte messages. */
	char buf[TAM_BUFFER];
	char auxBuf[TAM_BUFFER];

	char *route_txt = route;

	/* Create the socket. */
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1)
	{
		perror(name);
		fprintf(stderr, "%s: unable to create socket\n", name);
		exit(1);
	}

	/* clear out address structures */
	memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));

	/* Set up the peer address to which we will connect. */
	servaddr_in.sin_family = AF_INET;

	/* Get the host information for the hostname that the
	 * user passed in. */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	/* esta funci�n es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
	errcode = getaddrinfo(host, NULL, &hints, &res);
	if (errcode != 0)
	{
		/* Name was not found.  Return a
		 * special value signifying the error. */
		fprintf(stderr, "%s: No es posible resolver la IP de %s\n",
				name, host);
		exit(1);
	}
	else
	{
		/* Copy address of host */
		servaddr_in.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
	}
	freeaddrinfo(res);

	/* puerto del servidor en orden de red*/
	servaddr_in.sin_port = htons(PUERTO);

	/* Try to connect to the remote server at the address
	 * which was just built into peeraddr.
	 */
	if (connect(s, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1)
	{
		perror(name);
		fprintf(stderr, "%s: unable to connect to remote\n", name);
		exit(1);
	}
	/* Since the connect call assigns a free address
	 * to the local end of this connection, let's use
	 * getsockname to see what it assigned.  Note that
	 * addrlen needs to be passed in as a pointer,
	 * because getsockname returns the actual length
	 * of the address.
	 */
	addrlen = sizeof(struct sockaddr_in);
	if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1)
	{
		perror(name);
		fprintf(stderr, "%s: unable to read socket address\n", name);
		exit(1);
	}

	/* Print out a startup message for the user. */
	time(&timevar);
	/* The port number must be converted first to host byte
	 * order before printing.  On most hosts, this is not
	 * necessary, but the ntohs() call is included here so
	 * that this program could easily be ported to a host
	 * that does require it.
	 */
	printf("Connected to %s on port %u at %s",
		   host, ntohs(myaddr_in.sin_port), (char *)ctime(&timevar));

	// IMPLEMENTAR LA LOGICA DE ENVIO DE MENSAJES AL SERVIDOR MEDIANTE LECTURA DE COMANDOS POR TECLADO

	// Abrir el archivo
	FILE *archivo = fopen(route_txt, "r");
	if (archivo == NULL)
	{
		perror("Error al abrir el archivo");
		exit(1);
	}

	while (fgets(buf, sizeof(buf), archivo) != NULL)
	{

		// Leer comando desde la terminal
		// printf("Ingrese un comando: ");
		// fgets(buf, TAM_BUFFER - 2, stdin); // Dejar espacio para CR-LF

		// Eliminar el carácter de nueva línea al final del comando
		buf[strcspn(buf, "\n")] = '\0';

		// Evitar líneas vacías
		strtrim(buf);
		// Agregar el CR-LF al final del comando
		strcat(buf, "\r\n");

		// Imprimir la línea antes de enviarla (para depuración)
		// printf("Línea a enviar: \"%s\"\n", buf);

		if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER)
		{
			fprintf(stderr, "%s: Connection aborted on error ", name);
			exit(1);
		}

		strcpy(auxBuf, buf);
		// limpiar buffer
		memset(buf, 0, TAM_BUFFER);

		/* Now, start receiving all of the replys from the server.
		 * This loop will terminate when the recv returns zero,
		 * which is an end-of-file condition.  This will happen
		 * after the server has sent all of its replies, and closed
		 * its end of the connection.
		 */
		i = recv(s, buf, TAM_BUFFER, 0);
		if (i == -1)
		{
			perror(name);
			fprintf(stderr, "%s: error reading result\n", name);
			exit(1);
		}
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
		 * the begining of the next reply.
		 */
		while (i < TAM_BUFFER)
		{
			j = recv(s, &buf[i], TAM_BUFFER - i, 0);
			if (j == -1)
			{
				perror(name);
				fprintf(stderr, "%s: error reading result\n", name);
				exit(1);
			}
			i += j;
		}

		// printf("%s", buf);

		// limpiar buffer
		memset(buf, 0, TAM_BUFFER);

		// Salir del bucle si el comando es "exit"
		if (strcmp(auxBuf, "ADIOS\r\n") == 0)
		{
			printf("Saliendo del programa.\n");
			break;
		}
	}

	/* Now, shutdown the connection for further sends.
	 * This will cause the server to receive an end-of-file
	 * condition after it has received all the requests that
	 * have just been sent, indicating that we will not be
	 * sending any further requests.
	 */
	if (shutdown(s, 1) == -1)
	{
		perror(name);
		fprintf(stderr, "%s: unable to shutdown socket\n", name);
		exit(1);
	}

	/* Print message indicating completion of task. */
	time(&timevar);
	printf("All done at %s", (char *)ctime(&timevar));
}

void clienteUDP(char *name, char *host, char *route)
{
	int i, errcode;
	int s;							/* socket descriptor */
	long timevar;					/* contains time returned by time() */
	struct sockaddr_in myaddr_in;	/* for local socket address */
	struct sockaddr_in servaddr_in; /* for server socket address */
	struct in_addr reqaddr;			/* for returned internet address */
	int addrlen, n_retry = RETRIES;
	struct sigaction vec;
	char hostname[MAXHOST];
	struct addrinfo hints, *res;

	char buf[TAM_BUFFER];

	char *route_txt = route;

	/* Create the socket. */
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s == -1)
	{
		perror(name);
		fprintf(stderr, "%s: unable to create socket\n", name);
		exit(1);
	}

	/* clear out address structures */
	memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));

	/* Bind socket to some local address so that the
	 * server can send the reply back.  A port number
	 * of zero will be used so that the system will
	 * assign any available port number.  An address
	 * of INADDR_ANY will be used so we do not have to
	 * look up the internet address of the local host.
	 */
	myaddr_in.sin_family = AF_INET;
	myaddr_in.sin_port = 0;
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	if (bind(s, (const struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1)
	{
		perror(name);
		fprintf(stderr, "%s: unable to bind socket\n", name);
		exit(1);
	}
	addrlen = sizeof(struct sockaddr_in);
	if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1)
	{
		perror(name);
		fprintf(stderr, "%s: unable to read socket address\n", name);
		exit(1);
	}

	/* Print out a startup message for the user. */
	time(&timevar);
	/* The port number must be converted first to host byte
	 * order before printing.  On most hosts, this is not
	 * necessary, but the ntohs() call is included here so
	 * that this program could easily be ported to a host
	 * that does require it.
	 */
	printf("Connected to %s on port %u at %s", host, ntohs(myaddr_in.sin_port), (char *)ctime(&timevar));

	/* Set up the server address. */
	servaddr_in.sin_family = AF_INET;
	/* Get the host information for the server's hostname that the
	 * user passed in.
	 */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	/* esta funci�n es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
	errcode = getaddrinfo(host, NULL, &hints, &res);
	if (errcode != 0)
	{
		/* Name was not found.  Return a
		 * special value signifying the error. */
		fprintf(stderr, "%s: No es posible resolver la IP de %s\n",
				name, host);
		exit(1);
	}
	else
	{
		/* Copy address of host */
		servaddr_in.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
	}
	freeaddrinfo(res);
	/* puerto del servidor en orden de red*/
	servaddr_in.sin_port = htons(PUERTO);

	/* Registrar SIGALRM para no quedar bloqueados en los recvfrom */
	vec.sa_handler = (void *)handler;
	vec.sa_flags = 0;
	if (sigaction(SIGALRM, &vec, (struct sigaction *)0) == -1)
	{
		perror(" sigaction(SIGALRM)");
		fprintf(stderr, "%s: unable to register the SIGALRM signal\n", name);
		exit(1);
	}

	// Abrir el archivo
	FILE *archivo = fopen(route_txt, "r");
	if (archivo == NULL)
	{
		perror("Error al abrir el archivo");
		exit(1);
	}

	// Escribir logica patatacaliente
	// ##############################
	char response[TAM_BUFFER];
	while ((fgets(buf, sizeof(buf), archivo) != NULL) | (n_retry == 0))
	{

		// Leer comando desde la terminal
		// printf("Ingrese un comando: ");
		// fgets(buf, TAM_BUFFER - 2, stdin); // Dejar espacio para CR-LF, leemos por teclado y dejamos dos carcateres libres \r\n

		buf[strcspn(buf, "\n")] = '\0';

		// Evitar líneas vacías
		strtrim(buf);

		strcat(buf, "\r\n");

		if (sendto(s, buf, sizeof(buf), 0, (struct sockaddr *)&servaddr_in, addrlen) == -1)
		{
			perror(name);
			fprintf(stderr, "%s: unable to send request\n", name);
			exit(1);
		}

		alarm(TIMEOUT);
		/* Wait for the reply to come in. */
		if (recvfrom(s, response, sizeof(response), 0, (struct sockaddr *)&servaddr_in, &addrlen) == -1)
		{
			if (errno == EINTR)
			{
				/* Alarm went off and aborted the receive.
				 * Need to retry the request if we have
				 * not already exceeded the retry limit.
				 */
				printf("attempt %d (retries %d).\n", n_retry, RETRIES);
				n_retry--;
			}
			else
			{
				printf("Unable to get response from");
				exit(1);
			}
		}
		else
		{
			alarm(0);
		}

		// printf("%s", response); // recibimos msge y printeamos
		if (strcmp(buf, "ADIOS\r\n") == 0)
		{
			printf("Saliendo del programa.\n");
			break;
		}
		memset(buf, 0, TAM_BUFFER);
		memset(response, 0, TAM_BUFFER);
	}
	// ##############################
	time(&timevar);
	printf("All done at %s", (char *)ctime(&timevar));
}

void strtrim(char *str)
{
	int end = strlen(str) - 1;

	while (end >= 0 && isspace(str[end]))
	{
		end--;
	}

	str[end + 1] = '\0';
}