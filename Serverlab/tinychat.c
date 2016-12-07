
/*
 * tinychat.c - [Starting code for] a web-based chat server.
 *
 * Based on:
 *  tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *      GET method to serve static and dynamic content.
 *   Tiny Web server
 *   Dave O'Hallaron
 *   Carnegie Mellon University
 */
#include "csapp.h"
#include "dictionary.h"
#include "more_string.h"

void doit(int fd);

dictionary_t *read_requesthdrs(rio_t *rp);

void read_postquery(rio_t *rp, dictionary_t *headers, dictionary_t *d);

void parse_query(const char *uri, dictionary_t *d);

void serve_form(int fd, const char *pre_cont_header, const char *conversation, const char *name, const char *topic);

void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg);

static void print_stringdictionary(dictionary_t *d);

void *go_doit(void *connfdp);

dictionary_t *global_dic; //define the conversations dictionary

sem_t sem_lock; //Lock to be added.

int main(int argc, char **argv) {
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    global_dic = make_dictionary(COMPARE_CASE_SENS, free);
    Sem_init(&sem_lock, 0, 1);

    /* Check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);

    /* Don't kill the server if there's an error, because
       we want to survive errors due to a client. But we
       do want to report errors. */
    exit_on_error(0);

    /* Also, don't stop on broken connections: */
    Signal(SIGPIPE, SIG_IGN);

    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);
        if (connfd >= 0) {
            Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE,
                        port, MAXLINE, 0);
            printf("Accepted connection from (%s, %s)\n", hostname, port);
            int *connfdp;
            pthread_t th;
            connfdp = malloc(sizeof(int));
            *connfdp = connfd;
            Pthread_create(&th, NULL, go_doit, connfdp);
            Pthread_detach(th);
        }
    }
}

void *go_doit(void *connfdp) {
    int connfd = *(int *) connfdp;
    free(connfdp);
    doit(connfd);
    Close(connfd);
    return NULL;
}

/*
 * doit - handle one HTTP request/response transaction
 */
