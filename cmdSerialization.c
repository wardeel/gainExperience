static void fibocom_udhcpc_start(void)
{
    ifconfig_up_and_down("usb0", "up");
    robot_internalData_cfg internalDataCfg;
    config_field_read(CONFIG_TYPE_ROBOT_INTERNAL_DATA, &internalDataCfg);
    init_net_if_4g(&internalDataCfg.mobile4g.net, &internalDataCfg.mobile4g.mac, FALSE, "usb0");
	config_field_write(CONFIG_TYPE_ROBOT_INTERNAL_DATA, &internalDataCfg);

}

static void fibocom_udhcpc_stop(void)
{
    stop_dhcp_task("usb0");
    printf("lxy>>%s,%d\n",__FUNCTION__,__LINE__);
    ifconfig_up_and_down("usb0", "down");
}
static int signal_control_fd[2];

static void send_signo_to_main(int signo) {
     if (write(signal_control_fd[0], &signo, sizeof(signo)) == -1) {};
}

static int s_link = -1;
static void fibocom_usbnet_link_change(int link) 
{
    if (s_link == link)
        return;

        
    s_link = link;
    if (link) 
    {
        flag_4g_start_dhcp = 1;
        fibocom_udhcpc_start();
    } else 
    {
        flag_4g_start_dhcp = 0;
        fibocom_udhcpc_stop();
    }
}

/**
 * @brief 4g守护线程
 * 
 */
void fibocom_LTEguard_thread(void)
{
    int signo;

    while (fibocom_get_fd() < 0)
    {
        usleep(50* 1000);
    }
    int ret = -1;

    if (socketpair( AF_LOCAL, SOCK_STREAM, 0, signal_control_fd) < 0 ) 
    {
        LTE_LOG("%s Faild to create main_control_fd: %d (%s)", __func__, errno, strerror(errno));
        return -1;
    }

    
    fibocom_closeRspDisplay_api();

    ret = fibocom_checkUsbmode_api();
    if(ret == 1)      //查询结果不是74
    {
        fibocom_setUsbmode_api();//设置成74后重启
    }
    /*获取LTE相关信息*/
    fibocom_getIMEI_api();
    fibocom_getICCID_api();
    /*获取LTE相关信息*/

    /*设置LTE必要信息*/
    fibocom_setPDPParam_api();
    fibocom_setRNDIS_api();
    /*设置LTE必要信息*/

    fibocom_usbnet_link_change(0);

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
            LTE_LOG("%s poll=%d, errno: %d (%s)", __func__, ret, errno, strerror(errno));
            goto __main_quit;
        }
        for (ne = 0; ne < nevents; ne++) 
        {
            int fd = pollfds[ne].fd;
            short revents = pollfds[ne].revents;

            if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
                LTE_LOG("%s poll err/hup", __func__);
                LTE_LOG("epoll fd = %d, events = 0x%04x", fd, revents);
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
                            
                            break;
                        }

                        case SIG_EVENT_CHECK:
                        {
                            fibocom_checkSignal_api();
                            usleep(200*1000);
                            fibocom_checkCOSP_api();
                            usleep(200*1000);
                            fibocom_checkGPRSReg_api();
                            usleep(200*1000);
                            fibocom_checkEPSReg_api();
                            usleep(200*1000);
                            fibocom_usbnet_link_change(1);
                            break;
                        }  

                        case SIG_EVENT_STOP:
                        {
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
    fibocom_usbnet_link_change(0);
    close(signal_control_fd[0]);
    close(signal_control_fd[1]);
    LTE_LOG("%s exit", __func__);

    return;
}
