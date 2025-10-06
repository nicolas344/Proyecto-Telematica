
import socket
import threading
import tkinter as tk
from tkinter import scrolledtext

SERVER_IP = "127.0.0.1"   # Cambia si el server está en otra máquina
SERVER_PORT = 8080

class ObserverClientGUI:
    def __init__(self, master):
        self.master = master
        self.master.title("Cliente OBSERVER - Telemetría en tiempo real")
        self.master.geometry("500x400")

        # Área de texto para mostrar telemetría
        self.output = scrolledtext.ScrolledText(master, wrap=tk.WORD, state="disabled", height=20, width=60)
        self.output.pack(padx=10, pady=10)

        # Botón para desconectar
        self.disconnect_btn = tk.Button(master, text="Desconectar", command=self.disconnect, state="disabled")
        self.disconnect_btn.pack(pady=5)

        # Variables de conexión
        self.sock = None
        self.running = False

        # Iniciar conexión automáticamente
        self.connect()

    def connect(self):
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((SERVER_IP, SERVER_PORT))

            self.running = True
            self.disconnect_btn.config(state="normal")

            self.write_output(f"✅ Conectado al servidor {SERVER_IP}:{SERVER_PORT} como OBSERVER\n")

            # Mandar CONNECT como OBSERVER
            connect_msg = (
                "VATP/1.0 CONNECT 0\r\n"
                "User-Type: OBSERVER\r\n"
                "Username: observer\r\n"
                "\r\n"
            )
            self.sock.sendall(connect_msg.encode())

            # Hilo para escuchar mensajes
            threading.Thread(target=self.listen_server, daemon=True).start()

        except Exception as e:
            self.write_output(f"❌ Error al conectar: {e}\n")

    def listen_server(self):
        while self.running:
            try:
                data = self.sock.recv(2048).decode()
                if not data:
                    self.write_output("❌ Conexión cerrada por el servidor\n")
                    break
                self.write_output("\n📡 Telemetría recibida:\n" + data + "\n")
            except Exception as e:
                self.write_output(f"⚠️ Error recibiendo datos: {e}\n")
                break

    def disconnect(self):
        if self.sock:
            try:
                disconnect_msg = (
                    "VATP/1.0 DISCONNECT 0\r\n"
                    "Username: observer\r\n"
                    "\r\n"
                )
                self.sock.sendall(disconnect_msg.encode())
            except:
                pass
            self.sock.close()
            self.sock = None

        self.running = False
        self.disconnect_btn.config(state="disabled")
        self.write_output("⏹ Cliente desconectado\n")

    def write_output(self, message):
        self.output.config(state="normal")
        self.output.insert(tk.END, message + "\n")
        self.output.yview(tk.END)  # Scroll automático
        self.output.config(state="disabled")


if __name__ == "__main__":
    root = tk.Tk()
    app = ObserverClientGUI(root)
    root.mainloop()
