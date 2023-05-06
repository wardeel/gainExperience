void protocol_uart_recv_msg(void)
{
	int ret = ERROR;
	int cmd;
	protocol_ctl_t *protocol_Ctl;
	protocol_ctl_t protocol_ctl_recv;
	protocol_Ctl = &protocol_ctl_recv;
	int Flag_uart = SEND_TO_DOOR_LOCK;
	unsigned int len;
	unsigned char buf[PROTOCOL_BUF_SIZE];
	memset(buf,0,PROTOCOL_BUF_SIZE);
	int i = 0;
	int j = 0;
	int read_idx = 0;
	int packet_begin_pos = -1;
	int packet_end_pos = -1;
	int packet_len = 0;
	int cur_wIdx = 0;
	int len_check = 0;
	int len1_check = 0;
	int buf_store_len = 0;
	int no_overflow_flag = 0;
	FOREVER
	{
		cur_wIdx = g_doorlock_data_recv.wIdx;
		if(cur_wIdx != g_doorlock_data_recv.rIdx)
		{
			packet_begin_pos = -1;
			packet_end_pos = -1;
			read_idx = g_doorlock_data_recv.rIdx;
			if(cur_wIdx > g_doorlock_data_recv.rIdx)
			{
				if((buf_store_len = cur_wIdx - g_doorlock_data_recv.rIdx) >= 10)		//一包数据最少字节数
				{
					//printf("[1]rIdx:%d wIdx:%d \n",g_doorlock_data_recv.rIdx,cur_wIdx);
					#if 0
					for(i=g_doorlock_data_recv.rIdx;i!=cur_wIdx;)
					{
						printf("[%d:0x%x]",i,g_doorlock_data_recv.buf[i]);
						//printf("{%d}",i);
						i=(i+1)%PROTOCOL_BUF_CAP;
						//printf("{%d-}",i);
					}
					printf("\n");
					#endif
					for(read_idx = g_doorlock_data_recv.rIdx;read_idx <= cur_wIdx - 2;read_idx++)
					{
						if((0xfc == g_doorlock_data_recv.buf[read_idx]) && \
							((0x00 == g_doorlock_data_recv.buf[read_idx+1]) || (0x01 == g_doorlock_data_recv.buf[read_idx+1])))
						{
							packet_begin_pos = read_idx;
							break;
						}
					}
					if(-1 != packet_begin_pos)
					{						
						if(0x00 == g_doorlock_data_recv.buf[packet_begin_pos+1])
							packet_len = g_doorlock_data_recv.buf[packet_begin_pos+3];
						else
							packet_len = g_doorlock_data_recv.buf[packet_begin_pos+2]<<8 | g_doorlock_data_recv.buf[packet_begin_pos+3];
						//printf("[1]length:%d %d %d \n",packet_len,g_doorlock_data_recv.buf[packet_begin_pos+1],buf_store_len);
						#if 1
						if((packet_begin_pos + packet_len - 1 < cur_wIdx) || (buf_store_len >= 200))	//缓存一包数据或缓存数据超过200Bytes
						{
							#if 1
							if((packet_begin_pos + packet_len - 1 < cur_wIdx) && (0xfe == g_doorlock_data_recv.buf[packet_begin_pos + packet_len - 1]))
							{
								packet_end_pos = packet_begin_pos+packet_len-1;
							}
							else
							#endif
							{
								/* 先寻找包尾，后反向寻找包头，防止两包拼接，第一包包尾被第二包覆盖 */
								for(read_idx = packet_begin_pos;read_idx<cur_wIdx;read_idx++)
								{
									if(0xfe == g_doorlock_data_recv.buf[read_idx])
									{
										if(cur_wIdx - read_idx > 1)		//代表后面还有数据
										{
											if((cur_wIdx - read_idx > 5) && (0xff == g_doorlock_data_recv.buf[read_idx + 1])\
												&& (0xfc == g_doorlock_data_recv.buf[read_idx + 2]) && (0xfc == g_doorlock_data_recv.buf[read_idx + 3])\
												&& (0xfc == g_doorlock_data_recv.buf[read_idx + 4]) && (0xfc == g_doorlock_data_recv.buf[read_idx + 5]))
											{
												packet_end_pos = read_idx;
												break;
											}
										}
										else							//后面没有数据,升级包纵使固定包头，但是升级包是半双工的，最后只会走这条分支
										{
											packet_end_pos = read_idx;
											break;
										}									
									}
								}
								if(-1 == packet_end_pos)
								{
//									PROTOCOL_MCU_DBG(RT_ERROR, "[1]Data in buffer is too less, rIdx:%d wIdx:%d \n",g_doorlock_data_recv.rIdx,cur_wIdx);				
									usleep(10*1000);
									continue;
								}
								packet_begin_pos = -1;
								for(read_idx = packet_end_pos-2;read_idx>=g_doorlock_data_recv.rIdx;read_idx--)
								{
									if((0xfc == g_doorlock_data_recv.buf[read_idx]) &&\
										((0x00 == g_doorlock_data_recv.buf[read_idx + 1]) || (0x01 == g_doorlock_data_recv.buf[read_idx + 1])))
									{
										if(0x00 == g_doorlock_data_recv.buf[read_idx + 1])
										{
											/* 防止包内数据段存在FC 00 */
											if(read_idx - g_doorlock_data_recv.rIdx >= 5)
											{
												if((0xff == g_doorlock_data_recv.buf[read_idx - 5]) &&\
												(0xfc == g_doorlock_data_recv.buf[read_idx - 4]) && (0xfc == g_doorlock_data_recv.buf[read_idx - 3])&&\
												(0xfc == g_doorlock_data_recv.buf[read_idx - 2]) && (0xfc == g_doorlock_data_recv.buf[read_idx - 1]))
												{
													packet_begin_pos = read_idx;
													break;
												}
											}
											else
											{
												packet_begin_pos = read_idx;
												break;
											}
										}
										else
										{
											/* 升级包没有固定包头 */
											packet_begin_pos = read_idx;
											break;
										}	
									}
								}
								if(-1 == packet_begin_pos)
								{
									g_doorlock_data_recv.rIdx = (packet_end_pos+1)%PROTOCOL_BUF_CAP;
									PROTOCOL_MCU_DBG(RT_ERROR, "[1]Analysis data wrong, rIdx:%d wIdx:%d read_idx:%d \n",g_doorlock_data_recv.rIdx,cur_wIdx,read_idx);				
									usleep(10*1000);
									continue;
								}
							}
						}
						else
						#endif
						{
//							PROTOCOL_MCU_DBG(RT_ERROR, "[2]Data in buffer is too less, rIdx:%d wIdx:%d \n",g_doorlock_data_recv.rIdx,cur_wIdx);				
							usleep(10*1000);
							continue;
						}						
					}
					else
					{
						#if 0
						printf("abandon data from Idx %d ~ %d :\n",g_doorlock_data_recv.rIdx,read_idx-1);
						for(i=g_doorlock_data_recv.rIdx;i<read_idx;i++)
							printf("[%d:0x%x]-",i,g_doorlock_data_recv.buf[i]);
						printf("\n");
						#endif
						g_doorlock_data_recv.rIdx = read_idx;						//寻找下一个固定传输头
						PROTOCOL_MCU_DBG(RT_ERROR, "[1]Recv data wrong, rIdx:%d wIdx:%d \n",g_doorlock_data_recv.rIdx,cur_wIdx);
						usleep(10*1000);
						continue;
					}
				}
				else
				{
//					PROTOCOL_MCU_DBG(RT_ERROR, "[3]Data in buffer is too less, rIdx:%d wIdx:%d \n",g_doorlock_data_recv.rIdx,cur_wIdx);				
					usleep(10*1000);
					continue;
				}
			}
			else
			{
				printf("[2]rIdx:%d wIdx:%d \n",g_doorlock_data_recv.rIdx,cur_wIdx);
				#if 0
				for(i=g_doorlock_data_recv.rIdx;i!=cur_wIdx;)
				{
					printf("[%d:0x%x]",i,g_doorlock_data_recv.buf[i]);
					//printf("{%d}",i);
					i=(i+1)%PROTOCOL_BUF_CAP;
					//printf("{%d-}",i);
				}
				printf("\n");
				#endif

				if((buf_store_len = PROTOCOL_BUF_CAP - g_doorlock_data_recv.rIdx + cur_wIdx) >= 10)
				{
					for(i = 0;i < buf_store_len-1;i++)
					{
						read_idx = (g_doorlock_data_recv.rIdx + i) % PROTOCOL_BUF_CAP;
						if((0xfc == g_doorlock_data_recv.buf[read_idx]) &&\
							((0x00 == g_doorlock_data_recv.buf[(read_idx+1)%PROTOCOL_BUF_CAP]) || (0x01 == g_doorlock_data_recv.buf[(read_idx+1)%PROTOCOL_BUF_CAP])))
						{
							packet_begin_pos = read_idx;
							break;
						}
					}
					if(-1 != packet_begin_pos)
					{
						if(0x00 == g_doorlock_data_recv.buf[(packet_begin_pos+1)%PROTOCOL_BUF_CAP])
							packet_len = g_doorlock_data_recv.buf[(packet_begin_pos+3)%PROTOCOL_BUF_CAP];
						else
							packet_len = g_doorlock_data_recv.buf[(packet_begin_pos+2)%PROTOCOL_BUF_CAP]<<8 | g_doorlock_data_recv.buf[(packet_begin_pos+3)%PROTOCOL_BUF_CAP];
						
						no_overflow_flag = ((packet_begin_pos + packet_len - 1 >= PROTOCOL_BUF_CAP) && ((packet_begin_pos + packet_len - 1)%PROTOCOL_BUF_CAP < cur_wIdx))\
							|| (packet_begin_pos + packet_len - 1 < PROTOCOL_BUF_CAP);
						printf("[2]length:%d %d %d\n",packet_len,g_doorlock_data_recv.buf[(packet_begin_pos+1)%PROTOCOL_BUF_CAP],no_overflow_flag);
						#if 1
						if(no_overflow_flag || (buf_store_len >= 200))
						{
							#if 1
							if(no_overflow_flag && (0xfe == g_doorlock_data_recv.buf[(packet_begin_pos + packet_len - 1)%PROTOCOL_BUF_CAP]))
							{
								packet_end_pos = (packet_begin_pos + packet_len - 1)%PROTOCOL_BUF_CAP;
							}
							else
							#endif
							{
								if(packet_begin_pos > cur_wIdx)
								{
									len_check = PROTOCOL_BUF_CAP - packet_begin_pos + cur_wIdx;
								}
								else
								{
									len_check = cur_wIdx - packet_begin_pos;
								}
								for(i = 0;i < len_check;i++)
								{
									if(0xfe == g_doorlock_data_recv.buf[(packet_begin_pos + i)%PROTOCOL_BUF_CAP])
									{
										if((packet_begin_pos + i)%PROTOCOL_BUF_CAP > cur_wIdx)
										{
											len1_check = PROTOCOL_BUF_CAP - (packet_begin_pos + i)%PROTOCOL_BUF_CAP -1 + cur_wIdx;
										}
										else
										{
											len1_check = cur_wIdx - (packet_begin_pos + i)%PROTOCOL_BUF_CAP - 1;
										}
										j = (packet_begin_pos + i)%PROTOCOL_BUF_CAP;
										if(len1_check > 0)
										{
											if(len1_check >= 5 && (0xff == g_doorlock_data_recv.buf[(j+1)%PROTOCOL_BUF_CAP])\
												&& (0xfc == g_doorlock_data_recv.buf[(j+2)%PROTOCOL_BUF_CAP])\
												&& (0xfc == g_doorlock_data_recv.buf[(j+3)%PROTOCOL_BUF_CAP])\
												&& (0xfc == g_doorlock_data_recv.buf[(j+4)%PROTOCOL_BUF_CAP])\
												&& (0xfc == g_doorlock_data_recv.buf[(j+5)%PROTOCOL_BUF_CAP]))
											{
												packet_end_pos = j;
												break;
											}
										}
										else
										{
											packet_end_pos = j;
											break;
										}
									}
								}
								if(-1 == packet_end_pos)
								{									
//									PROTOCOL_MCU_DBG(RT_ERROR, "[4]Data in buffer is too less, rIdx:%d wIdx:%d \n",g_doorlock_data_recv.rIdx,cur_wIdx); 			
									usleep(10*1000);
									continue;
								}
								if(g_doorlock_data_recv.rIdx > packet_end_pos)
								{
									len_check = PROTOCOL_BUF_CAP - g_doorlock_data_recv.rIdx + packet_end_pos + 1;
								}
								else
								{
									len_check = packet_end_pos - g_doorlock_data_recv.rIdx + 1;
								}
								packet_begin_pos = -1;
								for(i = len_check-2;i >= 0;i--)
								{
									if((0xfc == g_doorlock_data_recv.buf[(g_doorlock_data_recv.rIdx + i)%PROTOCOL_BUF_CAP]) && \
										((0x00 == g_doorlock_data_recv.buf[(g_doorlock_data_recv.rIdx + i + 1)%PROTOCOL_BUF_CAP]) || \
										(0x01 == g_doorlock_data_recv.buf[(g_doorlock_data_recv.rIdx + i + 1)%PROTOCOL_BUF_CAP])))
									{
										if(0x00 == g_doorlock_data_recv.buf[(g_doorlock_data_recv.rIdx + i + 1)%PROTOCOL_BUF_CAP])
										{
											if(i >= 5)
											{
												if((0xff == g_doorlock_data_recv.buf[(g_doorlock_data_recv.rIdx + i - 5)%PROTOCOL_BUF_CAP]) &&\
													(0xfc == g_doorlock_data_recv.buf[(g_doorlock_data_recv.rIdx + i - 4)%PROTOCOL_BUF_CAP]) &&\
													(0xfc == g_doorlock_data_recv.buf[(g_doorlock_data_recv.rIdx + i - 3)%PROTOCOL_BUF_CAP]) &&\
													(0xfc == g_doorlock_data_recv.buf[(g_doorlock_data_recv.rIdx + i - 2)%PROTOCOL_BUF_CAP]) &&\
													(0xfc == g_doorlock_data_recv.buf[(g_doorlock_data_recv.rIdx + i - 1)%PROTOCOL_BUF_CAP]))
												{
													packet_begin_pos = (g_doorlock_data_recv.rIdx + i)%PROTOCOL_BUF_CAP;
													break;
												}
											}
											else
											{
												packet_begin_pos = (g_doorlock_data_recv.rIdx + i)%PROTOCOL_BUF_CAP;
												break;
											}
										}
										else
										{
											packet_begin_pos = (g_doorlock_data_recv.rIdx + i)%PROTOCOL_BUF_CAP;
											break;
										}
									}
								}
								if(-1 == packet_begin_pos)
								{
									g_doorlock_data_recv.rIdx = (packet_end_pos+1)%PROTOCOL_BUF_CAP;
									PROTOCOL_MCU_DBG(RT_ERROR, "[3]Analysis data wrong, rIdx:%d wIdx:%d \n",g_doorlock_data_recv.rIdx,cur_wIdx);				
									usleep(10*1000);
									continue;
								}
							}
							
						}
						else
						#endif
						{
//							PROTOCOL_MCU_DBG(RT_ERROR, "[5]Data in buffer is too less, rIdx:%d wIdx:%d \n",g_doorlock_data_recv.rIdx,cur_wIdx); 			
							usleep(10*1000);
							continue;
						}

					}
					else
					{
						#if 0
						printf("[2]abandon data from Idx %d ~ %d :\n",g_doorlock_data_recv.rIdx,read_idx);
						for(i=g_doorlock_data_recv.rIdx;i!=read_idx;)
						{
							printf("[%d:0x%x]-",i,g_doorlock_data_recv.buf[i]);
							i = (i+1)%PROTOCOL_BUF_CAP;
						}
						printf("\n");
						#endif
						g_doorlock_data_recv.rIdx = (g_doorlock_data_recv.rIdx + i) % PROTOCOL_BUF_CAP;
						PROTOCOL_MCU_DBG(RT_ERROR, "[2]Recv data wrong, rIdx:%d wIdx:%d \n",g_doorlock_data_recv.rIdx,cur_wIdx);
						usleep(10*1000);
						continue;
					}
				}
				else
				{
//					PROTOCOL_MCU_DBG(RT_ERROR, "[6]Data in buffer is too less, rIdx:%d wIdx:%d \n",g_doorlock_data_recv.rIdx,cur_wIdx);				
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
		//printf("packet_begin_pos:%d packet_end_pos:%d rIdx:%d \n",packet_begin_pos,packet_end_pos,g_doorlock_data_recv.rIdx);
		if((-1 != packet_begin_pos) && (-1 != packet_end_pos) && (packet_begin_pos != packet_end_pos))
		{
			if(packet_begin_pos < packet_end_pos)
			{
				len = packet_end_pos - packet_begin_pos + 1;
				if(len <= PROTOCOL_BUF_SIZE)
				{
					//pthread_mutex_lock(&g_doorlock_data_recv.data_access_msem);
					memcpy(buf,g_doorlock_data_recv.buf+packet_begin_pos,len);
					g_doorlock_data_recv.rIdx = (packet_end_pos + 1)%PROTOCOL_BUF_CAP;
					//pthread_mutex_unlock(&g_doorlock_data_recv.data_access_msem);
				}
				else
				{	
					g_doorlock_data_recv.rIdx = (packet_end_pos + 1)%PROTOCOL_BUF_CAP;
					PROTOCOL_MCU_DBG(RT_ERROR, "[1]one packet data is more than 256 bytes:%d \n",len);	
					usleep(10*1000);
					continue;
				}
			}
			else
			{
				len = PROTOCOL_BUF_CAP - packet_begin_pos + packet_end_pos + 1;
				if(len <= PROTOCOL_BUF_SIZE)
				{
					//pthread_mutex_lock(&g_doorlock_data_recv.data_access_msem);
					memcpy(buf,g_doorlock_data_recv.buf+packet_begin_pos,PROTOCOL_BUF_CAP - packet_begin_pos);
					memcpy(buf + PROTOCOL_BUF_CAP - packet_begin_pos,g_doorlock_data_recv.buf,packet_end_pos + 1);
					g_doorlock_data_recv.rIdx = (packet_end_pos + 1)%PROTOCOL_BUF_CAP;
					//pthread_mutex_unlock(&g_doorlock_data_recv.data_access_msem);
				}
				else
				{
					g_doorlock_data_recv.rIdx = (packet_end_pos + 1)%PROTOCOL_BUF_CAP;
					PROTOCOL_MCU_DBG(RT_ERROR, "[2]one packet data is more than 256 bytes:%d \n",len);	
					usleep(10*1000);
					continue;
				}
			}			
		}
		else
		{
			PROTOCOL_MCU_DBG(RT_ERROR, "Analysis data mechanism error,packet_begin_pos:%d packet_end_pos:%d \n",packet_begin_pos,packet_end_pos);
			usleep(10*1000);
			continue;
		}
		
//		printf("[2]packet_begin_pos:%d packet_end_pos:%d rIdx:%d \n",packet_begin_pos,packet_end_pos,g_doorlock_data_recv.rIdx);
		cmd = protocol_parse(buf, len, Flag_uart, protocol_Ctl);
		//以下消息需要批量集中处理
		printf(">>>recv cmd : 0x%x\n",cmd);
		if(cmd == 0xffff)
		{
			for(i=0;i<len;i++)
				printf("buf[%d]:0x%x --",i,buf[i]);
			printf("\n");
		}
		if((cmd==0xb301)||(cmd==0x6401)||(cmd==0x2581)\
			||(cmd==0x1c1)||(cmd==0x8901)||(cmd==0xd401)||(cmd==0x8401)||(cmd==0x6ba0)||(cmd==0x6201))
		{
			memcpy(g_event_msg.buf_pool[g_event_msg.num_idex],buf,len);
			g_event_msg.buf_pool_len[g_event_msg.num_idex] = len;
			g_event_msg.Flag_uart[g_event_msg.num_idex] = Flag_uart;
			g_event_msg.num_idex++;
			if(cmd==0x8401||(g_event_msg.num_idex==PROTOCOL_BUF_NUM))
			{
				event_end_flag=0;					//统一处理时不允许获取事件
				sem_post(&event_msg_signal);				
	            printf("Event msgs batch processing\n");
				continue;
			}
		}
		
		switch(cmd)
		{
			case PROTOCOL_GET_ASK_UPGRADE_ACK:
				if(msg_cpu_flag == 1)
				{
					msg_cpu_flag = 0;
				}
				protocol_get_ask_upgrade_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_FORCE_REBOOT_ACK:
				protocol_get_force_reboot_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_CHECK_IN_BOOT_ACK:				
				if(msg_cpu_flag == 1)
				{
					msg_cpu_flag = 0;
				}
				protocol_get_check_in_boot_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_UPGRADE_PACKAGE_START_ACK:
				protocol_get_upgrade_package_start_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_UPGRADE_PACKAGE_ACK:
				protocol_get_upgrade_package_ack(protocol_Ctl,Flag_uart);
				break;
/*			case PROTOCOL_GET_ERROR:			//发送的包结构出错的，需要重新要求发送，不用解锁，发完再解锁
		 	{
				printf("Receive msg ERROR!!!\n");
                protocol_get_error(protocol_Ctl,Flag_uart);
				break;
		 	}
*/
			case PROTOCOL_GET_TIME_ACK:
				protocol_get_time_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_UPGRADE_PACKAGE_END_ACK:
				protocol_get_upgrade_package_end_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_CLEAR_MSGS_ACK:
				protocol_get_clear_msgs_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_DOOR_SYS_VERSION_ACK:
				protocol_get_door_sys_version_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_REQUEST_DOOR_SLEEP_ACK:
				protocol_get_request_door_sleep_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_DOOR_ENCRYPT_ACK:
				protocol_get_door_encrypt_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_CHECK_MAIN_USER_ACK:
				protocol_get_check_main_user_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_TEMPORARY_PASSWARD_ACK:
				protocol_get_temporary_passward_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_DEL_TEMPORARY_PASSWARD_ACK:

				protocol_get_del_temporary_passward_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_DEL_USER_ACK:
				protocol_get_del_user_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_CHECK_USER_STATUS_ACK:
				protocol_get_check_user_status_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_USER_POWER_ACK:
				protocol_get_user_power_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_ACCESS_WAY_ACK:
				protocol_get_access_way_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_CHECK_DOOR_STATUS_ACK:
				protocol_get_check_door_status_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_CHECK_VERSION_ACK:
				protocol_get_check_version_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_EVENT_ACK:
				protocol_get_event_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_CHECK_MCU_VERSION_ACK:
				protocol_get_check_mcu_version_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_CHECK_POWER_ACK:
				protocol_get_check_power_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_MCU_LOG_ACK:
				protocol_get_mcu_log_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_SLEEP_ACK:
				protocol_get_sleep_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_LIGHT_ACK:
				protocol_get_light_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_DOORBELL_ACK:
				protocol_get_doorbell_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_WIFI_ACK:
				protocol_get_wifi_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_PN_ACK:
				protocol_get_pn_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_DAYNIGHT_ACK:
				protocol_get_daynight_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_DOORTIME_ACK:
				protocol_get_doortime_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_INITOK_ACK:
				protocol_get_initok_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_FACE_VERIFY_ACK:
				protocol_get_face_verify_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_CHECK_USER_ACK:
				if(msg_cpu_flag == 1)
				{
					msg_cpu_flag = 0;
				}
				protocol_get_check_user_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_SLEEP_POWER_ACK:
				protocol_get_sleep_power_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_KEY_VALUE_ACK:
				protocol_get_key_value_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_KEY_ACK:
				protocol_get_key_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_ANTILOCK_ACK:
				protocol_get_antilock_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_VOLUME_ACK:
				protocol_get_volume_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_LOCKSTATUS_ACK:
				protocol_get_lockstatus_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_VOLUME_CHECK_ACK:
				protocol_get_volume_check_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_SPEAKER_ACK:
				protocol_get_speaker_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_FACE_DETECT_START_ACK:
				protocol_get_face_detect_start_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_FACE_DETECT_END_ACK:
				protocol_get_face_detect_end_ack(protocol_Ctl,Flag_uart);
				break;
           case PROTOCOL_GET_COMMUNICATION_TEST_ACK:
				protocol_get_communication_test_ack(protocol_Ctl,Flag_uart);
				break;
		   case PROTOCOL_GET_FACE_IDENTIFY_MODE_ACK:
		   		protocol_get_face_identify_mode_ack(protocol_Ctl,Flag_uart);
				break;
		   case PROTOCOL_GET_LOCKVERSION_FORCEUP_ACK:
		   		protocol_get_lockversion_forceup_ack(protocol_Ctl,Flag_uart);
				break;
		   case PROTOCOL_GET_CHECK_SNAP_PWD_ACK:
		   		protocol_get_check_snap_pwd_ack(protocol_Ctl,Flag_uart);
		   		break;
			case PROTOCOL_GET_REMOTE_UNLOCK_ACK:
				protocol_get_remote_unlock_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_REMOTE_BIND_VERIFY_ACK:
				protocol_get_remote_bind_verify_ack(protocol_Ctl,Flag_uart);
				break;
			//优先需要处理的上报事件
			case PROTOCOL_GET_SYS_INIT:
				protocol_get_sys_init(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_REPORT_USER_STATUS:
				protocol_get_report_user_status(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_FACE_DETECT_EVENT:
				protocol_get_face_detect_event(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_CHECK_UPGRADE_VERSION:
				protocol_get_check_upgrade_version(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_DOOR_STATUS:			//开门报警事件优先级提高
				protocol_get_door_status(protocol_Ctl,Flag_uart);
		   		break;
			case PROTOCOL_GET_BELL_REPORT:
				protocol_get_bell_report(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_DOOR_EXCEPTION_NOTICE:
				protocol_get_door_exception_notice(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_FACE_DEL:
				protocol_get_face_del(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_WAKEUP_ACK:
				protocol_get_wakeup_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_STATUS_CHANGE:
				protocol_get_status_change(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_INDOOR_OPEN_EVENT:
				protocol_get_indoor_open_event(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_SAMPLE_LOCK_MODE:
				protocol_get_sample_lock_mode(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_USER_CHANGE_NOTICE:
				protocol_get_user_change_notice(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_SMART_SIMPLE_EVENT:
				protocol_get_smart_simple_event(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_CLEAR_IDENTIFY_ERROR:
				protocol_get_clear_identify_error(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_REQUEST_CATAGEING:
				protocol_get_request_catageing(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_CPU_HEARTBEAT_ACK:
				protocol_get_cpu_heartbeat_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_CHECK_MCU_ABILITY_ACK:
				protocol_get_check_mcu_ability_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_WIFI_CONFIG:
				protocol_get_wifi_config(protocol_Ctl,Flag_uart);
				break;
			//人脸录入流程
			case PROTOCOL_GET_FACE_START:
				g_face_dclog_info.start_time = time(NULL);
				face_enter_processing = 1;
				sem_post(&face_enter_status_Signal);
				protocol_get_face_start(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_FACE_INPUT:
				protocol_get_face_input(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_FACE_EXCEPTION:
				protocol_get_face_exception(protocol_Ctl,Flag_uart);
				break;
			//门锁传来的消息
			case PROTOCOL_GET_SERIAL_ACK:
				protocol_get_serial_ack(protocol_Ctl,Flag_uart);
				break;
			case PROTOCOL_GET_T1_TEST_TERM:
				protocol_get_t1_test_term(protocol_Ctl,Flag_uart);
				break;
				
			default:
				if(upgrade_procesing_lock==1)
				{
					upgrade_procesing_lock = 0;
				}
/*		收到的重发收到的信令不正确，需要继续重发			
				if(msg_cpu_flag)
				{
					msg_cpu_flag = 0;
				}
*/
				if(face_enter_processing == 1)
				{
					face_enter_processing = 0;
				}
				break;					
		}	
		//printf("[end]rIdx:%d wIdx:%d \n",g_doorlock_data_recv.rIdx,g_doorlock_data_recv.wIdx);
	}
	PROTOCOL_MCU_DBG(RT_ERROR, "thread [%s] abnormal quit \n",__FUNCTION__);
}

