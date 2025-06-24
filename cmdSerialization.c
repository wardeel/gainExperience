enum
{
    SIG_EVENT_START,
    SIG_EVENT_CHECK,
    SIG_EVENT_STOP,
};

static int signal_control_fd[2];

static void send_signo_to_main(int signo) {
     if (write(signal_control_fd[0], &signo, sizeof(signo)) == -1) {};
}

static int s_link = -1;
static void link_change(int link) 
{
    if (s_link == link)
    {
        return;   
    }
    s_link = link;
    if (link) 
    {
        flag_4g_start_dhcp = 1;
    } 
    else 
    {
        flag_4g_start_dhcp = 0;
    }
}

/**
 * @brief 守护线程
 * 
 */
void fibocom_LTEguard_thread(void)
{
    int signo;

    int ret = -1;

    if (socketpair( AF_LOCAL, SOCK_STREAM, 0, signal_control_fd) < 0 ) 
    {
        printf("%s Faild to create main_control_fd: %d (%s)", __func__, errno, strerror(errno));
        return -1;
    }

    send_signo_to_main(SIG_EVENT_CHECK);
    while(1)
    {   
        struct pollfd pollfds[] = {{signal_control_fd[1], POLLIN, 0}};
        int ne, ret, nevents = sizeof(pollfds)/sizeof(pollfds[0]);

        do 
        {
            ret = poll(pollfds, nevents,  15*1000);
        } while ((ret < 0) && (errno == EINTR));

        if (ret == 0)
        {
            send_signo_to_main(SIG_EVENT_CHECK);
            continue;
        }

        if (ret <= 0) {
            printf("%s poll=%d, errno: %d (%s)", __func__, ret, errno, strerror(errno));
            goto __main_quit;
        }
        for (ne = 0; ne < nevents; ne++) 
        {
            int fd = pollfds[ne].fd;
            short revents = pollfds[ne].revents;

            if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
                printf("%s poll err/hup", __func__);
                printf("epoll fd = %d, events = 0x%04x", fd, revents);
                if (revents & POLLHUP)
                {
                    goto __main_quit;
                }
            }

            if ((revents & POLLIN) == 0)
            {
                continue;
            }

            if (fd == signal_control_fd[1])
            {
                if (read(fd, &signo, sizeof(signo)) == sizeof(signo))
                {
                    switch (signo)
                    {
                        case SIG_EVENT_START:
                        {
                            printf("SIG_EVENT_START\n");
                            break;
                        }

                        case SIG_EVENT_CHECK:
                        {
			    printf("SIG_EVENT_CHECK\n");
			    break;
                        }  

                        case SIG_EVENT_STOP:
                        {
		            printf("SIG_EVENT_CHECK\n");
                            break;
                        }     

                        default:
                        break;                                        
                    }
                }
            }
        }

        // fibocom_checkSimCard_api();
        // usleep(200*1000);
        // fibocom_checkSignal_api();
        // usleep(200*1000);
        // fibocom_checkCOSP_api();
        // usleep(200*1000);
        // fibocom_checkGPRSReg_api();
        // usleep(200*1000);
        // fibocom_checkEPSReg_api();
        // usleep(200*1000);
    }
__main_quit:
    link_change(0);
    close(signal_control_fd[0]);
    close(signal_control_fd[1]);
    printf("%s exit", __func__);

    return;
}

int main()
{
    int ret;
    do {
        ret = fibocom_LTEguard_thread();
        if (g_donot_exit_when_modem_hangup_fibocom > 0)
        {
            printf("restart fibocom mobile main\n");
            sleep(20);
        }
    } while (g_donot_exit_when_modem_hangup_fibocom > 0);

    return ret;
}
