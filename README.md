# OOM定位


这是一个查看各个进程VMRSS的指令可以用来定位内存溢出OOM  
`free -m;ps | grep -w /home/ezapp_memelf | grep -v grep | awk '{print $1}' | xargs -i cat /proc/{}/status |grep VmRSS;ps| grep -w /home/ezapp_memelfsub | grep -v grep | awk '{print $1}' | xargs -i cat /proc/{}/status |grep VmRSS;ps| grep -w /home/helloworld | grep -v grep | awk '{print $1}' | xargs -i cat /proc/{}/status |grep VmRSS`  

实验证明函数内部的static变量，只声明的话不会动到vmrss，在第一次访问以后，会搬到vmrss里面去



