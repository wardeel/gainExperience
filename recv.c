void* protocol_uart_recv_thread(void *argv)
{
	int offset = 0;
    static int fd = 0;
    static int ret,i= 10;
    fd_set rset;	
    char buf[PROTOCOL_BUF_SIZE];
	struct timeval start,over;
	int cur_rIdx = 0;

	memset(buf,0,PROTOCOL_BUF_SIZE);

	fd = get_protocol_fd(0);
    set_opt(fd, UART_BITRATES, 8, 'N', 1); //设置串口参数
    FD_ZERO(&rset);

    while (1)
    {
    	FD_SET(fd, &rset);
        ret = select(fd+1, &rset, NULL, NULL, NULL);//返回可用资源的个数
//        printf("%s : ret = %d\n",(char *)arg,ret);
        if (ret <= 0) 
		{
            if (ret == 0) 
            {
            	PROTOCOL_MCU_DBG(RT_ERROR, "ret == 0\n");
            }
            if (errno == EINTR) 
            {
            	PROTOCOL_MCU_DBG(RT_ERROR, "Thread_doorLock Select fail!\n");
				continue;
            }
        } 
        else if(FD_ISSET(fd,&rset)) //判定描述符是否在集合中，在返回非零
        {
        	ret = read(fd, buf, sizeof(buf));
			if(ret < 0)
			{
				PROTOCOL_MCU_DBG(RT_ERROR, "Thread_doorLock Rcv data fail!\n");
				continue;
			}
			else
			{
				
				#if 0
				printf("[recvdata]\n");
				for(i = 0;i<ret;i++)
				{
					printf("[buf[%d]=0x%x]-",i,buf[i]);
				}
				printf("\n");
				#endif
//				gettimeofday(&start,NULL);
//				pthread_mutex_lock(&g_doorlock_data_recv.data_access_msem);
				cur_rIdx = g_doorlock_data_recv.rIdx;				
//				printf("[start]tv_sec:%d tv_usec:%d wIdx:%d cur_rIdx:%d \n",start.tv_sec,start.tv_usec,g_doorlock_data_recv.wIdx,cur_rIdx);
				//printf("[start] wIdx:%d cur_rIdx:%d \n",g_doorlock_data_recv.wIdx,cur_rIdx);
				if(g_doorlock_data_recv.wIdx != cur_rIdx)
				{
					if(g_doorlock_data_recv.wIdx > cur_rIdx)
					{
						if(PROTOCOL_BUF_CAP - g_doorlock_data_recv.wIdx + cur_rIdx > ret)		//不能等于，写数据使得读写索引相等会导致数据来不及读而覆盖的问题
						{
							if(PROTOCOL_BUF_CAP - g_doorlock_data_recv.wIdx >= ret)
							{
								memcpy(g_doorlock_data_recv.buf + g_doorlock_data_recv.wIdx, buf, ret);
								g_doorlock_data_recv.wIdx += ret;
								g_doorlock_data_recv.wIdx %= PROTOCOL_BUF_CAP;
							}
							else
							{
								memcpy(g_doorlock_data_recv.buf + g_doorlock_data_recv.wIdx, buf, PROTOCOL_BUF_CAP - g_doorlock_data_recv.wIdx);
								memcpy(g_doorlock_data_recv.buf, buf + (PROTOCOL_BUF_CAP - g_doorlock_data_recv.wIdx),ret - (PROTOCOL_BUF_CAP - g_doorlock_data_recv.wIdx));
								g_doorlock_data_recv.wIdx = ret - (PROTOCOL_BUF_CAP - g_doorlock_data_recv.wIdx);
							}
						}
						else
						{
							PROTOCOL_MCU_DBG(RT_ERROR, "[1]Recv data buf overflow, rIdx:%d wIdx:%d len:%d \n",cur_rIdx,g_doorlock_data_recv.wIdx,ret);
						}
					}
					else
					{
						if(cur_rIdx - g_doorlock_data_recv.wIdx > ret)							//不能等于，写数据使得读写索引相等会导致数据来不及读而覆盖的问题
						{
							memcpy(g_doorlock_data_recv.buf + g_doorlock_data_recv.wIdx, buf, ret);
							g_doorlock_data_recv.wIdx = (g_doorlock_data_recv.wIdx + ret)%PROTOCOL_BUF_CAP;
						}
						else
						{
							PROTOCOL_MCU_DBG(RT_ERROR, "[2]Recv data buf overflow, rIdx:%d wIdx:%d len:%d \n",cur_rIdx,g_doorlock_data_recv.wIdx,ret);
						}
					}
				}
				else
				{
					if(PROTOCOL_BUF_CAP - g_doorlock_data_recv.wIdx >= ret)
					{
						memcpy(g_doorlock_data_recv.buf + g_doorlock_data_recv.wIdx,buf,ret);
					}
					else
					{
						memcpy(g_doorlock_data_recv.buf + g_doorlock_data_recv.wIdx,buf,PROTOCOL_BUF_CAP - g_doorlock_data_recv.wIdx);
						memcpy(g_doorlock_data_recv.buf,buf + (PROTOCOL_BUF_CAP - g_doorlock_data_recv.wIdx),ret - (PROTOCOL_BUF_CAP - g_doorlock_data_recv.wIdx));
					}
					g_doorlock_data_recv.wIdx = (g_doorlock_data_recv.wIdx + ret)%PROTOCOL_BUF_CAP;
				}
//				pthread_mutex_unlock(&g_doorlock_data_recv.data_access_msem);
				memset(buf, 0, sizeof(buf));
//				gettimeofday(&over,NULL);
//				printf("[over]tv_sec:%d tv_usec:%d interval:%d wIdx:%d cur_rIdx:%d \n",over.tv_sec,over.tv_usec,(over.tv_sec-start.tv_sec)*1000*1000+over.tv_usec-start.tv_usec,g_doorlock_data_recv.wIdx,cur_rIdx);
				//printf("[over]wIdx:%d cur_rIdx:%d \n",g_doorlock_data_recv.wIdx,cur_rIdx);
			}			
        }
		
    }

    return NULL;
}
