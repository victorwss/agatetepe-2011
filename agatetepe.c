/* 
 * Arquivo: agatetepe.c
 * 
 * Autor: Victor Williams Stafusa da Silva <stafusa@ime.usp.br>
 *
 * Adaptado do código original do Prof. Daniel Batista <batista@ime.usp.br>
 *
 * Iniciado em 12 de Setembro de 2011, 17:30
 * Respondeu o primeiro request decentemente com 404 em 14 de Setembro de 2011, 22:03
 * Respondeu o primeiro request com 413 em 14 de Setembro de 2011, 22:13
 * Respondeu o primeiro request com 200 em 14 de Setembro de 2011, 22:16
 * Respondeu o primeiro request com 400 em 14 de Setembro de 2011, 22:31
 * Respondeu o primeiro com uma figura como resultado em 14 de Setembro de 2011, 23:03
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#ifdef WIN32
#  include <Winsock2.h>
#else
#  include <sys/socket.h>
#  include <sys/param.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  indlude <netdb.h>
#endif

#define LISTENQ 1
#define MAXLINE 65535
#define MAXFILE 50000

#ifdef WIN32
#  define ERRO_SOCKET INVALID_SOCKET
#  define destroy_socket closesocket
#  define read_data(connfd, recvline) recv(connfd, recvline, MAXLINE, 0)
#  define write_data(connfd, recvline, length) send(connfd, recvline, length, 0)
#else
#  define ERRO_SOCKET -1
#  define destroy_socket close
#  define read_data(connfd, recvline) read(connfd, recvline, MAXLINE)
#  define write_data(connfd, recvline, length) write(connfd, recvline, length)
#endif

int starts_with(const char *great_string, const char *small_string) {
    int g = strlen(great_string);
    int s = strlen(small_string);
    if (s > g) return 0;

    int i;
    for (i = 0; i < s; i++) {
        if (great_string[i] != small_string[i]) return 0;
    }
    return 1;
}

int ends_with(const char *great_string, const char *small_string) {
    int g = strlen(great_string);
    int s = strlen(small_string);
    if (s > g) return 0;
    return strcmp(&great_string[g - s], small_string) == 0;
}

const char *status_string_of(int status) {
    switch (status) {
        case 200: return "OK";
        case 206: return "Partial Content";
        case 404: return "Not Found";
        case 400: return "Bad Request";
        case 413: return "Request entity too large";
        default: return "Unknown Server Error";
    }
}

const char *http_status_error_of(int status) {
    switch (status) {
        case 404: return "The document that you are looking for was not found. Sorry. :(";
        case 400: return "Your browser sent a request that the Agatetepe Server could not understand.";
        case 413: return "The document that you asked is greater than the 50000 bytes limit.";
        default: return "Unknown Server Error";
    }
}

int is_extension(const char *filename, const char *extension) {
    return ends_with(filename, extension);
}

const char *map_mime(const char *filename) {
    if (is_extension(filename, ".txt")) return "text/plain; charset=iso-8859-1";
    if (is_extension(filename, ".html")) return "text/html; charset=iso-8859-1";
    if (is_extension(filename, ".xml")) return "text/xml";
    if (is_extension(filename, ".js")) return "application/javascript; charset=iso-8859-1";
    if (is_extension(filename, ".es")) return "application/ecmascript; charset=iso-8859-1";
    if (is_extension(filename, ".json")) return "application/json; charset=iso-8859-1";
    if (is_extension(filename, ".css")) return "text/css; charset=iso-8859-1";
    if (is_extension(filename, ".png")) return "image/png";
    if (is_extension(filename, ".jpg")) return "image/jpeg";
    if (is_extension(filename, ".gif")) return "image/gif";
    if (is_extension(filename, ".xhtml")) return "application/xhtml+xml";
    return "application/x-unknown";
}

int write_response(int status, const char *content_type, const char *response_body, int body_size, char *full_response) {
    char headers[MAXLINE + 1];
    snprintf(headers, MAXLINE,
            "HTTP/1.0 %d %s\r\n"
            "Content-Length: %ld\r\n"
            /*"Date: Wed, 14 Sep 2011 01:10:19 GMT\r\n"*/
            "Server: Agatetepe/1.0\r\n"
            "Content-Type: %s\r\n"
            "\r\n",
            status, status_string_of(status), body_size, content_type);
    int headers_size = strlen(headers);
    memcpy(full_response, headers, headers_size);
    memcpy(&full_response[headers_size], response_body, body_size);
    return headers_size + body_size;
}

