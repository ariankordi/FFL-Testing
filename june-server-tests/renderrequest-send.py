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
    is_head_only = False
    expression_flag = 1
    resource_type = 1
    background_color = (0, 0, 0, 0)

    # Crafting the struct
    # struct format: 'I?' means a 4-byte unsigned int and a 1-byte bool
    # Adjust the format string according to your needs
    struct_format = '96sII?II4f'
    background_color_vec4 = [component / 255.0 for component in background_color]
    packed_data = struct.pack(struct_format, fflstoredata, resolution, tex_resolution, is_head_only, expression_flag, resource_type, background_color_vec4[0], background_color_vec4[1], background_color_vec4[2], background_color_vec4[3])

    # Write the packed data to the output file
    with open(output_file, 'wb') as file:
        file.write(packed_data)

    # Print the packed data in a human-readable format (hexadecimal)
    #print("Packed Data (hex):", packed_data.hex())

if __name__ == "__main__":
    main()