void doit(int fd) {
    char buf[MAXLINE], *method, *uri, *version;
    rio_t rio;
    dictionary_t *headers, *query;

    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    if (Rio_readlineb(&rio, buf, MAXLINE) <= 0)
        return;
    printf("%s", buf);

    if (!parse_request_line(buf, &method, &uri, &version)) {
        clienterror(fd, method, "400", "Bad Request",
                    "TinyChat did not recognize the request");
    } else {
        if (strcasecmp(version, "HTTP/1.0")
            && strcasecmp(version, "HTTP/1.1")) {
            clienterror(fd, version, "501", "Not Implemented",
                        "TinyChat does not implement that version");
        } else if (strcasecmp(method, "GET")
                   && strcasecmp(method, "POST")) {
            clienterror(fd, method, "501", "Not Implemented",
                        "TinyChat does not implement that method");
        } else {
            headers = read_requesthdrs(&rio);

            query = make_dictionary(COMPARE_CASE_SENS, free);
            parse_uriquery(uri, query);


            if (!strcasecmp(method, "POST")) {
                read_postquery(&rio, headers, query);

//                printf("*********************************** 1\n");
//                print_stringdictionary(query);
//                printf("*********************************** 1\n");

                const char *entry = dictionary_get(query, "entry");
                const char *name = dictionary_get(query, "name");
                const char *topic = dictionary_get(query, "topic");
                const char *new_header = append_strings("Tinychat - ", topic, NULL);
                const char *temp;


                //entry reported and found
                if (entry) {
                    //No exist conversation, create a new one
                    if (!dictionary_get(global_dic, topic)) {
                        const char *new_append = append_strings("", NULL);
                        P(&sem_lock);
                        dictionary_set(global_dic, topic, (void *) new_append);
                        V(&sem_lock);
                    } else {
                        P(&sem_lock);
                        temp = dictionary_get(global_dic, topic);
                        V(&sem_lock);
                        serve_form(fd, new_header, name, topic, temp);
                    }
//                    printf("*********************************** 2\n");
//                    print_stringdictionary(query);
//                    printf("*********************************** 2\n");


                    P(&sem_lock);
                    const char *last_mes = dictionary_get(global_dic, topic);
                    V(&sem_lock);
                    serve_form(fd, new_header, name, topic, last_mes);
                } else {
                    const char *new_message = dictionary_get(query, "message");

                    P(&sem_lock);
                    const char *last_mes = dictionary_get(global_dic, topic);
                    V(&sem_lock);

                    if (!strlen(new_message)) {
                        serve_form(fd, new_header, name, topic, last_mes);
                    } else {
                        const char *new_mes = append_strings(last_mes, "<br>", name, ": ", new_message, NULL);
                        P(&sem_lock);
                        dictionary_set(global_dic, topic, (void *) new_mes);
                        V(&sem_lock);
                        serve_form(fd, new_header, name, topic, new_mes);
                    }
                }
//                print_stringdictionary(global_dic);
            } else if (!strcasecmp(method, "GET")) {
                parse_uriquery(uri, query);
                if (starts_with("/conversation", uri)) {

                    const char *topic = dictionary_get(query, "topic");
                    P(&sem_lock);
                    const char *last_mes = dictionary_get(global_dic, topic);
                    V(&sem_lock);

                    if (!last_mes) {
                        char *new_mes = append_strings("", NULL);
                        P(&sem_lock);
                        dictionary_set(global_dic, topic, (void *) new_mes);
                        V(&sem_lock);

                    } else {
                        serve_form(fd, "", "", topic, last_mes);
                    }
                } else if (starts_with("/say", uri)) {

                    const char *topic = dictionary_get(query, "topic");

                    const char *name = dictionary_get(query, "user");

                    const char *content = dictionary_get(query, "content");

                    const char *all_cont = dictionary_get(global_dic, topic);

                    const char *new_conv = append_strings(all_cont, "<br>", name, ": ", content, NULL);

                    P(&sem_lock);
                    dictionary_set(global_dic, topic, (void *) new_conv);
                    V(&sem_lock);

                    serve_form(fd, "", "", "", "");
                } else if (starts_with("/import", uri)) {
                    //       const char *topic = dictionary_get(query, "topic");
                    char *host = dictionary_get(query, "host");
                    void *port = dictionary_get(query, "port");
                    int port_num = *((int *) port);
                    //Pretend your server is a client and get contents from other server and append to your specific topic of conversation
                    int clientfd;
                    clientfd = Open_clientfd(host, port_num);

                    exit_on_error(0);

                    Signal(SIGPIPE, SIG_IGN);

                    Rio_readinitb(&rio, clientfd);
                    if (Rio_readlineb(&rio, buf, MAXLINE) <= 0)
                        return;

                    //     printf("!!!!!!!!!!!!!!!!!!!!!!+++++++++++++++++++++++++++++++++++++++!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n%s",
                    //          buf);

                    if (!parse_request_line(buf, &method, &uri, &version)) {
                        clienterror(fd, method, "400", "Bad Request",
                                    "TinyChat did not recognize the request");
                    } else {
                        if (strcasecmp(version, "HTTP/1.0")
                            && strcasecmp(version, "HTTP/1.1")) {
                            clienterror(fd, version, "501", "Not Implemented",
                                        "TinyChat does not implement that version");
                        } else if (strcasecmp(method, "GET")
                                   && strcasecmp(method, "POST")) {
                            clienterror(fd, method, "501", "Not Implemented",
                                        "TinyChat does not implement that method");
                        } else {
                            headers = read_requesthdrs(&rio);
                            query = make_dictionary(COMPARE_CASE_SENS, free);
                            parse_uriquery(uri, query);


                            const char *new_topic = dictionary_get(query, "topic");
                            const char *content = dictionary_get(query, "content");
                            const char *name = dictionary_get(query, "user");
                            const char *all_cont = dictionary_get(global_dic, new_topic);

                            const char *new_conv = append_strings(all_cont, "<br>", name, ": ", content, NULL);

                            P(&sem_lock);
                            dictionary_set(global_dic, new_topic, (void *) new_conv);
                            V(&sem_lock);

                            serve_form(fd, "", "", "", "");

                        }

                    }

                    

                } else {
                    serve_form(fd, "Welcome to TinyChat", "", "", "");//default
                }
                //                print_stringdictionary(global_dic);
            }

            /* Clean up */
            free_dictionary(query);
            free_dictionary(headers);
        }

        /* Clean up status line */
        free(method);
        free(uri);
        free(version);
    }
}


