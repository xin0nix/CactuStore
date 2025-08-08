import socket

def send_message():
    host = "127.0.0.1"   # <<< fixed typo
    port = 8080
    message = bytes([42])  # The byte value 42

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((host, port))
        s.sendall(message)
        print("Message sent!")

if __name__ == "__main__":
    send_message()