void send_response(int connfd, int status, const char *content_type, const char *response_body, int body_size) {
    char full_response[MAXLINE + 1];
    int total_size = write_response(status, content_type, response_body, body_size, full_response);
    write_data(connfd, full_response, total_size);
}

void send_error_response(int connfd, int status) {
    const char *mime_type = "text/html; charset=iso-8859-1";
    char response_body[MAXLINE + 1];
    snprintf(response_body, MAXLINE,
            "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN>\n"
            "<html><head><title>%d %s</title></head>\n"
            "<body><h1>%d %s</h1><p>%s</p>"
            "<p style=\"font-size: 10; color: blue;\">Powered by Agatetep&ecirc;</p>"
            "</body></html>",
            status, status_string_of(status), status, status_string_of(status), http_status_error_of(status));
    send_response(connfd, status, mime_type, response_body, strlen(response_body));
}

char *document_root;

int read_file(const char *filename, char *response_body, int *filesize_ret) {
    if (strlen(filename) > 256) return 404;
    if (strstr(filename, "..") != NULL) return 404;

    char complete_filename[256];
    int path_length = strlen(document_root);

    snprintf(complete_filename, 256 + path_length, "%s%s", document_root, filename);

    FILE *f = fopen(complete_filename, "rb");
    if (f == NULL) return 404;

    struct stat file_status;
    stat(complete_filename, &file_status);
    off_t filesize = file_status.st_size;

    if (filesize > MAXFILE) {
        fclose(f);
        return 413;
    }

    size_t written = fread(response_body, 1, filesize, f);
    fclose(f);

    *filesize_ret = written;
    return written == filesize ? 200 : 206;
}

int get_request_path(const char *line, char *path) {
    if (!starts_with(line, "GET ")) return 400;
    if (!ends_with(line, " HTTP/1.1") && !ends_with(line, " HTTP/1.0")) return 400;

    int i = 0;
    int max = strlen(line);

    /* 4 = strlen("GET "), 9 = strlen(" HTTP/1.1") */
    for (i = 4; i < max - 9; i++) {
        path[i - 4] = line[i];
    }
    path[max - 13] = 0;

    return 0;
}

void make_service(int connfd) {
    /* Armazena o tamanho da string lida do cliente. */
    ssize_t n;

    char recvline[MAXLINE + 1], line[MAXLINE + 1], path[MAXLINE + 1], body[MAXLINE + 1];
    n = read_data(connfd, recvline);
    recvline[n] = 0;

    char *end_of_line = strstr(recvline, "\r\n");
    if (end_of_line == NULL) {
        send_error_response(connfd, 400);
        return;
    }

    int line_size = ((long) end_of_line) - ((long) recvline);
    strncpy(line, recvline, line_size);

    int status = get_request_path(line, path);
    if (status >= 300) {
        send_error_response(connfd, status);
        return;
    }

    int filesize;
    status = read_file(path, body, &filesize);
    if (status >= 300) {
        send_error_response(connfd, status);
        return;
    }

    send_response(connfd, status, map_mime(path), body, filesize);
}

