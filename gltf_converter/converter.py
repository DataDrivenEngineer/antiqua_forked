import json

def pretty_print_json(data):
    print(json.dumps(data, indent = 2))

def main():
    in_file_name = "test_cube"
    min_x, min_y, min_z = 3*(float("inf"),)
    max_x, max_y, max_z = 3*(float("-inf"),)

    with open(f"../data/{in_file_name}.gltf", "r") as inf, open(f"../data/{in_file_name}.mdl", "w") as outf:
        outf.write("#all values are in right handed coordinate system\n")
        outf.write("#buffer name\n")
        outf.write("#number of meshes\n")
        outf.write("#min corner of bounding box - global across all meshes\n")
        outf.write("#max corner of bounding box - global across all meshes\n")
        outf.write("#indices byte offset {space} indices count {space} position byte offset {space}\n")

        buffer_name_written = False

        data = json.load(inf)
        for mesh in data["meshes"]:
            position_attr_accessor_index = int(mesh["primitives"][0]["attributes"]["POSITION"])
            indices_accessor_index = int(mesh["primitives"][0]["indices"])

            position_accessor = data["accessors"][position_attr_accessor_index]
            indices_accessor = data["accessors"][indices_accessor_index]

            min_x = min(min_x, float(position_accessor["min"][0]))
            min_y = min(min_y, float(position_accessor["min"][1]))
            min_z = min(min_z, float(position_accessor["min"][2]))

            max_x = max(max_x, float(position_accessor["max"][0]))
            max_y = max(max_y, float(position_accessor["max"][1]))
            max_z = max(max_z, float(position_accessor["max"][2]))

            position_vertex_count = position_accessor["count"]
            indices_count = indices_accessor["count"]

            position_buffer_view = data["bufferViews"][int(position_accessor["bufferView"])]
            indices_buffer_view = data["bufferViews"][int(indices_accessor["bufferView"])]

            indices_buffer_index = int(indices_buffer_view["buffer"])
            indices_buffer_name = data["buffers"][indices_buffer_index]["uri"]
            indices_byte_offset = int(indices_buffer_view["byteOffset"])
            indices_byte_length = int(indices_buffer_view["byteLength"])
            if not buffer_name_written:
                outf.write(f"{indices_buffer_name} \n")
                buffer_name_written = True
                outf.write(f"{len(data['meshes'])} \n")
                outf.write("\n")
                outf.write("\n")
            outf.write(f"{indices_byte_offset} {indices_count} ")

            position_buffer_index = int(position_buffer_view["buffer"])
            position_buffer_name = data["buffers"][position_buffer_index]["uri"]
            position_byte_offset = int(position_buffer_view["byteOffset"])
            position_byte_length = int(position_buffer_view["byteLength"])
            outf.write(f"{position_byte_offset} \n")

    with open(f"../data/{in_file_name}.mdl", "r") as outf:
        outData = outf.readlines()
    outData[8] = f"{min_x} {min_y} {min_z} \n"
    outData[9] = f"{max_x} {max_y} {max_z} \n"
    with open(f"../data/{in_file_name}.mdl", "w") as outf:
       outf.writelines(outData) 

if __name__ == "__main__":
    main()