/*
 * read_requesthdrs - read HTTP request headers
 */
dictionary_t *read_requesthdrs(rio_t *rp) {
    char buf[MAXLINE];
    dictionary_t *d = make_dictionary(COMPARE_CASE_INSENS, free);

    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    while (strcmp(buf, "\r\n")) {
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
        parse_header_line(buf, d);
    }

    return d;
}

void read_postquery(rio_t *rp, dictionary_t *headers, dictionary_t *dest) {
    char *len_str, *type, *buffer;
    int len;

    len_str = dictionary_get(headers, "Content-Length");
    len = (len_str ? atoi(len_str) : 0);

    type = dictionary_get(headers, "Content-Type");


    buffer = malloc(len + 1);
    Rio_readnb(rp, buffer, len);
    buffer[len] = 0;

    if (!strcasecmp(type, "application/x-www-form-urlencoded")) {
        parse_query(buffer, dest);
    }

    free(buffer);
}

static char *ok_header(size_t len, const char *content_type) {
    char *len_str, *header;

    header = append_strings("HTTP/1.0 200 OK\r\n",
                            "Server: TinyChat Web Server\r\n",
                            "Connection: close\r\n",
                            "Content-length: ", len_str = to_string(len), "\r\n",
                            "Content-type: ", content_type, "\r\n\r\n",
                            NULL);
    free(len_str);

    return header;
}

/*
 * serve_form - sends a form to a client
 */
void serve_form(int fd, const char *pre_content, const char *name, const char *topic, const char *conversation) {
    size_t len;
    char *body, *header;
    if (!strlen(pre_content)) body = append_strings(conversation, "<br>", NULL);
    else if (strcmp(pre_content, "Welcome to TinyChat") == 0) {
        body = append_strings("<html><body>\r\n",
                              "<p>Welcome to TinyChat</p>",
                              "\r\n<form action=\"reply\" method=\"post\"",
                              " enctype=\"application/x-www-form-urlencoded\"",
                              " accept-charset=\"UTF-8\">\r\n",
                              "Name: <input type=\"text\" name=\"name\"><br>\r\n",
                              "Topic: <input type=\"text\" name=\"topic\"><br>\r\n",
                              "<input type=\"hidden\" name=\"entry\" value=\"yes\">\r\n",
                              "<input type=\"submit\" value=\"Join Conversation\">\r\n",
                              "</form></body></html>\r\n",
                              NULL);

    } else {
        body = append_strings("<html><body>\r\n",
                              "<p>", pre_content, "</p>",
                              "\r\n<form action=\"reply\" method=\"post\"",
                              " enctype=\"application/x-www-form-urlencoded\"",
                              " accept-charset=\"UTF-8\">\r\n",
                              conversation, "<br>\r\n", name, ": ",
                              "<input type=\"hidden\" name=\"name\" value=", name, ">\r\n",
                              "<input type=\"hidden\" name=\"topic\" value=", topic, ">\r\n",
                              "<input type=\"text\" name=\"message\">\r\n",
                              "<input type=\"submit\" value=\"Add\">\r\n",
                              "</form></body></html>\r\n",
                              NULL);
    }

    len = strlen(body);

    /* Send response headers to client */
    header = ok_header(len, "text/html; charset=utf-8");
    Rio_writen(fd, header, strlen(header));
    printf("Response headers:\n");
    printf("%s", header);

    free(header);

    /* Send response body to client */
    Rio_writen(fd, body, len);

    free(body);
}

/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg) {
    size_t len;
    char *header, *body, *len_str;

    body = append_strings("<html><title>Tiny Error</title>",
                          "<body bgcolor=""ffffff"">\r\n",
                          errnum, " ", shortmsg,
                          "<p>", longmsg, ": ", cause,
                          "<hr><em>The Tiny Web server</em>\r\n",
                          NULL);
    len = strlen(body);

    /* Print the HTTP response */
    header = append_strings("HTTP/1.0 ", errnum, " ", shortmsg,
                            "Content-type: text/html; charset=utf-8\r\n",
                            "Content-length: ", len_str = to_string(len), "\r\n\r\n",
                            NULL);
    free(len_str);

    Rio_writen(fd, header, strlen(header));
    Rio_writen(fd, body, len);

    free(header);
    free(body);
}