int child(int connfd) {

    /* Agora pode ler do socket e escrever no socket. Isto tem
     * que ser feito em sincronia com o cliente. Não faz sentido
     * ler sem ter o que ler. Ou seja, neste caso está sendo
     * considerado que o cliente vai enviar algo para o servidor.
     * O servidor vai processar o que tiver sido enviado e vai
     * enviar uma resposta para o cliente (Que precisará estar
     * esperando por esta resposta).
     */

    /* ========================================================= */
    /* ========================================================= */
    /*                         EP1 INÍCIO                        */
    /* ========================================================= */
    /* ========================================================= */

    make_service(connfd);

    /* ========================================================= */
    /* ========================================================= */
    /*                         EP1 FIM                           */
    /* ========================================================= */
    /* ========================================================= */

    return 0;
}

void *thread_fork(void *data) {
    child((int) data);
    pthread_exit(NULL);
}

int create_child(int connfd) {
    pthread_t new_thread;
    return pthread_create(&new_thread, NULL, thread_fork, (void *) connfd);
}

int server_listen(int porta) {

    printf("Criando socket...\n");
    fflush(stdout);

    /* Criação de um socket. É como se fosse um descritor de arquivo. É
     * possível fazer operações como read, write e close. Neste
     * caso o socket criado é um socket IPv4 (por causa do AF_INET),
     * que vai usar TCP (por causa do SOCK_STREAM), já que o HTTP
     * funciona sobre TCP, e será usado para uma aplicação convencional sobre
     * a Internet (por causa do número 0).
     */
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == ERRO_SOCKET) {
        printf("\n%d", errno);
        perror("socket :(\n");
        return 2;
    }

    printf("Vinculando socket...\n");
    fflush(stdout);

    /* Informações sobre o socket (endereço e porta) ficam nesta struct. */
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));

    /* Agora é necessário informar os endereços associados a este
     * socket. É necessário informar o endereço / interface e a porta,
     * pois mais adiante o socket ficará esperando conexões nesta porta
     * e neste(s) endereços. Para isso é necessário preencher a struct
     * servaddr. É necessário colocar lá o tipo de socket (No nosso
     * caso AF_INET porque é IPv4), em qual endereço / interface serão
     * esperadas conexões (Neste caso em qualquer uma -- INADDR_ANY) e
     * qual a porta. Neste caso será a porta que foi passada como
     * argumento no shell.
     */
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(porta);

    /* Associa o socket a um endereço. */
    int bind_result = bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if (bind_result == -1) {
        perror("bind :(\n");
        destroy_socket(listenfd);
        return 3;
    }

    printf("Preparando para ouvir socket servidor...");
    fflush(stdout);

    /* Como este código é o código de um servidor, o socket será um
     * socket passivo. Para isto é necessário chamar a função listen
     * que define que este é um socket de servidor que ficará esperando
     * por conexões nos endereços definidos na função bind.
     */
    int listen_result = listen(listenfd, LISTENQ);
    if (listen_result == -1) {
        perror("listen :(\n");
        destroy_socket(listenfd);
        return 4;
    }

    printf("[Servidor no ar. Aguardando conexoes na porta %d]\n", porta);
    printf("[Para finalizar, pressione CTRL+c ou rode um kill ou killall]\n");
    fflush(stdout);

    /* O servidor no final das contas é um loop infinito de espera por
     * conexões e processamento de cada uma individualmente.
     */
    while (1) {

        /* O socket inicial que foi criado é o socket que vai aguardar
         * pela conexão na porta especificada. Mas pode ser que existam
         * diversos clientes conectando no servidor. Por isso deve-se
         * utilizar a função accept. Esta função vai retirar uma conexão
         * da fila de conexões que foram aceitas no socket listenfd e
         * vai criar um socket específico para esta conexão. O descritor
         * deste novo socket é o retorno da função accept.
         */
        int connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
        if (connfd == -1) {
            perror("accept :(\n");
            destroy_socket(listenfd);
            return 5;
        }

        /* Agora o servidor precisa tratar este cliente de forma
         * separada. Para isto é criado um processo filho usando a
         * função fork. O processo vai ser uma cópia deste. Depois da
         * função fork, os dois processos (pai e filho) estarão no mesmo
         * ponto do código, mas cada um terá um PID diferente. Assim é
         * possível diferenciar o que cada processo terá que fazer. O
         * filho tem que processar a requisição do cliente. O pai tem
         * que voltar no loop para continuar aceitando novas conexões.
         * Se o retorno da função fork for zero, é porque está no
         * processo filho.
         */
        int create_child_result = create_child(connfd);

        if (create_child_result != 0) {
            perror("Failed to create a child :(\n");
            destroy_socket(listenfd);
            return 6;
        }
    }

    destroy_socket(listenfd);
    return 0;
}

