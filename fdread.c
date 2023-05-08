void robot_bluetooth_recv_thread(void)
{
    int ret = 0;
    int fd = -1;
    fd_set rset;
    int len = 0;

    char buf[MIN_RECV_BUFF_LEN] = {0};
    volatile unsigned char recv_buf[MIN_RECV_BUFF_LEN] = {0};

    fd = robot_bluetooth_get_fd();
    if(fd < 0)
    {
        printf("robot_bluetooth_recv_thread failed\n");
        return;
    }

    FD_ZERO(&rset);
    FD_SET(fd, &rset);

    while(1)
    {
        memset(buf,0,MIN_RECV_BUFF_LEN);
        ret = select(fd+1, &rset, NULL, NULL, NULL);
        if(ret > 0)
        {
            if(FD_ISSET(fd,&rset)) 
            {
                ret = read(fd, (char *)buf , MIN_RECV_BUFF_LEN);
                if (ret < 0)
                {
                    printf("read is failed\r\n");
                    continue;
                }
                else
                {
                    memcpy(recv_buf + len, buf, ret);
                    len += ret;
                    memset(buf, 0, MIN_RECV_BUFF_LEN);
                    if(len >= MIN_RECV_BUFF_LEN)
                    {
                        len = 0;
                        robot_bluetooth_message_parse(recv_buf);
                        memset(recv_buf, 0, MIN_RECV_BUFF_LEN);
                    }
                }
            } 
        }
    }
}
