/** 
 *  \brief		接收数据线程
 *  \param[in]  无
 *  \return     无
 */
void robot_bluetooth_recv_thread(void)
{
    int ret = 0;
    int fd = -1;
    fd_set rset;
    
    int i = 0;

    // volatile unsigned char rxbuf[MAX_RECV_LEN] = {0};          //读取数据长度到32后开始处理的buffer，一包
    // volatile unsigned char processed_buf[MAX_RECV_LEN] = {0};  //经过简单处理的buffer
    volatile unsigned char fd_read_buf[MAX_RECV_LEN] = {0};       //从fd中读取的buffer
    unsigned char len = 0;

    fd = robot_bluetooth_get_fd();
    if(fd < 0)
    {
        printf("[bluetooth]robot_bluetooth_recv_thread failed\n");
        return;
    }

    FD_ZERO(&rset);
    FD_SET(fd, &rset);


    int cur_rIdx = 0;
    while(1)
    {
        // memset(fd_read_buf,0,MAX_RECV_LEN);
        ret = select(fd+1, &rset, NULL, NULL, NULL);
        if(ret > 0)
        {
            if(FD_ISSET(fd,&rset)) 
            {
                ret = read(fd, (char *)fd_read_buf , MAX_RECV_LEN);
                if (ret < 0)
                {
                    printf("[bluetooth]read is failed\r\n");
                    continue;
                }
                else
                {
                    cur_rIdx = read_idx;
                    if(write_idx != cur_rIdx)
                    {
                        if(write_idx > cur_rIdx)
                        {
                            if(MAX_RECV_CAP - write_idx + cur_rIdx > ret)
                            {
                                if(MAX_RECV_CAP - write_idx >= ret)
                                {
                                    memcpy(recv_buf + write_idx, fd_read_buf, ret);
                                    write_idx += ret;
                                    write_idx %= MAX_RECV_CAP;
                                }
                                else
                                {
                                    memcpy(recv_buf + write_idx, fd_read_buf, MAX_RECV_CAP - write_idx);
								    memcpy(recv_buf, fd_read_buf + (MAX_RECV_CAP - write_idx),ret - (MAX_RECV_CAP - write_idx));
								    write_idx = ret - (MAX_RECV_CAP - write_idx);
                                }
                            }
                            else
                            {
                                printf("[bluetooth]overflow[1], rIdx:%d wIdx:%d len:%d \n",cur_rIdx,write_idx,ret);
                            }
                        }
                        else
                        {
                            if(cur_rIdx - write_idx > ret)							//不能等于，写数据使得读写索引相等会导致数据来不及读而覆盖的问题
						    {
						    	memcpy(recv_buf + write_idx, fd_read_buf, ret);
						    	write_idx = (write_idx + ret)%MAX_RECV_CAP;
						    }
						    else
						    {
						    	printf("[bluetooth]overflow[2], rIdx:%d wIdx:%d len:%d \n",cur_rIdx,write_idx,ret);
						    }
                        }
                    }
                    else
                    {
                        if(MAX_RECV_CAP - write_idx >= ret)
					    {
					    	memcpy(recv_buf + write_idx,fd_read_buf,ret);
					    }
					    else
					    {
					    	memcpy(recv_buf + write_idx,fd_read_buf,MAX_RECV_CAP - write_idx);
					    	memcpy(recv_buf,fd_read_buf + (MAX_RECV_CAP - write_idx),ret - (MAX_RECV_CAP - write_idx));
					    }
					    write_idx = (write_idx + ret)%MAX_RECV_CAP;
                    }
                    memset(fd_read_buf, 0, sizeof(fd_read_buf));
                }
            } 
        }
    }
}

/** 
 *  \brief		接收数据线程初始化
 *  \param[in]  无
 *  \return     无
 */
int robot_bluetooth_recv_init(void)
{
	int ret = 0;
	pthread_t robot_bluetooth_pthread_t;

    memset(&reply_msg, 0, sizeof(robot_bluetooth_recv_reply_msg));

    read_idx = 0;
    write_idx = 0; 
    memset(recv_buf, 0, sizeof(recv_buf));

	
	ret = create_thread(&robot_bluetooth_pthread_t, PRIO_LOWEST, DEFAULT_STACK_SIZE,robot_bluetooth_recv_thread,NULL);
	if (ret < 0) 
	{
		printf("[bluetooth]-->robot_bluetooth_recv_init failed\n");
		return ret;
	}
	return ret;
}