int inner_main(int argc, char **argv) {

    int porta;

    if (argc == 1) {
        porta = 50000;
        document_root = "./docroot";
    } else if (argc == 2) {
        porta = atoi(argv[1]);
        document_root = "./docroot";
    } else if (argc == 3) {
        porta = atoi(argv[1]);
        document_root = argv[2];
    } else {
        fprintf(stderr, "Uso: %s [Porta [DocRoot]]\n", argv[0]);
        fprintf(stderr, "Vai rodar um servidor de echo na porta TCP dada.\n");
        fprintf(stderr, "O servidor servirá documentos na pasta informada em DocRoot.\n");
        fprintf(stderr, "Exemplo de uso: %s 2345 /home/webdocs.\n", argv[0]);
        return 1;
    }

    printf("Iniciando Agatetepe na porta %d...\n", porta);
    printf("Os documentos serao lidos de %s\n", document_root);
    fflush(stdout);

#ifdef WIN32
    WSADATA wsaData;
    int werror = WSAStartup(MAKEWORD(1, 1), &wsaData);
    if (werror == SOCKET_ERROR) {
        printf("Winsucks :( - %d\n", werror);
        return 666;
    }
#endif

    int result = server_listen(porta);

#ifdef WIN32
    WSACleanup();
#endif

    return result;
}

int test(char *test_name, int result) {
    printf("%s: %s\n", test_name, result ? "Ok" : "Falhou");
}

int run_tests() {
    test("start1", starts_with("abcdef", "abc"));
    test("start2", !starts_with("abcdef", "bc"));
    test("start3", !starts_with("abcdef", "gg"));
    test("end1", ends_with("abcdef", "def"));
    test("end2", !ends_with("abcdef", "bc"));
    test("end3", !ends_with("abcdef", "gg"));
    test("mapmime1", strcmp("image/gif", map_mime("ze/joao.gif")) == 0);

    char c[256];
    memset(c, 'a', 256);
    test("getrequestpath1-a", get_request_path("GET /joao/pedro/maria.txt HTTP/1.1", c) == 0);
    test("getrequestpath1-b", strcmp("/joao/pedro/maria.txt", c) == 0);

    test("getrequestpath2-a", get_request_path("POST xxxxxxxx HTTP/1.1", c) == 400);
    test("getrequestpath2-b", strcmp("/joao/pedro/maria.txt", c) == 0);

    test("getrequestpath3-a", get_request_path("GET xxxxxxxx HTTP/5.1", c) == 400);
    test("getrequestpath3-b", strcmp("/joao/pedro/maria.txt", c) == 0);

    test("getrequestpath4-a", get_request_path("sdfsdf s fsd sdf sdf sdf", c) == 400);
    test("getrequestpath4-b", strcmp("/joao/pedro/maria.txt", c) == 0);

    int size;
    int status = read_file("teste.txt", c, &size);
    printf("%d - %s\n", status, c);
    test("readfile1-a", status == 200);
    test("readfile1-b", strncmp(c, "Bem vindo ao meu teste.", size) == 0);
    test("readfile1-c", size == strlen("Bem vindo ao meu teste."));

    char buf[10240];
    write_response(200, "text/plain", "banana", 6, buf);
    printf("%s", buf);
}

int main(int argc, char **argv) {
    /*run_tests();*/
    printf("Ola, bem vindo ao Servidor Agatetepe\n");
    fflush(stdout);
    int value = inner_main(argc, argv);
    pthread_exit((void *) value);
    return 0;
}