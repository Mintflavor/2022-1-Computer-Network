#include <stdio.h>
#include <sys/types.h> // 여러 데이터 타입 정의 헤더파일
#include <sys/socket.h> // sockets 통신 관련 헤더파일
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>

void error(char *msg) // 오류메세지 출력
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int serverSocket, clientSocket; // 소켓 descriptor 선언
    socklen_t clientLength;
    char buff[1048576]; // 버퍼 선언
    struct sockaddr_in serverAddr, clientAddr; // 소켓 주소 구조체 선언

    if (argc < 2) // 서버 실행 시 인자 값 확인
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    serverSocket = socket(PF_INET, SOCK_STREAM, 0); // 서버 소켓 선언
    if (serverSocket < 0) // 서버 소켓 바인드 오류 등 소켓 선언 실패 시 오류 출력
        error("Socket open ERROR");

    memset(&serverAddr, 0, sizeof(serverAddr)); // 소켓 구조체 serverAddr을 0으로 채움
    serverAddr.sin_family = PF_INET; // sin_family PF_INET 으로 지정
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // 서버 주소 입력
    serverAddr.sin_port = htons(atoi(argv[1])); // 서버 실행 인자로 부터 포트 번호 입력

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) // 서버 소켓에 IP와 포트번호 바인드
        error("Socket bind ERROR");

    if (listen(serverSocket, 5) < 0) // client 대기
        error("Listening ERROR");

    while (1) // 연결 유지
    {
        int n, m; // 클라이언트 소켓에서 읽어온 바이트 수와 client가 요청한 파일에서 읽어온 바이트 수를 저장할 변수 선언
        clientLength = sizeof(clientAddr); // clientAddr 구조체 크기
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLength); // 서버 소켓과 클라이언트 소켓 연결

        if (clientSocket < 0) // 클라이언트 소켓 연결 오류 시 에러 출력
            error("Socket Accept ERROR");

        bzero(buff, 1048576); // 버퍼 메모리 0으로 초기화
        n = read(clientSocket, buff, 1048576); // 클라이언트 소켓 데이터 버퍼로 읽어오기
        if (n < 0)
            error("Socket reading ERROR");
        printf("%s\n", buff); // Request 콘솔에 출력

        char request[1048576] = {}; // Request 버퍼 선언
        char file[1048576]; // 파일 버퍼 선언
        int fileNameIndex = 5; // file name 시작 index

        while (buff[fileNameIndex] != ' ') // 파일 이름 공백이 나올 때까지
            fileNameIndex++; // 파일 이름 인덱스 +1
        strncpy(request, &buff[5], fileNameIndex - 5); // Request에서 파일 이름 버퍼로 복사

        char *response; // Response 변수 선언
        int fd = open(request, O_RDONLY); // 파일 discriptor 선언
        if (fd < 0) // 서버에 요청한 파일에 없으면 404 에러
        {
            if (write(clientSocket, "HTTP/1.1 404 NOT FOUND\r\nContent-Type: text/html\r\n\r\n", 52) < 0) // 클라이언트 소켓 헤더에 404 오류를 전달, 실패 시 오류 출력
                error("Socket write ERROR");
            else
                if (write(clientSocket, "<HTML><BODY><H1><pre></pre><pre></pre>404 NOT FOUND</H1></BODY></HTML>\n", 72) < 0) // 클라이언트 소켓 바디에 html 전달, 실패 시 오류 출력
                    error("Socket write ERROR");
        }
        else // 요청한 파일이 있으면
        {
            char *response;
            // Request에서 client가 원하는 파일 서브스트링으로 추출
            // Response 변수에 HTTP 응답 코드와 content-type 헤더 전달
            // 클라이언트 소켓에 response를 전달하고 실패 시, 오류 출력
            if (strstr(request, ".html")) // html
            {
                response = "HTTP/1.1 200 OK\r\nContent-Type: text/html;charset=UTF-8\r\n\r\n";
                if (write(clientSocket, response, strlen(response)) < 0)
                    error("Socket html writing ERROR");
            }
            else if (strstr(request, ".gif")) // gif
            {
                response = "HTTP/1.1 200 OK\r\nContent-Type: image/gif;\r\n\r\n";
                if (write(clientSocket, response, strlen(response)) < 0)
                    error("Socket gif writing ERROR");
            }
            else if (strstr(request, ".jpg") || strstr(request, ".jpeg")) // jpg or jpeg
            {
                response = "HTTP/1.1 200 OK\r\nContent-Type: image/jpeg;\r\n\r\n";
                if (write(clientSocket, response, strlen(response)) < 0)
                    error("Socket jpeg writing ERROR");
            }
            else if (strstr(request, ".mp3")) // mp3
            {
                response = "HTTP/1.1 200 OK\r\nContent-Type: audio/mp3;\r\n\r\n";
                if (write(clientSocket, response, strlen(response)) < 0)
                    error("Socket mp3 writing ERROR");
            }
            else if (strstr(request, ".pdf")) // pdf
            {
                response = "HTTP/1.1 200 OK\r\nContent-Type: application/pdf;\r\n\r\n";
                if (write(clientSocket, response, strlen(response)) < 0)
                    error("Socket pdf writing ERROR");
            }
            else // 나머지 파일 타입은 404 에러
            {
                if (write(clientSocket, "HTTP/1.1 404 NOT FOUND\r\nContent-Type: text/html\r\n\r\n", 52) < 0)
                    error("Socket write ERROR");
                else
                    if (write(clientSocket, "<HTML><BODY><H1><pre></pre><pre></pre>404 NOT FOUND</H1></BODY></HTML>\n", 72) < 0)
                        error("Socket write ERROR");
            }

            while (1)
            {
                m = read(fd, file, 1048575); // 파일 discriptor에서 file 버퍼로 읽어오기
                if (m == 0)
                    break;
                else if (m < 0)
                    error("File discriptor reading ERROR");
                else
                    if (write(clientSocket, file, m) < 0) // 소켓에 파일 전송, 실패 시 오류 출력
                        error("Socket write ERROR");
            }
            close(fd);
        }
        close(clientSocket);
    }
    close(serverSocket);

    return 0;
}