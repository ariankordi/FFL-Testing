import struct
import sys

def read_fflstoredata(file_path):
    with open(file_path, 'rb') as file:
        return file.read(96)

def main():
    if len(sys.argv) != 4:
        print("Usage: python script.py <path_to_FFLStoreData_file> <resolution> <output_file>")
        return

    fflstoredata_file = sys.argv[1]
    resolution = int(sys.argv[2])
    output_file = sys.argv[3]

    fflstoredata = read_fflstoredata(fflstoredata_file)

    #resolution = 1600
    tex_resolution = 768
    data_length = len(fflstoredata)
    view_type = 0  # face
    expression = 0
    resource_type = 1
    shader_type = 0
    camera_rotate = [0, 0, 0]
    background_color = [0, 0, 0, 0]
    verify_charinfo = True
    light_enable = True

    # Ensure fflstoredata is exactly 96 bytes
    #fflstoredata = fflstoredata[:96] + b'\x00' * (96 - len(fflstoredata))
    # Crafting the struct
    # struct format: 'I?' means a 4-byte unsigned int and a 1-byte bool
    # Adjust the format string according to your needs
    struct_format = '96sHII4Biii4B??2x'  # padding at the end
    #background_color_vec4 = [component / 255.0 for component in background_color]
    packed_data = struct.pack(
        struct_format,
        fflstoredata,                   # data: 96s
        data_length,                    # dataLength: H (uint16_t)
        resolution,                     # resolution: I (unsigned int)
        tex_resolution,                 # texResolution: I (unsigned int)
        view_type,                      # viewType: B (uint8_t)
        expression,                     # expression: B (uint8_t)
        resource_type,                  # resourceType: B (uint8_t)
        shader_type,                    # shaderType: B (uint8_t)
        camera_rotate[0],               # cameraRotate.x: i (int32_t)
        camera_rotate[1],               # cameraRotate.y: i (int32_t)
        camera_rotate[2],               # cameraRotate.z: i (int32_t)
        background_color[0],            # backgroundColor[0]: B (uint8_t)
        background_color[1],            # backgroundColor[1]: B (uint8_t)
        background_color[2],            # backgroundColor[2]: B (uint8_t)
        background_color[3],            # backgroundColor[3]: B (uint8_t)
        verify_charinfo,                # verifyCharInfo: ? (bool)
        light_enable                    # lightEnable: ? (bool),
    )

    # Write the packed data to the output file
    with open(output_file, 'wb') as file:
        file.write(packed_data)

    # Print the packed data in a human-readable format (hexadecimal)
    #print("Packed Data (hex):", packed_data.hex())

if __name__ == "__main__":
    main()
