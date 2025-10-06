import javax.swing.*;
import java.awt.*;
import java.io.*;
import java.net.*;

public class AdminClientGUI extends JFrame {
    private static final String SERVER_IP = "127.0.0.1";
    private static final int SERVER_PORT = 8080;

    private Socket socket;
    private PrintWriter out;
    private BufferedReader in;
    private String authToken;
    private String username = "admin";

    // Componentes UI
    private JTextArea outputArea;
    private JButton connectBtn, disconnectBtn;
    private JButton speedUpBtn, slowDownBtn, turnLeftBtn, turnRightBtn;

    public AdminClientGUI() {
        setTitle("Vehículo Autónomo - Cliente Administrador (Java)");
        setSize(700, 500);
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setLayout(new BorderLayout(10, 10));
        initComponents();
        setLocationRelativeTo(null);
    }

    private void initComponents() {
        // Panel superior - Conexión
        JPanel topPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        topPanel.setBorder(BorderFactory.createTitledBorder("Conexión"));

        connectBtn = new JButton("Conectar");
        connectBtn.setBackground(new Color(46, 204, 113));
        connectBtn.setForeground(Color.WHITE);
        connectBtn.addActionListener(e -> connectToServer());

        disconnectBtn = new JButton("Desconectar");
        disconnectBtn.setBackground(new Color(231, 76, 60));
        disconnectBtn.setForeground(Color.WHITE);
        disconnectBtn.setEnabled(false);
        disconnectBtn.addActionListener(e -> disconnectFromServer());

        topPanel.add(connectBtn);
        topPanel.add(disconnectBtn);

        add(topPanel, BorderLayout.NORTH);

        // Panel de comandos
        JPanel commandPanel = new JPanel(new GridLayout(2, 2, 10, 10));
        commandPanel.setBorder(BorderFactory.createTitledBorder("Comandos de Control"));

        speedUpBtn = createCommandButton("▲ SPEED UP", "SPEED_UP");
        slowDownBtn = createCommandButton("▼ SLOW DOWN", "SLOW_DOWN");
        turnLeftBtn = createCommandButton("◄ TURN LEFT", "TURN_LEFT");
        turnRightBtn = createCommandButton("► TURN RIGHT", "TURN_RIGHT");

        setCommandButtonsEnabled(false);

        commandPanel.add(speedUpBtn);
        commandPanel.add(slowDownBtn);
        commandPanel.add(turnLeftBtn);
        commandPanel.add(turnRightBtn);

        add(commandPanel, BorderLayout.CENTER);

        // Panel inferior - Log
        JPanel bottomPanel = new JPanel(new BorderLayout());
        bottomPanel.setBorder(BorderFactory.createTitledBorder("Log de Comunicación"));

        outputArea = new JTextArea(12, 50);
        outputArea.setEditable(false);
        outputArea.setFont(new Font("Monospaced", Font.PLAIN, 11));
        JScrollPane scrollPane = new JScrollPane(outputArea);

        bottomPanel.add(scrollPane, BorderLayout.CENTER);
        add(bottomPanel, BorderLayout.SOUTH);
    }

    private JButton createCommandButton(String text, String command) {
        JButton btn = new JButton(text);
        btn.setFont(new Font("Arial", Font.BOLD, 12));
        btn.addActionListener(e -> sendCommand(command));
        return btn;
    }

    private void logOutput(String text) {
        outputArea.append(text + "\n");
        outputArea.setCaretPosition(outputArea.getDocument().getLength());
    }

    private void connectToServer() {
        try {
            socket = new Socket(SERVER_IP, SERVER_PORT);
            out = new PrintWriter(socket.getOutputStream(), true);
            in = new BufferedReader(new InputStreamReader(socket.getInputStream()));

            logOutput("✅ Conectado a " + SERVER_IP + ":" + SERVER_PORT);

            // CONNECT como ADMIN
            String connectMsg = "VATP/1.0 CONNECT 0\r\n" +
                    "User-Type: ADMIN\r\n" +
                    "Username: admin\r\n" +
                    "Password: admin123\r\n\r\n";
            sendMessage(connectMsg);

            // AUTH
            String authMsg = "VATP/1.0 AUTH 0\r\n" +
                    "Username: admin\r\n" +
                    "Password: admin123\r\n\r\n";
            String response = sendMessage(authMsg);

            // Intentar extraer token si existe, si no, usar uno falso
            extractToken(response);
            if (authToken == null) {
                authToken = "TOKEN_FAKE";
                logOutput("⚠️ No se encontró token real, se usará token simulado.");
            }

            logOutput("🔑 Token activo: " + authToken);
            setCommandButtonsEnabled(true);
            connectBtn.setEnabled(false);
            disconnectBtn.setEnabled(true);

        } catch (IOException e) {
            logOutput("❌ Error al conectar: " + e.getMessage());
            JOptionPane.showMessageDialog(this,
                    "No se pudo conectar al servidor",
                    "Error", JOptionPane.ERROR_MESSAGE);
        }
    }

    private String sendMessage(String message) throws IOException {
        out.print(message);
        out.flush();

        logOutput(">>> " + message.replace("\r\n", " ").trim());

        StringBuilder response = new StringBuilder();
        String line;

        while ((line = in.readLine()) != null) {
            response.append(line).append("\n");
            if (line.trim().isEmpty()) break;
        }

        String resp = response.toString();
        logOutput("<<< " + resp.replace("\n", " ").trim());
        return resp;
    }

    private void sendCommand(String command) {
        if (socket == null || socket.isClosed()) {
            JOptionPane.showMessageDialog(this, "No hay conexión con el servidor",
                    "Error", JOptionPane.WARNING_MESSAGE);
            return;
        }

        String commandMsg = "VATP/1.0 COMMAND 0\r\n" +
                "Username: admin\r\n" +
                "Auth-Token: " + authToken + "\r\n" +
                "Command: " + command + "\r\n\r\n";
        try {
            sendMessage(commandMsg);
        } catch (IOException e) {
            logOutput("❌ Error enviando comando: " + e.getMessage());
        }
    }

    private void extractToken(String response) {
        for (String line : response.split("\n")) {
            if (line.contains("TOKEN_")) {
                int idx = line.indexOf("TOKEN_");
                if (idx >= 0) {
                    authToken = line.substring(idx).trim();
                    break;
                }
            }
        }
    }

    private void disconnectFromServer() {
        try {
            if (socket != null && !socket.isClosed()) {
                String disconnectMsg = "VATP/1.0 DISCONNECT 0\r\n" +
                        "Username: admin\r\n\r\n";
                out.print(disconnectMsg);
                out.flush();
                socket.close();
            }
        } catch (IOException e) {
            logOutput("⚠️ Error al desconectar: " + e.getMessage());
        } finally {
            socket = null;
            out = null;
            in = null;
            authToken = null;
            setCommandButtonsEnabled(false);
            connectBtn.setEnabled(true);
            disconnectBtn.setEnabled(false);
            logOutput("❌ Cliente desconectado");
        }
    }

    private void setCommandButtonsEnabled(boolean enabled) {
        speedUpBtn.setEnabled(enabled);
        slowDownBtn.setEnabled(enabled);
        turnLeftBtn.setEnabled(enabled);
        turnRightBtn.setEnabled(enabled);
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> {
            AdminClientGUI gui = new AdminClientGUI();
            gui.setVisible(true);
        });
    }
}