static void print_stringdictionary(dictionary_t *d) {
    int i, count;

    count = dictionary_count(d);
    for (i = 0; i < count; i++) {
        printf("%s ========================================== %s\n",
               dictionary_key(d, i),
               (const char *) dictionary_value(d, i));
    }
    printf("\n");
}



//
///*
// * tinychat.c - [Starting code for] a web-based chat server.
// *
// * Based on:
// *  tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
// *      GET method to serve static and dynamic content.
// *   Tiny Web server
// *   Dave O'Hallaron
// *   Carnegie Mellon University
// */
//#include "csapp.h"
//#include "dictionary.h"
//#include "more_string.h"
//
//void doit(int fd);
//dictionary_t *read_requesthdrs(rio_t *rp);
//void read_postquery(rio_t *rp, dictionary_t *headers, dictionary_t *d);
//void parse_query(const char *uri, dictionary_t *d);
//void serve_form(int fd, const char *pre_content);
//void clienterror(int fd, char *cause, char *errnum,
//                 char *shortmsg, char *longmsg);
//static void print_stringdictionary(dictionary_t *d);
//
//int main(int argc, char **argv)
//{
//    int listenfd, connfd;
//    char hostname[MAXLINE], port[MAXLINE];
//    socklen_t clientlen;
//    struct sockaddr_storage clientaddr;
//
//    /* Check command line args */
//    if (argc != 2) {
//        fprintf(stderr, "usage: %s <port>\n", argv[0]);
//        exit(1);
//    }
//
//    listenfd = Open_listenfd(argv[1]);
//
//    /* Don't kill the server if there's an error, because
//       we want to survive errors due to a client. But we
//       do want to report errors. */
//    exit_on_error(0);
//
//    /* Also, don't stop on broken connections: */
//    Signal(SIGPIPE, SIG_IGN);
//
//    while (1) {
//        clientlen = sizeof(clientaddr);
//        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
//        if (connfd >= 0) {
//            Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE,
//                        port, MAXLINE, 0);
//            printf("Accepted connection from (%s, %s)\n", hostname, port);
//            doit(connfd);
//            Close(connfd);
//        }
//    }
//}
//
///*
// * doit - handle one HTTP request/response transaction
// */
//void doit(int fd)
//{
//    char buf[MAXLINE], *method, *uri, *version;
//    rio_t rio;
//    dictionary_t *headers, *query;
//
//    /* Read request line and headers */
//    Rio_readinitb(&rio, fd);
//    if (Rio_readlineb(&rio, buf, MAXLINE) <= 0)
//        return;
//    printf("%s", buf);
//
//    if (!parse_request_line(buf, &method, &uri, &version)) {
//        clienterror(fd, method, "400", "Bad Request",
//                    "TinyChat did not recognize the request");
//    } else {
//        if (strcasecmp(version, "HTTP/1.0")
//            && strcasecmp(version, "HTTP/1.1")) {
//            clienterror(fd, version, "501", "Not Implemented",
//                        "TinyChat does not implement that version");
//        } else if (strcasecmp(method, "GET")
//                   && strcasecmp(method, "POST")) {
//            clienterror(fd, method, "501", "Not Implemented",
//                        "TinyChat does not implement that method");
//        } else {
//            headers = read_requesthdrs(&rio);
//
//            /* Parse all query arguments into a dictionary */
//            query = make_dictionary(COMPARE_CASE_SENS, free);
//            parse_uriquery(uri, query);
//            if (!strcasecmp(method, "POST"))
//                read_postquery(&rio, headers, query);
//
//            /* For debugging, print the dictionary */
//            print_stringdictionary(query);
//
//            /* The start code sends back a text-field form: */
//            serve_form(fd, "Welcome to TinyChat");
//
//            /* Clean up */
//            free_dictionary(query);
//            free_dictionary(headers);
//        }
//
//        /* Clean up status line */
//        free(method);
//        free(uri);
//        free(version);
//    }
//}
//
///*
// * read_requesthdrs - read HTTP request headers
// */
//dictionary_t *read_requesthdrs(rio_t *rp)
//{
//    char buf[MAXLINE];
//    dictionary_t *d = make_dictionary(COMPARE_CASE_INSENS, free);
//
//    Rio_readlineb(rp, buf, MAXLINE);
//    printf("%s", buf);
//    while(strcmp(buf, "\r\n")) {
//        Rio_readlineb(rp, buf, MAXLINE);
//        printf("%s", buf);
//        parse_header_line(buf, d);
//    }
//
//    return d;
//}
//
//void read_postquery(rio_t *rp, dictionary_t *headers, dictionary_t *dest)
//{
//    char *len_str, *type, *buffer;
//    int len;
//
//    len_str = dictionary_get(headers, "Content-Length");
//    len = (len_str ? atoi(len_str) : 0);
//
//    type = dictionary_get(headers, "Content-Type");
//
//    buffer = malloc(len+1);
//    Rio_readnb(rp, buffer, len);
//    buffer[len] = 0;
//
//    if (!strcasecmp(type, "application/x-www-form-urlencoded")) {
//        parse_query(buffer, dest);
//    }
//
//    free(buffer);
//}
//
//static char *ok_header(size_t len, const char *content_type) {
//    char *len_str, *header;
//
//    header = append_strings("HTTP/1.0 200 OK\r\n",
//                            "Server: TinyChat Web Server\r\n",
//                            "Connection: close\r\n",
//                            "Content-length: ", len_str = to_string(len), "\r\n",
//                            "Content-type: ", content_type, "\r\n\r\n",
//                            NULL);
//    free(len_str);
//
//    return header;
//}
//
///*
// * serve_form - sends a form to a client
// */
//void serve_form(int fd, const char *pre_content)
//{
//    size_t len;
//    char *body, *header;
//
//    body = append_strings("<html><body>\r\n",
//                          "<p>Welcome to TinyChat</p>",
//                          "\r\n<form action=\"reply\" method=\"post\"",
//                          " enctype=\"application/x-www-form-urlencoded\"",
//                          " accept-charset=\"UTF-8\">\r\n",
//                          "<input type=\"text\" name=\"content\">\r\n",
//                          "<input type=\"submit\" value=\"Send\">\r\n",
//                          "</form></body></html>\r\n",
//                          NULL);
//
//    len = strlen(body);
//
//    /* Send response headers to client */
//    header = ok_header(len, "text/html; charset=utf-8");
//    Rio_writen(fd, header, strlen(header));
//    printf("Response headers:\n");
//    printf("%s", header);
//
//    free(header);
//
//    /* Send response body to client */
//    Rio_writen(fd, body, len);
//
//    free(body);
//}
//
///*
// * clienterror - returns an error message to the client
// */
//void clienterror(int fd, char *cause, char *errnum,
//                 char *shortmsg, char *longmsg)
//{
//    size_t len;
//    char *header, *body, *len_str;
//
//    body = append_strings("<html><title>Tiny Error</title>",
//                          "<body bgcolor=""ffffff"">\r\n",
//                          errnum, " ", shortmsg,
//                          "<p>", longmsg, ": ", cause,
//                          "<hr><em>The Tiny Web server</em>\r\n",
//                          NULL);
//    len = strlen(body);
//
//    /* Print the HTTP response */
//    header = append_strings("HTTP/1.0 ", errnum, " ", shortmsg,
//                            "Content-type: text/html; charset=utf-8\r\n",
//                            "Content-length: ", len_str = to_string(len), "\r\n\r\n",
//                            NULL);
//    free(len_str);
//
//    Rio_writen(fd, header, strlen(header));
//    Rio_writen(fd, body, len);
//
//    free(header);
//    free(body);
//}
//
//static void print_stringdictionary(dictionary_t *d)
//{
//    int i, count;
//
//    count = dictionary_count(d);
//    for (i = 0; i < count; i++) {
//        printf("%s ================================== %s\n",
//               dictionary_key(d, i),
//               (const char *)dictionary_value(d, i));
//    }
//    printf("\n");
//}

