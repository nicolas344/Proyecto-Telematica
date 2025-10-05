import socket
import tkinter as tk
from tkinter import messagebox, scrolledtext

SERVER_IP = "127.0.0.1"
SERVER_PORT = 8080

class AdminClientApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Vehículo Autónomo - Admin Client")
        self.sock = None
        self.token = None

        # Botones de conexión
        self.connect_btn = tk.Button(root, text="Conectar", command=self.connect_to_server, bg="green", fg="white")
        self.connect_btn.grid(row=0, column=0, padx=10, pady=10)

        self.disconnect_btn = tk.Button(root, text="Desconectar", command=self.disconnect_from_server, bg="red", fg="white", state=tk.DISABLED)
        self.disconnect_btn.grid(row=0, column=1, padx=10, pady=10)

        # Área de respuestas
        self.output = scrolledtext.ScrolledText(root, width=60, height=20, state=tk.DISABLED)
        self.output.grid(row=1, column=0, columnspan=4, padx=10, pady=10)

        # Botones de comandos
        self.btn_speed_up = tk.Button(root, text="SPEED UP", command=lambda: self.send_command("SPEED_UP"), width=15)
        self.btn_speed_up.grid(row=2, column=0, padx=5, pady=5)

        self.btn_slow_down = tk.Button(root, text="SLOW DOWN", command=lambda: self.send_command("SLOW_DOWN"), width=15)
        self.btn_slow_down.grid(row=2, column=1, padx=5, pady=5)

        self.btn_turn_left = tk.Button(root, text="TURN LEFT", command=lambda: self.send_command("TURN_LEFT"), width=15)
        self.btn_turn_left.grid(row=2, column=2, padx=5, pady=5)

        self.btn_turn_right = tk.Button(root, text="TURN RIGHT", command=lambda: self.send_command("TURN_RIGHT"), width=15)
        self.btn_turn_right.grid(row=2, column=3, padx=5, pady=5)

        # Deshabilitar comandos al inicio
        self.set_command_buttons_state(tk.DISABLED)

    def log_output(self, text):
        self.output.config(state=tk.NORMAL)
        self.output.insert(tk.END, text + "\n")
        self.output.yview(tk.END)
        self.output.config(state=tk.DISABLED)

    def send_message(self, message):
        if not self.sock:
            return None
        self.sock.sendall(message.encode())
        response = self.sock.recv(2048).decode()
        self.log_output(">>> " + message.strip())
        self.log_output("<<< " + response.strip())
        return response

    def connect_to_server(self):
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((SERVER_IP, SERVER_PORT))
            self.log_output(f"✅ Conectado a {SERVER_IP}:{SERVER_PORT}")

            # CONNECT como ADMIN
            connect_msg = (
                "VATP/1.0 CONNECT 0\r\n"
                "User-Type: ADMIN\r\n"
                "Username: admin\r\n"
                "Password: admin123\r\n"
                "\r\n"
            )
            self.send_message(connect_msg)

            # AUTH
            auth_msg = (
                "VATP/1.0 AUTH 0\r\n"
                "Username: admin\r\n"
                "Password: admin123\r\n"
                "\r\n"
            )
            response = self.send_message(auth_msg)

            # Buscar token en cualquier parte de la respuesta
            for line in response.splitlines():
                if "TOKEN_" in line:
                    parts = line.split("TOKEN_")
                    if len(parts) > 1:
                        self.token = "TOKEN_" + parts[1].strip()
                        self.log_output(f"🔑 Token recibido: {self.token}")
                        break

            self.set_command_buttons_state(tk.NORMAL)
            self.connect_btn.config(state=tk.DISABLED)
            self.disconnect_btn.config(state=tk.NORMAL)

        except Exception as e:
            messagebox.showerror("Error", f"No se pudo conectar: {e}")
            self.sock = None

    def send_command(self, cmd):
        if not self.token:
            messagebox.showwarning("Sin token", "No estás autenticado como ADMIN.")
            return
        command_msg = (
            "VATP/1.0 COMMAND 0\r\n"
            "Username: admin\r\n"
            f"Auth-Token: {self.token}\r\n"
            f"Command: {cmd}\r\n"
            "\r\n"
        )
        self.send_message(command_msg)

    def disconnect_from_server(self):
        if self.sock:
            disconnect_msg = (
                "VATP/1.0 DISCONNECT 0\r\n"
                "Username: admin\r\n"
                "\r\n"
            )
            self.send_message(disconnect_msg)
            self.sock.close()
            self.sock = None
            self.token = None
            self.set_command_buttons_state(tk.DISABLED)
            self.connect_btn.config(state=tk.NORMAL)
            self.disconnect_btn.config(state=tk.DISABLED)
            self.log_output("❌ Cliente desconectado.")

    def set_command_buttons_state(self, state):
        self.btn_speed_up.config(state=state)
        self.btn_slow_down.config(state=state)
        self.btn_turn_left.config(state=state)
        self.btn_turn_right.config(state=state)

if __name__ == "__main__":
    root = tk.Tk()
    app = AdminClientApp(root)
    root.mainloop()
