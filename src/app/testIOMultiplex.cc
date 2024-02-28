#include <iostream>
#include <sys/select.h>
#include <sys/poll.h>
#include <unistd.h>
#include <sys/epoll.h>

using namespace std;

void testpoll();
void testselect();
void testepoll();

int main() {
    testepoll();
    // testpoll();
    // testselect();
    return 0;
}

void testpoll() {
    int ret;
    pollfd* fds = new pollfd[2];
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[1].fd = 1;
    fds[1].events = POLLOUT;


    while(1) {

        // ret = select(2, &rfds, &wfds, nullptr, &tv);
        ret = poll(fds, 2, 2000);
        if(ret == -1) {
            perror("select()");
        } else if(fds[0].revents & POLLIN) {
            printf("data is availiable\n");
        } else if(fds[1].revents & POLLOUT) {
            printf("data is writable\n");
        }else{
            printf("timeout\n");
        }
        sleep(1);
    }
}

void testselect() {
    int ret;
    fd_set rfds, wfds;
    timeval tv;

    tv.tv_sec = 5;
    tv.tv_usec = 0;

    while(1) {
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        FD_SET(1, &wfds);

        ret = select(2, &rfds, &wfds, nullptr, &tv);
        if(ret == -1) {
            perror("select()");
        } else if(FD_ISSET(0, &rfds)) {
            printf("data is availiable\n");
        } else if(FD_ISSET(1, &wfds)) {
            printf("data is writable\n");
        }else{
            printf("timeout\n");
        }
        sleep(5);
    }
}

void testepoll() {
    int ret;
    int epollfd = epoll_create(1);
    epoll_event* events = new epoll_event[2];
    events[0].data.fd = 0;
    events[0].events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, 0, &events[0]);
    events[1].data.fd = 1;
    events[1].events = EPOLLOUT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, 1, &events[1]);

    while(1) {

        // ret = select(2, &rfds, &wfds, nullptr, &tv);
        // ret = poll(fds, 2, 2000);
        ret = epoll_wait(epollfd, events, 2, -1);
        if(ret == -1 && errno != EINTR) {
            perror("epoll_wait()");
        } 
        for(int i = 0; i < ret; i++) {
            if(events[i].events & EPOLLIN) {
                printf("data is avialable\n");
            } else if(events[i].events & EPOLLOUT) {
                printf("data is writable\n");
            }
        }
        sleep(2);
    }
}