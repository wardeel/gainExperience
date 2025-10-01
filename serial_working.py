import serial
import serial.tools.list_ports
import time
from datetime import datetime

# 自动检测可用串口
def find_serial_ports():
    ports = serial.tools.list_ports.comports()
    available_ports = []
    for port in ports:
        available_ports.append(port.device)
    return available_ports

# 显示可用串口
print("正在检测可用串口...")
available_ports = find_serial_ports()
if available_ports:
    print("找到以下串口:")
    for port in available_ports:
        print(f"  {port}")
else:
    print("未找到可用串口！")
    input("按回车键退出...")
    exit()

# RS485接口配置
PORT = available_ports[0] if available_ports else 'COM1'
BAUDRATE = 9600
BYTESIZE = serial.EIGHTBITS    # 8位数据位
PARITY = serial.PARITY_EVEN    # 偶校验
STOPBITS = serial.STOPBITS_ONE # 1位停止位
TIMEOUT = 1

print(f"RS485接口配置:")
print(f"  串口: {PORT}")
print(f"  波特率: {BAUDRATE}bps")
print(f"  数据位: {BYTESIZE}位")
print(f"  校验位: 偶校验")
print(f"  停止位: {STOPBITS}位")

# 消息映射配置
MESSAGE_MAP = {
    bytes([0xAA, 0x55, 0x00, 0x00, 0x03, 0x02, 0x01, 0xFF, 0xFB, 0x03]): 
    bytes([0xAA, 0x55, 0x00, 0x00, 0x05, 0x82, 0x01, 0x00, 0x47, 0x04, 0x2D, 0x03]),
    
    bytes([0xAA, 0x55, 0x00, 0x00, 0x04, 0x03, 0x01, 0xFF, 0x05, 0xF4, 0x03]): 
    bytes([0xAA, 0x55, 0x00, 0x00, 0x03, 0x83, 0x01, 0x01, 0x78, 0x03]),
    
    bytes([0xAA, 0x55, 0x00, 0x00, 0x03, 0x05, 0x01, 0x10, 0xE7, 0x03]): 
    bytes([0xAA, 0x55, 0x00, 0x00, 0x05, 0x85, 0x01, 0x00, 0x47, 0x04, 0x2A, 0x03])
}

# 定义消息结束标志（根据你的协议，0x03可能是结束符）
MESSAGE_END_MARKER = bytes([0x03])

def bytes_to_hex_string(data_bytes):
    """将字节数据转换为十六进制字符串显示"""
    return ' '.join([f'{byte:02X}' for byte in data_bytes])

class SerialMessageHandler:
    def __init__(self, port, baudrate, bytesize, parity, stopbits, timeout):
        self.ser = None
        self.port = port
        self.baudrate = baudrate
        self.bytesize = bytesize
        self.parity = parity
        self.stopbits = stopbits
        self.timeout = timeout
        self.running = False
        self.receive_buffer = bytearray()  # 累积接收缓冲区
        self.last_receive_time = 0         # 上次接收时间
        self.message_timeout = 0.1         # 消息超时时间（秒）
        
    def connect(self):
        """连接串口"""
        try:
            print(f"尝试连接RS485串口 {self.port}...")
            self.ser = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                bytesize=self.bytesize,
                parity=self.parity,
                stopbits=self.stopbits,
                timeout=self.timeout
            )
            # 清空输入缓冲区
            self.ser.reset_input_buffer()
            print(f"✅ RS485串口 {self.port} 连接成功")
            return True
        except serial.SerialException as e:
            print(f"❌ 串口连接失败: {e}")
            return False
    
    def is_complete_message(self, data):
        """检查是否为完整消息"""
        # 方法1：检查是否有已知消息匹配
        for target_msg in MESSAGE_MAP.keys():
            if target_msg in data:
                return True, target_msg
        
        # 方法2：检查结束标志（如果协议有固定结束符）
        if MESSAGE_END_MARKER and data.endswith(MESSAGE_END_MARKER):
            # 进一步验证消息长度或格式
            if len(data) >= 5:  # 最小消息长度
                return True, data
        
        return False, None
    
    def process_received_data(self):
        """处理接收到的数据（改进版本）"""
        if not self.ser or not self.ser.is_open:
            return False
            
        current_time = time.time()
        
        # 读取新数据
        if self.ser.in_waiting > 0:
            new_data = self.ser.read(self.ser.in_waiting)
            self.receive_buffer.extend(new_data)
            self.last_receive_time = current_time
            
            hex_str = bytes_to_hex_string(new_data)
            timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
            print(f"📥 [{timestamp}] 收到数据片段: {hex_str} (长度: {len(new_data)})")
        
        # 检查是否应该处理缓冲区（超时或缓冲区较大）
        should_process = False
        if self.receive_buffer:
            time_since_last = current_time - self.last_receive_time
            if time_since_last > self.message_timeout or len(self.receive_buffer) > 100:
                should_process = True
        
        if should_process and self.receive_buffer:
            # 处理累积的缓冲区数据
            buffer_copy = bytes(self.receive_buffer)
            timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
            print(f"🔍 [{timestamp}] 处理缓冲区: {bytes_to_hex_string(buffer_copy)} (总长度: {len(buffer_copy)})")
            
            # 查找完整消息
            is_complete, matched_msg = self.is_complete_message(buffer_copy)
            
            if is_complete:
                # 找到匹配消息，发送回复
                if matched_msg in MESSAGE_MAP:
                    reply_msg = MESSAGE_MAP[matched_msg]
                    
                    # 半双工通信：先接收完，再发送回复
                    time.sleep(0.02)  # 确保接收完成
                    
                    self.ser.write(reply_msg)
                    reply_hex = bytes_to_hex_string(reply_msg)
                    current_time_str = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                    
                    print(f"📤 [{current_time_str}] 发送回复: {reply_hex}")
                    print(f"   回复长度: {len(reply_msg)} 字节")
                    print("-" * 60)
                
                # 清空缓冲区（简单处理：清空整个缓冲区）
                self.receive_buffer.clear()
            
            else:
                # 如果没有完整消息，但缓冲区太大，清空一部分
                if len(self.receive_buffer) > 200:
                    print("⚠️  缓冲区过大，清空前100字节")
                    self.receive_buffer = self.receive_buffer[100:]
        
        return True
    
    def start(self):
        """启动串口监听"""
        if not self.connect():
            print("连接失败，程序退出")
            return
            
        self.running = True
        print("🚀 RS485半双工通信监听已启动")
        print("📡 使用累积缓冲区处理分段数据...")
        print("按 Ctrl+C 退出程序")
        print("=" * 60)
        
        try:
            while self.running:
                self.process_received_data()
                time.sleep(0.005)  # 更短的延迟，提高响应速度
        except KeyboardInterrupt:
            print("\n🛑 用户中断程序")
        except Exception as e:
            print(f"❌ 错误: {e}")
        finally:
            self.stop()
    
    def stop(self):
        """停止并关闭串口"""
        self.running = False
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("🔒 RS485串口已关闭")

def main():
    print("=" * 60)
    print("RS485半双工通信 - 改进版消息处理脚本")
    print("=" * 60)
    
    handler = SerialMessageHandler(
        port=PORT,
        baudrate=BAUDRATE,
        bytesize=BYTESIZE,
        parity=PARITY,
        stopbits=STOPBITS,
        timeout=TIMEOUT
    )
    handler.start()

if __name__ == "__main__":
    main()
    input("按回车键退出...")
