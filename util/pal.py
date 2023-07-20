def read_palette_file(filename):
    rgb_values = []
    with open(filename, 'r') as file:
        lines = file.readlines()
        for line in lines[3:]:  # Skip the header lines
            if line.strip() and not line.startswith("#"):
                r, g, b, name = line.split()
                rgb_values.append((int(r), int(g), int(b)))
    return rgb_values

def flatten_rgb_array(rgb_array):
    print(len(rgb_array))
    flat_array = [channel for rgb in rgb_array for channel in rgb]
    return flat_array

# Example usage:
gpl_file_path = 'Cranes.gpl'
flat_list_of_colors = flatten_rgb_array(read_palette_file(gpl_file_path))

print(flat_list_of_colors)