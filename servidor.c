/* Informacion acerca del codigo, como autor y fecha */
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/sendfile.h>
#include<netinet/in.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<string.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sqlite3.h>

//Contenido de la pagina Web

char paginaweb[]=
    "HTTP/1.1 200 Ok\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n\r\n"
    "<!DOCTYPE html>\r\n"
    "<html><head><title>Servidor Web </title>\r\n"
    "<style>body { background-color: #A9D0F5 }</style></head>\r\n"
    "<body><center><h1>Formulario de Prueba</h1><br>\r\n"
    "<form action=\"/guardar\">\r\n"
    "Valor:<br>\r\n"
    "<input type=\"text\" name=\"usuario\" value=\"Valor\"><br>\r\n"
    "<div class=\"button\"><button type=\"Enviar\">Enviar</button></div>\r\n"
    "<input type=\"submit\" value=\"Submit\">\r\n"
    "</form>\r\n"
    "</center></body></html>\r\n";

char pag_respuesta[]=
    "HTTP/1.1 200 Ok\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n\r\n"
    "<!DOCTYPE html>\r\n"
    "<html><head><title>Servidor Web </title>\r\n"
    "<style>body { background-color: #A9D0F5 }</style></head>\r\n"
    "<body><center><h1>Enviado</h1><br>\r\n"
    "<a href=\"/\">Regresar</a>\r\n"
    "<a href=\"/revisar\">Revisar Base</a>"
    "</center></body>\r\n";

//Funcion principal
// int callback(void *, int, char **, char **);
int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    NotUsed=0;
    for(int i=0; i< argc; i++) {
	printf("Entrando a callback\n");
	printf("%s=%s\n",azColName[i],argv[i]);
    }
    printf("\n");
    return 0;
}

int main()
{
    struct sockaddr_in server_addr, client_addr;
    socklen_t sin_len=sizeof(client_addr);
    int fd_server, fd_client,rc;
    char buf[2048];
    char buf2[2048];
    int on=1, pdbg=0;
    int bind_result,max_con,idx;
    // FILE *fp; // No usada
    sqlite3 *db;
    sqlite3_stmt *stmt;
    char *sql, *err_msg,  *getenv();
    // char *data, *infobase; // No usada
    char *method,*uri, *valor;
	
#ifdef debug
    pdbg=1;
    printf("Definido Modo Debug\n");
#endif
    
    rc=sqlite3_open("base_nombres.db",&db);
    if(rc) {
	fprintf(stderr,"No puede abrir base de Datos: %s\n", sqlite3_errmsg(db));
	sqlite3_close(db);
	return(1);
    }
    else {
	printf("Base creada\n");
    }

    sql="DROP TABLE IF EXISTS Usuarios;"
	"CREATE TABLE Usuarios(Name TEXT);";
    rc=sqlite3_exec(db,sql,0,0, &err_msg);

    if(rc != SQLITE_OK) {
	fprintf(stderr,"SQL error: %s\n",err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(db);
	return 1;
    } else {
	printf("Tabla Creada\n");
    }

    fd_server=socket(AF_INET,SOCK_STREAM,0);//llamada al socket
    if(fd_server<0){
	setsockopt(fd_server,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(int));
	perror("socket");
	exit(1);
    }else{
	printf("Socket Montado\n");
    }
    //Familia de la direccion
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);
    //Conexiones del socket
    bind_result=bind(fd_server,(struct sockaddr *)&server_addr,sizeof(server_addr));

    if(bind_result==-1){//Error al momento de conectar
	perror("bind");
	close(fd_server);//Se cierra el socket
	exit(1);
    }
    max_con=listen(fd_server,10);//Maxima cantidad de conexiones
    if(max_con==-1){
	perror("listen");
	close(fd_server);
	exit(1);
    }
    //Bucle para gestionar las conexiones
    while(1){
	//Aceptar nuevas conexiones
	fd_client=accept(fd_server,(struct sockaddr *)&client_addr,&sin_len);
	if(fd_client == 1){
	    perror("Conexion Fallida...\n");
	    continue;
	}
	printf("Conexion establecida\n");
	if(!fork()){
	    memset(buf,0,2048);//limpieza de buffer
	    read(fd_client,buf,2047);//cargar contenido del cliente
	    if(pdbg==1){printf("Contenido bufer: %s\n",buf);}
	    strcpy(buf2,buf);
	    method=strtok(buf2," \t\r\n");
	    if(pdbg==1){printf("metodo: %s\n",method);}
	    uri=strtok(NULL," \t");
	    if(pdbg==1){printf("Uri: %s\n",uri);}	
	    if(!strcmp(method,"GET") && !strcmp(uri,"/")){
		write(fd_client,paginaweb,sizeof(paginaweb)-1);
	    }
	    else if(!strncmp(buf, "GET /icono.png",14)){
		write(fd_client,paginaweb,sizeof(paginaweb)-1);
	    }else if(!strcmp(method,"GET") && !strncmp(uri,"/guardar",8)){
		valor=strtok(uri,"=");
		if(valor!=NULL){
		    valor=strtok(NULL,"=");
		}
		printf("%s\n",valor);
		rc=sqlite3_prepare_v2(db,"INSERT INTO Usuarios (Name) VALUES (?)",-1,&stmt,NULL);
		printf("%i\n",rc);
		if(rc != SQLITE_OK){
		    printf("Orden Failed");
		    write(fd_client,paginaweb,sizeof(paginaweb)-1);
		    return 1;
		}else{
		    printf("SQL statement prepared: OK\n\n\r");
		}
		idx=strlen(valor);
		printf("%i\n",idx);
		rc=sqlite3_bind_text(stmt,1,valor,idx,0);
		if(rc!=SQLITE_OK){
		    printf("Failed\n");
		    return 1;
		}else{
		    printf("done\n");
		}
		rc=sqlite3_step(stmt);
		if(rc!= SQLITE_DONE){
		    printf("Failed to execute statement: %s\n\r", sqlite3_errstr(rc));
		    sqlite3_close(db);
		    return 1;
		}
		else{
		    write(fd_client,pag_respuesta,sizeof(pag_respuesta)-1);
		}
		sqlite3_finalize(stmt);
	    }else if(!strcmp(method,"GET") && !strncmp(uri,"/revisar",8)){
		rc=sqlite3_exec(db,"SELECT * FROM Usuarios",callback,0,&err_msg);
		if(rc!=SQLITE_OK){
		    fprintf(stderr, "Failed to select data\n");
		    fprintf(stderr, "SQL error: %s\n",err_msg);
		    sqlite3_free(err_msg);
		    sqlite3_close(db);
		    return 1;
		} else {
		    write(fd_client,paginaweb,sizeof(paginaweb)-1);
		}
	    }
	    else{
		printf("Entra en 3\n");
		write(fd_client,paginaweb,sizeof(paginaweb)-1);
	    }
	    close(fd_client);
	    printf("Cerrando...\n");
	}
	close(fd_client);
    }
    return 0;
}
