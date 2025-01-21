import struct
import socket
import sys

def read_fflstoredata(file_path):
    with open(file_path, 'rb') as file:
        return file.read(96)

def main():
    """
    if len(sys.argv) < 4:
        print(f"Usage: {sys.argv[0]} <path to storedata> <resolution> <output file>")
        return

    fflstoredata_file = sys.argv[1]
    resolution = int(sys.argv[2])
    output_file = sys.argv[3]
    expression = int(sys.argv[4]) if len(sys.argv) > 4 else 0
    export_as_gltf = True if len(sys.argv) > 5 else False
    """
    if len(sys.argv) < 5:
        print(f"Usage: {sys.argv[0]} <path to storedata> <resolution> <host> <port> <output file ('-' for stdout)> [expression] [response_format: 1=gltf, 2=tga]")
        return

    fflstoredata_file = sys.argv[1]
    resolution = int(sys.argv[2])
    host = sys.argv[3]
    port = int(sys.argv[4])
    output_file = sys.argv[5]
    expression = int(sys.argv[6]) if len(sys.argv) > 6 else 0
    response_format = int(sys.argv[7]) if len(sys.argv) > 7 else 0

    fflstoredata = read_fflstoredata(fflstoredata_file)

    #resolution = 1600
    tex_resolution = 512
    data_length = len(fflstoredata)
    view_type = 0  # face
    #expression = 0
    resource_type = -1
    shader_type = 0
    camera_rotate = [0, 0, 0]
    model_rotate = [0, 0, 0]
    background_color = [255, 255, 255, 0]
    verify_charinfo = True  #  False
    verify_crc16 = True  #  False
    light_enable = True
    clothes_color = -1
    pants_color = 0  # gray i think

    # Ensure fflstoredata is exactly 96 bytes
    #fflstoredata = fflstoredata[:96] + b'\x00' * (96 - len(fflstoredata))

    struct_format = '96sHBBHhBbBBIIIhhhhhhBBBBBB???bBbbbBBhhhB'
    packed_data = struct.pack(
        struct_format,
        fflstoredata,         # data: 96s
        data_length,          # dataLength: H (uint16_t)
        1 << 0,  # 1 << 5,               # modelType: B (uint8_t)
        response_format,      # responseFormat: B (uint8_t)
        resolution,           # resolution: H (uint16_t)
        tex_resolution,       # texResolution: h (int16_t)
        view_type,            # viewType: B (uint8_t)
        resource_type,        # resourceType: B (uint8_t)
        shader_type,          # shaderType: B (uint8_t)
        expression,           # expression: B (uint8_t)
        0, 0, 0,              # expressionFlag: III (uint32_t[3])
        camera_rotate[0],     # cameraRotate.x: h (int16_t)
        camera_rotate[1],     # cameraRotate.y: h (int16_t)
        camera_rotate[2],     # cameraRotate.z: h (int16_t)
        model_rotate[0],      # modelRotate.x: h (int16_t)
        model_rotate[1],      # modelRotate.y: h (int16_t)
        model_rotate[2],      # modelRotate.z: h (int16_t)
        background_color[0],  # backgroundColor[0]: B (uint8_t)
        background_color[1],  # backgroundColor[1]: B (uint8_t)
        background_color[2],  # backgroundColor[2]: B (uint8_t)
        background_color[3],  # backgroundColor[3]: B (uint8_t)
        0,                    # aaMethod: B (uint8_t)
        0,  # 3,                    # drawStageMode: B (uint8_t)
        verify_charinfo,      # verifyCharInfo: ? (bool)
        verify_crc16,         # verifyCRC16: ? (bool)
        light_enable,         # lightEnable: ? (bool)
        clothes_color,        # clothesColor: b (int8_t)
        pants_color,          # pantsColor: b (int8_t)
        -1,                   # bodyType: b (int8_t)
        -1,                   # headwearIndex: b (int8_t)
        -1,                   # headwearColor: b (int8_t)
        0,                    # instanceCount: B (uint8_t)
        0,                    # instanceRotationMode: B (uint8_t)
        -1,                   # lightDirection.x: h (int16_t)
        -1,                   # lightDirection.y: h (int16_t)
        -1,                   # lightDirection.z: h (int16_t)
        0                     # splitMode: B (uint8_t)
    )
    """
    # Write the packed data to the output file
    with open(output_file, 'wb') as file:
        file.write(packed_data)
    """
    # Send the packed data over a TCP socket and capture the full response
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((host, port))
        s.sendall(packed_data)

        # Read all data from the socket until it's closed by the server
        response = bytearray()
        while True:
            chunk = s.recv(4096)  # Receive in chunks of 4096 bytes
            if not chunk:
                break  # Stop when no more data is available
            response.extend(chunk)

        # Output the response to the specified output file or stdout
        if output_file == '-':
            sys.stdout.buffer.write(response)
        else:
            with open(output_file, 'wb') as file:
                file.write(response)

    # Print the packed data in a human-readable format (hexadecimal)
    #print("Packed Data (hex):", packed_data.hex())

if __name__ == "__main__":
    main()