void robot_bluetooth_parse_thread(void)
{
    printf("lxy>>here\n");
    int cur_wIdx = 0;   
    int packet_begin_pos = -1;
    int packet_end_pos = -1;
    int buf_store_len = 0;
    int packet_len = 0;
    int readindex = 0;
    int i = 0;
	int j = 0;
    int no_overflow_flag = 0;
    int len_check = 0;
    int len1_check = 0;
    unsigned int len;
    unsigned char buf[MAX_RECV_LEN];
    while(1)
    {
        cur_wIdx = write_idx;
        if(cur_wIdx != write_idx)
        {
            packet_begin_pos = -1;
			packet_end_pos = -1;
            readindex = read_idx;
            if(cur_wIdx > read_idx)
            {
                if((buf_store_len = cur_wIdx - read_idx) >= 8)
                {
                    printf("[bluetooth][1]rIdx:%d wIdx:%d \n",read_idx,cur_wIdx);
                    for(i=read_idx;i!=cur_wIdx;)
					{
						printf("[%d:0x%x]",i,recv_buf[i]);
						//printf("{%d}",i);
						i=(i+1)%MAX_RECV_CAP;
						//printf("{%d-}",i);
					}
					printf("\n");
                    for(readindex = read_idx; readindex <= cur_wIdx - 2; readindex++)
					{
						if((0xfc == recv_buf[readindex]) && (0x01 == recv_buf[readindex + 1]))
						{
							packet_begin_pos = readindex;
							break;
						}
					}
                    if(packet_begin_pos != -1)
                    {
                        packet_len = recv_buf[packet_begin_pos + 2];
                        printf("[bluetooth][1]length:%d %d %d \n",packet_len,recv_buf[packet_begin_pos+1],buf_store_len);
                        if(packet_begin_pos + packet_len - 1 < cur_wIdx)      //缓存一包数据
                        {
                            if((packet_begin_pos + packet_len - 1 < cur_wIdx) && (0xfe == recv_buf[packet_begin_pos + packet_len - 1]))
                            {
                                packet_end_pos = packet_begin_pos + packet_len - 1;
                            }
                            else
                            {
                                for(readindex = packet_begin_pos; readindex < cur_wIdx; readindex++)
						    	{
						    		if(0xfe == recv_buf[readindex])
						    		{

						    			packet_end_pos = readindex;
						    			break;
						    		}
						    	}
                                if(-1 == packet_end_pos)
                                {
                                    printf("[bluetooth][1]Data in buffer is too less, rIdx:%d wIdx:%d \n",read_idx,cur_wIdx);	
                                    usleep(10*1000);
						    		continue;
                                }
                                packet_begin_pos = -1;
                                for(readindex = packet_end_pos - 2; readindex >= read_idx; readindex--)
						    	{
						    		if((0xfc == recv_buf[readindex]) && (0x01 == recv_buf[readindex + 1]))
						    		{
						    			packet_begin_pos = readindex;
						    			break;
						    		}
						    	}
                                if(-1 == packet_begin_pos)
                                {
                                    read_idx = (packet_end_pos + 1) % MAX_RECV_CAP;
                                    printf("[bluetooth][1]analysis wrong ridx:%d, widx:%d\n", read_idx, cur_wIdx);
                                    usleep(10*1000);
						    		continue;
                                }
                            }
                        }
                        else
                        {
                            printf("[bluetooth][2]Data in buffer is too less, rIdx:%d wIdx:%d \n",read_idx,cur_wIdx);		
                            usleep(10*1000);
							continue;
                        }
                        
                    }
                    else
                    {
                        read_idx = readindex;
                        printf("[bluetooth][1]recv data wrong ridx:%d, widx:%d\n", read_idx, cur_wIdx);
                        usleep(10*1000);
						continue;
                    }
                }
                else
                {
                    usleep(10*1000);
					continue;
                }
            }
            else
            {
                printf("[bluetooth][2]rIdx:%d wIdx:%d \n",read_idx, cur_wIdx);
                for(i=read_idx;i!=cur_wIdx;)
                {
                    printf("[%d:0x%x]",i,recv_buf[i]);
                    //printf("{%d}",i);
                    i=(i+1)%MAX_RECV_CAP;
                    //printf("{%d-}",i);
                }
                printf("\n");
                if((buf_store_len = MAX_RECV_CAP - read_idx + cur_wIdx) >= 8)
                {
                    for(i = 0; i < buf_store_len - 1; i++)
                    {
                        readindex = (read_idx + i) % MAX_RECV_CAP;
                        if((recv_buf[readindex] == 0xfc) && (recv_buf[readindex + 1] == 0x01))
                        {
                            packet_begin_pos = readindex;
							break;
                        }
                    }
                    if(-1 != packet_begin_pos)
                    {
                        packet_len = recv_buf[(packet_begin_pos + 2) % MAX_RECV_CAP];
                        no_overflow_flag =((packet_begin_pos + packet_len - 1 >= MAX_RECV_CAP) && ((packet_begin_pos + packet_len - 1) % MAX_RECV_CAP < cur_wIdx))|| (packet_begin_pos + packet_len - 1 < MAX_RECV_CAP);
                        printf("[bluetooth][2]length:%d %d %d\n",packet_len, recv_buf[(packet_begin_pos + 2) % MAX_RECV_CAP], no_overflow_flag);
                        if(no_overflow_flag || (buf_store_len >= 200))
                        {
                            if(no_overflow_flag && (0xfe == recv_buf[(packet_begin_pos + packet_len - 1) % MAX_RECV_CAP]))
                            {
                                packet_end_pos = (packet_begin_pos + packet_len - 1) % MAX_RECV_CAP;
                            }
                            else
                            {
                                if(packet_begin_pos > cur_wIdx)
                                {
                                    len_check = MAX_RECV_CAP - packet_begin_pos + cur_wIdx;
                                }
                                else
                                {
                                    len_check = cur_wIdx - packet_begin_pos;
                                }
                                for(i = 0; i < len_check; i++)
                                {
                                    if(0xfe == recv_buf[(packet_begin_pos + i) % MAX_RECV_CAP])
                                    {
                                        if((packet_begin_pos + i) % MAX_RECV_CAP > cur_wIdx)
										{
											len1_check = MAX_RECV_CAP - (packet_begin_pos + i) % MAX_RECV_CAP - 1 + cur_wIdx;
										}
										else
										{
											len1_check = cur_wIdx - (packet_begin_pos + i) % MAX_RECV_CAP - 1;
										}
                                        j = (packet_begin_pos + i) % MAX_RECV_CAP;
                                        packet_end_pos = j;
										break;
                                    }
                                }
                                if(-1 == packet_end_pos)
                                {
                                    printf("[bluetooth][4]Data in buffer is too less, rIdx:%d wIdx:%d \n",read_idx,cur_wIdx); 
                                    usleep(10*1000);
									continue;
                                }
                                if(read_idx > packet_end_pos)
								{
									len_check = MAX_RECV_CAP - read_idx + packet_end_pos + 1;
								}
								else
								{
									len_check = packet_end_pos - read_idx + 1;
								}
								packet_begin_pos = -1;
                                for(i = len_check-2;i >= 0;i--)
								{
									if((0xfc == recv_buf[(read_idx + i)%MAX_RECV_CAP]) && (0x01 == recv_buf[(read_idx + i + 1)%MAX_RECV_CAP]))
									{
										packet_begin_pos = (read_idx + i)%MAX_RECV_CAP;
										break;
									}
								}
                                if(-1 == packet_begin_pos)
								{
									read_idx = (packet_end_pos + 1) % MAX_RECV_CAP;
									printf("[bluetooth][3]Analysis data wrong, rIdx:%d wIdx:%d \n",read_idx,cur_wIdx);				
									usleep(10*1000);
									continue;
								}
                            }
                        }
                        else
                        {
                            usleep(10*1000);
							continue;
                        }
                    }
                    else
                    {
                        read_idx = (read_idx + i) % MAX_RECV_CAP;
                        printf("[bluetooth][2]recv data wrong ridx:%d, widx:%d\n", read_idx, cur_wIdx);
                        usleep(10*1000);
						continue;
                    }

                }
                else
                {
                    usleep(10*1000);
					continue;
                }
            }
        }
        else
        {
            usleep(10*1000);
			continue;
        }
        if((-1 != packet_begin_pos) && (-1 != packet_end_pos) && (packet_begin_pos != packet_end_pos))
        {
            if(packet_begin_pos < packet_end_pos)
			{
				len = packet_end_pos - packet_begin_pos + 1;
				if(len <= MAX_RECV_CAP)
				{
					memcpy(buf,recv_buf+packet_begin_pos,len);
					read_idx = (packet_end_pos + 1)%MAX_RECV_CAP;
				}
				else
				{	
					read_idx = (packet_end_pos + 1)%MAX_RECV_CAP;
					printf("[bluetooth][1]one packet data is more than 256 bytes:%d \n",len);	
					usleep(10*1000);
					continue;
				}
			}
			else
			{
				len = MAX_RECV_CAP - packet_begin_pos + packet_end_pos + 1;
				if(len <= MAX_RECV_CAP)
				{
					memcpy(buf,recv_buf+packet_begin_pos,MAX_RECV_CAP - packet_begin_pos);
					memcpy(buf + MAX_RECV_CAP - packet_begin_pos,recv_buf,packet_end_pos + 1);
					read_idx = (packet_end_pos + 1)%MAX_RECV_CAP;
				}
				else
				{
					read_idx = (packet_end_pos + 1)%MAX_RECV_CAP;
					printf("[bluetooth][2]one packet data is more than 256 bytes:%d \n",len);	
					usleep(10*1000);
					continue;
				}
			}			
        }
        else
        {
            printf("[bluetooth]Analysis data mechanism error,packet_begin_pos:%d packet_end_pos:%d \n",packet_begin_pos,packet_end_pos);
            usleep(10*1000);
			continue;
        }

        robot_bluetooth_message_parse(buf, len);
    }
}
