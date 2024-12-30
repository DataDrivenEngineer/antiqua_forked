import os
import json

def pretty_print_json(data):
    print(json.dumps(data, indent = 2))

def main():
    with open("running_brute.gltf", "r") as inf, open("running_brute.mdl", "w") as outf:
        outf.write("MESH NAME\n")
        outf.write("BUFFER NAME\n")
        outf.write(f"INDICES - byte offset - byte length\n")
        outf.write(f"POSITION - byte offset - byte length\n")
        outf.write("\n")

        data = json.load(inf)
        for mesh in data["meshes"]:
            outf.write(f"{mesh['name']}\n")

            position_attr_accessor_index = int(mesh["primitives"][0]["attributes"]["POSITION"])
            indices_accessor_index = int(mesh["primitives"][0]["indices"])

            position_accessor = data["accessors"][position_attr_accessor_index]
            indices_accessor = data["accessors"][indices_accessor_index]

            position_buffer_view = data["bufferViews"][int(position_accessor["bufferView"])]
            indices_buffer_view = data["bufferViews"][int(indices_accessor["bufferView"])]

            indices_buffer_index = int(indices_buffer_view["buffer"])
            indices_buffer_name = data["buffers"][indices_buffer_index]["uri"]
            indices_byte_offset = int(indices_buffer_view["byteOffset"])
            indices_byte_length = int(indices_buffer_view["byteLength"])
            outf.write(f"{indices_buffer_name}\n")
            outf.write(f"{indices_byte_offset} {indices_byte_length}\n")

            position_buffer_index = int(position_buffer_view["buffer"])
            position_buffer_name = data["buffers"][position_buffer_index]["uri"]
            position_byte_offset = int(position_buffer_view["byteOffset"])
            position_byte_length = int(position_buffer_view["byteLength"])
            outf.write(f"{position_byte_offset} {position_byte_length}\n")

if __name__ == "__main__":
    main()
