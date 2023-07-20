from PIL import Image

def get_image_data(image_path):
    try:
        # Open the image
        image = Image.open(image_path)

        # Get width and height
        width, height = image.size

        # Convert the image to RGB mode (in case it's grayscale or another mode)
        image_rgb = image.convert("RGB")

        # Get pixel data as a flat RGB array
        pixels = list(image_rgb.getdata())

        # Format width and height as strings in "w h " format
        dimensions_str = f"{width} {height} "

        return dimensions_str, pixels
    except Exception as e:
        print(f"Error: {e}")
        return None, None

def save_image_data_to_file(image_path, output_filename):
    width_height_str, pixels = get_image_data(image_path)

    if width_height_str and pixels:
        with open(output_filename, 'wb') as file:
            # Write the dimensions as a string
            file.write(width_height_str.encode('utf-8'))

            # Write the pixel data (24-bit RGB)
            for pixel in pixels:
                r, g, b = pixel
                file.write(bytes([r, g, b]))

        print(f"Image data saved to {output_filename}")

# Replace "mori.png" with the path to your image file.
input_image_path = "mori.png"
output_filename = "../mori.rg2"
save_image_data_to_file(input_image_path, output_filename)
