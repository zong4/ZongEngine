def image_to_hex_array(image_path, output_path):
    # 打开图片文件并读取为字节数据
    with open(image_path, 'rb') as file:
        byte_data = file.read()
    
    # 将字节数据转换为16进制数组
    hex_array = [f"0x{byte:02x}" for byte in byte_data]
    
    # 写入文件，格式化为类似 { 0x... , 0x... , ... } 的形式
    with open(output_path, 'w') as output_file:
        output_file.write("{\n")
        for i, hex_value in enumerate(hex_array):
            # 每行最多写32个数据，便于查看
            output_file.write(f"{hex_value}")
            if (i + 1) % 16 == 0:
                output_file.write(", \n")
            else:
                output_file.write(", ")
        output_file.write("\n}\n")

# 使用示例
image_path = 'C:\\Users\\zzoon\\Desktop\\ZongEngine\\Editor\\Resources\\Editor\\Logo_square.jpg'  # 替换成你的图片路径
output_path = 'output_hex.txt'  # 输出文件的路径
image_to_hex_array(image_path, output_path)
