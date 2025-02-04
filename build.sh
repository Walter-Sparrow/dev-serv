clang++ -c -o file_utils.o file_utils.cpp
clang++ -c -o sse.o sse.cpp
clang++ -c -o server.o server.cpp
clang++ -c -o main.o main.cpp

clang++ -o dev-serv main.o server.o sse.o file_utils.o -framework CoreServices

rm *.o
